#ifndef CTP_CMD_H
#define CTP_CMD_H

#include "system/includes.h"

int ctp_cmd_analysis(const char *topic, char *content, void *priv);
void ctp_all_get_cmd_run(void *priv, char *content);
int CTP_CMD_COMBINED(void *priv, u32 err, const char *_req, const char *mothod, char *str);
int send_ctp_string(int cmd_type, char *buf, const char *_req, void *priv);

#endif // CTP_CMD_H
