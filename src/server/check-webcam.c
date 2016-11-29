#include <glib.h>
#include <errno.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <gio/gio.h>
#include "../common/webmonit.h"


gint open_device (const gchar *dev_name);

gint xioctl (gint fh, gulong request, void *arg);

gint init_device (gint fd, const gchar *dev_name, gchar **ignore_apps);

gint get_webcam_status (gint fd, const gchar *dev_name, gchar **ignore_apps);

gboolean ignored_app_using_webcam (const gchar *dev_name, gchar **ignored_apps);

gboolean get_webcam_from_open_fd (const gchar *dev_name, guint pid);


void
check_webcam (const gchar *dev_name, gchar **ignore_apps)
{
    gint fd = open_device (head->dev_name);
    init_device (fd, head->dev_name, ignore_apps);

    if (fd >= 0)
        close (fd);
}


gint
open_device (const gchar *dev_name)
{
    struct stat st;

    if (stat (dev_name, &st) == -1) {
        g_printerr ("Cannot identify '%s': %d, %s\n", dev_name, errno, g_strerror (errno));
        return GENERIC_ERROR;
    }

    if (!S_ISCHR (st.st_mode)) {
        g_printerr ("%s is no device\n", dev_name);
        return GENERIC_ERROR;
    }

    int fd = open (dev_name, O_RDWR | O_NONBLOCK, 0);

    if (fd == -1) {
        g_printerr ("Cannot open '%s': %d, %s\n", dev_name, errno, g_strerror (errno));
        return GENERIC_ERROR;
    }

    return fd;
}


gint
xioctl (gint fh, gulong request, void *arg)
{
    gint r;

    do {
        r = ioctl (fh, request, arg);
    } while (r == -1 && errno == EINTR);

    return r;
}


gint
init_device (gint fd, const gchar *dev_name, gchar **ignore_apps)
{
    struct v4l2_capability cap;

    if (xioctl (fd, VIDIOC_QUERYCAP, &cap) == -1) {
        if (errno == EINVAL) {
            g_printerr ("%s is no V4L2 device\n", dev_name);
            return GENERIC_ERROR;
        } else {
            g_printerr ("VIDIOC_QUERYCAP: %s\n", g_strerror (errno));
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        g_printerr ("%s is no video capture device\n", dev_name);
        return GENERIC_ERROR;
    }


    get_webcam_status (fd, dev_name, ignore_apps);
}


gint
get_webcam_status (gint fd, const gchar *dev_name, gchar **ignore_apps)
{
    struct v4l2_requestbuffers req;

    memset (&req, 0, sizeof (req));

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl (fd, VIDIOC_REQBUFS, &req) == -1) {
        if (errno == EINVAL) {
            g_printerr ("%s does not support memory mapping\n", dev_name);
            return GENERIC_ERROR;
        } else {
            if (!ignored_app_using_webcam (dev_name, ignore_apps)) {
                g_print ("Webcam is being used\n");
                return WEBCAM_ALREADY_IN_USE;
            }
            return WEBCAM_USED_BY_IGNORED_APP;
        }
    } else {
        g_print ("Webcam is not being used\n");
        return WEBCAM_NOT_IN_USE;
    }
}


gboolean
ignored_app_using_webcam (const gchar *dev_name, gchar **ignored_apps)
{
    if (ignored_apps == NULL) {
        return FALSE;
    }

    for (gint i = 0; i < g_strv_length (ignored_apps); i++) {
        guint pid = get_ppid_from_pname (ignored_apps[i]);
        if (pid != 0) {
            if (get_webcam_from_open_fd (dev_name, pid)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}


gboolean
get_webcam_from_open_fd (const gchar *dev_name, guint pid)
{
    gchar *pid_str = g_strdup_printf ("%u", pid);
    gchar *path = g_strconcat ("/proc/", pid_str, "/fd", NULL);

    GError *err = NULL;
    GDir *dir = g_dir_open (path, 0, &err);
    if (err != NULL) {
        g_printerr ("%s\n", err->message);
        return GENERIC_ERROR;
    }

    const gchar *subdir = g_dir_read_name (dir);
    while (subdir != NULL) {
        gchar *complete_path = g_strconcat (path, "/", subdir, NULL);
        GFile *file = g_file_new_for_path (complete_path);
        GFileInfo *file_info = g_file_query_info (file, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, &err);
        if (err != NULL) {
            g_printerr("%s\n", err->message);
            g_clear_error(&err);
        } else {
            const gchar *target = g_file_info_get_symlink_target (file_info);
            if (g_strcmp0 (dev_name, target) == 0) {
                g_free (complete_path);
                g_dir_close (dir);
                g_free (pid_str);
                g_free (path);
                g_object_unref (file);
                return TRUE;
            }
        }
        g_free (complete_path);
        g_object_unref (file);
        subdir = g_dir_read_name (dir);
    }

    g_dir_close (dir);
    g_free (pid_str);
    g_free (path);

    return FALSE;
}