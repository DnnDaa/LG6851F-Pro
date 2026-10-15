#ifndef __WF_TXS_PPDU_REGS_H__
#define __WF_TXS_PPDU_REGS_H__
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

#define WF_TXS_PPDU_RATE_TX_RATE_DW                                    0
#define WF_TXS_PPDU_RATE_TX_RATE_ADDR                                  0
#define WF_TXS_PPDU_RATE_TX_RATE_MASK \
	0x0000003f // 5- 0
#define WF_TXS_PPDU_RATE_TX_RATE_SHIFT                                 0
#define WF_TXS_PPDU_RATE_TX_MODE_DW                                    0
#define WF_TXS_PPDU_RATE_TX_MODE_ADDR                                  0
#define WF_TXS_PPDU_RATE_TX_MODE_MASK \
	0x000003c0 // 9- 6
#define WF_TXS_PPDU_RATE_TX_MODE_SHIFT                                 6
#define WF_TXS_PPDU_RATE_NSTS_DW                                       0
#define WF_TXS_PPDU_RATE_NSTS_ADDR                                     0
#define WF_TXS_PPDU_RATE_NSTS_MASK \
	0x00003c00 // 13-10
#define WF_TXS_PPDU_RATE_NSTS_SHIFT                                    10
#define WF_TXS_PPDU_TXS2M_DW                                           0
#define WF_TXS_PPDU_TXS2M_ADDR                                         0
#define WF_TXS_PPDU_TXS2M_MASK \
	0x00004000 // 14-14
#define WF_TXS_PPDU_TXS2M_SHIFT                                        14
#define WF_TXS_PPDU_TXS2H_DW                                           0
#define WF_TXS_PPDU_TXS2H_ADDR                                         0
#define WF_TXS_PPDU_TXS2H_MASK \
	0x00008000 // 15-15
#define WF_TXS_PPDU_TXS2H_SHIFT                                        15
#define WF_TXS_PPDU_TXSFM_DW                                           0
#define WF_TXS_PPDU_TXSFM_ADDR                                         0
#define WF_TXS_PPDU_TXSFM_MASK \
	0x01800000 // 24-23
#define WF_TXS_PPDU_TXSFM_SHIFT                                        23
#define WF_TXS_PPDU_AM_DW                                              0
#define WF_TXS_PPDU_AM_ADDR                                            0
#define WF_TXS_PPDU_AM_MASK \
	0x02000000 // 25-25
#define WF_TXS_PPDU_AM_SHIFT                                           25
#define WF_TXS_PPDU_TID_DW                                             0
#define WF_TXS_PPDU_TID_ADDR                                           0
#define WF_TXS_PPDU_TID_MASK \
	0x1c000000 // 28-26
#define WF_TXS_PPDU_TID_SHIFT                                          26
#define WF_TXS_PPDU_TBW_DW                                             0
#define WF_TXS_PPDU_TBW_ADDR                                           0
#define WF_TXS_PPDU_TBW_MASK \
	0xe0000000 // 31-29
#define WF_TXS_PPDU_TBW_SHIFT                                          29
#define WF_TXS_PPDU_TX_POWER_DW                                        1
#define WF_TXS_PPDU_TX_POWER_ADDR                                      4
#define WF_TXS_PPDU_TX_POWER_MASK \
	0x000000ff // 7- 0
#define WF_TXS_PPDU_TX_POWER_SHIFT                                     0
#define WF_TXS_PPDU_RXV_SN_DW                                          1
#define WF_TXS_PPDU_RXV_SN_ADDR                                        4
#define WF_TXS_PPDU_RXV_SN_MASK \
	0x0000ff00 // 15- 8
#define WF_TXS_PPDU_RXV_SN_SHIFT                                       8
#define WF_TXS_PPDU_RESP_LEGACY_RATE_DW                                1
#define WF_TXS_PPDU_RESP_LEGACY_RATE_ADDR                              4
#define WF_TXS_PPDU_RESP_LEGACY_RATE_MASK \
	0x000f0000 // 19-16
#define WF_TXS_PPDU_RESP_LEGACY_RATE_SHIFT                             16
#define WF_TXS_PPDU_SN_DW                                              1
#define WF_TXS_PPDU_SN_ADDR                                            4
#define WF_TXS_PPDU_SN_MASK \
	0xfff00000 // 31-20
#define WF_TXS_PPDU_SN_SHIFT                                           20
#define WF_TXS_PPDU_TRANSMISSION_DELAY_DW                              2
#define WF_TXS_PPDU_TRANSMISSION_DELAY_ADDR                            8
#define WF_TXS_PPDU_TRANSMISSION_DELAY_MASK \
	0x0000ffff // 15- 0
#define WF_TXS_PPDU_TRANSMISSION_DELAY_SHIFT                           0
#define WF_TXS_PPDU_WLAN_IDX_DW                                        2
#define WF_TXS_PPDU_WLAN_IDX_ADDR                                      8
#define WF_TXS_PPDU_WLAN_IDX_MASK \
	0x0fff0000 // 27-16
#define WF_TXS_PPDU_WLAN_IDX_SHIFT                                     16
#define WF_TXS_PPDU_FR_DW                                              2
#define WF_TXS_PPDU_FR_ADDR                                            8
#define WF_TXS_PPDU_FR_MASK \
	0x10000000 // 28-28
#define WF_TXS_PPDU_FR_SHIFT                                           28
#define WF_TXS_PPDU_RATE_STBC_DW                                       2
#define WF_TXS_PPDU_RATE_STBC_ADDR                                     8
#define WF_TXS_PPDU_RATE_STBC_MASK \
	0x20000000 // 29-29
#define WF_TXS_PPDU_RATE_STBC_SHIFT                                    29
#define WF_TXS_PPDU_BF_STS_DW                                          2
#define WF_TXS_PPDU_BF_STS_ADDR                                        8
#define WF_TXS_PPDU_BF_STS_MASK \
	0xc0000000 // 31-30
#define WF_TXS_PPDU_BF_STS_SHIFT                                       30
#define WF_TXS_PPDU_LAST_TX_RATE_IDX_DW                                3
#define WF_TXS_PPDU_LAST_TX_RATE_IDX_ADDR                              12
#define WF_TXS_PPDU_LAST_TX_RATE_IDX_MASK \
	0x00000007 // 2- 0
#define WF_TXS_PPDU_LAST_TX_RATE_IDX_SHIFT                             0
#define WF_TXS_PPDU_CO_ANT_DW                                          3
#define WF_TXS_PPDU_CO_ANT_ADDR                                        12
#define WF_TXS_PPDU_CO_ANT_MASK \
	0x00000008 // 3- 3
#define WF_TXS_PPDU_CO_ANT_SHIFT                                       3
#define WF_TXS_PPDU_SRC_DW                                             3
#define WF_TXS_PPDU_SRC_ADDR                                           12
#define WF_TXS_PPDU_SRC_MASK \
	0x00000030 // 5- 4
#define WF_TXS_PPDU_SRC_SHIFT                                          4
#define WF_TXS_PPDU_PID_DW                                             3
#define WF_TXS_PPDU_PID_ADDR                                           12
#define WF_TXS_PPDU_PID_MASK \
	0xff000000 // 31-24
#define WF_TXS_PPDU_PID_SHIFT                                          24
#define WF_TXS_PPDU_TIMESTAMP_DW                                       4
#define WF_TXS_PPDU_TIMESTAMP_ADDR                                     16
#define WF_TXS_PPDU_TIMESTAMP_MASK \
	0xffffffff // 31- 0
#define WF_TXS_PPDU_TIMESTAMP_SHIFT                                    0
#define WF_TXS_PPDU_MPDU_TX_BYTE_DW                                    5
#define WF_TXS_PPDU_MPDU_TX_BYTE_ADDR                                  20
#define WF_TXS_PPDU_MPDU_TX_BYTE_MASK \
	0x0000ffff // 15- 0
#define WF_TXS_PPDU_MPDU_TX_BYTE_SHIFT                                 0
#define WF_TXS_PPDU_MPDU_TX_CNT_DW                                     5
#define WF_TXS_PPDU_MPDU_TX_CNT_ADDR                                   20
#define WF_TXS_PPDU_MPDU_TX_CNT_MASK \
	0x7ff00000 // 30-20
#define WF_TXS_PPDU_MPDU_TX_CNT_SHIFT                                  20
#define WF_TXS_PPDU_MPDU_FAIL_BYTE_DW                                  6
#define WF_TXS_PPDU_MPDU_FAIL_BYTE_ADDR                                24
#define WF_TXS_PPDU_MPDU_FAIL_BYTE_MASK \
	0x0000ffff // 15- 0
#define WF_TXS_PPDU_MPDU_FAIL_BYTE_SHIFT                               0
#define WF_TXS_PPDU_MPDU_FAIL_CNT_DW                                   6
#define WF_TXS_PPDU_MPDU_FAIL_CNT_ADDR                                 24
#define WF_TXS_PPDU_MPDU_FAIL_CNT_MASK \
	0x7ff00000 // 30-20
#define WF_TXS_PPDU_MPDU_FAIL_CNT_SHIFT                                20
#define WF_TXS_PPDU_MPDU_RTY_BYTE_DW                                   7
#define WF_TXS_PPDU_MPDU_RTY_BYTE_ADDR                                 28
#define WF_TXS_PPDU_MPDU_RTY_BYTE_MASK \
	0x0000ffff // 15- 0
#define WF_TXS_PPDU_MPDU_RTY_BYTE_SHIFT                                0
#define WF_TXS_PPDU_MPDU_RTY_CNT_DW                                    7
#define WF_TXS_PPDU_MPDU_RTY_CNT_ADDR                                  28
#define WF_TXS_PPDU_MPDU_RTY_CNT_MASK \
	0x7ff00000 // 30-20
#define WF_TXS_PPDU_MPDU_RTY_CNT_SHIFT                                 20
#define WF_TXS_PPDU_ANT_ID0_DW                                         8
#define WF_TXS_PPDU_ANT_ID0_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID0_MASK \
	0x00000007 // 2- 0
#define WF_TXS_PPDU_ANT_ID0_SHIFT                                      0
#define WF_TXS_PPDU_ANT_ID1_DW                                         8
#define WF_TXS_PPDU_ANT_ID1_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID1_MASK \
	0x00000038 // 5- 3
#define WF_TXS_PPDU_ANT_ID1_SHIFT                                      3
#define WF_TXS_PPDU_ANT_ID2_DW                                         8
#define WF_TXS_PPDU_ANT_ID2_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID2_MASK \
	0x000001c0 // 8- 6
#define WF_TXS_PPDU_ANT_ID2_SHIFT                                      6
#define WF_TXS_PPDU_ANT_ID3_DW                                         8
#define WF_TXS_PPDU_ANT_ID3_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID3_MASK \
	0x00000e00 // 11- 9
#define WF_TXS_PPDU_ANT_ID3_SHIFT                                      9
#define WF_TXS_PPDU_ANT_ID4_DW                                         8
#define WF_TXS_PPDU_ANT_ID4_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID4_MASK \
	0x00007000 // 14-12
#define WF_TXS_PPDU_ANT_ID4_SHIFT                                      12
#define WF_TXS_PPDU_ANT_ID5_DW                                         8
#define WF_TXS_PPDU_ANT_ID5_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID5_MASK \
	0x00038000 // 17-15
#define WF_TXS_PPDU_ANT_ID5_SHIFT                                      15
#define WF_TXS_PPDU_ANT_ID6_DW                                         8
#define WF_TXS_PPDU_ANT_ID6_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID6_MASK \
	0x001c0000 // 20-18
#define WF_TXS_PPDU_ANT_ID6_SHIFT                                      18
#define WF_TXS_PPDU_ANT_ID7_DW                                         8
#define WF_TXS_PPDU_ANT_ID7_ADDR                                       32
#define WF_TXS_PPDU_ANT_ID7_MASK \
	0x00e00000 // 23-21
#define WF_TXS_PPDU_ANT_ID7_SHIFT                                      21

#define WF_TXS_PPDU_GET_RATE_TX_RATE(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_RATE_TX_RATE)
#define WF_TXS_PPDU_GET_RATE_TX_MODE(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_RATE_TX_MODE)
#define WF_TXS_PPDU_GET_RATE_NSTS(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_RATE_NSTS)
#define WF_TXS_PPDU_GET_TXS2M(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TXS2M)
#define WF_TXS_PPDU_GET_TXS2H(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TXS2H)
#define WF_TXS_PPDU_GET_TXSFM(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TXSFM)
#define WF_TXS_PPDU_GET_AM(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_AM)
#define WF_TXS_PPDU_GET_TID(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TID)
#define WF_TXS_PPDU_GET_TBW(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TBW)
#define WF_TXS_PPDU_GET_TX_POWER(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TX_POWER)
#define WF_TXS_PPDU_GET_RXV_SN(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_RXV_SN)
#define WF_TXS_PPDU_GET_RESP_LEGACY_RATE(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_RESP_LEGACY_RATE)
#define WF_TXS_PPDU_GET_SN(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_SN)
#define WF_TXS_PPDU_GET_TRANSMISSION_DELAY(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TRANSMISSION_DELAY)
#define WF_TXS_PPDU_GET_WLAN_IDX(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_WLAN_IDX)
#define WF_TXS_PPDU_GET_FR(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_FR)
#define WF_TXS_PPDU_GET_RATE_STBC(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_RATE_STBC)
#define WF_TXS_PPDU_GET_BF_STS(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_BF_STS)
#define WF_TXS_PPDU_GET_LAST_TX_RATE_IDX(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_LAST_TX_RATE_IDX)
#define WF_TXS_PPDU_GET_CO_ANT(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_CO_ANT)
#define WF_TXS_PPDU_GET_SRC(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_SRC)
#define WF_TXS_PPDU_GET_PID(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_PID)
#define WF_TXS_PPDU_GET_TIMESTAMP(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_TIMESTAMP)
#define WF_TXS_PPDU_GET_MPDU_TX_BYTE(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_MPDU_TX_BYTE)
#define WF_TXS_PPDU_GET_MPDU_TX_CNT(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_MPDU_TX_CNT)
#define WF_TXS_PPDU_GET_MPDU_FAIL_BYTE(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_MPDU_FAIL_BYTE)
#define WF_TXS_PPDU_GET_MPDU_FAIL_CNT(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_MPDU_FAIL_CNT)
#define WF_TXS_PPDU_GET_MPDU_RTY_BYTE(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_MPDU_RTY_BYTE)
#define WF_TXS_PPDU_GET_MPDU_RTY_CNT(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_MPDU_RTY_CNT)
#define WF_TXS_PPDU_GET_ANT_ID0(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID0)
#define WF_TXS_PPDU_GET_ANT_ID1(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID1)
#define WF_TXS_PPDU_GET_ANT_ID2(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID2)
#define WF_TXS_PPDU_GET_ANT_ID3(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID3)
#define WF_TXS_PPDU_GET_ANT_ID4(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID4)
#define WF_TXS_PPDU_GET_ANT_ID5(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID5)
#define WF_TXS_PPDU_GET_ANT_ID6(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID6)
#define WF_TXS_PPDU_GET_ANT_ID7(reg32) \
	READ_FIELD((reg32), WF_TXS_PPDU_ANT_ID7)

#define WF_TXS_PPDU_SET_RATE_TX_RATE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_RATE_TX_RATE, val32)
#define WF_TXS_PPDU_SET_RATE_TX_MODE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_RATE_TX_MODE, val32)
#define WF_TXS_PPDU_SET_RATE_NSTS(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_RATE_NSTS, val32)
#define WF_TXS_PPDU_SET_TXS2M(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TXS2M, val32)
#define WF_TXS_PPDU_SET_TXS2H(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TXS2H, val32)
#define WF_TXS_PPDU_SET_TXSFM(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TXSFM, val32)
#define WF_TXS_PPDU_SET_AM(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_AM, val32)
#define WF_TXS_PPDU_SET_TID(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TID, val32)
#define WF_TXS_PPDU_SET_TBW(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TBW, val32)
#define WF_TXS_PPDU_SET_TX_POWER(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TX_POWER, val32)
#define WF_TXS_PPDU_SET_RXV_SN(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_RXV_SN, val32)
#define WF_TXS_PPDU_SET_RESP_LEGACY_RATE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_RESP_LEGACY_RATE, val32)
#define WF_TXS_PPDU_SET_SN(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_SN, val32)
#define WF_TXS_PPDU_SET_TRANSMISSION_DELAY(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TRANSMISSION_DELAY, val32)
#define WF_TXS_PPDU_SET_WLAN_IDX(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_WLAN_IDX, val32)
#define WF_TXS_PPDU_SET_FR(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_FR, val32)
#define WF_TXS_PPDU_SET_RATE_STBC(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_RATE_STBC, val32)
#define WF_TXS_PPDU_SET_BF_STS(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_BF_STS, val32)
#define WF_TXS_PPDU_SET_LAST_TX_RATE_IDX(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_LAST_TX_RATE_IDX, val32)
#define WF_TXS_PPDU_SET_CO_ANT(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_CO_ANT, val32)
#define WF_TXS_PPDU_SET_SRC(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_SRC, val32)
#define WF_TXS_PPDU_SET_PID(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_PID, val32)
#define WF_TXS_PPDU_SET_TIMESTAMP(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_TIMESTAMP, val32)
#define WF_TXS_PPDU_SET_MPDU_TX_BYTE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_MPDU_TX_BYTE, val32)
#define WF_TXS_PPDU_SET_MPDU_TX_CNT(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_MPDU_TX_CNT, val32)
#define WF_TXS_PPDU_SET_MPDU_FAIL_BYTE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_MPDU_FAIL_BYTE, val32)
#define WF_TXS_PPDU_SET_MPDU_FAIL_CNT(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_MPDU_FAIL_CNT, val32)
#define WF_TXS_PPDU_SET_MPDU_RTY_BYTE(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_MPDU_RTY_BYTE, val32)
#define WF_TXS_PPDU_SET_MPDU_RTY_CNT(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_MPDU_RTY_CNT, val32)
#define WF_TXS_PPDU_SET_ANT_ID0(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID0, val32)
#define WF_TXS_PPDU_SET_ANT_ID1(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID1, val32)
#define WF_TXS_PPDU_SET_ANT_ID2(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID2, val32)
#define WF_TXS_PPDU_SET_ANT_ID3(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID3, val32)
#define WF_TXS_PPDU_SET_ANT_ID4(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID4, val32)
#define WF_TXS_PPDU_SET_ANT_ID5(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID5, val32)
#define WF_TXS_PPDU_SET_ANT_ID6(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID6, val32)
#define WF_TXS_PPDU_SET_ANT_ID7(reg32, \
	val32) WRITE_FIELD((reg32), WF_TXS_PPDU_ANT_ID7, val32)

#define WF_TXS_PPDU_CLR_RATE_TX_RATE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_RATE_TX_RATE)
#define WF_TXS_PPDU_CLR_RATE_TX_MODE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_RATE_TX_MODE)
#define WF_TXS_PPDU_CLR_RATE_NSTS(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_RATE_NSTS)
#define WF_TXS_PPDU_CLR_TXS2M(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TXS2M)
#define WF_TXS_PPDU_CLR_TXS2H(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TXS2H)
#define WF_TXS_PPDU_CLR_TXSFM(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TXSFM)
#define WF_TXS_PPDU_CLR_AM(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_AM)
#define WF_TXS_PPDU_CLR_TID(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TID)
#define WF_TXS_PPDU_CLR_TBW(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TBW)
#define WF_TXS_PPDU_CLR_TX_POWER(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TX_POWER)
#define WF_TXS_PPDU_CLR_RXV_SN(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_RXV_SN)
#define WF_TXS_PPDU_CLR_RESP_LEGACY_RATE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_RESP_LEGACY_RATE)
#define WF_TXS_PPDU_CLR_SN(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_SN)
#define WF_TXS_PPDU_CLR_TRANSMISSION_DELAY(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TRANSMISSION_DELAY)
#define WF_TXS_PPDU_CLR_WLAN_IDX(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_WLAN_IDX)
#define WF_TXS_PPDU_CLR_FR(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_FR)
#define WF_TXS_PPDU_CLR_RATE_STBC(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_RATE_STBC)
#define WF_TXS_PPDU_CLR_BF_STS(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_BF_STS)
#define WF_TXS_PPDU_CLR_LAST_TX_RATE_IDX(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_LAST_TX_RATE_IDX)
#define WF_TXS_PPDU_CLR_CO_ANT(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_CO_ANT)
#define WF_TXS_PPDU_CLR_SRC(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_SRC)
#define WF_TXS_PPDU_CLR_PID(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_PID)
#define WF_TXS_PPDU_CLR_TIMESTAMP(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_TIMESTAMP)
#define WF_TXS_PPDU_CLR_MPDU_TX_BYTE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_MPDU_TX_BYTE)
#define WF_TXS_PPDU_CLR_MPDU_TX_CNT(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_MPDU_TX_CNT)
#define WF_TXS_PPDU_CLR_MPDU_FAIL_BYTE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_MPDU_FAIL_BYTE)
#define WF_TXS_PPDU_CLR_MPDU_FAIL_CNT(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_MPDU_FAIL_CNT)
#define WF_TXS_PPDU_CLR_MPDU_RTY_BYTE(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_MPDU_RTY_BYTE)
#define WF_TXS_PPDU_CLR_MPDU_RTY_CNT(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_MPDU_RTY_CNT)
#define WF_TXS_PPDU_CLR_ANT_ID0(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID0)
#define WF_TXS_PPDU_CLR_ANT_ID1(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID1)
#define WF_TXS_PPDU_CLR_ANT_ID2(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID2)
#define WF_TXS_PPDU_CLR_ANT_ID3(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID3)
#define WF_TXS_PPDU_CLR_ANT_ID4(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID4)
#define WF_TXS_PPDU_CLR_ANT_ID5(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID5)
#define WF_TXS_PPDU_CLR_ANT_ID6(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID6)
#define WF_TXS_PPDU_CLR_ANT_ID7(reg32) \
	CLEAR_FIELD((reg32), WF_TXS_PPDU_ANT_ID7)

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __WF_TXS_PPDU_REGS_H__
