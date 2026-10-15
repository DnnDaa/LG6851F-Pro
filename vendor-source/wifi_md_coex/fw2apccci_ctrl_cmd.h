/* SPDX-License-Identifier: LGPL-2.1 */
/*
 * Definitons for fw2apccci_ctrl_cmd
 *
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Cheery Chen <Cheery.Chen@mediatek.com>
 */

#ifndef __FW2APCCCI_CTRL_CMD_H__
#define __FW2APCCCI_CTRL_CMD_H__

#include "mt_wifi_md_coex.h"

#define TRUE    1
#define FALSE   0

#define MAX_QUEUE_LEN 30

#define THREAD_PID_INIT_VALUE NULL

/* send signal(SIGKILL) from kernel */
#define SEND_SIG_SRC_KERNEL 1

#ifdef COEX_DIRECT_PATH
#define MAX_IRQ_NAME_LEN 32

enum msi_bit {
	MSI_BIT_19 = 19,
	MSI_BIT_21 = 21,
	MSI_BIT_22 = 22,
	MSI_MAX_BIT
};

struct pci_irq_data {
	unsigned int irq;
	unsigned char name[MAX_IRQ_NAME_LEN];
};
#endif

enum _task_status {
	TASK_STAT_UNKNOWN = 0,
	TASK_STAT_INITED = 1,
	TASK_STAT_RUNNING = 2,
	TASK_STAT_STOPPED = 3,
};

struct _os_task {
	void *priv;
	enum _task_status task_status;
	struct task_struct *kthread_task;
	wait_queue_head_t kthread_q;
	unsigned char kthread_running;
};

/* define fw2apccci relevant structure */
struct _fw2apccci_cmd_queue_elmt {
	void *buffer;
	unsigned int buffer_length;
	spinlock_t lock;
	struct kref ref_cnt;
	struct _fw2apccci_cmd_queue_elmt *next;
};

struct _fw2apccci_cmd_queue {
	unsigned int size;
	struct _fw2apccci_cmd_queue_elmt *head;
	struct _fw2apccci_cmd_queue_elmt *tail;
	unsigned int cmd_queue_state;
};

struct _fw2apccci_ctrl_txd {
	void *p_information_buffer;
	unsigned int info_buffer_length;
};

struct _fw2apccci_ctrl_t {
	struct _fw2apccci_cmd_queue fw2apccci_ctrl_queue;
	spinlock_t fw2apccci_ctrl_queue_lock; /* cmd queue spinlock */
	struct _os_task fw2apccci_ctrl_task;
	unsigned int total_cnt;
};

struct _wifi_md_coex_adapter {
	unsigned char debug_level;
	unsigned char pci_num;
	/* apccci port */
	int apccci_idx;
	/* tx flow */
	struct _md_msg md_msg;
	struct task_struct *p_apccci2fw_tx_task;
	/* rx flow */
	unsigned int fw2apccci_seq_num;
	struct _wifi_dtb_info dtb_info;
	struct _fw2apccci_ctrl_t fw2apccci_ctrl;
	struct notifier_block fw2apccci_rx_notifier;
	struct dentry *debugfs_dir;

#ifdef COEX_DIRECT_PATH
	atomic_t notify_seq;
	struct mutex notify_lock;
	wait_queue_head_t notify_wait;
	unsigned int recv_seq;
	atomic_t register_cnt;
	unsigned long request_msi_bit;
	/* coex sharing memory addr */
	phys_addr_t mcif_bank4_start_addr;
	unsigned int mcif_addr_len;
	unsigned long csr_addr;
	unsigned char ioremap_flag;
	unsigned char md_check_timer_retries;
	struct task_struct *p_monitor_fw_task;
	/* MCIF memory maping layout of WIFI DTB*/
	struct _coex_mcif_layout coex_mcif_layout;
#endif /* COEX_DIRECT_PATH */
};

int fw2apccci_ctrl_init(struct _wifi_md_coex_adapter *pAd);
int fw2apccci_ctrl_exit(struct _wifi_md_coex_adapter *pAd);
int fw2apccci_ctrl_basic_enq(struct _wifi_md_coex_adapter *pAd, int len, void *p_buffer);
void os_cmd_up(struct _os_task *p_cmd_queue_task);

#endif /*__FW2APCCCI_CTRL_CMD_H__*/
