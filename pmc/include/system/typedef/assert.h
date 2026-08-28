#ifndef __ASSERT_H__
#define __ASSERT_H__

extern const int config_asser_log;

#define ASSERT(a,...)   \
    do { \
        if(!(a)){ \
            if (config_asser_log) { printf("file:%s, line:%d", __FILE__, __LINE__); } \
            if (config_asser_log) { printf("ASSERT-FAILD: "#a" "__VA_ARGS__); } \
            while(1); \
        } \
    }while(0);




#endif /* #ifndef _COMMON_H_ */
