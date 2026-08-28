
/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2023-06-09 11:57:36
 * @LastEditTime: 2024-09-03 17:25:29
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#include "includes.h"
#include "vc30fx_core.h"
#include "vc30fx_core_com.h"

#if TCFG_VC30fx_ENABLE


static const char CORE30Fx_VERSION[] = "core30fx_v0.16";
/* rxgain+pdres ~ level */
struct vc30fx_core_res_table {
    unsigned char level;
    unsigned char pdres;
    unsigned char rxgain;
};
/*
vc30fs max oversample=2^5,rxgain~oversample.
vc30fc max oversample=2^3,rxgain~oversample.
*/
#if VC30Fx_CFG_DEFAULT_PPGGAIN_EN
#define MAX_RES_LEVEL 21
#define MIN_RES_LEVEL 0
#define MAX_CURRENT 0x7f
#define MIN_CURRENT 0
const struct vc30fx_core_res_table vc30fx_res_table[] = {
    /*  level, pdres, rxgain */
    {0, 0, 1}, /*200K*/
    {1, 1, 1},
    {2, 2, 1}, /*400K*/
    {3, 3, 1},
    {4, 4, 1}, /*800K*/
    {5, 5, 1},
    {6, 6, 1}, /*1.6M*/
    {7, 7, 1},
    {8, 8, 1}, /*3.2M*/
    {9, 9, 1},
    {10, 10, 1}, /*6.4M*/
    {11, 11, 1}, /*8.96M*/
    {12, 12, 1},
    {13, 13, 1}, /*17.92*/
    {14, 12, 2},
    {15, 13, 2}, /*35.84*/
    {16, 12, 3},
    {17, 13, 3}, /*71.68*/
    {18, 12, 4},
    {19, 13, 4}, /* 143.36 */
    {20, 12, 5},
    {21, 13, 5}, /* 286.72 */
};
#else
#define MAX_RES_LEVEL 23
#define MIN_RES_LEVEL 0
#define MAX_CURRENT 0x7f
#define MIN_CURRENT 0
const struct vc30fx_core_res_table vc30fx_res_table[] = {
    /*  level, pdres, rxgain */
    {0, 0, 0}, /*100K*/
    {1, 1, 0},
    {2, 2, 0}, /*200K*/
    {3, 3, 0},
    {4, 4, 0}, /*400K*/
    {5, 5, 0},
    {6, 6, 0}, /*800K*/
    {7, 7, 0},
    {8, 8, 0}, /*1.6M*/
    {9, 9, 0},
    {10, 10, 0}, /*3.2M*/
    {11, 11, 0},
    {12, 12, 0}, /*6.4M*/
    {13, 13, 0}, /*8.96M*/
    {14, 12, 1},
    {15, 13, 1}, /*17.92*/
    {16, 12, 2},
    {17, 13, 2}, /*35.84*/
    {18, 12, 3},
    {19, 13, 3}, /*71.68*/
    {20, 12, 4},
    {21, 13, 4}, /* 143.36 */
    {22, 12, 5},
    {23, 13, 5}, /* 286.72 */
};
#endif

CORE30Fx_FUNC_RAM_SECTION static unsigned char get_level_by_res_rxgain(unsigned char pdres, unsigned char rxgain)
{
    unsigned char index = 0;
    for (index = 0; index < sizeof(vc30fx_res_table) / sizeof(vc30fx_res_table[0]); index++) {
        if ((vc30fx_res_table[index].pdres == pdres) && (vc30fx_res_table[index].rxgain == rxgain)) {
            return vc30fx_res_table[index].level;
        }
    }
    return 0;
}
CORE30Fx_FUNC_RAM_SECTION static int get_res_rxgain_by_level(unsigned char level, unsigned char *ppdres, unsigned char *prxgain)
{
    *ppdres = vc30fx_res_table[level].pdres;
    *prxgain = vc30fx_res_table[level].rxgain;
    return 0;
}
/****************************************************************
 * CORE_API.read()
 * read IC event or sample data
 ***************************************************************/
CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_software_version(char **ppver)
{
    if (ppver) {
        *ppver = (char *)CORE30Fx_VERSION;
        return 0;
    }
    return -1;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_id(unsigned char *pid1, unsigned char *pid2) //
{
    int ret = 0;
    if ((NULL == pid1) || (NULL == pid2)) {
        return 1;
    }
    ret = vc30fx_read_register(CORE30Fx_DEV_VER_ID_H_ADDR, pid1, 1);
    ret = vc30fx_read_register(CORE30Fx_FDY_EXT_ID_L_ADDR, pid2, 1);
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_event_status(vc30fx_sample_info_t *psample_ret, unsigned char *pevt, unsigned char *povsta, unsigned char *pwindex_bak, unsigned char *pwindex_rel)
{
    int ret = 0;
    unsigned char reg[4] = {0};
    if (!psample_ret || !pevt || !povsta || !pwindex_bak || !pwindex_rel) {
        return -1;
    }
    if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID0 || psample_ret->ic_FICV == VC30FC_FDY_VER_ID0) {
        ret = vc30fx_read_register(CORE30Fx_WORK_STATUS1_ADDR, reg, 3);
        *pevt = reg[0];
        *povsta = reg[1];
        *pwindex_bak = reg[2];
    } else if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID1 || psample_ret->ic_FICV == VC30FC_FDY_VER_ID1) {
        ret = vc30fx_read_register(CORE30Fx_WORK_STATUS1_ADDR, reg, 4);
        *pevt = reg[0];
        *povsta = reg[1];
        *pwindex_bak = reg[2];
        *pwindex_rel = reg[3];
    }
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_rcosc_tick(vc30fx_sample_info_t *psample_ret, unsigned short int *ptick0, unsigned short int *ptick1)
{
    int ret = 0;
    unsigned char reg[4] = {0};
    if (!psample_ret) {
        return -1;
    }
    if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID0 || psample_ret->ic_FICV == VC30FC_FDY_VER_ID0) {
        ret = vc30fx_read_register(0x05, reg, 4);
    } else if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID1 || psample_ret->ic_FICV == VC30FC_FDY_VER_ID1) {
        ret = vc30fx_read_register(0x06, reg, 4);
    }
    if (ptick0) {
        *ptick0 = (reg[0] << 8) + reg[1];
    }
    if (ptick1) {
        *ptick1 = (reg[2] << 8) + reg[3];
    }
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_output(vc30fx_sample_info_t *psample_ret)
{
    int ret = 0;
    unsigned char read_addr = 0x09;
    if (!psample_ret) {
        return -1;
    }
    if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID0 || psample_ret->ic_FICV == VC30FC_FDY_VER_ID0) {
        read_addr = 0x09;
    } else if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID1 || psample_ret->ic_FICV == VC30FC_FDY_VER_ID1) {
        read_addr = 0x0A;
    }
    if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FS) {
        core30fx_reg_result_t reg_result = {0};
        ret = vc30fx_read_register(read_addr, (unsigned char *)&reg_result, sizeof(core30fx_reg_result_t));
        psample_ret->slot_result[0].env_data = (reg_result.slot0_envh << 8) | reg_result.slot0_envl;
        psample_ret->slot_result[1].env_data = (reg_result.slot1_envh << 8) | reg_result.slot1_envl;
        psample_ret->slot_result[2].env_data = (reg_result.slot2_envh << 8) | reg_result.slot2_envl;

        psample_ret->slot_result[0].u.ps_data = reg_result.slot0_ps;
        psample_ret->slot_result[1].u.ps_data = reg_result.slot1_ps;
        psample_ret->slot_result[2].u.ps_data = reg_result.slot2_ps;

        psample_ret->extra_result.bioinn_data = (reg_result.bioinn_valh << 8) | reg_result.bioinn_vall;
        psample_ret->extra_result.bioext_data = (reg_result.bioext_valh << 8) | reg_result.bioext_vall;
        psample_ret->extra_result.tempinn_data = (reg_result.tempinn_valh << 8) | reg_result.tempinn_vall;
        psample_ret->extra_result.tempext_data = (reg_result.tempext_valh << 8) | reg_result.tempext_vall;
        psample_ret->extra_result.tempref_data = (reg_result.tempref_valh << 8) | reg_result.tempref_vall;
    } else if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FC) {
        core30fc_reg_result_t reg_result = {0};
        ret = vc30fx_read_register(read_addr, (unsigned char *)&reg_result, sizeof(core30fc_reg_result_t));
        psample_ret->slot_result[0].env_data = reg_result.slot0_envl;
        psample_ret->slot_result[1].env_data = reg_result.slot1_envl;
        psample_ret->slot_result[2].env_data = reg_result.slot2_envl;

        psample_ret->slot_result[0].u.ps_data = reg_result.slot0_ps;
        psample_ret->slot_result[1].u.ps_data = reg_result.slot1_ps;
        psample_ret->slot_result[2].u.ps_data = reg_result.slot2_ps;
    }
    return ret;
}

/*-------------------------------------------------------------------------------------------------------------------------------------
fifo 解析数据相关的函数功能模块
-------------------------------------------------------------------------------------------------------------------------------------*/
CORE30Fx_FUNC_RAM_SECTION static inline int fifo_calc_sample_slot_nums(vc30fx_sample_info_t *psample_ret, unsigned char *p_ppgfreq_slotnum, unsigned char *p_psfreq_slotnum)
{
    /* 1 ppg slot infomation */
    if (psample_ret->slot_result[0].slot_enwork == 1) {
        (*p_ppgfreq_slotnum)++;
    }
    if (psample_ret->slot_result[1].slot_enwork == 1) {
        (*p_ppgfreq_slotnum)++;
    }
    /* 2 ps slot infomation */
    if ((psample_ret->slot_result[2].slot_enwork == 2) && (psample_ret->slot_result[2].slot_storefifo == VC30Fx_IN_STORE)) {
        (*p_psfreq_slotnum)++;
    }
    /* 3 bio slot infomation */
    if ((psample_ret->extra_result.bioext_enwork == 1 || psample_ret->extra_result.bioinn_enwork == 1) && (psample_ret->extra_result.biodata_storefifo == VC30Fx_IN_STORE)) {
        if (psample_ret->extra_result.bioext_enwork == 1) {
            (*p_psfreq_slotnum)++;
        }
        if (psample_ret->extra_result.bioinn_enwork == 1) {
            (*p_psfreq_slotnum)++;
        }
    }
    CORE30Fx_DEBUG_LOG("slotnum_ppgfreq[%d],slotnum_psfreq[%d]", *p_ppgfreq_slotnum, *p_psfreq_slotnum);
    return 0;
}

CORE30Fx_FUNC_RAM_SECTION static inline int read_data_from_fifo_buffer(const unsigned char *pbuffer, unsigned char *poffset)
{
    int data = 0;
    data = (pbuffer[(*poffset)] << 8) + pbuffer[(*poffset) + 1];
    *poffset += 2;
    return data;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_fifo_data(vc30fx_sample_info_t *psample_ret, unsigned char *pread_fifoaddr, unsigned char read_windex) //
{
    int ret = 0;
    unsigned char i = 0, j = 0;
    unsigned char rindex = *pread_fifoaddr;
    unsigned char read_size = 0, wait_size = 0;
    unsigned char fifo_data[128] = {0};
    /* fifo有效通道数量 */
    unsigned char ppg_freq_slot_num = 0; /* ppg频率下ppg_slot的数量*/
    unsigned char ps_freq_slot_num = 0;	 /* ps频率下psbio_slot的数量*/
    unsigned char ppg_ps_fifo_ratio = 0; /* ppg频率与ps频率的占比，用于计算有效ps频率的fifo数据位置 */

    unsigned char fifo_bytes = 0; /* fifo_bytes= group_len*fifo_div */
    unsigned char group_len = 0; /* ppg0(slot1,2) ppg1(slot1,2) ps(1,2) */
    unsigned char group_num = 0; /* group_len * group_num = fifosize */
    /* array(ppg_freq+ps_freq) */
    unsigned char ppg_freq_array_len = 0;
    unsigned char ps_freq_array_len = 0;
    unsigned char array_offset = 0; /* group 组内偏移 */
    unsigned char slot_index = 0;
    /* addr_max，vc30fs=0x80,vc30fc=0x60 */
    unsigned char addr_max = 0; /* 解析地址翻转值，VC30FS=0x80,VC30FC=0x60 */
    /************* 1.获取配置参数 *************/
    if (!psample_ret) {
        return -1;
    }
    if (VC30Fx_IC_TYPE_VC30FS == psample_ret->ic_type) {
        addr_max = 0x80;
    } else {
        addr_max = 0x60;
    }
    CORE30Fx_DEBUG_LOG("psample_ret=%x addr_max=%d read_windex=%d", psample_ret, addr_max, read_windex);
    if (read_windex > addr_max) {
        return -1;
    }
    ppg_ps_fifo_ratio = psample_ret->ps_div;										/* 假设25/10=2.5hz，ppg频率中间隔10个，插入一个ps */
    fifo_calc_sample_slot_nums(psample_ret, &ppg_freq_slot_num, &ps_freq_slot_num); /* 获取各个fifo信息 */
    /* calc ppgfreq_fifodata/psfreq_fifodata array length, group length=ppg*ratio+ps  */
    ppg_freq_array_len += (ppg_freq_slot_num * 2);
    ps_freq_array_len += (ps_freq_slot_num * 2);
    group_len = ppg_freq_array_len * ppg_ps_fifo_ratio + ps_freq_array_len;
    fifo_bytes = group_len * psample_ret->fifo_div;
    CORE30Fx_DEBUG_LOG("group_len(%d)=(%d*%d)+%d", group_len, ppg_freq_array_len, ppg_ps_fifo_ratio, ps_freq_array_len);
    if (group_len == 0 || fifo_bytes == 0) {
        return -1;
    }
    /************** 2.校验参数读取 *************/
    /* group=[ ppg0(slot0,slot1)+ppg1(slot0,slot1)+...ppgN(slot0,slot1)+ps(ps,bioinn,bioext) ]
       fifo =[ group0,group1... ]n */
    if (0 == psample_ret->fifo_enable) {
        CORE30Fx_DEBUG_LOG("psample_ret->fifo_enable=%d", psample_ret->fifo_enable);
        rindex = 0x00; /*disable fifo:fifo=ps频率，数据始于,0x00 */
    }
    if (read_windex == rindex) { /* 数据溢出或则无数据 */
        return -2;
    }
    if (read_windex > rindex) { /* rindex-windex */
        wait_size = (read_windex - rindex);
    } else { /* [rindex-addr_max]+[0-windex] */
        wait_size = addr_max - rindex + read_windex;
    }
    if (wait_size < fifo_bytes) {
        CORE30Fx_DEBUG_ERR("[%x~%x]waite_size=%d, fifo_bytes=%d", rindex, read_windex, wait_size, fifo_bytes);
        return -3;
    }
    /* windex error process, the windex cover all fifo area and more than the rindex. */
    if (wait_size % fifo_bytes) {
        read_size = (wait_size / fifo_bytes) * fifo_bytes;
        CORE30Fx_DEBUG_ERR("waite_read_fifo_bytes(%d)/fifo_bytes(%d) != 0 read_size[%d]", wait_size, fifo_bytes, read_size);
        ret = 1;
        /* 如果是新版带windex_bak的,一定是错位了，直接返回，不再读取错误的fifo */
        if (VC30FS_FDY_VER_ID1 == psample_ret->ic_FICV || VC30FC_FDY_VER_ID1 == psample_ret->ic_FICV) {
            return ret;
        }
    } else {
        read_size = wait_size;
    }
    *pread_fifoaddr = ((rindex + read_size) >= addr_max) ? (rindex + read_size - addr_max) : (rindex + read_size);
    vc30fx_write_register(CORE30Fx_FIFO_READADDR_INDEX, &rindex, 1);
    if (read_size > sizeof(fifo_data)) {
        return -4;
    }
    vc30fx_read_register(CORE30Fx_FIFO_DATA_ADDR, &fifo_data[0], read_size);

    CORE30Fx_DEBUG_LOG("[%x]-[%x],fifo_bytes=%d,", rindex, read_windex, read_size);
    CORE30Fx_DEBUG_LOG("PPG_data[0- 8]:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x", fifo_data[0], fifo_data[1], fifo_data[2], fifo_data[3], fifo_data[4], fifo_data[5], fifo_data[6], fifo_data[7]);
    CORE30Fx_DEBUG_LOG("PPG_data[9-16]:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x", fifo_data[8], fifo_data[9], fifo_data[10], fifo_data[11], fifo_data[12], fifo_data[13], fifo_data[14], fifo_data[15]);
    CORE30Fx_DEBUG_LOG("..............:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x", fifo_data[read_size - 8], fifo_data[read_size - 7], fifo_data[read_size - 6], fifo_data[read_size - 5], fifo_data[read_size - 4], fifo_data[read_size - 3], fifo_data[read_size - 2], fifo_data[read_size - 1]);
    /******** 3.group,array,cell analysis ********/
    group_num = read_size / group_len;
    for (i = 0; i < group_num; i++) { /* i=0~N，每次所有通道的采样PPG群数，每个群的处理 */
        array_offset = i * group_len;
        /* ppg_freq array analysis*/
        for (j = 0; j < ppg_ps_fifo_ratio; j++) { /* j=0-n,每个群里面拥有的ppg_freq的组数轮询 */
            /* 3.2.1.1 ppg_ppg_freq_cell 解析 */
            for (slot_index = 0; slot_index < 2; slot_index++) {
                if (1 == psample_ret->slot_result[slot_index].slot_enwork && VC30Fx_FREQ_PPG == psample_ret->slot_result[slot_index].slot_freq_sel) {
                    if (i * ppg_ps_fifo_ratio + j >= VC30Fx_PPGSLOT_BUFFER_NUM || array_offset >= 127) {
                        return 2;
                    }
                    psample_ret->slot_result[slot_index].ppg_num += 1;
                    psample_ret->slot_result[slot_index].ppg_buffer[i * ppg_ps_fifo_ratio + j] = read_data_from_fifo_buffer(fifo_data, &array_offset);
                    CORE30Fx_DEBUG_LOG("slot_result[%d].ppg_data[%d]=%d", slot_index, i * ppg_ps_fifo_ratio + j, psample_ret->slot_result[slot_index].ppg_buffer[i * ppg_ps_fifo_ratio + j]);
                }
            }
        }
        /* ps_freq array analysis */
        /* 数据交换处理 */
        if (psample_ret->extra_result.biomode_work == 0) {
            if (2 == psample_ret->slot_result[2].slot_enwork && VC30Fx_IN_STORE == psample_ret->slot_result[2].slot_storefifo) {
                if (i >= VC30Fx_PS2SLOT_BUFFER_NUM || array_offset >= 127) {
                    return 2;
                }
                psample_ret->slot_result[2].ppg_num += 1;
                psample_ret->slot_result[2].ppg_buffer[i] = read_data_from_fifo_buffer(fifo_data, &array_offset);
                CORE30Fx_DEBUG_LOG("slot_result[2].ps_data[%d]=%d", i, psample_ret->slot_result[2].ppg_buffer[i]);
            }
            if ((1 == psample_ret->extra_result.bioinn_enwork || 1 == psample_ret->extra_result.bioext_enwork) && VC30Fx_IN_STORE == psample_ret->extra_result.biodata_storefifo) {
                if (i >= VC30Fx_PS2SLOT_BUFFER_NUM || array_offset >= 127) {
                    return 2;
                }
                psample_ret->extra_result.bio_fifo_num += 1;
                if (1 == psample_ret->extra_result.bioinn_enwork) {
                    psample_ret->extra_result.bioinn_buffer[i] = read_data_from_fifo_buffer(fifo_data, &array_offset);
                    CORE30Fx_DEBUG_LOG("result.bioinn_buffer[%d]=%d", i, psample_ret->extra_result.bioinn_buffer[i]);
                }
                if (i >= VC30Fx_PS2SLOT_BUFFER_NUM || array_offset >= 127) {
                    return 2;
                }
                if (1 == psample_ret->extra_result.bioext_enwork) {
                    psample_ret->extra_result.bioext_buffer[i] = read_data_from_fifo_buffer(fifo_data, &array_offset);
                    CORE30Fx_DEBUG_LOG("result.bioext_buffer[%d]=%d", i, psample_ret->extra_result.bioext_buffer[i]);
                }
            }
        } else {
            if ((1 == psample_ret->extra_result.bioinn_enwork || 1 == psample_ret->extra_result.bioext_enwork) && VC30Fx_IN_STORE == psample_ret->extra_result.biodata_storefifo) {
                if (i >= VC30Fx_PS2SLOT_BUFFER_NUM || array_offset >= 127) {
                    return 2;
                }
                psample_ret->extra_result.bio_fifo_num += 1;
                if (1 == psample_ret->extra_result.bioinn_enwork) {
                    psample_ret->extra_result.bioinn_buffer[i] = read_data_from_fifo_buffer(fifo_data, &array_offset);
                    CORE30Fx_DEBUG_LOG("result.bioinn_buffer[%d]=%d", i, psample_ret->extra_result.bioinn_buffer[i]);
                }
                if (i >= VC30Fx_PS2SLOT_BUFFER_NUM || array_offset >= 127) {
                    return 2;
                }
                if (1 == psample_ret->extra_result.bioext_enwork) {
                    psample_ret->extra_result.bioext_buffer[i] = read_data_from_fifo_buffer(fifo_data, &array_offset);
                    CORE30Fx_DEBUG_LOG("result.bioext_buffer[%d]=%d", i, psample_ret->extra_result.bioext_buffer[i]);
                }
            }
            if (i >= VC30Fx_PS2SLOT_BUFFER_NUM || array_offset >= 127) {
                return 2;
            }
            if (2 == psample_ret->slot_result[2].slot_enwork && VC30Fx_IN_STORE == psample_ret->slot_result[2].slot_storefifo) {
                psample_ret->slot_result[2].ppg_num += 1;
                psample_ret->slot_result[2].ppg_buffer[i] = read_data_from_fifo_buffer(fifo_data, &array_offset);
                CORE30Fx_DEBUG_LOG("slot_result[2].ps_data[%d]=%d", i, psample_ret->slot_result[2].ppg_buffer[i]);
            }
        }
    } /* end of for(i<group_num), all fifodata */
    return ret;
}

/****************************************************************
 * CORE_API.write()/set()
 * slot configuration,adjust current,resistance and sampleTime
 * write or set config registers
 ***************************************************************/
CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_slot_enable_config(VC30Fx_PPG_SLOT_I slot_index, VC30Fx_CFG_FUNC config_enable)
{
    int ret = 0;
    core30fx_slot_config_t slotcfg = {0};
    unsigned char cfg0_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + CORE30Fx_CONFIG_SLOT_SIZE * slot_index;
    ret = vc30fx_read_register(cfg0_addr, (unsigned char *)&slotcfg.slot_cfg0.byte, 1);
    slotcfg.slot_cfg0.bits.enable = config_enable;
    ret = vc30fx_write_register(cfg0_addr, (unsigned char *)&slotcfg.slot_cfg0.byte, 1);
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_slot_fix_param_config(VC30Fx_PPG_SLOT_I slot_index, unsigned char rxgain, VC30Fx_PPG_TXCH_I txslot, VC30Fx_CFG_FUNC deps_en,
        unsigned char oversample_psdirturb, VC30Fx_SAMPLE_TIM sample_tim, unsigned char ic_type, unsigned char ic_ficv)
{
    int ret = 0;
    core30fx_slot_config_t slotcfg = {0};
    unsigned char slot_scx = 0;
    unsigned char slot_base_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + CORE30Fx_CONFIG_SLOT_SIZE * slot_index;
    ret = vc30fx_read_register(slot_base_addr, (unsigned char *)&slotcfg, sizeof(core30fx_slot_config_t));
    slotcfg.slot_cfg0.bits.rxgain = rxgain;
    slotcfg.slot_cfg1.bits.afech = txslot;
    if (VC30Fx_PPG_SLOT_2 == slot_index) {
        if (VC30Fx_IC_TYPE_VC30FC == ic_type && deps_en) {
            slotcfg.slot_cfg0.ps_bits.deps_enable  = VC30Fx_CFG_ENABLE;
            slotcfg.slot_cfg0.ps_bits.deps_dirturb = (oversample_psdirturb > 7) ? 7 : oversample_psdirturb;
        } else {
            if (ic_ficv == VC30FS_FDY_VER_ID0) {
                oversample_psdirturb = deps_en ? oversample_psdirturb : 0;
                slotcfg.slot_cfg0.ps_bits.deps_dirturb = (oversample_psdirturb > 3) ? 3 : oversample_psdirturb;
            } else if (ic_ficv == VC30FS_FDY_VER_ID1) {
                slotcfg.slot_cfg0.ps_bits.deps_enable  = (deps_en) ? 0 : 1; /* 20240719-Necchip(晶合)版本的30fs,为了向前兼容，0为使能，1为失能 */
                slotcfg.slot_cfg0.ps_bits.deps_dirturb = (oversample_psdirturb > 7) ? 7 : oversample_psdirturb;
            }
        }
    } else {
        /* vc30fc,oversample max=2^5=32;vc30fs,oversample max=2^7=128; */
        if (VC30Fx_IC_TYPE_VC30FC == ic_type) {
            slotcfg.slot_cfg0.bits.oversample = (oversample_psdirturb >= 3) ? 3 : oversample_psdirturb;
        } else {
            slotcfg.slot_cfg0.bits.oversample = (oversample_psdirturb >= 7) ? 7 : oversample_psdirturb;
        }
    }
    switch (sample_tim) {
    case VC30Fx_SAMPLE_TIM_50US:
        slotcfg.slot_cfg1.bits.env_time = 2;
        slotcfg.slot_cfg1.bits.sample_time = 0;
        slot_scx = 2;
        break;
    case VC30Fx_SAMPLE_TIM_100US:
        slotcfg.slot_cfg1.bits.env_time = 5;
        slotcfg.slot_cfg1.bits.sample_time = 1;
        slot_scx = 5;
        break;
    case VC30Fx_SAMPLE_TIM_150US:
        slotcfg.slot_cfg1.bits.env_time = 8;
        slotcfg.slot_cfg1.bits.sample_time = 2;
        slot_scx = 8;
        break;
    case VC30Fx_SAMPLE_TIM_200US:
        slotcfg.slot_cfg1.bits.env_time = 11;
        slotcfg.slot_cfg1.bits.sample_time = 3;
        slot_scx = 11;
        break;
    default:
        slotcfg.slot_cfg1.bits.env_time = 5;
        slotcfg.slot_cfg1.bits.sample_time = 1;
        slot_scx = 5;
        break;
    }
    slotcfg.slot_cfg3.bits.sc = slot_scx;
    ret = vc30fx_write_register(slot_base_addr, (unsigned char *)&slotcfg, sizeof(core30fx_slot_config_t));
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_psslot_storefifo_config(VC30Fx_STORE_FIFO infifo)
{
    int ret = 0;
    core30fx_slot_config_t slotcfg = {0};
    ret = vc30fx_read_register(0x49, (unsigned char *)&slotcfg.slot_cfg1, 1);
    slotcfg.slot_cfg1.ps_bits.storefifo = infifo;
    ret = vc30fx_write_register(0x49, (unsigned char *)&slotcfg.slot_cfg1, 1);
    return ret;
}

/* PPG通道及调整的函数接口 */
CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_slot_adjust_param(unsigned char *pmax_res, unsigned char *pmin_res, unsigned char *pmax_curr, unsigned char *pmin_curr)
{
    *pmax_res = MAX_RES_LEVEL;
    *pmin_res = MIN_RES_LEVEL;
    *pmax_curr = MAX_CURRENT;
    *pmin_curr = MIN_CURRENT;
    return 0;
}

/* 电流+电阻等级的读写,这部分是在使用过程中会反复调用的 */
CORE30Fx_FUNC_RAM_SECTION int __core30fx_read_slot_current_res(VC30Fx_PPG_SLOT_I slot_index, unsigned char *pslot_curr, unsigned char *pslot_reslvl)
{
    int ret = 0;
    core30fx_slot_config_t slotcfg = {0};
    /* read current */
    unsigned char cfg2_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + (slot_index * CORE30Fx_CONFIG_SLOT_SIZE) + 2;
    ret = vc30fx_read_register(cfg2_addr, (unsigned char *)&slotcfg.slot_cfg2, 1);
    if (pslot_curr) {
        *pslot_curr = slotcfg.slot_cfg2.bits.current;
    }
    /* read reslvl */
    unsigned char cfg0_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + (slot_index * CORE30Fx_CONFIG_SLOT_SIZE);
    unsigned char cfg3_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + (slot_index * CORE30Fx_CONFIG_SLOT_SIZE) + 3;
    vc30fx_read_register(cfg0_addr, (unsigned char *)&slotcfg.slot_cfg0, 1);
    vc30fx_read_register(cfg3_addr, (unsigned char *)&slotcfg.slot_cfg3, 1);
    if (pslot_reslvl) {
        *pslot_reslvl = slotcfg.slot_cfg3.bits.resistance;
        *pslot_reslvl = get_level_by_res_rxgain(slotcfg.slot_cfg3.bits.resistance, slotcfg.slot_cfg0.bits.rxgain);
    }
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_ppgslot_current(VC30Fx_PPG_SLOT_I slot_index, unsigned char current)
{
    int ret = 0;
    core30fx_slot_config_t slotcfg = {0};
    unsigned char cfg2_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + (slot_index * CORE30Fx_CONFIG_SLOT_SIZE) + 2;
    slotcfg.slot_cfg2.bits.current = (current >= MAX_CURRENT) ? MAX_CURRENT : current;
    ret = vc30fx_write_register(cfg2_addr, (unsigned char *)&slotcfg.slot_cfg2, 1);
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_ppgslot_resistance(VC30Fx_PPG_SLOT_I slot_index, unsigned char res_level, unsigned char allow_oversample, unsigned char type)  //
{
    int ret = 0;
    unsigned char res = 0, rxgain = 0;
    core30fx_slot_config_t slotcfg = {0};
    unsigned char cfg0_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + (slot_index * CORE30Fx_CONFIG_SLOT_SIZE);
    unsigned char cfg3_addr = CORE30Fx_CONFIG_SLOT_ADDR_BASE + (slot_index * CORE30Fx_CONFIG_SLOT_SIZE) + 3;
    ret = vc30fx_read_register(cfg0_addr, (unsigned char *)&slotcfg.slot_cfg0, 1);
    ret = vc30fx_read_register(cfg3_addr, (unsigned char *)&slotcfg.slot_cfg3, 1);
    res_level = (res_level >= MAX_RES_LEVEL) ? MAX_RES_LEVEL : res_level;
    get_res_rxgain_by_level(res_level, &res, &rxgain);
    slotcfg.slot_cfg0.bits.rxgain = rxgain;
    slotcfg.slot_cfg3.bits.resistance = res;
#if 1
    if (slot_index <= 1 && allow_oversample) {
#if VC30Fx_CFG_DEFAULT_PPGGAIN_EN
        slotcfg.slot_cfg0.bits.oversample = 1 * (rxgain - 1);
#else
        slotcfg.slot_cfg0.bits.oversample = 1 * rxgain;
#endif
        if (VC30Fx_IC_TYPE_VC30FC == type) {
            slotcfg.slot_cfg0.bits.oversample = (slotcfg.slot_cfg0.bits.oversample >= 3) ? (3) : (slotcfg.slot_cfg0.bits.oversample);
        }
    }
#endif
    ret = vc30fx_write_register(cfg0_addr, (unsigned char *)&slotcfg.slot_cfg0, 1);
    ret = vc30fx_write_register(cfg3_addr, (unsigned char *)&slotcfg.slot_cfg3, 1);
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_common_config(vc30fx_sample_info_t *psample_ret, VC30Fx_CFG_FUNC deenv_en, VC30Fx_CFG_FUNC fifo_en, unsigned char adcspeed, unsigned char cpd)
{
    int ret = 0;
    core30fx_com_cfgbit com_config = {0};
    com_config.deenv_en = deenv_en;
    com_config.fifo_en = fifo_en;
    com_config.adcspeed = adcspeed;
    com_config.cpd = cpd;
    if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FC) {
        if (psample_ret->ic_FICV == VC30FC_FDY_VER_ID0) {
            com_config.com_bit = 0b00;    /* [reserved : env_clkdiv ] */
        } else if (psample_ret->ic_FICV == VC30FC_FDY_VER_ID1) {
            com_config.com_bit = 0b00;    /* [pesel]:00-inn_PD(A+B), 01-inn_PDA, 10-inn_PDB, 11-ext_PD */
        }
    } else if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FS) {
        if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID0) {
            com_config.com_bit = 0b10;
        } else if (psample_ret->ic_FICV == VC30FS_FDY_VER_ID1) {
            com_config.com_bit = 0b00;    /* undefine */
        }
    }
    ret = vc30fx_write_register(0x4C, (unsigned char *)&com_config, sizeof(com_config));
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_sample_frequency(vc30fx_sample_info_t *psample_ret)
{
    int ret = 0;
    unsigned char freq_config[4] = {0};
    if (!psample_ret) {
        return -1;
    }
    freq_config[0] = ((psample_ret->ppg_div - 1) >> 8);
    freq_config[1] = ((psample_ret->ppg_div - 1) & 0xFF);
    freq_config[2] = psample_ret->ps_div - 1;
    if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FC && psample_ret->ic_FICV == VC30FC_FDY_VER_ID1) {
        freq_config[3] = (0 << 6) | ((psample_ret->fifo_div - 1) & 0x3f); /* VC30FC-12S_038, [7]~EnvConvertClkDiv */
    } else if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FS && psample_ret->ic_FICV == VC30FS_FDY_VER_ID1) {
        freq_config[3] = (0 << 6) | ((psample_ret->fifo_div - 1) & 0x3f); /* VC30FS */
    } else {
        freq_config[3] = (psample_ret->fifo_div - 1) & 0x3f;
    }
    ret = vc30fx_write_register(0x4D, freq_config, sizeof(freq_config));
    return ret;
}
CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_div_frequency(unsigned char div_index, unsigned short int div)
{
    int ret = 0;
    unsigned char div1_config[2] = {0};
    unsigned char div2 = 0;
    unsigned char div3 = 0;
    if (div_index == 1) {
        div1_config[0] = (div >> 8);
        div1_config[1] = (div & 0xff);
        ret = vc30fx_write_register(0x4D, div1_config, 2);
    } else if (div_index == 2) {
        div2 = (unsigned char)div;
        ret = vc30fx_write_register(0x4F, &div2, 1);
    } else if (div_index == 3) {
        div3 = (unsigned char)div;
        ret = vc30fx_write_register(0x50, &div3, 1);
    } else {
        return 1;
    }
    return ret;
}

/****************************************************************
 * CORE_API.write()/set()
 * txchx_comconfig & ppg threshold configuration
 * write or set config registers
 ***************************************************************/
CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_threshold_config(unsigned char ppgth_h, unsigned char ppgth_l)
{
    int ret = 0;
    core30fx_ppgth_cfgbit ppgth_config = {0};
    ppgth_config.ppgthh = (ppgth_h >= 0xf) ? 0xf : ppgth_h;
    ppgth_config.ppgthl = (ppgth_l >= 0xf) ? 0xf : ppgth_l;
    ret = vc30fx_write_register(0x51, (unsigned char *)&ppgth_config, 1);
    return ret;
}
/****************************************************************
 * CORE_API.write()/set()
 * extra(ADC,BIO,temperature,AT/DT) configuration
 * write or set config registers
 ***************************************************************/
CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_interrupt_config(VC30Fx_IO_MODE int_mode, VC30Fx_IO_DIR int_dir, VC30Fx_IO_PULSE_TIM int_len, unsigned char evt_en)
{
    int ret = 0;
    core30fx_int_cfgbit int_config = {0};
    int_config.intmode = (unsigned char)int_mode;
    int_config.intdir = (unsigned char)int_dir;
    int_config.intlen = (unsigned char)int_len;
    if (evt_en & VC30Fx_EN_EVT_FREQ2) {
        int_config.intfreq2_en = 1;
    }
    if (evt_en & VC30Fx_EN_EVT_FIFO) {
        int_config.intfifo_en = 1;
    }
    if (evt_en & VC30Fx_EN_EVT_OVLD) {
        int_config.intov_en = 1;
    }
    if (evt_en & VC30Fx_EN_EVT_PPG) {
        int_config.intppg_en = 1;
    }
    ret = vc30fx_write_register(0x52, (unsigned char *)&int_config, sizeof(int_config));
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_bioslot_config(VC30Fx_CFG_FUNC bioinn_en, VC30Fx_CFG_FUNC bioext_en, VC30Fx_STORE_FIFO biofifo_store, VC30Fx_CFG_FUNC biomode, unsigned char biofreq, unsigned char biocap, unsigned char biotime) //
{
    int ret = 0;
    core30fx_bio_cfg0bit bio_config0 = {0};
    core30fx_bio_cfg1bit bio_config1 = {0};
    bio_config0.bio_inn_en = bioinn_en;
    bio_config0.bio_ext_en = bioext_en;
    bio_config0.bio_infifo = biofifo_store;
    bio_config0.bio_mode = biomode;

    bio_config0.bio_freq = (biofreq > 3) ? 3 : biofreq;

    bio_config1.bio_capadd = (biocap > 0x1f) ? (0x1f) : biocap;
    bio_config1.bio_time = (biotime > 0x7) ? (0xf) : biotime;
    ret = vc30fx_write_register(0x53, (unsigned char *)&bio_config0, 1);
    ret = vc30fx_write_register(0x54, (unsigned char *)&bio_config1, 1);
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_bioinn_addcap(unsigned char biocap)
{
    int ret = 0;
    core30fx_bio_cfg1bit bio_config1 = {0};
    vc30fx_read_register(0x54, (unsigned char *)&bio_config1, 1);
    bio_config1.bio_capadd = (biocap > 0x1f) ? (0x1f) : biocap;
    ret = vc30fx_write_register(0x54, (unsigned char *)&bio_config1, 1);
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_temperature_config(VC30Fx_CFG_FUNC tempext_en, VC30Fx_CFG_FUNC tempinn_en, unsigned char current)
{
    int ret = 0;
    core30fx_temp_cfgbit temp_config = {0};
    temp_config.temp_inn_en = tempinn_en;
    temp_config.temp_ext_en = tempext_en;
    temp_config.temp_currset = (current >= 7) ? 7 : current;
    ret = vc30fx_write_register(0x55, (unsigned char *)&temp_config, sizeof(temp_config));
    return ret;
}

CORE30Fx_FUNC_RAM_SECTION int __core30fx_set_rcosc_frequency(unsigned char oscset, unsigned char type)
{
    int ret = 0;
    if (VC30Fx_IC_TYPE_VC30FS == type) {
        ret = vc30fx_write_register(0x56, &oscset, 1);
    } else if (VC30Fx_IC_TYPE_VC30FC == type) {
        ret = vc30fx_write_register(0x53, &oscset, 1);
    }
    return ret;
}
/****************************************************************
 * CORE_API.write()/set()
 * workmode,debug_config
 * write or set config registers
 ***************************************************************/
CORE30Fx_FUNC_RAM_SECTION int __core30fx_write_work_cmd(VC30Fx_WORK_CMD cmd) //
{
    int ret = 0;
    unsigned char wcmd = 0;
    switch (cmd) {
    case VC30Fx_STOP_WORK_CMD:
        wcmd = 0x00;
        break;
    case VC30Fx_START_WORK_CMD:
        wcmd = 0x5A;
        break;
    case VC30Fx_RESET_SOFT_CMD:
        wcmd = 0xA5;
        break;
    default:
        return 1;
    }
    ret = vc30fx_write_register(CORE30Fx_WORK_CMD_ADDR_BASE, &wcmd, 1);
    return ret;
}

#endif//TCFG_VC30fx_ENABLE
