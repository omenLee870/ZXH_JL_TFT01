/**
* 注意点：
* 0.此文件变化，在工具端会自动同步修改到工具配置中
* 1.功能块通过【---------xxx------】和 【#endif // xxx 】，是工具识别的关键位置，请勿随意改动
* 2.目前工具暂不支持非文件已有的C语言语法，此文件应使用文件内已有的语法增加业务所需的代码，避免产生不必要的bug
* 3.修改该文件出现工具同步异常或者导出异常时，请先检查文件内语法是否正常
**/


// ------------蓝牙配置.json------------
const int CONFIG_A2DP_DELAY_TIME_SBC  = 300;  //A2DP延时SBC(msec)
const int CONFIG_A2DP_DELAY_TIME_SBC_LO  = 100;  //A2DP低延时SBC(msec)
const int CONFIG_A2DP_DELAY_TIME_AAC  = 300;  //A2DP延时AAC(msec)
const int CONFIG_A2DP_DELAY_TIME_AAC_LO  = 100;  //A2DP低延时AAC(msec)
const int CONFIG_A2DP_ADAPTIVE_MAX_LATENCY  = 550;  //A2DP自适应最大延时(msec)
const int CONFIG_EDR_INIT_TIMEOUT  = 60000;  //超时时间
// ------------蓝牙配置.json------------

// ------------升级配置.json------------
const int CONFIG_UPDATE_STORAGE_DEV_EN  = 1;  //设备升级
const int CONFIG_UPDATE_BLE_TEST_EN  = 0;  //ble蓝牙升级
const int CONFIG_UPDATE_BT_LMP_EN  = 1;  //edr蓝牙升级
// ------------升级配置.json------------