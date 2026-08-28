
echo copy head

if [ -d "../../../../pmc/apps/bsp/cpu/br33" ]; then
    echo find path
    cp ../../../cpu/br33/msys_pmc_app.h ../../../../pmc/apps/bsp/cpu/br33/config/msys_pmc_app.h
    {
    echo "#ifndef MSYS_SDK_CONFIG_H"
    echo "#define MSYS_SDK_CONFIG_H"
    echo ""
    sed -n "/^#define SUB_CASE_TYPE_/p" ../../../apps/watch/board/br33/board_config.h
    echo ""
    grep "#define TCFG_SUB_CASE_TYPE" ../../../apps/watch/board/br33/sdk_config.h
    echo ""
    echo "#endif"
    } > ../../../cpu/br33/msys_sdk_config.h
    cp ../../../cpu/br33/msys_sdk_config.h ../../../../pmc/apps/bsp/cpu/br33/config/msys_sdk_config.h
    rm ../../../cpu/br33/msys_sdk_config.h

fi


if [ -d "../../../../loader/update/apps/watch/board/br33" ]; then
    echo find ota ui path
    {
    echo "#ifndef MSYS_SDK_CONFIG_H"
    echo "#define MSYS_SDK_CONFIG_H"
    echo ""
    sed -n "/^#define SUB_CASE_TYPE_/p" ../../../apps/watch/board/br33/board_config.h
    echo ""
    grep "#define TCFG_SUB_CASE_TYPE" ../../../apps/watch/board/br33/sdk_config.h
    echo ""
    grep -E "#define TCFG_PSRAM_DEV_ENABLE|#define TCFG_PSRAM_SIZE" ../../../apps/watch/board/br33/sdk_config.h
    echo ""
    grep "#define TCFG_CLOCK_OSC_HZ" ../../../apps/watch/board/br33/sdk_config.h
    echo ""
    sed -n "/^#define TCFG_UI_ENABLE\b/,/^#endif \/\/ TCFG_UI_ENABLE/p" ../../../apps/watch/board/br33/sdk_config.h
    echo ""
    echo "#endif"
    } > ../../../cpu/br33/msys_sdk_config.h
    cp ../../../cpu/br33/msys_sdk_config.h ../../../../loader/update/apps/watch/board/br33/msys_sdk_config.h
    rm ../../../cpu/br33/msys_sdk_config.h

fi

