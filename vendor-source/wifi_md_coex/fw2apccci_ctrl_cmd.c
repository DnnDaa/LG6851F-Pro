// SPDX-License-Identifier: LGPL-2.1
/*
 * receive data from wifi FW and send data to APCCCI
 *
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Cheery Chen <Cheery.Chen@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>

#include "fw2apccci_ctrl_cmd.h"
void os_cmd_up(struct _os_task *p_cmd_queue_task)
{
	struct _os_task *p_task = p_cmd_queue_task;

	p_task->kthread_running = TRUE;
	wake_up(&(p_task)->kthread_q);
}

static void free_fw2apccci_cmd(struct kref *ref)
{
	struct _fw2apccci_cmd_queue_elmt *p_cmd_queue_elmt =
	    container_of(ref, struct _fw2apccci_cmd_queue_elmt, ref_cnt);

	if (p_cmd_queue_elmt == NULL)
		COEX_LOG_ERROR("\n%s: p_cmd_queue_elmt = NULL!!!\n", __func__);
	else {
		/* step1. free buffer */
		if (p_cmd_queue_elmt->buffer == NULL)
			COEX_LOG_ERROR("\n%s: buffer = NULL!!!\n", __func__);
		else {
			kfree(p_cmd_queue_elmt->buffer);
			p_cmd_queue_elmt->buffer = NULL;
		}
		/* step2. free p_cmd_queue_elmt. buffer may be NULL */
		kfree(p_cmd_queue_elmt);
		p_cmd_queue_elmt = NULL;
	}
}

int fw2apccci_enqueue_cmd(struct _wifi_md_coex_adapter *pAd,
			  struct _fw2apccci_ctrl_txd fw2apccci_ctrl_txd)
{
	int status = STATUS_SUCCESS;
	struct _fw2apccci_cmd_queue_elmt *p_cmd_queue_elmt = NULL;
	struct _fw2apccci_cmd_queue *p_cmd_queue = NULL;
	struct _fw2apccci_ctrl_t *p_fw2apccci_ctrl = &pAd->fw2apccci_ctrl;

	p_cmd_queue_elmt =
	    kmalloc(sizeof(struct _fw2apccci_cmd_queue_elmt), GFP_ATOMIC);
	if (p_cmd_queue_elmt == NULL) {
		COEX_LOG_ERROR("\n%s: p_cmd_queue_elmt is NULL !!!\n", __func__);
		return STATUS_FAILURE;
	}
	memset(p_cmd_queue_elmt, 0, sizeof(struct _fw2apccci_cmd_queue_elmt));
	/* init lock */
	spin_lock_init(&p_cmd_queue_elmt->lock);

	if (fw2apccci_ctrl_txd.info_buffer_length > 0) {
		p_cmd_queue_elmt->buffer =
		    kmalloc(fw2apccci_ctrl_txd.info_buffer_length, GFP_ATOMIC);
		if (p_cmd_queue_elmt->buffer == NULL) {
			COEX_LOG_ERROR("\n%s: p_cmd_queue_elmt->buffer is NULL !!!\n", __func__);
			if (p_cmd_queue_elmt)
				kfree(p_cmd_queue_elmt);
			return STATUS_FAILURE;
		}
		/* init buffer */
		memmove(p_cmd_queue_elmt->buffer,
			fw2apccci_ctrl_txd.p_information_buffer,
			fw2apccci_ctrl_txd.info_buffer_length);
		p_cmd_queue_elmt->buffer_length = fw2apccci_ctrl_txd.info_buffer_length;
	}

	/* init reference count*/
	kref_init(&p_cmd_queue_elmt->ref_cnt);

	spin_lock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);
	/* check queue size status */
	if (p_fw2apccci_ctrl->fw2apccci_ctrl_queue.size >= MAX_QUEUE_LEN) {
		COEX_LOG_ERROR("\n%s: fw2apccci_ctrl_queue.size is too big!!!\n", __func__);
		spin_unlock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);
		status = STATUS_FAILURE;
		goto end;
	}

	/*add reference count for cmd send to thread*/
	kref_get(&p_cmd_queue_elmt->ref_cnt);

	/* enqueue to p_cmd_queue */
	p_cmd_queue = &p_fw2apccci_ctrl->fw2apccci_ctrl_queue;

	if (p_cmd_queue->size == 0)
		p_cmd_queue->head = p_cmd_queue_elmt;
	else
		p_cmd_queue->tail->next = p_cmd_queue_elmt;

	p_cmd_queue->tail = p_cmd_queue_elmt;
	p_cmd_queue_elmt->next = NULL;
	p_cmd_queue->size++;

	spin_unlock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);

	/* wake up the cmd thread */
	os_cmd_up(&p_fw2apccci_ctrl->fw2apccci_ctrl_task);

end:
	/* kref_put: decrement refcount for object. */
	kref_put(&p_cmd_queue_elmt->ref_cnt, free_fw2apccci_cmd);
	return status;
}

int fw2apccci_ctrl_basic_enq(struct _wifi_md_coex_adapter *pAd, int len,
			     void *p_buffer)
{
	int ret = STATUS_SUCCESS;
	struct _fw2apccci_ctrl_txd fw2apccci_ctrl_txd;

	memset(&fw2apccci_ctrl_txd, 0, sizeof(struct _fw2apccci_ctrl_txd));
	fw2apccci_ctrl_txd.info_buffer_length = len;
	fw2apccci_ctrl_txd.p_information_buffer = p_buffer;
	ret = fw2apccci_enqueue_cmd(pAd, fw2apccci_ctrl_txd);
	return ret;
}

void fw2apccci_ctrl_deq_cmd(struct _fw2apccci_cmd_queue *p_cmd_queue,
			    struct _fw2apccci_cmd_queue_elmt **pp_cmd_queue_elmt)
{
	*pp_cmd_queue_elmt = p_cmd_queue->head;
	if (*pp_cmd_queue_elmt != NULL) {
		p_cmd_queue->head = p_cmd_queue->head->next;
		p_cmd_queue->size--;

		if (p_cmd_queue->size == 0)
			p_cmd_queue->tail = NULL;
	}
}

void fw2apccci_cmd_handler(struct _wifi_md_coex_adapter *pAd)
{
	struct _fw2apccci_msg *p_fw2apccci_msg = NULL;
	struct _fw2apccci_cmd_queue_elmt *p_cmd_queue_elmt = NULL;
	struct _fw2apccci_ctrl_t *p_fw2apccci_ctrl = &pAd->fw2apccci_ctrl;

	while (pAd && p_fw2apccci_ctrl->fw2apccci_ctrl_queue.size > 0) {
		spin_lock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);
		fw2apccci_ctrl_deq_cmd(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue,
				       &p_cmd_queue_elmt);
		spin_unlock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);

		if (p_cmd_queue_elmt == NULL)
			break;
		if (pAd->apccci_idx <= 0) {
			COEX_LOG_ERROR("%s: open port fail!!!\n idx = %d", __func__,
			       pAd->apccci_idx);
			break;
		}

		p_fw2apccci_msg = (struct _fw2apccci_msg *)p_cmd_queue_elmt->buffer;

		/* send fw2apccci msg to APCCCI*/
		if (mtk_ccci_write_data(pAd->apccci_idx, (char *)p_fw2apccci_msg->data,
					p_fw2apccci_msg->data_len) < 0)
			COEX_LOG_ERROR("%s: write_data fail !\n", __func__);

		kref_put(&p_cmd_queue_elmt->ref_cnt, free_fw2apccci_cmd);
	}
}

int fw2apccci_ctrl_thread(void *data)
{
	struct _wifi_md_coex_adapter *pAd = NULL;
	struct _os_task *p_task = NULL;
	struct _fw2apccci_cmd_queue_elmt *p_cmd_queue_elmt = NULL;
	struct _fw2apccci_ctrl_t *p_fw2apccci_ctrl = NULL;
	bool *was_frozen = FALSE;

	p_task = (struct _os_task *)data;
	pAd = (struct _wifi_md_coex_adapter *)p_task->priv;
	if (pAd == NULL)
		return 0;

	//register the interrupt signal listened by the thread (i.e. be waken up by SIGKILL)
	allow_signal(SIGKILL);

	p_fw2apccci_ctrl = &pAd->fw2apccci_ctrl;
	do {
		if (signal_pending(current)) {
			COEX_LOG_ERROR("%s: catch SIGKILL, thread exit!\n", __func__);
			break;
		}

		/* when unloading wifi_md_coex.ko, SIGKILL needs to be sent to exit the thread */
		/* suspend kthread. wait to wake up */
		wait_event_freezable(p_task->kthread_q,
				     p_task->kthread_running);
		p_task->kthread_running = FALSE;

		/* dequeue cmd handler*/
		fw2apccci_cmd_handler(pAd);
	} while (!kthread_freezable_should_stop(was_frozen));

	/* Clear the p_cmd_queue_elmt. */
	spin_lock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);
	while (p_fw2apccci_ctrl->fw2apccci_ctrl_queue.size) {
		fw2apccci_ctrl_deq_cmd(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue,
				       &p_cmd_queue_elmt);

		if (p_cmd_queue_elmt) {
			kref_put(&p_cmd_queue_elmt->ref_cnt, free_fw2apccci_cmd);
		}
	}
	spin_unlock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);

	return 0;
}

int fw2apccci_ctrl_init(struct _wifi_md_coex_adapter *pAd)
{
	struct _fw2apccci_ctrl_t *p_fw2apccci_ctrl = &pAd->fw2apccci_ctrl;
	struct _fw2apccci_cmd_queue *p_cmd_queue =
	    &p_fw2apccci_ctrl->fw2apccci_ctrl_queue;
	struct _os_task *p_task = &p_fw2apccci_ctrl->fw2apccci_ctrl_task;

	/* init p_cmd_queue */
	spin_lock_init(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);
	spin_lock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);
	p_cmd_queue->head = NULL;
	p_cmd_queue->tail = NULL;
	p_cmd_queue->size = 0;
	spin_unlock_bh(&p_fw2apccci_ctrl->fw2apccci_ctrl_queue_lock);
	p_fw2apccci_ctrl->total_cnt = 0;

	/* init os task */
	memset(p_task, 0, sizeof(struct _os_task));
	p_task->priv = (void *)pAd;
	init_waitqueue_head(&(p_task)->kthread_q);

	/* init kthread */
	p_task->kthread_task = kthread_run(fw2apccci_ctrl_thread,
					   (void *)p_task, "fw2apccci_ctrl_task");
	if (IS_ERR(p_task->kthread_task)) {
		p_task->kthread_task = NULL;
		COEX_LOG_ERROR("%s:kthread create failiure!!!\n", __func__);
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}

int fw2apccci_ctrl_exit(struct _wifi_md_coex_adapter *pAd)
{
	struct _fw2apccci_ctrl_t *p_fw2apccci_ctrl = &pAd->fw2apccci_ctrl;
	struct _os_task *p_task = &p_fw2apccci_ctrl->fw2apccci_ctrl_task;

	/* flush all queued command */
	fw2apccci_cmd_handler(pAd);

	/* stop kthread */
	if (!IS_ERR(p_task->kthread_task)) {
		/* send signal(SIGKILL) to exit the thread*/
		send_sig(SIGKILL, p_task->kthread_task, SEND_SIG_SRC_KERNEL);
		kthread_stop(p_task->kthread_task);
		p_task->kthread_task = NULL;
	}
	return STATUS_SUCCESS;
}
