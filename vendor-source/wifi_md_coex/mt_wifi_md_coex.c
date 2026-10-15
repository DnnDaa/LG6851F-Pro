// SPDX-License-Identifier: LGPL-2.1
/*
 * receive data from apccci and send to wifi FW
 *
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Cheery Chen <Cheery.Chen@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/pci.h>
#include <linux/irq.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>

#include "fw2apccci_ctrl_cmd.h"
#include "mt_wifi_md_coex.h"

MODULE_LICENSE("Dual BSD/GPL");

/*
 *Tx Notifier Chain Flow (APCCCI->COEX->WLAN_DRIVER->WLAN_FW)
 *Rx Notifier Chain Flow (APCCCI<-COEX<-WLAN_DRIVER<-WLAN_FW)
 */

/* init coex_adapter structure */
struct _wifi_md_coex_adapter wifi_md_coex_adapter;
static bool hwifi_notifier_registered = false;
static bool coex_notifier_registered = false;

/* irq data */
struct pci_irq_data g_irq_data[] = {
	{MSI_BIT_19, "MD-COEX-MSI19"},
	{MSI_BIT_21, "MD-COEX-MSI21"},
	{MSI_BIT_22, "MD-COEX-MSI22"},
	/* add new MSI bit above here */
};

/* Init notifier head */
static BLOCKING_NOTIFIER_HEAD(wifi_md_coex_chain);

#define CHECK_INTERVAL 100 //ms
static struct timer_list md_check_timer;

/* Just for debug */
void dump_all_coex_data(void *data, int len)
{
	printk("dump len = %d\n", len);
	print_hex_dump(KERN_ERR, "coex data : ", DUMP_PREFIX_OFFSET, 16, 1, data, len, 0);
}

/* Define register_wifi_md_coex_notifier func */
int register_wifi_md_coex_notifier(struct notifier_block *nb, enum notifier_id id)
{
	int err = 0;

	COEX_LOG_TRACE("%s\n", __func__);
	switch(id) {
		case HWIFI_NOTIFIER:
			if (!hwifi_notifier_registered) {
				err = blocking_notifier_chain_register(&wifi_md_coex_chain, nb);
				COEX_LOG_TRACE("reg_wifi_md_coex_chain = %p, nb=%p\n", &wifi_md_coex_chain, nb);
				if (err)
					COEX_LOG_ERROR("hwifi_notifier register fail !err = %d\n", err);
				else
					hwifi_notifier_registered = true;
			} else
				COEX_LOG_ERROR("hwifi_notifier register alreay!\n");
			break;

		case COEX_NOTIFIER:
			if (!coex_notifier_registered) {
				err = blocking_notifier_chain_register(&wifi_md_coex_chain, nb);
				COEX_LOG_TRACE("reg_wifi_md_coex_chain = %p, nb=%p\n", &wifi_md_coex_chain, nb);
				if (err)
					COEX_LOG_ERROR("coex_notifier register fail !err = %d\n", err);
				else
					coex_notifier_registered = true;
			} else
				COEX_LOG_ERROR("coex_notifier register alreay!\n");
			break;

		default:
			COEX_LOG_ERROR("Unknown notifier id: %d\n", id);
			err = -EINVAL;
			break;
	}

	return err;
}
EXPORT_SYMBOL(register_wifi_md_coex_notifier);

/* Define unregister_wifi_md_coex_notifier func */
int unregister_wifi_md_coex_notifier(struct notifier_block *nb, enum notifier_id id)
{
	int err = 0;

	COEX_LOG_TRACE("%s\n", __func__);
	switch(id) {
		case HWIFI_NOTIFIER:
			if (hwifi_notifier_registered) {
				err = blocking_notifier_chain_unregister(&wifi_md_coex_chain, nb);
				COEX_LOG_TRACE("unreg_wifi_md_coex_chain = %p, nb=%p\n", &wifi_md_coex_chain, nb);
				if (err)
					COEX_LOG_ERROR("hwifi_notifier unregister fail !err = %d\n\n", err);
				else
					hwifi_notifier_registered = false;
			} else
				COEX_LOG_TRACE("hwifi_notifier unregister alreay!\n");
			break;

		case COEX_NOTIFIER:
			if (coex_notifier_registered) {
				err = blocking_notifier_chain_unregister(&wifi_md_coex_chain, nb);
				COEX_LOG_TRACE("unreg_wifi_md_coex_chain = %p, nb=%p\n", &wifi_md_coex_chain, nb);
				if (err)
					COEX_LOG_ERROR("coex_notifier unregister fail !err = %d\n\n", err);
				else
					coex_notifier_registered = false;
			} else
				COEX_LOG_TRACE("coex_notifier unregister alreay!\n");
			break;

		default:
			COEX_LOG_ERROR("Unknown notifier id: %d\n", id);
			err = -EINVAL;
			break;

	}

	return err;
}
EXPORT_SYMBOL(unregister_wifi_md_coex_notifier);

/* Define call notifier chain func */
int call_wifi_md_coex_notifier(unsigned long event, void *msg)
{
	return blocking_notifier_call_chain(&wifi_md_coex_chain, event, msg);
}
EXPORT_SYMBOL(call_wifi_md_coex_notifier);

static int coex_read_data_from_md(int index, char *buf, size_t count)
{
	int ret = 0;
	int retry_cnt = 0;

	do {
		ret = mtk_ccci_read_data(index, buf, count);
		if (ret < 0 || ret > count) {
			retry_cnt++;
			msleep(CHECK_MD_STATUS_TIME);
		} else {
			COEX_LOG_TRACE("%s: retry count = %d, recv data from MD success\n", __func__, retry_cnt);
			return ret;
		}
	} while (retry_cnt < CHECK_MD_STATUS_MAX_COUNT);
	COEX_LOG_TRACE("%s: retry count = %d, recv data from MD failed\n", __func__, retry_cnt);

	/* read date len : 0 */
	return 0;
}

static int coex_send_data_to_md(int index, char *buf, size_t count)
{
	int ret = 0;
	int retry_cnt = 0;

	do {
		ret = mtk_ccci_write_data(index, buf, count);
		if (ret < 0) {
			retry_cnt++;
			msleep(CHECK_MD_STATUS_TIME);
		} else {
			COEX_LOG_TRACE("%s: retry count = %d, send to MD success\n", __func__, retry_cnt);
			return STATUS_SUCCESS;
		}
	} while (retry_cnt < CHECK_MD_STATUS_MAX_COUNT);
	COEX_LOG_TRACE("%s: retry count = %d, send to MD failed\n", __func__, retry_cnt);
	return STATUS_FAILURE;
}


/* merge specific msg from fw2apccci */

static void coex_update_wifi_dtb_info(struct _wifi_md_coex_adapter *pAd)
{
	struct idc_wifi_oper_freq_ntf *p_oper_freq_ntf = NULL;
	unsigned char i;

	p_oper_freq_ntf = (struct idc_wifi_oper_freq_ntf *)&pAd->dtb_info.oper_freq_ntf;

	for (i = 0; i < MAX_WIFI_FREQ_NUM; i++)
		p_oper_freq_ntf->oper_freq_3wire_group[i] = 0;

	switch (pAd->pci_num) {
		case 1:
			break;

		case 2:
			for (i=0; i<3; i++) {
				switch (pAd->dtb_info.idx[i]) {
					case COEX_WIFI_2G:
						p_oper_freq_ntf->oper_freq_3wire_group[0] = 1;
						break;
					case COEX_WIFI_DBDC_5G_HIGH:
						p_oper_freq_ntf->oper_freq_3wire_group[0] = 1;
						p_oper_freq_ntf->oper_freq_3wire_group[1] = 1;
						pAd->dtb_info.tri_band = 1;
						break;
					case COEX_WIFI_5G_HIGH:
						p_oper_freq_ntf->oper_freq_3wire_group[2] = 2;
						pAd->dtb_info.tri_band = 1;
						break;
					case COEX_WIFI_DBDC_5G_LOW:
					case COEX_WIFI_5G_LOW:
						pAd->dtb_info.tri_band = 1;
						break;
					default:
						break;
				}
			}
			break;

		case 3:
			pAd->dtb_info.tri_band = 1;
			for (i=0; i<3; i++) {
				switch (pAd->dtb_info.idx[i]) {
					case COEX_WIFI_2G:
						p_oper_freq_ntf->oper_freq_3wire_group[0] = 1;
						break;
					case COEX_WIFI_5G_HIGH:
						p_oper_freq_ntf->oper_freq_3wire_group[2] = 2;
						break;
					default:
						break;
				}
			}
			break;

		default:
			break;
	}

	COEX_LOG_TRACE("%s: card_num=%d, tri_band=%d\n", __func__, pAd->pci_num, pAd->dtb_info.tri_band);
	for (i = 0; i < 3; i++) {
		COEX_LOG_TRACE("idx[%d]=%d, oper_freq_3wire_group[%d]=%d\n", i, pAd->dtb_info.idx[i],
			i, pAd->dtb_info.oper_freq_ntf.oper_freq_3wire_group[i]);
	}
}

#ifdef COEX_DIRECT_PATH	/* TBD for tri-band */

static void coex_get_mcif_bank4_memory_addr(phys_addr_t* base, unsigned int* size)
{
	phys_addr_t mdPhy = 0;
	int ret = 0;

	mdPhy = get_smem_phy_start_addr(MD_SYS1, SMEM_USER_RAW_MD_CONSYS, &ret);
	if (ret && mdPhy) {
		COEX_LOG_TRACE("MCIF base=0x%llx size=0x%x", mdPhy, ret);
		if (base)
			*base = mdPhy;
		if (size)
			*size = ret;
	} else {
		COEX_LOG_ERROR("MCIF is not supported");
		if (base)
			*base = 0;
		if (size)
			*size = 0;
	}
}

static irqreturn_t coex_isr(int irq, void *data)
{
	COEX_LOG_TRACE("%s: irq = %d\n", __func__, irq);
	return IRQ_HANDLED;
}

static void coex_release_msi(struct _wifi_md_coex_adapter *pAd,
				    struct _fw2apccci_msg *p_fw2apccci_msg)
{
	int nr = 0;
	int irq;
	struct pci_dev *pdev = NULL;
	struct pci_irq_data *irq_data = g_irq_data;

	pdev = (struct pci_dev *)p_fw2apccci_msg->device;
	if (!pdev) {
		COEX_LOG_ERROR("%s: pdev is NULL !!!\n", __func__);
		return;
	}

	for (nr = 0; nr < ARRAY_SIZE(g_irq_data); nr++) {
		if (!test_bit(nr, &pAd->request_msi_bit))
			continue;

		irq = pci_irq_vector(pdev, irq_data[nr].irq);
		if (irq < 0) {
			COEX_LOG_ERROR("%s: MSI vector %d unavailable: %d\n", __func__, irq_data[nr].irq, irq);
			clear_bit(nr, &pAd->request_msi_bit);
			continue;
		}


		COEX_LOG_TRACE("%s: release MSI(%d:%d)\n",
			__func__, irq_data[nr].irq, irq);

		free_irq(irq, pAd);
		clear_bit(nr, &pAd->request_msi_bit);
	}
}

//Apply for MSI(BIT19/21/22) Interruption for MD (MD cannot access the linux kernel)
static void coex_request_msi(struct _wifi_md_coex_adapter *pAd,
				    struct _fw2apccci_msg *p_fw2apccci_msg)
{
	int nr = 0;
	int ret = 0;
	int irq;
	struct irq_data *data;
	struct pci_dev *pdev = NULL;
	struct pci_irq_data *irq_data = g_irq_data;

	pdev = (struct pci_dev *)p_fw2apccci_msg->device;
	if (!pdev) {
		COEX_LOG_ERROR("%s: pdev is NULL!!!\n", __func__);
		return;
	}

	for (nr = 0; nr < ARRAY_SIZE(g_irq_data); nr++) {
		if (test_bit(nr, &pAd->request_msi_bit))
			continue;

		/* request irq */
		irq = pci_irq_vector(pdev, irq_data[nr].irq);
		if (irq < 0) {
			COEX_LOG_ERROR("%s: MSI vector %d unavailable: %d\n", __func__, irq_data[nr].irq, irq);
			goto err;
		}


		COEX_LOG_TRACE("%s: request MSI(%d:%d).\n",
			__func__, irq_data[nr].irq, irq);

		ret = request_irq(irq, coex_isr, IRQF_SHARED, irq_data[nr].name, pAd);
		if (ret) {
			COEX_LOG_ERROR("%s: request MSI(%d:%d) failed, ret = %d!!!!\n",
				__func__, irq_data[nr].irq, irq, ret);
			goto err;
		}

		set_bit(nr, &pAd->request_msi_bit);

		/* unmask (to AP MCU) */
		data = irq_get_irq_data(irq);
		if (!data) {
			COEX_LOG_ERROR("%s: Get MSI(%d:%d) data failed, ret = %d!!!!\n",
				__func__, irq_data[nr].irq, irq, ret);
			goto err;
		}
		irq_chip_unmask_parent(data);

	}
	return;
err:
	coex_release_msi(pAd, p_fw2apccci_msg);
}

static void fill_coex_mcif_single_layout(unsigned int pci_base_addr,
		unsigned short chip_id, struct _coex_mcif_layout *layout)
{
	if (!layout) {
		COEX_LOG_ERROR("%s:layout is NULL!\n\n", __func__);
		return;
	}

	if (chip_id == MT7990_CHIP_ID) {
		layout->consys_mapping_addr = pci_base_addr + MT7990_MAPPING_ADDR_OFFSET;
		layout->mapping_size = MT7990_MAPPING_SIZE;
		layout->sys_type = ((MT7990_CHIP_TYPE << 0x8) | (MT7990_SYS_ID));
		layout->mapping_irq_reg = pci_base_addr + MT7990_MAPPING_IRQ_REG_OFFSET;
		layout->irq_reg_val = MT7990_IRQ_REG_VAL;
	} else if (chip_id == MT7992_CHIP_ID || chip_id == MT7993_CHIP_ID) {
		layout->consys_mapping_addr = pci_base_addr + MT7992_MT7993_MAPPING_ADDR_OFFSET;
		layout->mapping_size = MT7992_MT7993_MAPPING_SIZE;
		layout->sys_type = ((MT7992_MT7993_CHIP_TYPE << 0x8) | (MT7992_MT7993_SYS_ID));
		layout->mapping_irq_reg = pci_base_addr + MT7992_MT7993_MAPPING_IRQ_REG_OFFSET;
		layout->irq_reg_val = MT7992_MT7993_IRQ_REG_VAL;
	} else {
		COEX_LOG_ERROR("%s:chip 0x%x error!\n\n", __func__, chip_id);
		return;
	}
}

static void fill_single_layout_to_mcif_bank4(
		struct _wifi_md_coex_adapter *pAd, unsigned short slot_id)
{
	struct _coex_mcif_layout *p_coex_mcif_layout_offset;
	unsigned long tmp_csr_addr = pAd->csr_addr + (slot_id + 1) * MCIF_SINGLE_LAYOUT_SIZE;

	memcpy((void *)(uintptr_t)(tmp_csr_addr), &pAd->coex_mcif_layout, sizeof(struct _coex_mcif_layout));

	p_coex_mcif_layout_offset = (struct _coex_mcif_layout *)(uintptr_t)(tmp_csr_addr);
	COEX_LOG_TRACE("%s:Slot %d card info (0x%lu): \n", __func__, slot_id, tmp_csr_addr);
	COEX_LOG_TRACE("%s:consys_mapping_addr 0x%x \n", __func__, p_coex_mcif_layout_offset->consys_mapping_addr);
	COEX_LOG_TRACE("%s:mapping_size 0x%x \n", __func__, p_coex_mcif_layout_offset->mapping_size);
	COEX_LOG_TRACE("%s:sys_type 0x%x \n", __func__, p_coex_mcif_layout_offset->sys_type);
	COEX_LOG_TRACE("%s:mapping_irq_reg 0x%x \n", __func__, p_coex_mcif_layout_offset->mapping_irq_reg);
	COEX_LOG_TRACE("%s:irq_reg_val 0x%x \n", __func__, p_coex_mcif_layout_offset->irq_reg_val);
}

static void coex_write_mcif_bank4_memory(struct _wifi_md_coex_adapter *pAd,
				    struct _fw2apccci_msg *p_fw2apccci_msg)
{
	struct pci_dev *pdev = NULL;
	unsigned int pci_base_addr = 0;
	unsigned long tmp_csr_addr = 0;
	unsigned short chip_id = 0;
	unsigned short slot_id = 0;

	pdev = (struct pci_dev *)p_fw2apccci_msg->device;
	if (!pdev) {
		COEX_LOG_ERROR("%s: pdev is NULL !!!\n", __func__);
		return;
	}

	pci_base_addr = (unsigned int)pci_resource_start(pdev, 0);
	chip_id = pdev->device;
	slot_id = pci_domain_nr(pdev->bus);
	COEX_LOG_TRACE("%s:device_id = 0x%x, slot_id = %d, pci_base_addr = %d\n\n",
		__func__, chip_id, slot_id, pci_base_addr);

	/* step1: query BANK4 sharing memory start addr from AP CCCI and init mem */
	if (!pAd->ioremap_flag) {
		pAd->ioremap_flag = TRUE;
		coex_get_mcif_bank4_memory_addr(&pAd->mcif_bank4_start_addr, &pAd->mcif_addr_len);
		pAd->csr_addr = (unsigned long)ioremap(pAd->mcif_bank4_start_addr, pAd->mcif_addr_len);
		if (!pAd->csr_addr) {
			COEX_LOG_ERROR("%s: ioremap failed !!!%lx \n", __func__, pAd->csr_addr);
			return;
		}
		/* init mapping mem to 0 */
		tmp_csr_addr = pAd->csr_addr;
		memset((void *)(uintptr_t)(tmp_csr_addr), 0, MCIF_TOTAL_SIZE);
		/* padding hearder */
		memcpy((void *)(uintptr_t)(tmp_csr_addr), MCIF_HEADER_CONTENT, MCIF_HEADER_SIZE);
		/* padding footer */
		memcpy((void *)(uintptr_t)(pAd->csr_addr + (MCIF_TOTAL_SIZE - MCIF_FOOTER_SIZE)),
				MCIF_FOOTER_CONTENT, MCIF_FOOTER_SIZE);
	}

	COEX_LOG_TRACE("%s: pAd->mcif_bank4_start_addr = 0x%llx \n", __func__, pAd->mcif_bank4_start_addr);
	COEX_LOG_TRACE("%s: pAd->mcif_addr_len = 0x%x \n", __func__, pAd->mcif_addr_len);
	COEX_LOG_TRACE("%s: pAd->csr_addr = 0x%lx \n", __func__, pAd->csr_addr);

	/* step2: write information to BANK4 memory */
	fill_coex_mcif_single_layout(pci_base_addr, chip_id, &pAd->coex_mcif_layout);
	fill_single_layout_to_mcif_bank4(pAd, slot_id);
}

static void coex_clear_mcif_bank4_memory(struct _wifi_md_coex_adapter *pAd,
					struct _fw2apccci_msg *p_fw2apccci_msg)
{
	struct pci_dev *pdev = NULL;
	unsigned long tmp_csr_addr = 0;
	unsigned int pci_base_addr = 0;
	unsigned short chip_id = 0;
	unsigned short slot_id = 0;
	struct _coex_mcif_layout *p_coex_mcif_layout_offset;

	pdev = (struct pci_dev *)p_fw2apccci_msg->device;
	if (!pdev) {
		COEX_LOG_ERROR("%s: pdev is NULL !!!\n", __func__);
		return;
	}

	chip_id = pdev->device;
	slot_id = pci_domain_nr(pdev->bus);
	pci_base_addr = (unsigned int)pci_resource_start(pdev, 0);

	tmp_csr_addr = pAd->csr_addr + (slot_id + 1) * MCIF_SINGLE_LAYOUT_SIZE;
	memset((void *)(uintptr_t)(tmp_csr_addr), 0, sizeof(struct _coex_mcif_layout));

	/* FIXME  DUMP DATA */
	COEX_LOG_TRACE("%s:pci_base_addr = 0x%x \n\n", __func__, pci_base_addr);
	p_coex_mcif_layout_offset = (struct _coex_mcif_layout *)(uintptr_t)(tmp_csr_addr);
	COEX_LOG_TRACE("%s:Slot %d card info (0x%lu): \n", __func__, slot_id, tmp_csr_addr);
	COEX_LOG_TRACE("%s:consys_mapping_addr 0x%x \n", __func__, p_coex_mcif_layout_offset->consys_mapping_addr);
	COEX_LOG_TRACE("%s:mapping_size 0x%x \n", __func__, p_coex_mcif_layout_offset->mapping_size);
	COEX_LOG_TRACE("%s:sys_type 0x%x \n", __func__, p_coex_mcif_layout_offset->sys_type);
	COEX_LOG_TRACE("%s:mapping_irq_reg 0x%x \n", __func__, p_coex_mcif_layout_offset->mapping_irq_reg);
	COEX_LOG_TRACE("%s:irq_reg_val 0x%x \n", __func__, p_coex_mcif_layout_offset->irq_reg_val);
}

static int coex_notify_to_md(struct _wifi_md_coex_adapter *pAd,
							struct _coex_notify_info *const notify_info)
{
	struct _md_msg msg;
	unsigned int header_size = sizeof(msg) - MAX_MSG_LEN;

	msg.dest_user_id = CCCI_USER_ID_COEX;
	msg.msg_id = CCCI_MSG_ID_COEX_NOTIFY;
	msg.buf_len = notify_info->buf_len + sizeof(struct _coex_notify_info);
	if (msg.buf_len > MAX_MSG_LEN) {
		COEX_LOG_ERROR("%s: buf len(%d) error!\n",
			__func__, msg.buf_len);
		return STATUS_FAILURE;
	}

	memcpy(msg.buf, notify_info, msg.buf_len);

	return coex_send_data_to_md(pAd->apccci_idx, (char *)&msg, msg.buf_len + header_size);
}


static int coex_notify_wifi_status_to_md(struct _wifi_md_coex_adapter *pAd,
	enum coex_notify_intf_status status, int seq)
{
	int buf_size = 0;
	int	ret = STATUS_SUCCESS;
	unsigned char *buffer = NULL;
	struct _coex_wifi_info *wifi_info;
	struct _coex_intf_info *intf_info;
	struct _coex_notify_info *notify_info;

	mutex_lock(&pAd->notify_lock);
	buf_size = (sizeof(struct _coex_notify_info) +
				sizeof(struct _coex_wifi_info) + sizeof(struct _coex_intf_info));
	buffer = kmalloc(buf_size, GFP_ATOMIC);
	if (!buffer) {
		COEX_LOG_ERROR("%s:Allocate memory failed, please check!\n", __func__);
		mutex_unlock(&pAd->notify_lock);
		return STATUS_FAILURE;
	}

	notify_info = (struct _coex_notify_info *) buffer;
	notify_info->version = 0; /*Fixed value*/
	notify_info->buf_len = sizeof(struct _coex_wifi_info) +
							sizeof(struct _coex_intf_info);
	notify_info->info_num = 1; /*Fixed value*/
	wifi_info = (struct _coex_wifi_info *) &(notify_info->buf[0]);
	wifi_info->info_id = COEX_NOTIFY_INFO_ONOFF;
	wifi_info->info_len = sizeof(struct _coex_intf_info);
	intf_info = (struct _coex_intf_info *) &(wifi_info->info[0]);
	intf_info->status = status;
	intf_info->ringNum = 0; /*useless value*/
	intf_info->seq = seq;

	ret = coex_notify_to_md(pAd, notify_info);
	COEX_LOG_TRACE("%s:status: %d, ret: %d, seq = %d\n",
					__func__, status, ret, seq);
	if (buffer) {
		kfree(buffer);
		buffer = NULL;
	}

	mutex_unlock(&pAd->notify_lock);
	return ret;
}

static void coex_notify_wifi_on_to_md(struct _wifi_md_coex_adapter *pAd)
{
	int ret;

	if (atomic_read(&pAd->notify_seq) >= COEX_NOTIFY_MAX_SEQ)
		atomic_set(&pAd->notify_seq, 0);
	atomic_inc(&pAd->notify_seq);

	COEX_LOG_TRACE("%s:notify wifi on to MD\n", __func__);

	ret = coex_notify_wifi_status_to_md(pAd,
				COEX_NOTIFY_WIFI_ON, atomic_read(&pAd->notify_seq));
	if (ret == STATUS_SUCCESS)
		COEX_LOG_TRACE("%s:notify wifi on to MD success\n", __func__);
	else
		COEX_LOG_ERROR("%s:notify wifi on to MD fail, ret = %d\n", __func__, ret);

}

static void md_check_timer_callback(struct timer_list *t)
{
	enum MD_STATE md_state = ccci_fsm_get_md_state(0);
	struct _wifi_md_coex_adapter *pAd = &wifi_md_coex_adapter;

    if (md_state == READY) {
		coex_notify_wifi_on_to_md(pAd);
		atomic_inc(&pAd->register_cnt);
		if (timer_pending(&md_check_timer))
			timer_delete_sync(&md_check_timer);
    } else {
		if (pAd->md_check_timer_retries < CHECK_MD_STATUS_TIMER_MAX_COUNT) {
			mod_timer(&md_check_timer, jiffies + msecs_to_jiffies(CHECK_INTERVAL));
			pAd->md_check_timer_retries ++;
		} else {
			if (timer_pending(&md_check_timer))
				timer_delete_sync(&md_check_timer);
			COEX_LOG_ERROR("[%s]Check error: md_check_timer_retries = %d,time out!\n", __func__, pAd->md_check_timer_retries);
		}
    }
}

static int coex_get_response(struct _wifi_md_coex_adapter *pAd,
											unsigned long expires)
{
	unsigned long timeout;

	if (!time_is_after_jiffies(expires))
		return STATUS_TIMEOUT;

	timeout = expires - jiffies;
	if (!wait_event_timeout(pAd->notify_wait,
			   (atomic_read(&pAd->notify_seq) == pAd->recv_seq),
			   timeout))
		return STATUS_TIMEOUT;

	return STATUS_SUCCESS;
}

static int coex_wait_for_md_off_complete(struct _wifi_md_coex_adapter *pAd)
{
	int wait_ret = 0;
	unsigned long expires = jiffies + WAIT_MD_OFF_TIMEOUT * HZ / 1000;

	COEX_LOG_TRACE("%s:wait seq = %d from MD\n", __func__,
			atomic_read(&pAd->notify_seq));

	wait_ret = coex_get_response(pAd, expires);
	if (wait_ret == STATUS_TIMEOUT)
		COEX_LOG_ERROR("%s:seq %d timeout\n", __func__,
				atomic_read(&pAd->notify_seq));
	else
		COEX_LOG_TRACE("%s:md off complete. seq(%d:%d)\n", __func__,
				atomic_read(&pAd->notify_seq), pAd->recv_seq);
	return wait_ret;
}

static void coex_notify_wifi_off_to_md(struct _wifi_md_coex_adapter *pAd)
{
	int wait_ret;
	int notify_ret;

	if (atomic_read(&pAd->notify_seq) >= COEX_NOTIFY_MAX_SEQ)
		atomic_set(&pAd->notify_seq, 0);
	atomic_inc(&pAd->notify_seq);

	COEX_LOG_TRACE("%s:notify wifi off to MD\n", __func__);

	notify_ret = coex_notify_wifi_status_to_md(pAd,
					COEX_NOTIFY_WIFI_OFF, atomic_read(&pAd->notify_seq));
	if (notify_ret == STATUS_SUCCESS) {
		wait_ret = coex_wait_for_md_off_complete(pAd);
		if (wait_ret == STATUS_SUCCESS)
			COEX_LOG_TRACE("%s:notify wifi off to MD success\n", __func__);
		else
			COEX_LOG_ERROR("%s:wait md off complete fail, ret = %d\n",
				__func__, wait_ret);
	} else {
		COEX_LOG_ERROR("%s:notify wifi off to MD fail, ret = %d\n",
				__func__, notify_ret);
	}
}

/* kthread coex_tx_handler */
int coex_monitor_fw_handler(void *data)
{
	bool *was_frozen = FALSE;
	struct _wifi_md_coex_adapter *pAd = data;

	if (!pAd) {
		COEX_LOG_ERROR("%s: pAd is NULL!!!\n", __func__);
		return STATUS_FAILURE;
	}

	do {
		call_wifi_md_coex_notifier(QUERY_FW_STATUS, "query WM staus");
		msleep_interruptible(300);
	} while (!kthread_freezable_should_stop(was_frozen));
	return STATUS_SUCCESS;
}
#else
unsigned char coex_merge_wifi_active_bmp(struct _wifi_md_coex_adapter *pAd)
{
	unsigned char i, j, tmp_bmp, merged_bmp = 0;

	for (i = 0; i < MAX_WIFI_DTB_NUM; i++) {
		COEX_LOG_TRACE("wifi_actv_bmp=0x%x\n", pAd->dtb_info.actv_bss_ntf[i].wifi_actv_bmp);
		tmp_bmp = pAd->dtb_info.actv_bss_ntf[i].wifi_actv_bmp;

		for (j = 0; j < 8; j++) {
			if (tmp_bmp & 0x1) {
				merged_bmp <<= 1;
				merged_bmp |= 0x1;
			}

			tmp_bmp >>= 1;
		}
	}

	return merged_bmp;
}

static void coex_fill_wifi_pwr_backoff_msg(struct _wifi_md_coex_adapter *pAd, struct idc_wifi_max_power_backoff_ntf *p_max_power_backoff_msg)
{
	unsigned char i, j, tmp_pwr_bmp[MAX_WIFI_FREQ_NUM] = {0}, tmp_pwr_en_bmp[MAX_WIFI_FREQ_NUM] = {0};

	for (i = 0; i < MAX_WIFI_DTB_NUM; i++) {
		COEX_LOG_TRACE("md_pwr_backoff_en=%d, wifi_pwr_bmp=%x, wf_pwr_backoff_en_bmp=%x, is_wf_scan=%d, lte_cell_bmp=%x, nr_cell_bmp=%x\n",
			pAd->dtb_info.max_power_backoff_ntf[i].md_pwr_backoff_en, pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp,
			pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp, pAd->dtb_info.max_power_backoff_ntf[i].is_wf_scan,
			pAd->dtb_info.max_power_backoff_ntf[i].lte_cell_bmp, pAd->dtb_info.max_power_backoff_ntf[i].nr_cell_bmp);
		p_max_power_backoff_msg->md_pwr_backoff_en |=
			pAd->dtb_info.max_power_backoff_ntf[i].md_pwr_backoff_en;
		p_max_power_backoff_msg->is_wf_scan |=
			pAd->dtb_info.max_power_backoff_ntf[i].is_wf_scan;
		p_max_power_backoff_msg->lte_cell_bmp |=
			pAd->dtb_info.max_power_backoff_ntf[i].lte_cell_bmp;
		p_max_power_backoff_msg->nr_cell_bmp |=
			pAd->dtb_info.max_power_backoff_ntf[i].nr_cell_bmp;

		/* wifi_pwr_bmp/wf_pwr_backoff_en_bmp should be merged per band, and align to oper_freq */
		switch (pAd->dtb_info.idx[i]) {
			case COEX_WIFI_2G:
				tmp_pwr_bmp[0] = pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp;
				tmp_pwr_en_bmp[0] = pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp;
				break;
			case COEX_WIFI_5G_LOW:
			case COEX_WIFI_5G:
				tmp_pwr_bmp[1] = pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp;
				tmp_pwr_en_bmp[1] = pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp;
				break;
			case COEX_WIFI_5G_HIGH:
				tmp_pwr_bmp[2] = pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp;
				tmp_pwr_en_bmp[2] = pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp;
				break;
			case COEX_WIFI_DBDC_5G_HIGH:
				tmp_pwr_bmp[0] = pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp & 0x1;
				tmp_pwr_bmp[2] = (pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp & 0x2) >> 1;
				tmp_pwr_en_bmp[0] = pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp & 0x1;
				tmp_pwr_en_bmp[2] = (pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp & 0x2) >> 1;
				break;
			case COEX_WIFI_DBDC:
			case COEX_WIFI_DBDC_5G_LOW:
				tmp_pwr_bmp[0] = pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp & 0x1;
				tmp_pwr_bmp[1] = (pAd->dtb_info.max_power_backoff_ntf[i].wifi_pwr_bmp & 0x2) >> 1;
				tmp_pwr_en_bmp[0] = pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp & 0x1;
				tmp_pwr_en_bmp[1] = (pAd->dtb_info.max_power_backoff_ntf[i].wf_pwr_backoff_en_bmp & 0x2) >> 1;
				break;
			default:
				break;
		}
	}

	j = 0;
	for (i = 0; i < MAX_WIFI_FREQ_NUM; i++) {
		if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[i] > 0) {
			p_max_power_backoff_msg->wifi_pwr_bmp |= tmp_pwr_bmp[i] << j;
			p_max_power_backoff_msg->wf_pwr_backoff_en_bmp |= tmp_pwr_en_bmp[i] << j;
			j++;
		}
	}

	COEX_LOG_TRACE("%s after merge: md_pwr_backoff_en=%d, wifi_pwr_bmp=%x, wf_pwr_backoff_en_bmp=%x, is_wf_scan=%d, lte_cell_bmp=%x, nr_cell_bmp=%x\n",
		__func__, p_max_power_backoff_msg->md_pwr_backoff_en, p_max_power_backoff_msg->wifi_pwr_bmp,
		p_max_power_backoff_msg->wf_pwr_backoff_en_bmp, p_max_power_backoff_msg->is_wf_scan,
		p_max_power_backoff_msg->lte_cell_bmp, p_max_power_backoff_msg->nr_cell_bmp);

	return;
}

static unsigned char merge_fw2apccci_msg(struct _wifi_md_coex_adapter *pAd,
				struct _fw2apccci_msg *p_fw2apccci_msg)
{
	struct idc_sw_msg_header *p_msg_header = NULL;
	struct idc_wifi_def_param_ntf *p_def_param_ntf = NULL;
	struct idc_wifi_oper_freq_ntf *p_oper_freq_ntf = NULL;
	struct idc_wifi_actv_bss_ntf *p_actv_bss_ntf = NULL;
	struct idc_wifi_consys_tdm_ntf *p_consys_tdm_ntf = NULL;
	struct idc_wifi_max_power_backoff_ntf *p_max_power_backoff_ntf = NULL;
	unsigned char slot_id = p_fw2apccci_msg->pci_slot_id;
	unsigned char i, j;

	p_msg_header = (struct idc_sw_msg_header *) p_fw2apccci_msg->data;
	p_msg_header->seq_num = ++pAd->fw2apccci_seq_num;
	p_msg_header->enqueue_time_stamp = jiffies_to_usecs(jiffies);

	COEX_LOG_TRACE("%s: pci_slot_id=%d, card_type=%d, msg_id=%d\n\n",
		__func__, p_fw2apccci_msg->pci_slot_id, p_fw2apccci_msg->card_type, p_msg_header->msg_id);

	switch (p_msg_header->msg_id) {
		case DEF_PARAM_MSG_ID:{
			COEX_LOG_TRACE("%s:merge DEF_PARAM_MSG_ID(%d), struct_len=%lu\n\n", __func__, DEF_PARAM_MSG_ID, sizeof(struct idc_wifi_def_param_ntf));
			p_def_param_ntf = (struct idc_wifi_def_param_ntf *)p_fw2apccci_msg->data;
			/* update wifi_actv_bmp when wifi_en not set, which means whole wifi card down */
			if (!p_def_param_ntf->wifi_en) {
				/* step1: clear wifi_actv_bmp */
				pAd->dtb_info.actv_bss_ntf[slot_id].wifi_actv_bmp = 0;

				/* step2: set clear_msg_flag */
				pAd->dtb_info.clear_msg_flag = TRUE;

				/* step3: UNREGISTER_WIFI_MD_DTB */
				pAd->dtb_info.idx[slot_id] = 0;
				switch (p_fw2apccci_msg->card_type) {
					case  COEX_WIFI_2G:
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[0] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_high[0] = 0;
						pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 0;
						break;
					case  COEX_WIFI_5G:
					case  COEX_WIFI_5G_LOW:
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_high[1] = 0;
						if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] > 0)
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
						else
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 0;
						break;
					case  COEX_WIFI_DBDC:
					case  COEX_WIFI_DBDC_5G_LOW:
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[0] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_high[0] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_high[1] = 0;
						pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 0;
						if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] > 0)
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
						else
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 0;
						break;
					case  COEX_WIFI_DBDC_5G_HIGH:
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[0] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_high[0] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_high[2] = 0;
						pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 0;
						if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] > 0)
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
						else
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 0;
						break;
					case  COEX_WIFI_5G_HIGH:
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] = 0;
						pAd->dtb_info.oper_freq_ntf.oper_freq_high[2] = 0;
						if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] > 0)
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
						else
							pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 0;
						break;

					default:
						COEX_LOG_ERROR("%s card type err!\n", __func__);
						break;
				}

				COEX_LOG_TRACE("%s: UNREGISTER_WIFI_MD_DTB\n", __func__);
				for (i = 0; i < MAX_WIFI_DTB_NUM; i++) {
					COEX_LOG_TRACE("idx=%d, oper_freq_low=%d, oper_freq_high=%d, oper_freq_3wire_group=%d\n", pAd->dtb_info.idx[i],
						pAd->dtb_info.oper_freq_ntf.oper_freq_low[i], pAd->dtb_info.oper_freq_ntf.oper_freq_high[i],
						pAd->dtb_info.oper_freq_ntf.oper_freq_3wire_group[i]);
				}
			}

			/* merge wifi_en */
			pAd->dtb_info.def_param_ntf[slot_id].wifi_en = p_def_param_ntf->wifi_en;
			for (i = 0; i < MAX_WIFI_DTB_NUM; i++) {
				COEX_LOG_TRACE("wifi_actv_bmp=0x%x\n", pAd->dtb_info.actv_bss_ntf[i].wifi_actv_bmp);
				COEX_LOG_TRACE("wifi_en[%d]=%d ", i, pAd->dtb_info.def_param_ntf[i].wifi_en);
				p_def_param_ntf->wifi_en |= pAd->dtb_info.def_param_ntf[i].wifi_en;
			}
			p_def_param_ntf->tri_band = pAd->dtb_info.tri_band;
			COEX_LOG_TRACE("\n");
		}
		break;
		case OPER_FREQ_MSG_ID: {
			COEX_LOG_WARN("%s:merge OPER_FREQ_MSG_ID(%d), struct_len=%lu\n\n", __func__, OPER_FREQ_MSG_ID, sizeof(struct idc_wifi_oper_freq_ntf));
			p_oper_freq_ntf = (struct idc_wifi_oper_freq_ntf *)p_fw2apccci_msg->data;
			COEX_LOG_WARN("%s: DUMP RAW DATA rx from FW: \n", __func__);
			COEX_WARN_DUMP_RAW_DATA(p_fw2apccci_msg->data, p_fw2apccci_msg->data_len);

			/* merge band0_freq_num/band1_freq_num/freq_low/freq_high */
			for (i = 0; i < MAX_WIFI_FREQ_NUM; i++) {
				COEX_LOG_WARN("received from fw: \noper_freq_low=%d, oper_freq_high=%d, oper_freq_3wire_group=%d\n",
					p_oper_freq_ntf->oper_freq_low[i], p_oper_freq_ntf->oper_freq_high[i],
					p_oper_freq_ntf->oper_freq_3wire_group[i]);
			}

			switch (p_fw2apccci_msg->card_type) {
				case  COEX_WIFI_2G:
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[0] = p_oper_freq_ntf->oper_freq_low[0];
					pAd->dtb_info.oper_freq_ntf.oper_freq_high[0] = p_oper_freq_ntf->oper_freq_high[0];
					pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 1;
					break;
				case  COEX_WIFI_5G:
				case  COEX_WIFI_5G_LOW:
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] = p_oper_freq_ntf->oper_freq_low[0];
					pAd->dtb_info.oper_freq_ntf.oper_freq_high[1] = p_oper_freq_ntf->oper_freq_high[0];
					if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] > 0)
						pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 2;
					else
						pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
					break;
				case  COEX_WIFI_DBDC:
				case  COEX_WIFI_DBDC_5G_LOW:
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[0] = p_oper_freq_ntf->oper_freq_low[0];
					pAd->dtb_info.oper_freq_ntf.oper_freq_high[0] = p_oper_freq_ntf->oper_freq_high[0];
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] = p_oper_freq_ntf->oper_freq_low[1];
					pAd->dtb_info.oper_freq_ntf.oper_freq_high[1] = p_oper_freq_ntf->oper_freq_high[1];
					for (i = 0; i < 2; i++) {
						if (p_oper_freq_ntf->oper_freq_low[i] > 0) {
							if (p_oper_freq_ntf->oper_freq_high[i] < 2500) /* 2.4G */
								pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 1;
							else if (p_oper_freq_ntf->oper_freq_high[i] < 5900) { /* 5G */
								if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] > 0)
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 2;
								else
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
							}
						} else { 	/* any band off on DBDC card */
							if (i == 0) { /* 2.4G */
								pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 0;
								pAd->dtb_info.actv_bss_ntf[slot_id].wifi_actv_bmp &= 0b10;
							}
							else { /* 5G */
								if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] > 0)
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
								else
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 0;

								pAd->dtb_info.actv_bss_ntf[slot_id].wifi_actv_bmp &= 0b01;
							}
						}
					}
					break;
				case  COEX_WIFI_DBDC_5G_HIGH:
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[0] = p_oper_freq_ntf->oper_freq_low[0];
					pAd->dtb_info.oper_freq_ntf.oper_freq_high[0] = p_oper_freq_ntf->oper_freq_high[0];
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] = p_oper_freq_ntf->oper_freq_low[1];
					pAd->dtb_info.oper_freq_ntf.oper_freq_high[2] = p_oper_freq_ntf->oper_freq_high[1];
					for (i = 0; i < 2; i++) {
						if (p_oper_freq_ntf->oper_freq_low[i] > 0) {
							if (p_oper_freq_ntf->oper_freq_high[i] < 2500) /* 2.4G */
								pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 1;
							else if (p_oper_freq_ntf->oper_freq_high[i] < 5900) { /* 5G */
								if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] > 0)
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 2;
								else
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
							}
						} else { 	/* any band off on DBDC card */
							if (i == 0) { /* 2.4G */
								pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num = 0;
								pAd->dtb_info.actv_bss_ntf[slot_id].wifi_actv_bmp &= 0b10;
							}
							else { /* 5G */
								if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] > 0)
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
								else
									pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 0;

								pAd->dtb_info.actv_bss_ntf[slot_id].wifi_actv_bmp &= 0b01;
							}
						}
					}
					break;
				case  COEX_WIFI_5G_HIGH:
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[2] = p_oper_freq_ntf->oper_freq_low[0];
					pAd->dtb_info.oper_freq_ntf.oper_freq_high[2] = p_oper_freq_ntf->oper_freq_high[0];
					if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[1] > 0)
						pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 2;
					else
						pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num = 1;
					break;

				default:
					COEX_LOG_ERROR("%s card type err!\n", __func__);
					break;
			}

			if (pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num > 0) {
				p_oper_freq_ntf->band0_oper_freq_num =
					pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num;
				p_oper_freq_ntf->band1_oper_freq_num =
					pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num;
			} else {
				p_oper_freq_ntf->band0_oper_freq_num =
					pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num;
				p_oper_freq_ntf->band1_oper_freq_num = 0;
			}
			COEX_LOG_TRACE("band0_oper_freq_num=%d, band1_oper_freq_num=%d\n",
				pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num, pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num);

			/* First initiate oper_freq fields in msg */
			for (i = 0; i < MAX_WIFI_FREQ_NUM; i++) {
				memset(p_oper_freq_ntf->oper_freq_low, 0, sizeof(p_oper_freq_ntf->oper_freq_low));
				memset(p_oper_freq_ntf->oper_freq_high, 0, sizeof(p_oper_freq_ntf->oper_freq_high));
				memset(p_oper_freq_ntf->oper_freq_3wire_group, 0, sizeof(p_oper_freq_ntf->oper_freq_3wire_group));
			}

			j = 0;
			for (i = 0; i < MAX_WIFI_FREQ_NUM; i++) {
				COEX_LOG_WARN("pAd: oper_freq_low=%d, oper_freq_high=%d, oper_freq_3wire_group=%d\n",
					pAd->dtb_info.oper_freq_ntf.oper_freq_low[i], pAd->dtb_info.oper_freq_ntf.oper_freq_high[i],
					pAd->dtb_info.oper_freq_ntf.oper_freq_3wire_group[i]);
				if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[i] > 0) {
					p_oper_freq_ntf->oper_freq_low[j] = pAd->dtb_info.oper_freq_ntf.oper_freq_low[i];
					p_oper_freq_ntf->oper_freq_high[j] = pAd->dtb_info.oper_freq_ntf.oper_freq_high[i];
					p_oper_freq_ntf->oper_freq_3wire_group[j] = pAd->dtb_info.oper_freq_ntf.oper_freq_3wire_group[i];
					j++;
				}
			}
		}
		break;
		case ACTV_BSS_MSG_ID: {
			COEX_LOG_TRACE("%s:merge ACTV_BSS_MSG_ID(%d), struct_len=%lu\n\n", __func__, ACTV_BSS_MSG_ID, sizeof(struct idc_wifi_actv_bss_ntf));
			p_actv_bss_ntf = (struct idc_wifi_actv_bss_ntf *)p_fw2apccci_msg->data;

			pAd->dtb_info.actv_bss_ntf[slot_id].wifi_actv_bmp = p_actv_bss_ntf->wifi_actv_bmp;
			p_actv_bss_ntf->wifi_actv_bmp = coex_merge_wifi_active_bmp(pAd);
		}
		break;
		case CONSYS_TDM_NTF: {
			COEX_LOG_TRACE("%s:merge CONSYS_TDM_NTF(%d), struct_len=%lu\n\n", __func__, CONSYS_TDM_NTF, sizeof(struct idc_wifi_consys_tdm_ntf));
			p_consys_tdm_ntf = (struct idc_wifi_consys_tdm_ntf *)p_fw2apccci_msg->data;
			switch (p_fw2apccci_msg->card_type) {
				case  COEX_WIFI_2G:
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[0] = p_consys_tdm_ntf->is_tdm_for_lte[0];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[0] = p_consys_tdm_ntf->is_tdm_for_nr[0];
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[0] = p_consys_tdm_ntf->lte_cc_bmp[0];
					pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[0] = p_consys_tdm_ntf->nr_cc_bmp[0];
					break;
				case  COEX_WIFI_5G:
				case  COEX_WIFI_5G_LOW:
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[1] = p_consys_tdm_ntf->is_tdm_for_lte[0];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[1] = p_consys_tdm_ntf->is_tdm_for_nr[0];
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[1] = p_consys_tdm_ntf->lte_cc_bmp[0];
					pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[1] = p_consys_tdm_ntf->nr_cc_bmp[0];
					break;
				case  COEX_WIFI_DBDC:
				case  COEX_WIFI_DBDC_5G_LOW:
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[0] = p_consys_tdm_ntf->is_tdm_for_lte[0];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[0] = p_consys_tdm_ntf->is_tdm_for_nr[0];
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[0] = p_consys_tdm_ntf->lte_cc_bmp[0];
					pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[0] = p_consys_tdm_ntf->nr_cc_bmp[0];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[1] = p_consys_tdm_ntf->is_tdm_for_lte[1];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[1] = p_consys_tdm_ntf->is_tdm_for_nr[1];
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[1] = p_consys_tdm_ntf->lte_cc_bmp[1];
					pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[1] = p_consys_tdm_ntf->nr_cc_bmp[1];
					break;
				case  COEX_WIFI_5G_HIGH:
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[2] = p_consys_tdm_ntf->is_tdm_for_lte[0];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[2] = p_consys_tdm_ntf->is_tdm_for_nr[0];
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[2] = p_consys_tdm_ntf->lte_cc_bmp[0];
					pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[2] = p_consys_tdm_ntf->nr_cc_bmp[0];
					break;
				case  COEX_WIFI_DBDC_5G_HIGH:
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[0] = p_consys_tdm_ntf->is_tdm_for_lte[0];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[0] = p_consys_tdm_ntf->is_tdm_for_nr[0];
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[0] = p_consys_tdm_ntf->lte_cc_bmp[0];
					pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[0] = p_consys_tdm_ntf->nr_cc_bmp[0];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[2] = p_consys_tdm_ntf->is_tdm_for_lte[1];
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[2] = p_consys_tdm_ntf->is_tdm_for_nr[1];
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[2] = p_consys_tdm_ntf->lte_cc_bmp[1];
					pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[2] = p_consys_tdm_ntf->nr_cc_bmp[1];
					break;
			}

			j = 0;
			for (i = 0; i < MAX_WIFI_FREQ_NUM; i++) {
				COEX_LOG_TRACE("is_tdm_for_lte=%d, is_tdm_for_nr=%d, lte_cc_bmp=%x, nr_cc_bmp=%x\n",
					pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[i], pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[i],
					pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[i], pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[i]);
				if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[i] > 0) {
					p_consys_tdm_ntf->is_tdm_for_lte[j] = pAd->dtb_info.consys_tdm_ntf.is_tdm_for_lte[i];
					p_consys_tdm_ntf->is_tdm_for_nr[j] = pAd->dtb_info.consys_tdm_ntf.is_tdm_for_nr[i];
					p_consys_tdm_ntf->lte_cc_bmp[j] = pAd->dtb_info.consys_tdm_ntf.lte_cc_bmp[i];
					p_consys_tdm_ntf->nr_cc_bmp[j] = pAd->dtb_info.consys_tdm_ntf.nr_cc_bmp[i];
					j++;
				}
			}
		}
		break;
		case MAX_POWER_BACKOFF_ID: {
			COEX_LOG_TRACE("%s:merge MAX_POWER_BACKOFF_ID(%d), struct_len=%lu\n\n", __func__, MAX_POWER_BACKOFF_ID, sizeof(struct idc_wifi_max_power_backoff_ntf));
			p_max_power_backoff_ntf = (struct idc_wifi_max_power_backoff_ntf *)p_fw2apccci_msg->data;
			pAd->dtb_info.max_power_backoff_ntf[slot_id].md_pwr_backoff_en =
				p_max_power_backoff_ntf->md_pwr_backoff_en;
			pAd->dtb_info.max_power_backoff_ntf[slot_id].wifi_pwr_bmp =
				p_max_power_backoff_ntf->wifi_pwr_bmp;
			pAd->dtb_info.max_power_backoff_ntf[slot_id].wf_pwr_backoff_en_bmp =
				p_max_power_backoff_ntf->wf_pwr_backoff_en_bmp;
			pAd->dtb_info.max_power_backoff_ntf[slot_id].is_wf_scan =
				p_max_power_backoff_ntf->is_wf_scan;
			pAd->dtb_info.max_power_backoff_ntf[slot_id].lte_cell_bmp =
				p_max_power_backoff_ntf->lte_cell_bmp;
			pAd->dtb_info.max_power_backoff_ntf[slot_id].nr_cell_bmp =
				p_max_power_backoff_ntf->nr_cell_bmp;

			p_max_power_backoff_ntf->wifi_pwr_bmp = 0;
			p_max_power_backoff_ntf->wf_pwr_backoff_en_bmp = 0;
			coex_fill_wifi_pwr_backoff_msg(pAd, p_max_power_backoff_ntf);

		}
		break;

		default:
			break;
	}

	return p_msg_header->msg_id;
}

/* simulate oper freq msg when DTB down */
static void simulate_oper_freq_msg(struct _wifi_md_coex_adapter *pAd)
{
	struct _fw2apccci_msg *p_sim_fw2apccci_msg;
	struct idc_sw_msg_header *p_msg_header = NULL;
	struct idc_wifi_oper_freq_ntf *p_oper_freq_ntf = NULL;
	unsigned char i, j;

	p_sim_fw2apccci_msg = kmalloc(sizeof(struct _fw2apccci_msg) +
		    sizeof(struct idc_wifi_oper_freq_ntf), GFP_ATOMIC);
	if (p_sim_fw2apccci_msg == NULL) {
		COEX_LOG_ERROR("%s: p_sim_fw2apccci_msg is NULL\n", __func__);
		return;
	}
	memset(p_sim_fw2apccci_msg, 0, sizeof(struct _fw2apccci_msg) +
		    sizeof(struct idc_wifi_oper_freq_ntf));

	p_oper_freq_ntf = (struct idc_wifi_oper_freq_ntf *)p_sim_fw2apccci_msg->data;
	p_msg_header = (struct idc_sw_msg_header *) p_sim_fw2apccci_msg->data;

	/* s1: simulate msg_id = 22 */
	p_msg_header->msg_id = OPER_FREQ_MSG_ID;
	p_msg_header->seq_num = ++pAd->fw2apccci_seq_num;
	p_msg_header->enqueue_time_stamp = jiffies_to_usecs(jiffies);

	/* s2: simulate freq_ntf data */
	p_sim_fw2apccci_msg->data_len = sizeof(struct idc_wifi_oper_freq_ntf);
	if (pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num > 0) {
		p_oper_freq_ntf->band0_oper_freq_num =
			pAd->dtb_info.oper_freq_ntf.band0_oper_freq_num;
		p_oper_freq_ntf->band1_oper_freq_num =
			pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num;
	} else {
		p_oper_freq_ntf->band0_oper_freq_num =
			pAd->dtb_info.oper_freq_ntf.band1_oper_freq_num;
		p_oper_freq_ntf->band1_oper_freq_num = 0;
	}

	j = 0;
	for (i = 0; i < MAX_WIFI_FREQ_NUM; i++) {
		if (pAd->dtb_info.oper_freq_ntf.oper_freq_low[i] > 0) {
			p_oper_freq_ntf->oper_freq_low[j] = pAd->dtb_info.oper_freq_ntf.oper_freq_low[i];
			p_oper_freq_ntf->oper_freq_high[j] = pAd->dtb_info.oper_freq_ntf.oper_freq_high[i];
			p_oper_freq_ntf->oper_freq_3wire_group[j] = pAd->dtb_info.oper_freq_ntf.oper_freq_3wire_group[i];
			j++;
		}
	}

	/* s3: send data */
	fw2apccci_ctrl_basic_enq(pAd,
				 sizeof(struct _fw2apccci_msg) +
				 p_sim_fw2apccci_msg->data_len,
				 p_sim_fw2apccci_msg);

	/* FIXME dump data */
	COEX_LOG_TRACE("%s: simulate_oper_freq_msg DUMP DATA: \n", __func__);
	COEX_DUMP_RAW_DATA(p_sim_fw2apccci_msg->data, p_sim_fw2apccci_msg->data_len);

	kfree(p_sim_fw2apccci_msg);
	p_sim_fw2apccci_msg = NULL;

	return;
}

/* simulate actv bss msg when DTB down */
static void simulate_actv_bss_msg(struct _wifi_md_coex_adapter *pAd)
{
	struct _fw2apccci_msg *p_sim_fw2apccci_msg;
	struct idc_sw_msg_header *p_msg_header = NULL;
	struct idc_wifi_actv_bss_ntf *p_actv_bss_ntf = NULL;

	p_sim_fw2apccci_msg =
	    kmalloc(sizeof(struct _fw2apccci_msg) +
		    sizeof(struct idc_wifi_actv_bss_ntf), GFP_ATOMIC);
	if (p_sim_fw2apccci_msg == NULL) {
		COEX_LOG_ERROR("%s: p_sim_fw2apccci_msg is NULL\n", __func__);
		return;
	}
	memset(p_sim_fw2apccci_msg, 0, sizeof(struct _fw2apccci_msg) +
		    sizeof(struct idc_wifi_actv_bss_ntf));
	p_actv_bss_ntf = (struct idc_wifi_actv_bss_ntf *)p_sim_fw2apccci_msg->data;
	p_msg_header = (struct idc_sw_msg_header *) p_sim_fw2apccci_msg->data;

	/* s1: simulate msg_id = 33 */
	p_msg_header->msg_id = ACTV_BSS_MSG_ID;
	p_msg_header->seq_num = ++pAd->fw2apccci_seq_num;
	p_msg_header->enqueue_time_stamp = jiffies_to_usecs(jiffies);

	/* s2: simulate bss data */
	p_sim_fw2apccci_msg->data_len = sizeof(struct idc_wifi_actv_bss_ntf);
	p_actv_bss_ntf->wifi_actv_bmp = coex_merge_wifi_active_bmp(pAd);

	/* s3: send data */
	fw2apccci_ctrl_basic_enq(pAd,
				 sizeof(struct _fw2apccci_msg) +
				 p_sim_fw2apccci_msg->data_len,
				 p_sim_fw2apccci_msg);

	/* FIXME dump data */
	COEX_LOG_TRACE("%s: simulate_actv_bss_msg DUMP DATA: \n", __func__);
	COEX_DUMP_RAW_DATA(p_sim_fw2apccci_msg->data, p_sim_fw2apccci_msg->data_len);

	kfree(p_sim_fw2apccci_msg);
	p_sim_fw2apccci_msg = NULL;

	return;
}
#endif /* COEX_DIRECT_PATH */

static int debug_level_set(void *data, u64 level)
{
	struct _wifi_md_coex_adapter *pAd = data;
	unsigned char old_level = pAd->debug_level;
	unsigned char new_level = (unsigned char)level;

	pAd->debug_level = new_level;
	printk("set debug_level=(%d->%d)\n",
			old_level, new_level);
	return STATUS_SUCCESS;
}

static int debug_level_get(void *data, u64 *level)
{
	struct _wifi_md_coex_adapter *pAd = data;
	printk("debug_level=%d\n", pAd->debug_level);
	printk("request_msi_bit=%ld\n", pAd->request_msi_bit);
	return STATUS_SUCCESS;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_debug_level, debug_level_get,
			 debug_level_set, "%lld\n");

/* recv msg from MD */
static void coex_tx_msg_handle(struct _wifi_md_coex_adapter *pAd,
								struct _md_msg *msg, unsigned long msg_len)
{
#ifdef COEX_DIRECT_PATH
	struct _md_notify_info *md_info;

	if (msg->dest_user_id != CCCI_USER_ID_COEX) {
		COEX_LOG_TRACE("%s: unaccepted user_id(%d)!\n",
			__func__, msg->dest_user_id);
		return;
	}

	switch (msg->msg_id) {
		case CCCI_MSG_ID_RESET_IND:
			COEX_LOG_TRACE("%s: received RESET IND\n", __func__);
			if (atomic_read(&pAd->register_cnt))
				coex_notify_wifi_on_to_md(pAd);
			break;
		case CCCI_MSG_ID_MD_NOTIFY:
			COEX_LOG_TRACE("%s: received MD NOTIFY\n", __func__);
			md_info = (struct _md_notify_info *)msg->buf;
			if (md_info->info_type == MD_NOTIFY_INFO_ONOFF) {
				pAd->recv_seq = md_info->buf[0];
				COEX_LOG_TRACE("%s:get seq = %d from MD\n",
						__func__, pAd->recv_seq);
				wake_up(&pAd->notify_wait);
			}
			break;
		default:
			COEX_LOG_ERROR("%s: unsupport RSP MDG_ID[%d] from MD.\n",
				__func__, msg->msg_id);
			break;

	}
#else
	msg->len = msg_len;
	call_wifi_md_coex_notifier(APCCCI_DRIVER_FW, msg);
#endif
}

/* Realize the Rx notifier chain func */
int coex_rx_event(struct notifier_block *nb, unsigned long event, void *msg)
{
	struct _fw2apccci_msg *p_fw2apccci_msg;
	struct _wifi_md_coex_adapter *pAd;
#ifdef COEX_DIRECT_PATH
	enum MD_STATE md_state = ccci_fsm_get_md_state(0);
#endif

	if (msg == NULL) {
		COEX_LOG_ERROR("%s: msg is null!\n", __func__);
		return NOTIFY_DONE;
	}

	pAd = container_of(nb, struct _wifi_md_coex_adapter, fw2apccci_rx_notifier);
	p_fw2apccci_msg = (struct _fw2apccci_msg *) msg;

	switch (event) {
#ifndef COEX_DIRECT_PATH
	case FW_DRIVER_APCCCI: {

		/* FIXME dump data */
		COEX_LOG_TRACE("%s: FW2APCCCI DUMP RAW DATA, pci_slot_id=%d, card_type=%d, len=%d\n",
			__func__, p_fw2apccci_msg->pci_slot_id, p_fw2apccci_msg->card_type, p_fw2apccci_msg->data_len);
 		COEX_DUMP_RAW_DATA(p_fw2apccci_msg->data, p_fw2apccci_msg->data_len);

		/* step1: parse the idx to determine whether merge */
		if (p_fw2apccci_msg->card_type != 0) {
			unsigned char msg_id = merge_fw2apccci_msg(pAd, p_fw2apccci_msg);

			/* FIXME dump data */
			if (msg_id == DEF_PARAM_MSG_ID || msg_id == OPER_FREQ_MSG_ID || msg_id == ACTV_BSS_MSG_ID) {
				COEX_LOG_WARN("%s: DUMP DATA AFTER MERGE: idx=%d, msg_id=%d\n", __func__, p_fw2apccci_msg->card_type, msg_id);
				COEX_WARN_DUMP_RAW_DATA(p_fw2apccci_msg->data, p_fw2apccci_msg->data_len);
			}
			else {
				COEX_LOG_TRACE("%s: DUMP DATA AFTER MERGE: idx=%d, msg_id=%d\n", __func__, p_fw2apccci_msg->card_type, msg_id);
				COEX_DUMP_RAW_DATA(p_fw2apccci_msg->data, p_fw2apccci_msg->data_len);
			}
		}

		/* step2: send data to apccci */
		fw2apccci_ctrl_basic_enq(pAd, sizeof(struct _fw2apccci_msg) +
						p_fw2apccci_msg->data_len,
						p_fw2apccci_msg);

		/* step3: simulate msg_id=22&33 and send to apccci */
		if (pAd->dtb_info.clear_msg_flag) {
			pAd->dtb_info.clear_msg_flag = FALSE;
			simulate_oper_freq_msg(pAd);
			simulate_actv_bss_msg(pAd);
		}
	}
	break;
#endif /* #ifndef COEX_DIRECT_PATH */
	case REGISTER_WIFI_MD_DTB: {
		if (!atomic_read(&pAd->register_cnt)) {
			pAd->dtb_info.idx[p_fw2apccci_msg->pci_slot_id] = p_fw2apccci_msg->card_type;
			pAd->pci_num = p_fw2apccci_msg->pci_num;
			coex_update_wifi_dtb_info(pAd);
			COEX_LOG_TRACE("%s: REGISTER_WIFI_MD_DTB, pci_slot_id=%d, card_type=%d, pci_num=%d, register_cnt=%d\n",
				__func__, p_fw2apccci_msg->pci_slot_id, p_fw2apccci_msg->card_type, pAd->pci_num, atomic_read(&pAd->register_cnt));
			COEX_LOG_TRACE("%s: idx=%d-%d-%d", __func__,
				pAd->dtb_info.idx[0], pAd->dtb_info.idx[1], pAd->dtb_info.idx[2]);
#ifdef COEX_DIRECT_PATH
			//Only the first interface can do the following operations
			coex_request_msi(pAd, p_fw2apccci_msg);
			coex_write_mcif_bank4_memory(pAd, p_fw2apccci_msg);
			if (md_state == READY) {
				coex_notify_wifi_on_to_md(pAd);
				atomic_inc(&pAd->register_cnt);
			} else {
				if (!timer_pending(&md_check_timer)) {
					timer_setup(&md_check_timer, md_check_timer_callback, 0);
					mod_timer(&md_check_timer, jiffies + msecs_to_jiffies(CHECK_INTERVAL));
				}
			}
#endif /* COEX_DIRECT_PATH */
		} else
			COEX_LOG_TRACE("%s: Alreadly register, Register_Cnt=%d\n", __func__, atomic_read(&pAd->register_cnt));
	}
	break;
	case UNREGISTER_WIFI_MD_DTB: {
#ifdef COEX_DIRECT_PATH
		COEX_LOG_TRACE("%s: UNREGISTER_WIFI_MD_DTB, pci_slot_id=%d, card_type=%d, register_cnt=%d\n",
			__func__, p_fw2apccci_msg->pci_slot_id, p_fw2apccci_msg->card_type, atomic_read(&pAd->register_cnt));
		//Only the last interface can do the following operations
		if (atomic_read(&pAd->register_cnt)) {
			coex_notify_wifi_off_to_md(pAd);
			coex_release_msi(pAd, p_fw2apccci_msg);
			coex_clear_mcif_bank4_memory(pAd, p_fw2apccci_msg);
			atomic_dec(&pAd->register_cnt);
		}
#endif /* COEX_DIRECT_PATH */
	}
	break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

/* kthread coex_tx_handler */
int coex_tx_handler(void *data)
{
	int ret = 0;
	char *msg = NULL;
	enum MD_STATE md_state;
	bool *was_frozen = FALSE;
	unsigned long msg_len = 0;
	struct _wifi_md_coex_adapter *pAd = data;

	if (!pAd) {
		COEX_LOG_ERROR("%s: pAd is NULL!!!\n", __func__);
		return STATUS_FAILURE;
	}

	//register the interrupt signal listened by the thread (i.e. be waken up by SIGKILL)
	allow_signal(SIGKILL);

#ifdef COEX_DIRECT_PATH
	msg = (char *)(&pAd->md_msg);
	msg_len = sizeof(pAd->md_msg);
#else
	msg = (char *)(pAd->md_msg.data);
	msg_len = MAX_MSG_LEN;
#endif /* COEX_DIRECT_PATH */

	do {
		if (signal_pending(current)) {
			COEX_LOG_ERROR("%s: catch SIGKILL, thread exit!\n", __func__);
			break;
		}

		if (pAd->apccci_idx <= 0) {
			COEX_LOG_ERROR("%s: open ccci port fail!!!\n", __func__);
			break;
		}

		md_state = ccci_fsm_get_md_state(0);
		if (md_state != READY) {
			/* modem not ready */
			msleep(100);
			continue;
		}

		/* when MD has no data, mtk_ccci_read_data will be blocked.
		 * when unloading wifi_md_coex.ko, SIGKILL needs to be sent to exit the thread
		*/
		ret = coex_read_data_from_md(pAd->apccci_idx, msg, msg_len);
		if (!ret) {
			COEX_LOG_ERROR("%s: read_data fail !!!\n", __func__);
			continue;
		}

		coex_tx_msg_handle(pAd, &pAd->md_msg, ret);
	} while (!kthread_freezable_should_stop(was_frozen));
	return STATUS_SUCCESS;
}

static __init int wifi_md_coex_init(void)
{
	struct _wifi_md_coex_adapter *pAd = &wifi_md_coex_adapter;

	/* init fw2apccci rx data */
	pAd->debug_level = COEX_DBG_LVL_WARN;
	pAd->fw2apccci_seq_num = 0;
	memset(&pAd->dtb_info, 0, sizeof(struct _wifi_dtb_info));
	pAd->fw2apccci_rx_notifier.notifier_call = coex_rx_event;
	register_wifi_md_coex_notifier(&pAd->fw2apccci_rx_notifier, COEX_NOTIFIER);
	COEX_LOG_TRACE("%s, addr= %p\n", __func__, &pAd->fw2apccci_rx_notifier);

#ifdef COEX_DIRECT_PATH
	atomic_set(&pAd->notify_seq, 0);
	atomic_set(&pAd->register_cnt, 0);
	pAd->request_msi_bit = 0;
	pAd->ioremap_flag = FALSE;
	mutex_init(&pAd->notify_lock);
	init_waitqueue_head(&pAd->notify_wait);
	pAd->p_monitor_fw_task = kthread_run(coex_monitor_fw_handler,
					       (void *) pAd, "monitor_fw_task");
	if (IS_ERR(pAd->p_monitor_fw_task)) {
		pAd->p_monitor_fw_task = NULL;
		COEX_LOG_ERROR("%s:monitor_fw_task create fail!!!\n", __func__);
	}

	/* init apccci port */
	pAd->apccci_idx = mtk_ccci_open_port("ccci_wifi7");
	if (pAd->apccci_idx <= 0)
		COEX_LOG_ERROR("%s: open port fail! idx = %d", __func__,
		       pAd->apccci_idx);
#else
	/* init apccci port */
	pAd->apccci_idx = mtk_ccci_open_port("ccci_wifi");
	if (pAd->apccci_idx <= 0)
		COEX_LOG_ERROR("%s: open port fail! idx = %d", __func__,
		       pAd->apccci_idx);
	/* init fw2apccci enq/deq ctrl */
	fw2apccci_ctrl_init(pAd);
#endif /* COEX_DIRECT_PATH */

	/* init p_apccci2fw kthread */
	pAd->p_apccci2fw_tx_task = kthread_run(coex_tx_handler,
					       (void *) pAd, "apccci2fw_task");
	if (IS_ERR(pAd->p_apccci2fw_tx_task)) {
		pAd->p_apccci2fw_tx_task = NULL;
		COEX_LOG_ERROR("%s:apccci2fw_task create fail!!!\n", __func__);
	}

	pAd->debugfs_dir = debugfs_create_dir(COEX_DEBUGFS_DIR_NAME, NULL);
	if (!pAd->debugfs_dir)
		return STATUS_FAILURE;

	debugfs_create_file(COEX_DEBUGFS_FILE_NAME, 0644,
				pAd->debugfs_dir, pAd, &fops_debug_level);

	COEX_LOG_TRACE("wifi md coex init\n");
	return STATUS_SUCCESS;
}

static __exit void wifi_md_coex_exit(void)
{
	struct _wifi_md_coex_adapter *pAd = &wifi_md_coex_adapter;

	debugfs_remove_recursive(pAd->debugfs_dir);

	/* exit p_apccci2fw kthread */
	if (!IS_ERR(pAd->p_apccci2fw_tx_task)) {
		/* send signal(SIGKILL) to exit the thread*/
		send_sig(SIGKILL, pAd->p_apccci2fw_tx_task, SEND_SIG_SRC_KERNEL);
		kthread_stop(pAd->p_apccci2fw_tx_task);
		pAd->p_apccci2fw_tx_task = NULL;
	}

#ifdef COEX_DIRECT_PATH
	/* exit p_monitor_fw kthread */
	if (!IS_ERR(pAd->p_monitor_fw_task)) {
		if (kthread_stop(pAd->p_monitor_fw_task) == 0)
			pAd->p_monitor_fw_task = NULL;
		else
			COEX_LOG_ERROR("%s:p_monitor_fw_task stop fail!!!\n", __func__);
	}

	/* exit apccci port */
	if (mtk_ccci_close_port(pAd->apccci_idx) < 0)
		COEX_LOG_ERROR("%s: close mtk ccci port fail!!!\n", __func__);

	pAd->ioremap_flag = FALSE;
	pAd->request_msi_bit = 0;
	pAd->md_check_timer_retries = 0;
	/* unmap virtual addr */
	iounmap((void *)(pAd->csr_addr));
	atomic_set(&pAd->register_cnt, 0);
	atomic_set(&pAd->notify_seq, 0);
#else /* COEX_DIRECT_PATH */
	/* exit fw2apccci enq/deq ctrl */
	fw2apccci_ctrl_exit(pAd);
#endif

	/* unregister rx notifier chain */
	unregister_wifi_md_coex_notifier(&pAd->fw2apccci_rx_notifier, COEX_NOTIFIER);
	pAd->fw2apccci_rx_notifier.notifier_call = NULL;

	COEX_LOG_TRACE("wifi md coex exit\n");
}

module_init(wifi_md_coex_init);
module_exit(wifi_md_coex_exit);
