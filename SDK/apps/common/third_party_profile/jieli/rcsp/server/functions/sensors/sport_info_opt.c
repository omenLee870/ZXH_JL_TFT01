#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sport_info_opt.data.bss")
#pragma data_seg(".sport_info_opt.data")
#pragma const_seg(".sport_info_opt.text.const")
#pragma code_seg(".sport_info_opt.text")
#endif
#include "rcsp_config.h"
#include "JL_rcsp_attr.h"
#include "sport_info_opt.h"
#include "rcsp_event.h"
#include "rcsp_manage.h"
#include "JL_rcsp_protocol.h"
#include "JL_rcsp_api.h"

#if (RCSP_MODE && JL_RCSP_SENSORS_DATA_OPT)

#define FUNCTION_UPDATE_MAX_LEN			(256)

#define ASSET_CMD_DATA_LEN(len, limit) 	\
	do{	\
		if(len >= limit){	\
		}else{				\
			return ;   \
		}\
	}while(0);

enum {
    SPORTS_INFO_OPT_GET,
    SPORTS_INFO_OPT_SET,
    SPORTS_INFO_OPT_NOTIFY,
};

static rcsp_sport_info_opt_t *g_rcsp_sport_info_opt = NULL;
#define __this g_rcsp_sport_info_opt

static void get_sport_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    printf("get_sport_info\n");
    u32 rlen = 0;
    u32 mask = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    if (NULL == resp) {
        printf("%s, %d, no ram!!\n", __func__, __LINE__);
        goto __get_sport_info_end;
    }

    rlen = attr_get(priv, resp, TARGET_FEATURE_RESP_BUF_SIZE, __this->sport_info_get_tab, __this->sport_info_get_tab_num, mask);

__get_sport_info_end:
    if (0 == rlen) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, (u16)rlen, 0, NULL);
    }

    if (resp) {
        free(resp);
    }
}

static void set_sport_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    printf("set_sport_info\n");
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    put_buf(data, len);
    attr_set(priv, data, len, __this->sport_info_set_tab, __this->sport_info_set_tab_num, 0, NULL);
    if (rcspModel->err_code) {
        rcspModel->err_code = 0;
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, 0, NULL);
    }
}

void sport_info_opt_update(u32 mask)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL || 0 == JL_rcsp_get_auth_flag()) {
        return ;
    }
    u32 rlen = 0;
    u8 *buf = zalloc(FUNCTION_UPDATE_MAX_LEN);
    if (buf == NULL) {
        printf("no ram err\n");
        return;
    }
    buf[0] = SPORTS_INFO_OPT_NOTIFY;

    rlen = attr_get((void *)rcspModel, buf + 1, FUNCTION_UPDATE_MAX_LEN - 1, __this->sport_info_get_tab, __this->sport_info_get_tab_num, mask);
    if (rlen) {
        JL_CMD_send(JL_OPCODE_SPORTS_DATA_INFO_OPT, buf, (u16)rlen + 1, JL_NOT_NEED_RESPOND, 0, NULL);
    }
    if (buf) {
        free(buf);
    }
}

int JL_rcsp_sports_info_funciton(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    int ret = -1;
    if (!__this) {
        return ret;
    }
    if (JL_OPCODE_SPORTS_DATA_INFO_OPT == OpCode) {
        u8 op = data[0];
        switch (op) {
        case SPORTS_INFO_OPT_GET:
            get_sport_info(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORTS_INFO_OPT_SET:
            set_sport_info(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORTS_INFO_OPT_NOTIFY:
            break;
        }
        ret = 0;
    }
    return ret;
}


int rcsp_register_sport_info_opt_interface(rcsp_sport_info_opt_t *rcsp_sport_info_opt_interface)
{
    g_rcsp_sport_info_opt = rcsp_sport_info_opt_interface;
    return 0;
}

#else

void sport_info_opt_update(u32 mask)
{

}

#endif

