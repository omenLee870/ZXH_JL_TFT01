#include "includes.h"

extern char sdk_version_begin[];
extern char sdk_version_end[];

/*
   打印 .sys.version 段中记录的所有版本信息
   每个版本项由 4 字节的 __TIMESTAMPNUM__ 和紧随其后的字符串组成,
   与 apps/sdk_version.z 中的记录格式保持一致
 */
void sdk_version_dump(void)
{
#if CONFIG_UART_DEBUG_ENABLE
    char *version;

    puts("=================Version===============\n");
    for (version = sdk_version_begin; version < sdk_version_end;) {
        version += 4;                       /* 跳过 __TIMESTAMPNUM__ */
        printf("%s\n", version);
        version += strlen(version) + 1;
    }
    puts("=======================================\n");
#endif /* #if CONFIG_UART_DEBUG_ENABLE */
}
