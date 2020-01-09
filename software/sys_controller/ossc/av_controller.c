//
// Copyright (C) 2019  Markus Hiienkari <mhiienka@niksula.hut.fi>
//
// This file is part of Open Source Scan Converter project.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "system.h"
#include "string.h"
#include "altera_avalon_pio_regs.h"
#include "sysconfig.h"
#include "utils.h"
#include "flash.h"
#include "sys/alt_timestamp.h"
#include "i2c_opencores.h"
#include "av_controller.h"
#include "isl51002.h"
#include "pcm1862.h"
#include "us2066.h"
#include "controls.h"
#include "mmc.h"
#include "si5351.h"
#include "adv7513.h"
#include "adv7611.h"
#include "sc_config_regs.h"
#include "video_modes.h"

#define FW_VER_MAJOR 0
#define FW_VER_MINOR 33

//fix PD and cec
#define ADV7513_MAIN_BASE 0x7a
#define ADV7513_EDID_BASE 0x7e
#define ADV7513_PKTMEM_BASE 0x70
#define ADV7513_CEC_BASE 0x60

#define ISL51002_BASE (0x9A>>1)
#define SI5351_BASE (0xC0>>1)

#define ADV7611_IO_BASE 0x98
#define ADV7611_CEC_BASE 0x80
#define ADV7611_INFOFRAME_BASE 0x7c
#define ADV7611_DPLL_BASE 0x4c
#define ADV7611_KSV_BASE 0x64
#define ADV7611_EDID_BASE 0x6c
#define ADV7611_HDMI_BASE 0x68
#define ADV7611_CP_BASE 0x44

#define BLKSIZE 512
#define BLKCNT 2

unsigned char pro_edid_bin[] = {
  0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x36, 0x51, 0x5c, 0x05,
  0x15, 0xcd, 0x5b, 0x07, 0x0a, 0x1d, 0x01, 0x04, 0xa5, 0x3c, 0x22, 0x78,
  0xfb, 0xde, 0x51, 0xa3, 0x54, 0x4c, 0x99, 0x26, 0x0f, 0x50, 0x54, 0xff,
  0xff, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x9e, 0x39, 0x80, 0x18, 0x71, 0x38,
  0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x32, 0x30, 0x35, 0x34, 0x38, 0x31, 0x32,
  0x35, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x2d,
  0x90, 0x0a, 0x96, 0x14, 0x01, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x00, 0x00, 0x00, 0xfc, 0x00, 0x4f, 0x53, 0x53, 0x43, 0x20, 0x50, 0x72,
  0x6f, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x01, 0xac, 0x02, 0x03, 0x11, 0xf4,
  0x44, 0x10, 0x04, 0x03, 0x02, 0x23, 0x09, 0x1f, 0x07, 0x83, 0x01, 0x00,
  0x00, 0xe8, 0xe4, 0x00, 0x50, 0xa0, 0xa0, 0x67, 0x50, 0x08, 0x20, 0xf8,
  0x0c, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a, 0x6f, 0xc2, 0x00, 0xa0, 0xa0,
  0xa0, 0x55, 0x50, 0x30, 0x20, 0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00,
  0x1a, 0x5a, 0xa0, 0x00, 0xa0, 0xa0, 0xa0, 0x46, 0x50, 0x30, 0x20, 0x35,
  0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a, 0xfb, 0x7e, 0x80, 0x88, 0x70,
  0x38, 0x12, 0x40, 0x18, 0x20, 0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00,
  0x1e, 0x86, 0x6f, 0x80, 0xa0, 0x70, 0x38, 0x40, 0x40, 0x30, 0x20, 0x35,
  0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x46
};

isl51002_dev isl_dev = {.i2c_addr = ISL51002_BASE, .xtal_freq = 27000000LU};

si5351_dev si_dev = {.i2c_addr = SI5351_BASE, .xtal_freq = 27000000LU};

adv7611_dev advrx_dev = {.io_base = ADV7611_IO_BASE,
                         .cec_base = ADV7611_CEC_BASE,
                         .infoframe_base = ADV7611_INFOFRAME_BASE,
                         .dpll_base = ADV7611_DPLL_BASE,
                         .ksv_base = ADV7611_KSV_BASE,
                         .edid_base = ADV7611_EDID_BASE,
                         .hdmi_base = ADV7611_HDMI_BASE,
                         .cp_base = ADV7611_CP_BASE,
                         .xtal_freq = 27000000LU,
                         .edid = pro_edid_bin,
                         .edid_len = sizeof(pro_edid_bin)};

adv7513_dev advtx_dev = {.main_base = ADV7513_MAIN_BASE,
                         .edid_base = ADV7513_EDID_BASE,
                         .pktmem_base = ADV7513_PKTMEM_BASE,
                         .cec_base = ADV7513_CEC_BASE};

volatile uint32_t *ddr = (volatile uint32_t*)MEM_IF_MAPPER_0_BASE;
volatile sc_regs *sc = (volatile sc_regs*)SC_CONFIG_0_BASE;

struct mmc *mmc_dev;
struct mmc * ocsdc_mmc_init(int base_addr, int clk_freq);

uint16_t sys_ctrl;

char row1[US2066_ROW_LEN+1];
char row2[US2066_ROW_LEN+1];

static const char *avinput_str[] = { "Test pattern", "AV1: RGBS", "AV1: RGsB", "AV1: YPbPr", "AV1: RGBHV", "AV1: RGBCS", "AV2: YPbPr", "AV2: RGsB", "AV3: RGBHV", "AV3: RGBCS", "AV3: RGsB", "AV3: YPbPr", "AV4", "Last used" };


void update_sc_config(mode_data_t *vm_in, mode_data_t *vm_out, vm_mult_config_t *vm_conf)
{
    h_in_config_reg h_in_config = {.data=0x00000000};
    h_in_config2_reg h_in_config2 = {.data=0x00000000};
    v_in_config_reg v_in_config = {.data=0x00000000};
    misc_config_reg misc_config = {.data=0x00000000};
    sl_config_reg sl_config = {.data=0x00000000};
    sl_config2_reg sl_config2 = {.data=0x00000000};
    h_out_config_reg h_out_config = {.data=0x00000000};
    h_out_config2_reg h_out_config2 = {.data=0x00000000};
    v_out_config_reg v_out_config = {.data=0x00000000};
    v_out_config2_reg v_out_config2 = {.data=0x00000000};

    // Set input params
    h_in_config.h_synclen = vm_in->h_synclen;
    h_in_config.h_backporch = vm_in->h_backporch;
    h_in_config.h_active = vm_in->h_active;
    h_in_config2.h_total = vm_in->h_total;
    v_in_config.v_synclen = vm_in->v_synclen;
    v_in_config.v_backporch = vm_in->v_backporch;
    v_in_config.v_active = vm_in->v_active;

    // Set output params
    h_out_config.h_synclen = vm_out->h_synclen;
    h_out_config.h_backporch = vm_out->h_backporch;
    h_out_config.h_active = vm_out->h_active;
    h_out_config.x_rpt = vm_conf->x_rpt;
    h_out_config2.h_total = vm_out->h_total;
    h_out_config2.x_start = vm_conf->x_start;
    h_out_config2.x_skip = vm_conf->x_skip;
    v_out_config.v_synclen = vm_out->v_synclen;
    v_out_config.v_backporch = vm_out->v_backporch;
    v_out_config.v_active = vm_out->v_active;
    v_out_config.y_rpt = vm_conf->y_rpt;
    v_out_config2.v_total = vm_out->v_total;
    v_out_config2.v_startline = vm_out->v_total-1-vm_conf->y_rpt;

    //printf("%lu %lu %lu %lu %lu %lu\n", ymult*mode->v_synclen, ymult*mode->v_backporch, ymult*mode->v_active, ymult-1, (mode->flags & MODE_INTERLACED) ? mode->v_total : ymult*mode->v_total, (mode->flags & MODE_INTERLACED) ? mode->v_total-2 : ymult*mode->v_total-ymult);

    sc->h_in_config = h_in_config;
    sc->h_in_config2 = h_in_config2;
    sc->v_in_config = v_in_config;
    sc->misc_config = misc_config;
    sc->sl_config = sl_config;
    sc->sl_config2 = sl_config2;
    sc->h_out_config = h_out_config;
    sc->h_out_config2 = h_out_config2;
    sc->v_out_config = v_out_config;
    sc->v_out_config2 = v_out_config2;
}

int init_hw()
{
    int ret;

    // unreset hw
    usleep(400000);
    sys_ctrl = SCTRL_ISL_RESET_N|SCTRL_HDMIRX_RESET_N;
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, sys_ctrl);

    I2C_init(I2CA_BASE,ALT_CPU_FREQ,400000);

    //remap conflicting ADV7513 I2C slave immediately
    //sniprintf(row1, US2066_ROW_LEN+1, "Init ADV7513");
    //us2066_write(row1, row2);
    adv7513_init(&advtx_dev);

    // Init character OLED
    us2066_init();

    // Init ISL51002
    sniprintf(row1, US2066_ROW_LEN+1, "Init ISL51002");
    us2066_write(row1, row2);
    ret = isl_init(&isl_dev);
    if (ret != 0) {
        printf("ISL51002 init fail\n");
        return ret;
    }

    // Init LPDDR2
    usleep(100);
    sys_ctrl |= SCTRL_EMIF_HWRESET_N;
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, sys_ctrl);
    usleep(1000);
    sys_ctrl |= SCTRL_EMIF_SWRESET_N;
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, sys_ctrl);
    usleep(10000);
    //sniprintf(row1, US2066_ROW_LEN+1, "Mem calib: 0x%x", (unsigned)IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE));
    printf("Mem calib: 0x%x\n", ((unsigned)IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE) >> 26) & 0x7);

    //us2066_write(row1, row2);

    // Init Si5351C
    sniprintf(row1, US2066_ROW_LEN+1, "Init Si5351C");
    us2066_write(row1, row2);
    si5351_init(&si_dev);

    //init ocsdc driver
    /*mmc_dev = ocsdc_mmc_init(SDC_CONTROLLER_QSYS_0_BASE, ALT_CPU_CPU_FREQ);
    if (!mmc_dev) {
        printf("ocsdc_mmc_init failed\n\r");
        //return -1;
    } else {
        printf("ocsdc_mmc_init success\n\r");

        mmc_dev->has_init = 0;
        int err = mmc_init(mmc_dev);
        if (err != 0 || mmc_dev->has_init == 0) {
            printf("mmc_init failed: %d\n\r", err);
            //return -1;
        } else {
            printf("mmc_init success\n\r");
            print_mmcinfo(mmc_dev);

            printf("attempting to read 1 block\n\r");
            if (mmc_bread(mmc_dev, 0, BLKCNT, buf) == 0) {
                printf("mmc_bread failed\n\r");
                //return -1;
            }
            printf("mmc_bread success\n\r");
            for (i=0; i<8; i++)
                printf("0x%.2x\n", buf[i]);
        }
    }*/

    // Init PCM1862
    sniprintf(row1, US2066_ROW_LEN+1, "Init PCM1862");
    us2066_write(row1, row2);
    ret = pcm1862_init();
    if (ret != 0) {
        printf("PCM1862 init fail\n");
        return ret;
    }
    //pcm_source_sel(PCM_INPUT1);

    sniprintf(row1, US2066_ROW_LEN+1, "Init ADV7611");
    us2066_write(row1, row2);
    adv7611_init(&advrx_dev);

    /*check_flash();
    ret = read_flash(0, 100, buf);
    printf("ret: %d\n", ret);
    for (i=0; i<100; i++)
        printf("%u: %.2x\n", i, buf[i]);*/

    set_default_vm_table();
    set_default_keymap();

    return 0;
}

int main()
{
    int ret, i, man_input_change;
    int mode, amode_match;
    uint8_t target_ymult=2, ymult=2;
    uint32_t pclk_hz, dotclk_hz, h_hz, v_hz_x100;
    uint32_t sys_status;
    uint16_t remote_code;
    uint8_t pixelrep;
    uint8_t btn_vec, btn_vec_prev=0;
    uint8_t remote_rpt, remote_rpt_prev=0;
    avinput_t avinput = AV_TESTPAT, target_avinput;
    isl_input_t target_isl_input;
    video_sync target_isl_sync=0;
    video_format target_format;
    video_type target_typemask=0;
    mode_data_t vmode_in, vmode_out;
    vm_mult_config_t vm_conf;
    si5351_ms_config_t si_ms_conf;
    int enable_isl=0, enable_hdmirx=0;

    ret = init_hw();
    if (ret == 0) {
        printf("### OSSC PRO INIT OK ###\n\n");
        sniprintf(row1, US2066_ROW_LEN+1, "OSSC Pro fw. %u.%.2u", FW_VER_MAJOR, FW_VER_MINOR);
        us2066_write(row1, row2);
    } else {
        sniprintf(row2, US2066_ROW_LEN+1, "failed (%d)", ret);
        us2066_write(row1, row2);
        while (1) {}
    }

    while (1) {
        // Read remote control and PCB button status
        sys_status = IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE);
        remote_code = (sys_status & SSTAT_RC_MASK) >> SSTAT_RC_OFFS;
        remote_rpt = (sys_status & SSTAT_RRPT_MASK) >> SSTAT_RRPT_OFFS;
        btn_vec = (~sys_status & SSTAT_BTN_MASK) >> SSTAT_BTN_OFFS;

        if ((remote_rpt == 0) || ((remote_rpt > 1) && (remote_rpt < 6)) || (remote_rpt == remote_rpt_prev))
            remote_code = 0;

        remote_rpt_prev = remote_rpt;

        if (btn_vec_prev == 0) {
            btn_vec_prev = btn_vec;
        } else {
            btn_vec_prev = btn_vec;
            btn_vec = 0;
        }

        target_avinput = avinput;
        parse_control(&target_avinput, remote_code, btn_vec, &target_ymult);

        if (target_avinput != avinput) {

            // defaults
            enable_isl = 1;
            enable_hdmirx = 0;
            target_typemask = VIDEO_LDTV|VIDEO_SDTV|VIDEO_EDTV|VIDEO_HDTV;
            target_isl_sync = SYNC_SOG;

            switch (target_avinput) {
            case AV1_RGBS:
                target_isl_input = ISL_CH0;
                target_format = FORMAT_RGBS;
                break;
            case AV1_RGsB:
                target_isl_input = ISL_CH0;
                target_format = FORMAT_RGsB;
                break;
            case AV1_YPbPr:
                target_isl_input = ISL_CH0;
                target_format = FORMAT_YPbPr;
                break;
            case AV1_RGBHV:
                target_isl_input = ISL_CH0;
                target_isl_sync = SYNC_HV;
                target_format = FORMAT_RGBHV;
                break;
            case AV1_RGBCS:
                target_isl_input = ISL_CH0;
                target_isl_sync = SYNC_CS;
                target_format = FORMAT_RGBS;
                break;
            case AV2_YPbPr:
                target_isl_input = ISL_CH1;
                target_format = FORMAT_YPbPr;
                break;
            case AV2_RGsB:
                target_isl_input = ISL_CH1;
                target_format = FORMAT_RGsB;
                break;
            case AV3_RGBHV:
                target_isl_input = ISL_CH2;
                target_format = FORMAT_RGBHV;
                target_isl_sync = SYNC_HV;
                target_typemask = VIDEO_PC;
                break;
            case AV3_RGBCS:
                target_isl_input = ISL_CH2;
                target_isl_sync = SYNC_CS;
                target_format = FORMAT_RGBS;
                break;
            case AV3_RGsB:
                target_isl_input = ISL_CH2;
                target_format = FORMAT_RGsB;
                break;
            case AV3_YPbPr:
                target_isl_input = ISL_CH2;
                target_format = FORMAT_YPbPr;
                break;
            case AV4:
                enable_isl = 0;
                enable_hdmirx = 1;
                target_format = FORMAT_YPbPr;
                break;
            default:
                enable_isl = 0;
                break;
            }

            printf("### SWITCH MODE TO %s ###\n", avinput_str[target_avinput]);

            avinput = target_avinput;
            isl_enable_power(&isl_dev, 0);
            isl_enable_outputs(&isl_dev, 0);

            sys_ctrl &= ~(SCTRL_CAPTURE_SEL|SCTRL_ISL_VS_POL|SCTRL_ISL_VS_TYPE);

            if (enable_isl) {
                pcm_source_sel(target_isl_input);
                isl_source_sel(&isl_dev, target_isl_input, target_isl_sync, target_format);
                isl_dev.sync_active = 0;

                // send current PLL h_total to isl_frontend for mode detection
                sc->h_in_config2.h_total = isl_get_pll_htotal(&isl_dev);

                // set some defaults
                if (target_isl_sync == SYNC_HV)
                    sys_ctrl |= SCTRL_ISL_VS_TYPE;
            } else if (enable_hdmirx) {
                advrx_dev.sync_active = 0;
                sys_ctrl |= SCTRL_CAPTURE_SEL;
            }

            IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, sys_ctrl);

            strncpy(row1, avinput_str[avinput], US2066_ROW_LEN+1);
            strncpy(row2, "    NO SYNC", US2066_ROW_LEN+1);
            us2066_write(row1, row2);
        }

        if (enable_isl) {
            if (isl_check_activity(&isl_dev, target_isl_input, target_isl_sync)) {
                if (isl_dev.sync_active) {
                    isl_enable_power(&isl_dev, 1);
                    isl_enable_outputs(&isl_dev, 1);
                    printf("ISL51002 sync up\n");
                } else {
                    isl_enable_power(&isl_dev, 0);
                    isl_enable_outputs(&isl_dev, 0);
                    strncpy(row1, avinput_str[avinput], US2066_ROW_LEN+1);
                    strncpy(row2, "    NO SYNC", US2066_ROW_LEN+1);
                    us2066_write(row1, row2);
                    printf("ISL51002 sync lost\n");
                }
            }

            if (isl_dev.sync_active) {
                if (isl_get_sync_stats(&isl_dev, sc->sc_status.vtotal, sc->sc_status.interlace_flag, sc->sc_status2.pcnt_frame) || (target_ymult != ymult)) {

#ifdef ISL_MEAS_HZ
                    if (isl_dev.sm.h_period_x16 > 0)
                        h_hz = (16*isl_dev.xtal_freq)/isl_dev.sm.h_period_x16;
                    else
                        h_hz = 0;
                    if ((isl_dev.sm.h_period_x16 > 0) && (isl_dev.ss.v_total > 0))
                        v_hz_x100 = (16*5*isl_dev.xtal_freq)/((isl_dev.sm.h_period_x16 * isl_dev.ss.v_total) / (100/5));
                    else
                        v_hz_x100 = 0;
#else
                    v_hz_x100 = (100*27000000UL)/isl_dev.ss.pcnt_frame;
                    h_hz = (100*27000000UL)/((100*isl_dev.ss.pcnt_frame)/isl_dev.ss.v_total);
#endif

                    if (isl_dev.ss.interlace_flag)
                        v_hz_x100 *= 2;

                    mode = get_adaptive_mode(isl_dev.ss.v_total, !isl_dev.ss.interlace_flag, v_hz_x100, &vm_conf, target_ymult, &vmode_in, &vmode_out, &si_ms_conf);

                    if (mode < 0) {
                        amode_match = 0;
                        mode = get_mode_id(isl_dev.ss.v_total, !isl_dev.ss.interlace_flag, v_hz_x100, target_typemask, 0, 0, &vm_conf, target_ymult, &vmode_in, &vmode_out);
                    } else {
                        amode_match = 1;
                    }

                    if (mode >= 0) {
                        printf("\nMode %s selected (%s linemult)\n", vmode_in.name, amode_match ? "Adaptive" : "Pure");

                        sniprintf(row1, US2066_ROW_LEN+1, "%-10s %4u%c x%u%c", avinput_str[avinput], isl_dev.ss.v_total, isl_dev.ss.interlace_flag ? 'i' : 'p', vm_conf.y_rpt+1, amode_match ? 'a' : ' ');
                        sniprintf(row2, US2066_ROW_LEN+1, "%lu.%.2lukHz %lu.%.2luHz %c%c\n", (h_hz+5)/1000, ((h_hz+5)%1000)/10,
                                                                                            (v_hz_x100/100),
                                                                                            (v_hz_x100%100),
                                                                                            isl_dev.ss.h_polarity ? '-' : '+',
                                                                                            isl_dev.ss.v_polarity ? '-' : '+');
                        us2066_write(row1, row2);

                        pclk_hz = h_hz * vmode_in.h_total;
                        dotclk_hz = estimate_dotclk(&vmode_in, h_hz);
                        printf("H: %u.%.2ukHz V: %u.%.2uHz\n", (h_hz+5)/1000, ((h_hz+5)%1000)/10, (v_hz_x100/100), (v_hz_x100%100));
                        printf("Estimated source dot clock: %lu.%.2uMHz\n", (dotclk_hz+5000)/1000000, ((dotclk_hz+5000)%1000000)/10000);
                        printf("PCLK_IN: %luHz PCLK_OUT: %luHz\n", pclk_hz, vm_conf.pclk_mult*pclk_hz);

                        isl_source_setup(&isl_dev, vmode_in.h_total);

                        isl_set_afe_bw(&isl_dev, dotclk_hz);
                        isl_phase_adj(&isl_dev);

                        // Setup Si5351
                        if (amode_match)
                            si5351_set_frac_mult(&si_dev, SI_PLLA, SI_CLK0, SI_CLKIN, &si_ms_conf);
                        else
                            si5351_set_integer_mult(&si_dev, SI_PLLA, SI_CLK0, SI_CLKIN, pclk_hz, vm_conf.pclk_mult);

                        // TODO: dont read polarity from ISL51002
                        sys_ctrl &= ~(SCTRL_ISL_VS_POL);
                        if ((target_isl_sync == SYNC_HV) && isl_dev.ss.v_polarity)
                            sys_ctrl |= SCTRL_ISL_VS_POL;
                        IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, sys_ctrl);

                        update_sc_config(&vmode_in, &vmode_out, &vm_conf);

                        // Setup VIC and pixel repetition
                        adv7513_set_pixelrep_vic(&advtx_dev, vm_conf.tx_pixelrep, vm_conf.hdmitx_pixr_ifr, vmode_out.vic);
                    }
                }
            } else {

            }
        } else if (enable_hdmirx) {
            if (adv7611_check_activity(&advrx_dev)) {
                if (advrx_dev.sync_active) {
                    printf("adv sync up\n");
                } else {
                    strncpy(row1, avinput_str[avinput], US2066_ROW_LEN+1);
                    strncpy(row2, "    free-run", US2066_ROW_LEN+1);
                    us2066_write(row1, row2);
                    printf("adv sync lost\n");
                }
            }

            if (advrx_dev.sync_active) {
                if (adv7611_get_sync_stats(&advrx_dev) || (target_ymult != ymult)) {
                    h_hz = advrx_dev.pclk_hz/advrx_dev.ss.h_total;
                    v_hz_x100 = (((h_hz*10000)/advrx_dev.ss.v_total)+50)/100;

                    if (advrx_dev.ss.interlace_flag)
                        v_hz_x100 *= 2;

                    printf("mode changed to: %ux%u%c\n", advrx_dev.ss.h_active, advrx_dev.ss.v_active, advrx_dev.ss.interlace_flag ? 'i' : 'p');
                    sniprintf(row1, US2066_ROW_LEN+1, "%s %ux%u%c", avinput_str[avinput], advrx_dev.ss.h_active, advrx_dev.ss.v_active, advrx_dev.ss.interlace_flag ? 'i' : 'p');
                    sniprintf(row2, US2066_ROW_LEN+1, "%lu.%.2lukHz %lu.%.2luHz %c%c\n", (h_hz+5)/1000, ((h_hz+5)%1000)/10,
                                                                                        (v_hz_x100/100),
                                                                                        (v_hz_x100%100),
                                                                                        (advrx_dev.ss.h_polarity ? '+' : '-'),
                                                                                        (advrx_dev.ss.v_polarity ? '+' : '-'));
                    us2066_write(row1, row2);

                    mode = get_mode_id(advrx_dev.ss.v_total, !advrx_dev.ss.interlace_flag, v_hz_x100, target_typemask, 0, 0, &vm_conf, target_ymult, &vmode_in, &vmode_out);

                    if (mode < 0) {
                        /*vmode_tmp.h_active = advrx_dev.ss.h_active;
                        vmode_tmp.v_active = advrx_dev.ss.v_active;
                        vmode_tmp.h_total = advrx_dev.ss.h_total;
                        vmode_tmp.v_total = advrx_dev.ss.v_total;
                        vmode_tmp.h_backporch = advrx_dev.ss.h_backporch;
                        vmode_tmp.v_backporch = advrx_dev.ss.v_backporch;
                        vmode_tmp.h_synclen = advrx_dev.ss.h_synclen;
                        vmode_tmp.v_synclen = advrx_dev.ss.v_synclen;

                        printf("Mode %s selected\n", vmode_tmp.name);

                        pclk_hz = h_hz * vmode_tmp.h_total * vm_conf.xmult;
                        printf("H: %u.%.2ukHz V: %u.%.2uHz PCLK_IN: %luHz\n\n", h_hz/1000, (((h_hz%1000)+5)/10), (v_hz_x100/100), (v_hz_x100%100), pclk_hz);

                        si5351_set_integer_mult(&si_dev, SI_PLLA, SI_CLK0, SI_CLKIN, advrx_dev.pclk_hz, vm_conf.ymult);
                        update_sc_config(&vmode_tmp, vm_conf.xmult, vm_conf.ymult);

                        // Setup VIC and pixel repetition
                        adv7513_set_pixelrep_vic(&advtx_dev, vm_conf.tx_pixelrep, vm_conf.hdmitx_pixr_ifr, vm_conf.hdmitx_vic);*/
                    }
                }
            } else {
                advrx_dev.pclk_hz = 0;
            }
        }

        adv7513_check_hpd_power(&advtx_dev);

        ymult = target_ymult;

        usleep(20000);
    }

    return 0;
}
