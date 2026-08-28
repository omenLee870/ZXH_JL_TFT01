#ifndef _JLOTA_H
#define _JLOTA_H

#include "jlota_err.h"
#include "update_loader_download.h"


typedef enum  {
    JLOTA_UPGRADE_TYPE_DEFAULT,
    JLOTA_UPGRADE_TYPE_SINGLE,
    JLOTA_UPGRADE_TYPE_DUAL,
    JLOTA_UPGRADE_TYPE_PUP2,
    JLOTA_UPGRADE_TYPE_WHOLE,
    JLOTA_UPGRADE_TYPE_MAX,
} jlota_upgrade_type;


/* active update ve param */
struct active2_param {
    u32 sem_wait;
    u32 once_len; //one package len, 0 mean default
    int mode_type; // rcsp/usb/bt/mmc/testbox/...
    int upgrade_type; //jlota_upgrade_type: single/dual/full/diff
    u16 uparam_len; //jlota updata_param len, = (LOADER_REC_LEN + support_jlota_ext_param_len)
    const char *ota_name; //ota loader name
};

struct active2_result;

struct update_param_record {
    u32 loader_saddr;
    u32 vmbak_saddr;
    u32 uparam_area_size;

    /* opt: 0 mean write, 1 mean read */
    int (*update_param_opt)(struct active2_result *result, u8 opt, u8 *buf, u32 off, u32 len);
};

struct update_mode_info_v2 {
    update_mode_info_t *uinfo;
    struct active2_param *param;
};

struct active2_result {
    int ret;
    void *priv[0];
};

struct active2_user {
    update_mode_info_t uinfo;
    struct active2_param param;
    int (*jlota_process_cbk)(struct active2_user *auser, int step, void *param);
    int (*jlota_sleep_cbk)(struct active2_user *auser);
    int (*jlota_resume_cbk)(struct active2_user *auser);
    u8(*jlota_transer_errcode_cbk)(struct active2_user *auser, int ret, update_ret_code_t *err);
    struct active2_result *result;
    void *priv;
};

int active_update_v2_init(struct active2_user *auser);
int active_update_v2_sleep(void *priv);
void active_update_v2_resume(void *priv);
int active_update_v2_running(void);

int jlota_update_user_start(struct update_mode_info_v2 *uinfo_v2);
int jlota_update_param_fill(struct active2_user *info, struct active2_result *result);
void jlota_update_param_ext_fill(UPDATA_PARM *p, u8 ext_type, u8 *ext_data, u8 ext_len);
void jlota_update_result_set(u16 result);

#endif
