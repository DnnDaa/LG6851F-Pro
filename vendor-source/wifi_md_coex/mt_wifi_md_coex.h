/* SPDX-License-Identifier: LGPL-2.1 */
/*
 * Definitons for mt_wifi_md_coex
 *
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Cheery Chen <Cheery.Chen@mediatek.com>
 */

#ifndef __MT_WIFI_MD_COEX_H__
#define __MT_WIFI_MD_COEX_H__

#include <mt-plat/mtk_ccci_common.h>

#define COEX_DBG_LVL_OFF 0
#define COEX_DBG_LVL_ERROR 1
#define COEX_DBG_LVL_WARN 2
#define COEX_DBG_LVL_TRACE 3

#define MAX_WIFI_DTB_NUM 4
#define MAX_WIFI_FREQ_NUM 4

/* merge idc_wifi_def_param_ntf */
#define DEF_PARAM_MSG_ID 20
/* merge idc_wifi_oper_freq_ntf */
#define OPER_FREQ_MSG_ID 22
/* merge idc_wifi_max_power_backoff_ntf */
#define MAX_POWER_BACKOFF_ID 32
/* merge idc_wifi_actv_bss_ntf */
#define ACTV_BSS_MSG_ID 33
/* merge idc_wifi_consys_tdm_ntf */
#define CONSYS_TDM_NTF 37

/* notification chain msg id define */
#define APCCCI_DRIVER_FW 0x0100
#define REGISTER_WIFI_MD_DTB 0x0300
#define UNREGISTER_WIFI_MD_DTB 0x0400
#define QUERY_FW_STATUS 0x0500

#define COEX_WIFI_2G 0x1
#define COEX_WIFI_5G 0x2
#define COEX_WIFI_6G 0x3
#define COEX_WIFI_DBDC 0x4
#define COEX_WIFI_5G_HIGH 0x5
#define COEX_WIFI_5G_LOW 0x6
#define COEX_WIFI_DBDC_5G_HIGH 0x7
#define COEX_WIFI_DBDC_5G_LOW 0x8

#define COEX_DEBUGFS_DIR_NAME "mtk_wifi_md_coex"
#define COEX_DEBUGFS_FILE_NAME "debug_level"

enum notifier_id {
	INVAILD_NOTIFIER	= 0,
	HWIFI_NOTIFIER		= 1,
	COEX_NOTIFIER		= 2,
};
struct _fw2apccci_msg {
	void *device;
	unsigned short data_len;
	unsigned char pci_num;
	unsigned char pci_slot_id;
	unsigned char card_type;
	unsigned char data[0];
} __attribute__((__packed__));

/* merge fw2apccci data structure */
struct idc_sw_msg_header {
	unsigned char msg_id;
	bool is_need_reply_ack;
	unsigned char rsvd[2];
	unsigned int seq_num;
	unsigned int enqueue_time_stamp;
} __attribute__((__packed__));

struct idc_wifi_def_param_ntf {
	struct idc_sw_msg_header msg_header;
	unsigned char idc_ver_major;
	unsigned char pta_ver;
	unsigned char chip_ver;
	unsigned char tri_band;
	bool wifi_en;
	bool is_reset;
	unsigned char idc_ver_minor;
	unsigned char rsvd2;
} __attribute__((__packed__));

struct idc_wifi_oper_freq_ntf {
	struct idc_sw_msg_header msg_header;
	unsigned char band0_oper_freq_num;
	unsigned char band1_oper_freq_num;
	unsigned char rsvd[2];
	unsigned int oper_freq_low[MAX_WIFI_FREQ_NUM];	/* array_idx 0: 2g, array_idx 1: 5g/5g-low, array_idx 2: 5g-high */
	unsigned int oper_freq_high[MAX_WIFI_FREQ_NUM];	/* array_idx 0: 2g, array_idx 1: 5g/5g-low, array_idx 2: 5g-high */
	unsigned char oper_freq_3wire_group[MAX_WIFI_FREQ_NUM];	/* array_idx 0: 2g, array_idx 1: 5g/5g-low, array_idx 2: 5g-high */
} __attribute__((__packed__));

struct idc_wifi_actv_bss_ntf {
	struct idc_sw_msg_header msg_header;
	unsigned char wifi_actv_bmp;
	bool is_sta;
	unsigned char rsvd[2];
} __attribute__((__packed__));

struct idc_wifi_consys_tdm_ntf {
	struct idc_sw_msg_header msg_header;
	bool is_tdm_for_lte[MAX_WIFI_FREQ_NUM];
	bool is_tdm_for_nr[MAX_WIFI_FREQ_NUM];
	unsigned short lte_cc_bmp[MAX_WIFI_FREQ_NUM];
	unsigned short nr_cc_bmp[MAX_WIFI_FREQ_NUM];
} __attribute__((__packed__));

struct idc_wifi_max_power_backoff_ntf {
	struct idc_sw_msg_header msg_header;
	bool md_pwr_backoff_en;
	unsigned char wifi_pwr_bmp;
	unsigned char wf_pwr_backoff_en_bmp;
	bool is_wf_scan;
	unsigned short lte_cell_bmp;
	unsigned short nr_cell_bmp;
} __attribute__((__packed__));

/* dtb info */
struct _wifi_dtb_info {
	unsigned int idx[MAX_WIFI_DTB_NUM]; /* PCIE-A=idx[0]; PCIE-B=idx[1]; PCIE-C=idx[2]; PCIE-D=idx[3] */
	unsigned char clear_msg_flag;
	unsigned char tri_band;
	struct idc_wifi_def_param_ntf def_param_ntf[MAX_WIFI_DTB_NUM];
	struct idc_wifi_oper_freq_ntf oper_freq_ntf;
	struct idc_wifi_actv_bss_ntf actv_bss_ntf[MAX_WIFI_DTB_NUM];
	struct idc_wifi_consys_tdm_ntf consys_tdm_ntf;
	struct idc_wifi_max_power_backoff_ntf max_power_backoff_ntf[MAX_WIFI_DTB_NUM];
};

#define COEX_LOG(Level, Fmt)	\
	do {	\
		if (wifi_md_coex_adapter.debug_level >= Level) \
			printk Fmt; \
	} while (0)

#define COEX_LOG_TRACE(...) COEX_LOG(COEX_DBG_LVL_TRACE, (__VA_ARGS__))
#define COEX_LOG_WARN(...) COEX_LOG(COEX_DBG_LVL_WARN, (__VA_ARGS__))
#define COEX_LOG_ERROR(...) COEX_LOG(COEX_DBG_LVL_ERROR, (__VA_ARGS__))

#define COEX_WARN_DUMP_RAW_DATA(data, len)	\
	do {	\
		if (wifi_md_coex_adapter.debug_level >= COEX_DBG_LVL_WARN) \
			dump_all_coex_data(data, len); \
	} while (0)

#define COEX_DUMP_RAW_DATA(data, len)	\
	do {	\
		if (wifi_md_coex_adapter.debug_level >= COEX_DBG_LVL_TRACE) \
			dump_all_coex_data(data, len); \
	} while (0)

#ifdef COEX_DIRECT_PATH
#define MAX_MSG_LEN 256
#define COEX_NOTIFY_MAX_SEQ 0x1FF
#define WAIT_MD_OFF_TIMEOUT 500 //unit:ms
#define CHECK_MD_STATUS_TIME 50 //unit:ms
#define CHECK_MD_STATUS_MAX_COUNT 5 //unit: 5*100ms
#define CHECK_MD_STATUS_TIMER_MAX_COUNT 300 //unit: 300*100ms(30s)
#define STATUS_SUCCESS 0
#define STATUS_FAILURE -1
#define STATUS_TIMEOUT -2

#define MT7990_CHIP_ID 0x7990
#define MT7992_CHIP_ID 0x7992
#define MT7993_CHIP_ID 0x7993

/* These register just use in bollinger eagle (mt7990) chip */
#define MT7990_MAPPING_ADDR_OFFSET 0x001a5801
#define MT7990_MAPPING_SIZE 0x0010
#define MT7990_CHIP_TYPE 0x0003
#define MT7990_SYS_ID 0x0
#define MT7990_MAPPING_IRQ_REG_OFFSET 0x0016f00C
#define MT7990_IRQ_REG_VAL 0x00000001

/* These register just use in bollinger kite (mt7992) chip */
#define MT7992_MT7993_MAPPING_ADDR_OFFSET 0x001a3401
#define MT7992_MT7993_MAPPING_SIZE 0x0010
#define MT7992_MT7993_CHIP_TYPE 0x0003
#define MT7992_MT7993_SYS_ID 0x0
#define MT7992_MT7993_MAPPING_IRQ_REG_OFFSET 0x0016f00C
#define MT7992_MT7993_IRQ_REG_VAL 0x00000001

/*
 ---------------------------------------------------------------------------------
  |   0x8       |     0x8     |      0x60           |     0x8     |    0x8      |
 ---------------------------------------------------------------------------------
  |             |             |                     |             |             |
  | MCIF_HEADER |    MCIF_P   |   MCIF_TOTAL_LAYOUT |    MCIF_P   | MCIF_FOOTER |
  |             | (dont care) |   (layout*max_num)  | (dont care) |             |
 ---------------------------------------------------------------------------------
*/
#define MCIF_HEADER_CONTENT "MCMMMCMM"
#define MCIF_FOOTER_CONTENT "MMCMMMCM"
#define MCIF_HEADER_SIZE 0x08
#define MCIF_FOOTER_SIZE 0x08
#define MCIF_P_NUM 2
#define MCIF_SIGNLE_P_SIZE 0X08
#define MCIF_LAYOUT_NUM 6
#define MCIF_SINGLE_LAYOUT_SIZE 0x10
#define MCIF_TOTAL_SIZE (MCIF_HEADER_SIZE + MCIF_FOOTER_SIZE + \
			             MCIF_SIGNLE_P_SIZE * MCIF_P_NUM + \
			             MCIF_SINGLE_LAYOUT_SIZE * MCIF_LAYOUT_NUM)

struct _coex_mcif_layout {
	unsigned int consys_mapping_addr;
	unsigned short mapping_size;
	unsigned short sys_type;
	unsigned int mapping_irq_reg;
	unsigned int irq_reg_val;
} __attribute__((__packed__));

enum ccci_user_id {
	CCCI_USER_ID_RESERVED,
	CCCI_USER_ID_COEX,
	CCCI_USER_ID_MAX
};

enum ccci_msg_id {
	CCCI_MSG_ID_COEX_NOTIFY = 15,
	CCCI_MSG_ID_MD_NOTIFY = 16,
	CCCI_MSG_ID_RESET_IND = 17,
};

enum md_notify_info_type {
	MD_NOTIFY_INFO_ONOFF = 5, //recv seq from MD
	MD_NOTIFY_INFO_INVAILD
};

enum coex_notify_info_type {
	COEX_NOTIFY_INFO_ONOFF = 2,
	COEX_NOTIFY_INFO_INVAILD
};

enum coex_notify_intf_status {
	COEX_NOTIFY_WIFI_OFF  = 2,
	COEX_NOTIFY_WIFI_ON = 4,
	COEX_NOTIFY_INVAILD_STATUS
};

struct _coex_intf_info {
	unsigned char status; //intf status
	unsigned char ringNum; //useless, follow gen4m
	unsigned int seq; //packet sequence number
} __packed __aligned(1);

struct _coex_wifi_info {
	unsigned char info_id;
	unsigned char info_len;
	unsigned char info[0];
};

struct _coex_notify_info {
	unsigned char version;
	unsigned char buf_len;
	unsigned char info_num;
	unsigned char buf[0];
};

struct _md_notify_info {
	unsigned char version;
	unsigned char info_type;
	unsigned char buf_len;
	unsigned char buf[0];
};

struct _md_msg {
	unsigned int dest_user_id;
	unsigned int msg_id;
	unsigned int buf_len;
	unsigned char buf[MAX_MSG_LEN];
} __attribute__((__packed__));

#else
/* FW max len = 1.6*1024=1638.4 bytes. Reserve some bytes for cmd's header */
#define MAX_MSG_LEN 1640
struct _md_msg {
	unsigned char data[MAX_MSG_LEN];
	unsigned long len;
};
#endif

void dump_all_coex_data(void *data, int len);

extern int mtk_ccci_open_port(char *port_name); /* return fd/index */
extern int mtk_ccci_read_data(int index, char *buf, size_t count);
extern int mtk_ccci_write_data(int index, char *buf, int size);
extern int mtk_ccci_close_port(int index);
extern enum MD_STATE ccci_fsm_get_md_state(int md_id);

extern struct _wifi_md_coex_adapter wifi_md_coex_adapter;

#endif /*__MT_WIFI_MD_COEX_H__*/
