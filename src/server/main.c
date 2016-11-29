#include <glib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include "../common/webmonit.h"

#define MIC_NOT_IN_USE 20
#define MIC_ALREADY_IN_USE 21


gint
main (gint argc, gchar **argv)
{
    /* TODO:
        - get interval and ignored app from file
        - check whether webcam are being used
        - check whether mic is being used
        - send notification
        - log somewhere
        - how to treat ignored apps?
        - sleep default or as specified by interval
    */
    struct _devs *head, *tmp;
    gint fd;

    ConfigValues *cfg_values = load_config_file ();

    head = list_webcam ();

    while (head) {
        check_webcam (head->dev_name, cfg_values->ignore_apps);
        g_free (head->dev_name);
        tmp = head;
        head = head->next;
        free (tmp);
    }

    if (cfg_values->microphone_device != NULL) {
        get_mic_status(cfg_values->microphone_device);
        g_free(cfg_values->microphone_device);
    }

    if (cfg_values->ignore_apps != NULL) {
        g_strfreev (cfg_values->ignore_apps);
    }

    g_free (cfg_values);

    return 0;
}


gint
get_mic_status (const gchar *mic)
{
    // TODO: grep capture /proc/asound/devices. If 1 then sysdefault? And if 2 what? DEAL WITH MULTIPLE MIC
    /* TODO arecord -L | grep -w sysdefault:CARD with system() could be a solution. Check the fork() thing to add more security
     * (https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=2130132)
     */
    snd_pcm_t *capture_handle = NULL;

    if (snd_pcm_open (&capture_handle, mic, SND_PCM_STREAM_CAPTURE, 0) < 0) {
        // TODO This fails also if the device name is not correct. Device presence should be checked
        return MIC_ALREADY_IN_USE;
    }
    else {
        snd_pcm_close (capture_handle);
        return MIC_NOT_IN_USE;
    }
}