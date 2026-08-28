#ifndef GENERIC_CPU_H
#define GENERIC_CPU_H

#include <asm/cpu.h>







#define assert_d(a,...)   \
        do { \
            if(config_asser){\
                if(!(a)){ \
					printf("cpu %d file:%s, line:%d",current_cpu_id(), __FILE__, __LINE__); \
					printf("ASSERT-FAILD: "#a" "__VA_ARGS__); \
                    {\
                        while(1);\
                    }\
                } \
            } \
        }while(0);











#endif

