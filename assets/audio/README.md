# 倒车提示音（上板学习测试资源）

- 来源页面：https://m.sc.chinaz.com/yinxiao/200824073681.html ，标题为“倒车请注意音效”。
- 下载地址：https://downsc.chinaz.net/Files/DownLoad/sound1/202008/13282.wav
- 下载日期：2026-10-09。
- 原文件：`reverse_notice_source.wav`，PCM 16-bit、22050 Hz、双声道，约 7.94 秒。
- 板端文件：`reverse_notice.wav`，PCM 16-bit、16000 Hz、单声道，2.085 秒；保留首句及其尾音，末尾 5ms 淡出，去掉后面的重复播报。
- 原文件 SHA256：`82694c3471816c3c839192a37229f358637314c70b2f49edc6703c02e80933e0`。

来源站说明资源仅供学习参考，不能据此认定有商用授权。本资源用于当前上板测试；产品发布前应替换成自录或已获授权的音频。文件格式和时长已检查，实际语音内容及音量需试听/上板确认。

## 接入及打包

2026-10-10 修正：原始约 7.94 秒文件内部连续重复四轮，波形重复周期约 1.9853 秒；只给整个文件添加间隔，会听到文件内部的连续播报。已将板端资源裁为首句，结束点取 2.085 秒（首句尾音后的低电平区，第二句开始前），原始下载文件仍保留。

宠物仪表按 OK 成功进入视频页后循环播报：每次说一句“倒车请注意”，静音间隔 3 秒再播下一轮（间隔从收到播放结束通知开始计算，不包含 2.085 秒语音长度）。间隔常量为 `SDK/apps/watch/common/pet_reverse_notice.c` 中的 `NOTICE_INTERVAL_MS`。

若手机原本正在播放，先发 AVRCP PAUSE，确认 A2DP 停止后播报；整个倒车页期间（含轮次间隔）保持暂停，退出页面后才恢复本次暂停的同一台手机。原本已暂停或蓝牙未连接时，不主动播放手机音乐。语音播放中或间隔中退出都会取消循环；退出消息投递失败时，应用层 100ms 周期检测也会取消循环，过期的进入回调不会在页面退出后重启播报。每轮使用新的回调标记，旧回调不会结束新一轮。间隔结束时若提示音播放器忙，则延后 3 秒重试，不积压提示音。

首次进入时未收到暂停反馈、语音缺失或播放器忙，会跳过播报并记录日志；播报启动失败或超时会停止当前循环，并尝试恢复本次暂停的音乐。

已启用 SDK 和 GUI 音频配置中的 WAV 解码。`tools/prepare_reverse_notice.ps1` 由 `SDK/cpu/br33/tools/download_befor.bat` 自动调用，将 `reverse.wav` 加入中文/英文提示音包。优先保留 GUI 生成的包，没有输出时使用 SDK dashboard 原始包；校验 SDFILE 文件表、每个资源及整个包的 CRC，并验证其他资源内容不变。重复打包不会重复追加。

单独准备资源（不下载板子）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/prepare_reverse_notice.ps1
```

需要 Python 3（仅标准库）。脚本优先使用 `ZXH_PYTHON`、PATH 的 Python，再使用当前电脑 Codex 随附的 Python；其他电脑可传 `-Python`。

替换音频时只保留一句，将新文件转为相同 PCM WAV 格式后覆盖 `reverse_notice.wav`，再次执行脚本即可。打包时检查格式和时长（不超过 4 秒），防止把原来的多句连续素材再次装入。时长检查不能替代实际试听。

音频裁剪修正必须重新下载包含新 `tone_zh.cfg` / `tone_en.cfg` 的提示音资源包，单独更新代码不会替换板上的旧录音。运行日志前缀：`[REVERSE_VOICE]`。

资源回归校验：`python tools/tests/test_reverse_notice_asset.py`，检查板端 WAV 时长/格式、旧多句素材拒绝、两种语言包中实际录音一致、其余资源保持不变和 CRC。

## PSRAM 播放内存

2026-10-10：倒车播报改用 `play_tone_file_alone_callback_psram()`，播放器对象通过 `malloc_psram()` 创建并清零，关闭时根据地址选择 `free_psram()`。对象申请失败会取消本轮，不回退占用 RAM。

WAV 解码适配器位于 SDK 的 `media.a`，没有配套源码。编译时 `SDK/build/prepare_wav_psram.ps1` 用杰理工具链的 `llvm-ar` 提取其副本，再用 `llvm-bcrename` 只把该对象的 `malloc/zalloc/free` 引用改为 `tone_wav_psram_malloc/zalloc/free`；这些接口使用匹配的 PSRAM 分配、清零及释放。生成对象优先参与链接，原始 `media.a` 不被修改。脚本检查原始及生成对象的符号，SDK 升级后若接口不匹配会终止编译。当前构建脚本使用 Windows PowerShell 和当前杰理工具链。

这一步作用于所有使用该适配器的 WAV 播放，包括倒车语音；其他格式的解码器、SDK 音频节点、文件系统句柄、任务栈及 DAC/DMA 输出缓冲仍使用原来的分配方式。音频文件仍保存在内置 Flash，按需分段读取，不整段缓存到 PSRAM。

正常每句播完，SDK 关闭 WAV 解码器并释放 PSRAM 工作区、音频流、资源文件和播放器后才进入 3 秒间隔；中途退出及超时沿用同一释放路径。间隔中只保留小型静态状态。每轮的 `[REVERSE_VOICE] player=... memory=PSRAM`、`player free=...` 及 `[WAV_PSRAM] alloc=... bytes=...` / `free=...` 用于板上核对分配地址及释放配对。

该改动需要重新编译并下载固件。电脑测试检查多轮分配/释放配对、清零、申请失败和空指针释放；音频内容、PSRAM 总线及 DAC 输出仍需上板验证。
