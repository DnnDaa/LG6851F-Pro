#ifndef __WF_TXS_MPDU_REGS_H__
#define __WF_TXS_MPDU_REGS_H__
/*
* Copyright (c) 2021, MediaTek Inc. All rights reserved.
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

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define WF_TXS_MPDU_RATE_TX_RATE_DW                                    0
#define WF_TXS_MPDU_RATE_TX_RATE_ADDR                                  0
#define WF_TXS_MPDU_RATE_TX_RATE_MASK \
	0x0000003f // 5- 0
#define WF_TXS_MPDU_RATE_TX_RATE_SHIFT                                 0
#define WF_TXS_MPDU_RATE_TX_MODE_DW                                    0
#define WF_TXS_MPDU_RATE_TX_MODE_ADDR                                  0
#define WF_TXS_MPDU_RATE_TX_MODE_MASK \
	0x000003c0 // 9- 6
#define WF_TXS_MPDU_RATE_TX_MODE_SHIFT                                 6
#define WF_TXS_MPDU_RATE_NSTS_DW                                       0
#define WF_TXS_MPDU_RATE_NSTS_ADDR                                     0
#define WF_TXS_MPDU_RATE_NSTS_MASK \
	0x00003c00 // 13-10
#define WF_TXS_MPDU_RATE_NSTS_SHIFT                                    10
#define WF_TXS_MPDU_TXS2M_DW                                           0
#define WF_TXS_MPDU_TXS2M_ADDR                                         0
#define WF_TXS_MPDU_TXS2M_MASK \
	0x00004000 // 14-14
#define WF_TXS_MPDU_TXS2M_SHIFT                                        14
#define WF_TXS_MPDU_TXS2H_DW                                           0
#define WF_TXS_MPDU_TXS2H_ADDR                                         0
#define WF_TXS_MPDU_TXS2H_MASK \
	0x00008000 // 15-15
#define WF_TXS_MPDU_TXS2H_SHIFT                                        15
#define WF_TXS_MPDU_ME_DW                                              0
#define WF_TXS_MPDU_ME_ADDR                                            0
#define WF_TXS_MPDU_ME_MASK \
	0x00010000 // 16-16
#define WF_TXS_MPDU_ME_SHIFT                                           16
#define WF_TXS_MPDU_RE_DW                                              0
#define WF_TXS_MPDU_RE_ADDR                                            0
#define WF_TXS_MPDU_RE_MASK \
	0x00020000 // 17-17
#define WF_TXS_MPDU_RE_SHIFT                                           17
#define WF_TXS_MPDU_LE_DW                                              0
#define WF_TXS_MPDU_LE_ADDR                                            0
#define WF_TXS_MPDU_LE_MASK \
	0x00040000 // 18-18
#define WF_TXS_MPDU_LE_SHIFT                                           18
#define WF_TXS_MPDU_BF_BFE_DW                                          0
#define WF_TXS_MPDU_BF_BFE_ADDR                                        0
#define WF_TXS_MPDU_BF_BFE_MASK \
	0x00080000 // 19-19
#define WF_TXS_MPDU_BF_BFE_SHIFT                                       19
#define WF_TXS_MPDU_TXOP_DW                                            0
#define WF_TXS_MPDU_TXOP_ADDR                                          0
#define WF_TXS_MPDU_TXOP_MASK \
	0x00100000 // 20-20
#define WF_TXS_MPDU_TXOP_SHIFT                                         20
#define WF_TXS_MPDU_PS_DW                                              0
#define WF_TXS_MPDU_PS_ADDR                                            0
#define WF_TXS_MPDU_PS_MASK \
	0x00200000 // 21-21
#define WF_TXS_MPDU_PS_SHIFT                                           21
#define WF_TXS_MPDU_BAF_DW                                             0
#define WF_TXS_MPDU_BAF_ADDR                                           0
#define WF_TXS_MPDU_BAF_MASK \
	0x00400000 // 22-22
#define WF_TXS_MPDU_BAF_SHIFT                                          22
#define WF_TXS_MPDU_TXSFM_DW                                           0
#define WF_TXS_MPDU_TXSFM_ADDR                                         0
#define WF_TXS_MPDU_TXSFM_MASK \
	0x01800000 // 24-23
#define WF_TXS_MPDU_TXSFM_SHIFT                                        23
#define WF_TXS_MPDU_AM_DW                                              0
#define WF_TXS_MPDU_AM_ADDR                                            0
#define WF_TXS_MPDU_AM_MASK \
	0x02000000 // 25-25
#define WF_TXS_MPDU_AM_SHIFT                                           25
#define WF_TXS_MPDU_TID_DW                                             0
#define WF_TXS_MPDU_TID_ADDR                                           0
#define WF_TXS_MPDU_TID_MASK \
	0x1c000000 // 28-26
#define WF_TXS_MPDU_TID_SHIFT                                          26
#define WF_TXS_MPDU_TBW_DW                                             0
#define WF_TXS_MPDU_TBW_ADDR                                           0
#define WF_TXS_MPDU_TBW_MASK \
	0xe0000000 // 31-29
#define WF_TXS_MPDU_TBW_SHIFT                                          29
#define WF_TXS_MPDU_TX_POWER_DW                                        1
#define WF_TXS_MPDU_TX_POWER_ADDR                                      4
#define WF_TXS_MPDU_TX_POWER_MASK \
	0x000000ff // 7- 0
#define WF_TXS_MPDU_TX_POWER_SHIFT                                     0
#define WF_TXS_MPDU_RXV_SN_DW                                          1
#define WF_TXS_MPDU_RXV_SN_ADDR                                        4
#define WF_TXS_MPDU_RXV_SN_MASK \
	0x0000ff00 // 15- 8
#define WF_TXS_MPDU_RXV_SN_SHIFT                                       8
#define WF_TXS_MPDU_RESP_LEGACY_RATE_DW                                1
#define WF_TXS_MPDU_RESP_LEGACY_RATE_ADDR                              4
#define WF_TXS_MPDU_RESP_LEGACY_RATE_MASK \
	0x000f0000 // 19-16
#define WF_TXS_MPDU_RESP_LEGACY_RATE_SHIFT                             16
#define WF_TXS_MPDU_SN_DW                                              1
#define WF_TXS_MPDU_SN_ADDR                                            4
#define WF_TXS_MPDU_SN_MASK \
	0xfff00000 // 31-20
#define WF_TXS_MPDU_SN_SHIFT                                           20
#define WF_TXS_MPDU_TRANSMISSION_DELAY_DW                              2
#define WF_TXS_MPDU_TRANSMISSION_DELAY_ADDR                            8
#define WF_TXS_MPDU_TRANSMISSION_DELAY_MASK \
	0x0000ffff // 15- 0
#define WF_TXS_MPDU_TRANSMISSION_DELAY_SHIFT                           0
#define WF_TXS_MPDU_WLAN_IDX_DW                                        2
#define WF_TXS_MPDU_WLAN_IDX_ADDR                                      8
#define WF_TXS_MPDU_WLAN_IDX_MASK \
	0x0fff0000 // 27-16
#define WF_TXS_MPDU_WLAN_IDX_SHIFT                                     16
#define WF_TXS_MPDU_FR_DW                                              2
#define WF_TXS_MPDU_FR_ADDR                                            8
#define WF_TXS_MPDU_FR_MASK \
	0x10000000 // 28-28
#define WF_TXS_MPDU_FR_SHIFT                                           28
#define WF_TXS_MPDU_RATE_STBC_DW                                       2
#define WF_TXS_MPDU_RATE_STBC_ADDR                                     8
#define WF_TXS_MPDU_RATE_STBC_MASK \
	0x20000000 // 29-29
#define WF_TXS_MPDU_RATE_STBC_SHIFT                                    29
#define WF_TXS_MPDU_BF_STS_DW                                          2
#define WF_TXS_MPDU_BF_STS_ADDR                                        8
#define WF_TXS_MPDU_BF_STS_MASK \
	0xc0000000 // 31-30
#define WF_TXS_MPDU_BF_STS_SHIFT                                       30
#define WF_TXS_MPDU_LAST_TX_RATE_IDX_DW                                3
#define WF_TXS_MPDU_LAST_TX_RATE_IDX_ADDR                              12
#define WF_TXS_MPDU_LAST_TX_RATE_IDX_MASK \
	0x00000007 // 2- 0
#define WF_TXS_MPDU_LAST_TX_RATE_IDX_SHIFT                             0
#define WF_TXS_MPDU_CO_ANT_DW                                          3
#define WF_TXS_MPDU_CO_ANT_ADDR                                        12
#define WF_TXS_MPDU_CO_ANT_MASK \
	0x00000008 // 3- 3
#define WF_TXS_MPDU_CO_ANT_SHIFT                                       3
#define WF_TXS_MPDU_SRC_DW                                             3
#define WF_TXS_MPDU_SRC_ADDR                                           12
#define WF_TXS_MPDU_SRC_MASK \
	0x00000030 // 5- 4
#define WF_TXS_MPDU_SRC_SHIFT                                          4
#define WF_TXS_MPDU_PID_DW                                             3
#define WF_TXS_MPDU_PID_ADDR                                           12
#define WF_TXS_MPDU_PID_MASK \
	0xff000000 // 31-24
#define WF_TXS_MPDU_PID_SHIFT                                          24
#define WF_TXS_MPDU_TIMESTAMP_DW                                       4
#define WF_TXS_MPDU_TIMESTAMP_ADDR                                     16
#define WF_TXS_MPDU_TIMESTAMP_MASK \
	0xffffffff // 31- 0
#define WF_TXS_MPDU_TIMESTAMP_SHIFT                                    0
#define WF_TXS_MPDU_FRONTTIME_DW                                       5
#define WF_TXS_MPDU_FRONTTIME_ADDR                                     20
#define WF_TXS_MPDU_FRONTTIME_MASK \
	0x01ffffff // 24- 0
#define WF_TXS_MPDU_FRONTTIME_SHIFT                                    0
#define WF_TXS_MPDU_MPDU_TX_CNT_DW                                     5
#define WF_TXS_MPDU_MPDU_TX_CNT_ADDR                                   20
#define WF_TXS_MPDU_MPDU_TX_CNT_MASK \
	0x3e000000 // 29-25
#define WF_TXS_MPDU_MPDU_TX_CNT_SHIFT                                  25
#define WF_TXS_MPDU_QOS_DW                                             5
#define WF_TXS_MPDU_QOS_ADDR                                           20
#define WF_TXS_MPDU_QOS_MASK \
	0x40000000 // 30-30
#define WF_TXS_MPDU_QOS_SHIFT                                          30
#define WF_TXS_MPDU_FM_DW                                              5
#define WF_TXS_MPDU_FM_ADDR                                            20
#define WF_TXS_MPDU_FM_MASK \
	0x80000000 // 31-31
#define WF_TXS_MPDU_FM_SHIFT                                           31
#define WF_TXS_MPDU_NOISE0_DW                                          6
#define WF_TXS_MPDU_NOISE0_ADDR                                        24
#define WF_TXS_MPDU_NOISE0_MASK \
	0x000000ff // 7- 0
#define WF_TXS_MPDU_NOISE0_SHIFT                                       0
#define WF_TXS_MPDU_NOISE1_DW                                          6
#define WF_TXS_MPDU_NOISE1_ADDR                                        24
#define WF_TXS_MPDU_NOISE1_MASK \
	0x0000ff00 // 15- 8
#define WF_TXS_MPDU_NOISE1_SHIFT                                       8
#define WF_TXS_MPDU_NOISE2_DW                                          6
#define WF_TXS_MPDU_NOISE2_ADDR                                        24
#define WF_TXS_MPDU_NOISE2_MASK \
	0x00ff0000 // 23-16
#define WF_TXS_MPDU_NOISE2_SHIFT                                       16
#define WF_TXS_MPDU_NOISE3_DW                                          6
#define WF_TXS_MPDU_NOISE3_ADDR                                        24
#define WF_TXS_MPDU_NOISE3_MASK \
	0xff000000 // 31-24
#define WF_TXS_MPDU_NOISE3_SHIFT                                       24
#define WF_TXS_MPDU_RCPI0_DW                                           7
#define WF_TXS_MPDU_RCPI0_ADDR                                         28
#define WF_TXS_MPDU_RCPI0_MASK \
	0x000000ff // 7- 0
#define WF_TXS_MPDU_RCPI0_SHIFT                                        0
#define WF_TXS_MPDU_RCPI1_DW                                           7
#define WF_TXS_MPDU_RCPI1_ADDR                                         28
#define WF_TXS_MPDU_RCPI1_MASK \
	0x0000ff00 // 15- 8
#define WF_TXS_MPDU_RCPI1_SHIFT                                        8
#define WF_TXS_MPDU_RCPI2_DW                                           7
#define WF_TXS_MPDU_RCPI2_ADDR                                         28
#define WF_TXS_MPDU_RCPI2_MASK \
	0x00ff0000 // 23-16
#define WF_TXS_MPDU_RCPI2_SHIFT                                        16
#define WF_TXS_MPDU_RCPI3_DW                                           7
#define WF_TXS_MPDU_RCPI3_ADDR                                         28
#define WF_TXS_MPDU_RCPI3_MASK \
	0xff000000 // 31-24
#define WF_TXS_MPDU_RCPI3_SHIFT                                        24
#define WF_TXS_MPDU_ANT_ID0_DW                                         8
#define WF_TXS_MPDU_ANT_ID0_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID0_MASK \
	0x00000007 // 2- 0
#define WF_TXS_MPDU_ANT_ID0_SHIFT                                      0
#define WF_TXS_MPDU_ANT_ID1_DW                                         8
#define WF_TXS_MPDU_ANT_ID1_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID1_MASK \
	0x00000038 // 5- 3
#define WF_TXS_MPDU_ANT_ID1_SHIFT                                      3
#define WF_TXS_MPDU_ANT_ID2_DW                                         8
#define WF_TXS_MPDU_ANT_ID2_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID2_MASK \
	0x000001c0 // 8- 6
#define WF_TXS_MPDU_ANT_ID2_SHIFT                                      6
#define WF_TXS_MPDU_ANT_ID3_DW                                         8
#define WF_TXS_MPDU_ANT_ID3_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID3_MASK \
	0x00000e00 // 11- 9
#define WF_TXS_MPDU_ANT_ID3_SHIFT                                      9
#define WF_TXS_MPDU_ANT_ID4_DW                                         8
#define WF_TXS_MPDU_ANT_ID4_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID4_MASK \
	0x00007000 // 14-12
#define WF_TXS_MPDU_ANT_ID4_SHIFT                                      12
#define WF_TXS_MPDU_ANT_ID5_DW                                         8
#define WF_TXS_MPDU_ANT_ID5_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID5_MASK \
	0x00038000 // 17-15
#define WF_TXS_MPDU_ANT_ID5_SHIFT                                      15
#define WF_TXS_MPDU_ANT_ID6_DW                                         8
#define WF_TXS_MPDU_ANT_ID6_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID6_MASK \
	0x001c0000 // 20-18
#define WF_TXS_MPDU_ANT_ID6_SHIFT                                      18
#define WF_TXS_MPDU_ANT_ID7_DW                                         8
#define WF_TXS_MPDU_ANT_ID7_ADDR                                       32
#define WF_TXS_MPDU_ANT_ID7_MASK \
	0x00e00000 // 23-21
#define WF_TXS_MPDU_ANT_ID7_SHIFT                                      21

#define WF_TXS_MPDU_GET_RATE_TX_RATE(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RATE_TX_RATE)
#define WF_TXS_MPDU_GET_RATE_TX_MODE(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RATE_TX_MODE)
#define WF_TXS_MPDU_GET_RATE_NSTS(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RATE_NSTS)
#define WF_TXS_MPDU_GET_TXS2M(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TXS2M)
#define WF_TXS_MPDU_GET_TXS2H(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TXS2H)
#define WF_TXS_MPDU_GET_ME(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ME)
#define WF_TXS_MPDU_GET_RE(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RE)
#define WF_TXS_MPDU_GET_LE(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_LE)
#define WF_TXS_MPDU_GET_BF_BFE(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_BF_BFE)
#define WF_TXS_MPDU_GET_TXOP(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TXOP)
#define WF_TXS_MPDU_GET_PS(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_PS)
#define WF_TXS_MPDU_GET_BAF(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_BAF)
#define WF_TXS_MPDU_GET_TXSFM(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TXSFM)
#define WF_TXS_MPDU_GET_AM(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_AM)
#define WF_TXS_MPDU_GET_TID(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TID)
#define WF_TXS_MPDU_GET_TBW(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TBW)
#define WF_TXS_MPDU_GET_TX_POWER(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TX_POWER)
#define WF_TXS_MPDU_GET_RXV_SN(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RXV_SN)
#define WF_TXS_MPDU_GET_RESP_LEGACY_RATE(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RESP_LEGACY_RATE)
#define WF_TXS_MPDU_GET_SN(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_SN)
#define WF_TXS_MPDU_GET_TRANSMISSION_DELAY(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TRANSMISSION_DELAY)
#define WF_TXS_MPDU_GET_WLAN_IDX(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_WLAN_IDX)
#define WF_TXS_MPDU_GET_FR(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_FR)
#define WF_TXS_MPDU_GET_RATE_STBC(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RATE_STBC)
#define WF_TXS_MPDU_GET_BF_STS(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_BF_STS)
#define WF_TXS_MPDU_GET_LAST_TX_RATE_IDX(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_LAST_TX_RATE_IDX)
#define WF_TXS_MPDU_GET_CO_ANT(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_CO_ANT)
#define WF_TXS_MPDU_GET_SRC(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_SRC)
#define WF_TXS_MPDU_GET_PID(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_PID)
#define WF_TXS_MPDU_GET_TIMESTAMP(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_TIMESTAMP)
#define WF_TXS_MPDU_GET_FRONTTIME(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_FRONTTIME)
#define WF_TXS_MPDU_GET_MPDU_TX_CNT(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_MPDU_TX_CNT)
#define WF_TXS_MPDU_GET_QOS(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_QOS)
#define WF_TXS_MPDU_GET_FM(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_FM)
#define WF_TXS_MPDU_GET_NOISE0(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_NOISE0)
#define WF_TXS_MPDU_GET_NOISE1(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_NOISE1)
#define WF_TXS_MPDU_GET_NOISE2(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_NOISE2)
#define WF_TXS_MPDU_GET_NOISE3(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_NOISE3)
#define WF_TXS_MPDU_GET_RCPI0(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RCPI0)
#define WF_TXS_MPDU_GET_RCPI1(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RCPI1)
#define WF_TXS_MPDU_GET_RCPI2(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RCPI2)
#define WF_TXS_MPDU_GET_RCPI3(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_RCPI3)
#define WF_TXS_MPDU_GET_ANT_ID0(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID0)
#define WF_TXS_MPDU_GET_ANT_ID1(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID1)
#define WF_TXS_MPDU_GET_ANT_ID2(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID2)
#define WF_TXS_MPDU_GET_ANT_ID3(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID3)
#define WF_TXS_MPDU_GET_ANT_ID4(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID4)
#define WF_TXS_MPDU_GET_ANT_ID5(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID5)
#define WF_TXS_MPDU_GET_ANT_ID6(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID6)
#define WF_TXS_MPDU_GET_ANT_ID7(reg32) \
	READ_FIELD((reg32), WF_TXS_MPDU_ANT_ID7)

#define WF_TXS_MPDU_SET_RATE_TX_RATE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RATE_TX_RATE, val32)
#define WF_TXS_MPDU_SET_RATE_TX_MODE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RATE_TX_MODE, val32)
#define WF_TXS_MPDU_SET_RATE_NSTS(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RATE_NSTS, val32)
#define WF_TXS_MPDU_SET_TXS2M(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TXS2M, val32)
#define WF_TXS_MPDU_SET_TXS2H(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TXS2H, val32)
#define WF_TXS_MPDU_SET_ME(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ME, val32)
#define WF_TXS_MPDU_SET_RE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RE, val32)
#define WF_TXS_MPDU_SET_LE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_LE, val32)
#define WF_TXS_MPDU_SET_BF_BFE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_BF_BFE, val32)
#define WF_TXS_MPDU_SET_TXOP(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TXOP, val32)
#define WF_TXS_MPDU_SET_PS(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_PS, val32)
#define WF_TXS_MPDU_SET_BAF(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_BAF, val32)
#define WF_TXS_MPDU_SET_TXSFM(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TXSFM, val32)
#define WF_TXS_MPDU_SET_AM(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_AM, val32)
#define WF_TXS_MPDU_SET_TID(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TID, val32)
#define WF_TXS_MPDU_SET_TBW(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TBW, val32)
#define WF_TXS_MPDU_SET_TX_POWER(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TX_POWER, val32)
#define WF_TXS_MPDU_SET_RXV_SN(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RXV_SN, val32)
#define WF_TXS_MPDU_SET_RESP_LEGACY_RATE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RESP_LEGACY_RATE, val32)
#define WF_TXS_MPDU_SET_SN(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_SN, val32)
#define WF_TXS_MPDU_SET_TRANSMISSION_DELAY(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TRANSMISSION_DELAY, val32)
#define WF_TXS_MPDU_SET_WLAN_IDX(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_WLAN_IDX, val32)
#define WF_TXS_MPDU_SET_FR(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_FR, val32)
#define WF_TXS_MPDU_SET_RATE_STBC(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RATE_STBC, val32)
#define WF_TXS_MPDU_SET_BF_STS(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_BF_STS, val32)
#define WF_TXS_MPDU_SET_LAST_TX_RATE_IDX(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_LAST_TX_RATE_IDX, val32)
#define WF_TXS_MPDU_SET_CO_ANT(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_CO_ANT, val32)
#define WF_TXS_MPDU_SET_SRC(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_SRC, val32)
#define WF_TXS_MPDU_SET_PID(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_PID, val32)
#define WF_TXS_MPDU_SET_TIMESTAMP(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_TIMESTAMP, val32)
#define WF_TXS_MPDU_SET_FRONTTIME(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_FRONTTIME, val32)
#define WF_TXS_MPDU_SET_MPDU_TX_CNT(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_MPDU_TX_CNT, val32)
#define WF_TXS_MPDU_SET_QOS(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_QOS, val32)
#define WF_TXS_MPDU_SET_FM(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_FM, val32)
#define WF_TXS_MPDU_SET_NOISE0(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_NOISE0, val32)
#define WF_TXS_MPDU_SET_NOISE1(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_NOISE1, val32)
#define WF_TXS_MPDU_SET_NOISE2(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_NOISE2, val32)
#define WF_TXS_MPDU_SET_NOISE3(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_NOISE3, val32)
#define WF_TXS_MPDU_SET_RCPI0(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RCPI0, val32)
#define WF_TXS_MPDU_SET_RCPI1(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RCPI1, val32)
#define WF_TXS_MPDU_SET_RCPI2(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RCPI2, val32)
#define WF_TXS_MPDU_SET_RCPI3(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_RCPI3, val32)
#define WF_TXS_MPDU_SET_ANT_ID0(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID0, val32)
#define WF_TXS_MPDU_SET_ANT_ID1(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID1, val32)
#define WF_TXS_MPDU_SET_ANT_ID2(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID2, val32)
#define WF_TXS_MPDU_SET_ANT_ID3(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID3, val32)
#define WF_TXS_MPDU_SET_ANT_ID4(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID4, val32)
#define WF_TXS_MPDU_SET_ANT_ID5(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID5, val32)
#define WF_TXS_MPDU_SET_ANT_ID6(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID6, val32)
#define WF_TXS_MPDU_SET_ANT_ID7(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_MPDU_ANT_ID7, val32)

#define WF_TXS_MPDU_CLR_RATE_TX_RATE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RATE_TX_RATE)
#define WF_TXS_MPDU_CLR_RATE_TX_MODE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RATE_TX_MODE)
#define WF_TXS_MPDU_CLR_RATE_NSTS(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RATE_NSTS)
#define WF_TXS_MPDU_CLR_TXS2M(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TXS2M)
#define WF_TXS_MPDU_CLR_TXS2H(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TXS2H)
#define WF_TXS_MPDU_CLR_ME(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ME)
#define WF_TXS_MPDU_CLR_RE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RE)
#define WF_TXS_MPDU_CLR_LE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_LE)
#define WF_TXS_MPDU_CLR_BF_BFE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_BF_BFE)
#define WF_TXS_MPDU_CLR_TXOP(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TXOP)
#define WF_TXS_MPDU_CLR_PS(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_PS)
#define WF_TXS_MPDU_CLR_BAF(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_BAF)
#define WF_TXS_MPDU_CLR_TXSFM(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TXSFM)
#define WF_TXS_MPDU_CLR_AM(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_AM)
#define WF_TXS_MPDU_CLR_TID(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TID)
#define WF_TXS_MPDU_CLR_TBW(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TBW)
#define WF_TXS_MPDU_CLR_TX_POWER(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TX_POWER)
#define WF_TXS_MPDU_CLR_RXV_SN(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RXV_SN)
#define WF_TXS_MPDU_CLR_RESP_LEGACY_RATE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RESP_LEGACY_RATE)
#define WF_TXS_MPDU_CLR_SN(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_SN)
#define WF_TXS_MPDU_CLR_TRANSMISSION_DELAY(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TRANSMISSION_DELAY)
#define WF_TXS_MPDU_CLR_WLAN_IDX(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_WLAN_IDX)
#define WF_TXS_MPDU_CLR_FR(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_FR)
#define WF_TXS_MPDU_CLR_RATE_STBC(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RATE_STBC)
#define WF_TXS_MPDU_CLR_BF_STS(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_BF_STS)
#define WF_TXS_MPDU_CLR_LAST_TX_RATE_IDX(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_LAST_TX_RATE_IDX)
#define WF_TXS_MPDU_CLR_CO_ANT(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_CO_ANT)
#define WF_TXS_MPDU_CLR_SRC(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_SRC)
#define WF_TXS_MPDU_CLR_PID(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_PID)
#define WF_TXS_MPDU_CLR_TIMESTAMP(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_TIMESTAMP)
#define WF_TXS_MPDU_CLR_FRONTTIME(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_FRONTTIME)
#define WF_TXS_MPDU_CLR_MPDU_TX_CNT(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_MPDU_TX_CNT)
#define WF_TXS_MPDU_CLR_QOS(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_QOS)
#define WF_TXS_MPDU_CLR_FM(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_FM)
#define WF_TXS_MPDU_CLR_NOISE0(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_NOISE0)
#define WF_TXS_MPDU_CLR_NOISE1(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_NOISE1)
#define WF_TXS_MPDU_CLR_NOISE2(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_NOISE2)
#define WF_TXS_MPDU_CLR_NOISE3(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_NOISE3)
#define WF_TXS_MPDU_CLR_RCPI0(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RCPI0)
#define WF_TXS_MPDU_CLR_RCPI1(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RCPI1)
#define WF_TXS_MPDU_CLR_RCPI2(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RCPI2)
#define WF_TXS_MPDU_CLR_RCPI3(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_RCPI3)
#define WF_TXS_MPDU_CLR_ANT_ID0(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID0)
#define WF_TXS_MPDU_CLR_ANT_ID1(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID1)
#define WF_TXS_MPDU_CLR_ANT_ID2(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID2)
#define WF_TXS_MPDU_CLR_ANT_ID3(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID3)
#define WF_TXS_MPDU_CLR_ANT_ID4(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID4)
#define WF_TXS_MPDU_CLR_ANT_ID5(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID5)
#define WF_TXS_MPDU_CLR_ANT_ID6(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID6)
#define WF_TXS_MPDU_CLR_ANT_ID7(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_MPDU_ANT_ID7)

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __WF_TXS_MPDU_REGS_H__
