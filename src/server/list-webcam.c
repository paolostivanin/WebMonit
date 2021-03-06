#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include "main.h"


struct _devs *list_webcam ()
{
    struct _devs *head, *curr;
    const gchar *common = "/dev/video";

    gint fd;
    head = NULL;

    // Getting the first 32 webcam should be more than enough :) If not, please adjust the number to suits your needs.
    for (int i = 0; i < 32; i++) {
        gchar *tmp_name = malloc (strlen (common) + 3);
        sprintf (tmp_name, "%s%d", common, i);
        if ((fd = open (tmp_name, O_RDONLY)) == -1) {
            free (tmp_name);
        } else {
            close (fd);
            curr = malloc (sizeof (struct _devs));
            curr->dev_name = g_strdup (tmp_name);
            curr->next = head;
            head = curr;
            free (tmp_name);
        }
    }

    return head;
}

