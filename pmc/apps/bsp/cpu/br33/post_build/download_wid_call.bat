
if exist "..\..\..\..\..\..\sdk\cpu\br33\tools\" (
    copy .\pmc_code.bin ..\..\..\..\..\..\sdk\cpu\br33\tools\pmc_code.bin
    copy .\p11_code.bin ..\..\..\..\..\..\sdk\cpu\br33\tools\p11_code.bin
    copy .\p11_code.lz4 ..\..\..\..\..\..\sdk\cpu\br33\tools\p11_code.lz4
    copy .\pmc_code%case_suffix%.bin ..\..\..\..\..\..\sdk\cpu\br33\tools\pmc_code%case_suffix%.bin
    copy .\p11_code%case_suffix%.bin ..\..\..\..\..\..\sdk\cpu\br33\tools\p11_code%case_suffix%.bin
    copy .\p11_code%case_suffix%.lz4 ..\..\..\..\..\..\sdk\cpu\br33\tools\p11_code%case_suffix%.lz4
    copy ..\..\..\..\..\apps\bsp\cpu\br33\config\msys_pmc_app.h ..\..\..\..\..\..\sdk\cpu\br33\msys_pmc_app.h
)

if exist "..\..\..\..\..\..\sdk\cpu\br33\tools\download.bat" (
    call ..\..\..\..\..\..\sdk\cpu\br33\tools\download.bat
)

::pause
