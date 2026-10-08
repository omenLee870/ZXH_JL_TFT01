# 音乐播放 UI 独立素材

九张独立素材已导入 JieLiUI 项目 `ui_lvgl/dashboard_1024x600_v1` 的 `import/image/music` 目录，并通过工具的本地 MCP 接口完成 `chongwuyibiao` 页静态排版。未修改 SDK/UI 代码，未接入蓝牙或播放逻辑。

`png` 目录中的九张 PNG 都带透明通道，可作为独立图片资源导入。`preview` 是单项效果和按钮状态预览，不是待导入的整屏界面。`source` 保留设计源文件。

|文件|尺寸|用途|
|---|---|---|
|album_vinyl.png|96 × 96|默认封面|
|bluetooth_connected.png|28 × 36|蓝牙已连接图标|
|button_next.png|40 × 40|下一曲|
|button_pause.png|40 × 40|暂停|
|button_play.png|40 × 40|播放|
|button_previous.png|40 × 40|上一曲|
|controls_background.png|268 × 84|浅色按钮底板|
|dog_headphones.png|216 × 142|耳机小狗与音符|
|music_bar.png|720 × 116|灰色音乐栏底板|

按 1024 × 600 仪表尺寸制作；小狗和文字可在工具中单独定位，避免将整条音乐栏做成一张图片。歌名、歌手和歌词需要动态文字控件，示例仅供排版参考。默认唱片可在收到真实专辑封面后替换。

在 JieLiUI 的图片资源 `music` 目录可查看这些独立资源。页面中的 `yinyuerongqi` 包含封面、歌名、歌词、小狗和播放按钮，`lanyalianjietubiao` 是顶部蓝牙图标。歌名、歌词和蓝牙状态目前均为静态示例；播放和暂停图片位于同一位置，默认显示暂停。原草地、云朵和原小狗控件已隐藏并保留。

工具导出的实际页面预览：`ui_lvgl/dashboard_1024x600_v1/music-player-layout.png`。这张整页图只用于查看排版，实际页面由独立控件构成。
