#pragma once

#define DEFAULT_CHECK_INTERVAL 30
#define DEFAULT_NOTIFICATION_TIMEOUT 5

#define SYSDEFAULT_DEV_NOT_FOUND -2
#define GENERIC_ERROR -1

#define WEBCAM_NOT_IN_USE 10
#define WEBCAM_ALREADY_IN_USE 11
#define WEBCAM_USED_BY_IGNORED_APP 12
#define WEBCAM_FOUND 13
#define WEBCAM_NOT_FOUND 14

#define MIC_NOT_IN_USE 20
#define MIC_ALREADY_IN_USE 21
#define SYSDEFAULT_FOUND 22



struct _devs {
    gchar *dev_name;
    struct _devs *next;
} *head, *curr;

typedef struct _conf_values_t {
    gulong check_interval;
    gint notification_timeout;
    gchar *microphone_device;
    gchar **ignore_apps;
} ConfigValues;

struct _devs *list_webcam (void);

ConfigValues *load_config_file (void);

gint get_mic_status (const gchar *mic);

void check_webcam (gint notif_server_status, const gchar *dev_name, gchar **ignore_apps);

guint get_ppid_from_pname (const gchar *pname);

gint check_sysdefault_dev (void);