/*
 * Copyright (c) [2020], MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. and/or its licensors.
 * Except as otherwise provided in the applicable licensing terms with
 * MediaTek Inc. and/or its licensors, any reproduction, modification, use or
 * disclosure of MediaTek Software, and information contained herein, in whole
 * or in part, shall be strictly prohibited.
*/

#ifndef __NODE_H__
#define __NODE_H__
#define NODE_TYPE_NONE			0x000
#define NODE_TYPE_STA			0x001
#define NODE_TYPE_AP			0x002
#define NODE_TYPE_WDS			0x004
#define NODE_TYPE_MONITOR		0x010
#define NODE_TYPE_ATE			0x020
#define NODE_TYPE_MCAST			0x400
#define NODE_TYPE_WDEV			0x800
#define NODE_TYPE_MASK			0xfff

#define NODE_AP				NODE_TYPE_AP
#define NODE_GO				(NODE_TYPE_AP | 0x01000)
#define NODE_TYPE_INFRA		NODE_TYPE_STA
#define NODE_TYPE_GC		(NODE_TYPE_STA | 0x01000)
#define NODE_TYPE_ADHOC		(NODE_TYPE_STA | 0x02000)
#define NODE_TYPE_APCLI		(NODE_TYPE_STA | 0x04000)
#define NODE_TYPE_DLS		(NODE_TYPE_STA | 0x08000)
#define NODE_TYPE_CLIENT	(NODE_TYPE_STA | 0x10000)
#define NODE_TYPE_REPEATER	(NODE_TYPE_STA | 0x20000)

enum vap_opmode {
	VAP_M_STA         = 1,              /* infrastructure station */
	VAP_M_IBSS        = 0,              /* IBSS (adhoc) station */
	VAP_M_AHDEMO      = 3,              /* Old lucent compatible adhoc demo */
	VAP_M_HOSTAP      = 6,              /* Software Access Point */
	VAP_M_MONITOR     = 8,              /* Monitor mode */
	VAP_M_WDS         = 2,              /* WDS link */
	VAP_M_BTAMP       = 9,              /* VAP for BT AMP */
	VAP_M_P2P_GO      = 33,             /* P2P GO */
	VAP_M_P2P_CLIENT  = 34,             /* P2P Client */
	VAP_M_P2P_DEVICE  = 35,             /* P2P Device */
	VAP_OPMODE_MAX    = VAP_M_BTAMP,    /* Highest numbered opmode in the list */
	VAP_M_ANY         = 0xFF            /* Any of the above; used by NDIS 6.x */
};

enum _ENUM_NODE_CONNECTED_STATE {
	NODE_NOT_AUTH,
	NODE_AUTH,
	NODE_ASSOC
};

struct vap_entry {
	struct net_device *dev;
	unsigned char  bssid[6]; /* vap address */
	unsigned short caps;
	unsigned int   dev_flags;
	unsigned int   opmode;
	unsigned int   vap_mlo_enable;
};

struct node_entry {
	struct vap_entry *vap;
	unsigned char  addr[6]; /* Station address */
	unsigned short aid;
	unsigned int   state; /* NODE_NOT_AUTH, NODE_AUTH, NODE_ASSOC */
	unsigned int   flags;
	unsigned int   node_type;
	unsigned int   node_mlo_enable;
};

#endif /* __NODE_H__ */
