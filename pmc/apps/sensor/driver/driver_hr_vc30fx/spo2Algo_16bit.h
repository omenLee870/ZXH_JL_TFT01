
#ifndef SPO2_ALGO_H
#define SPO2_ALGO_H

#include <stdint.h>

void spo2AlgoInit_16bit(void);
int32_t spo2Algo_16bit(int32_t r_ppg, int32_t ir_ppg, int32_t mode);

void spo2_algo_content_init_16bit(void);

char *
spo2Version_info_16bit(void);

#endif
