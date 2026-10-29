/*
Copyright Statement:

This software/firmware and related documentation ("MediaTek Software") are
protected under relevant copyright laws. The information contained herein is
confidential and proprietary to MediaTek Inc. and/or its licensors. Without
the prior written permission of MediaTek inc. and/or its licensors, any
reproduction, modification, use or disclosure of MediaTek Software, and
information contained herein, in whole or in part, shall be strictly
prohibited.

MediaTek Inc. (C) 2016. All rights reserved.

BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.

Auther: Garlic Tseng <garlic.tseng@mediatek.com>
*/

/*
usage:
1. call get_default_device to get the system default phonecall device
2. call enable_phone_call_AFE_path when phone call is enabled
   (may or may not use default device)
3. call disable_phone_call_AFE_path when phone call is disabled
*/

#ifndef _MODEM_ALSA_LIB_H_
#define _MODEM_ALSA_LIB_H_

typedef enum {
	DEV_SPK,
	DEV_SPK_EX,
	DEV_DIRECT,
	DEV_DIRECT_LO,
	DEV_HP,
	DEV_BT,
	DEV_DEFAULT,
	DEV_NUM = DEV_DEFAULT,
	DEV_INVALID = -1,
} Audio_device;

/***********************************************
 * The API below will make request to server.  *
 ***********************************************/

int quectel_audio_loopback(int enable);
int quectel_audio_playback_mixer_value(int enable);

//enable audio path for modem.
//return value: 0 if success, or negitive error number
int enable_phone_call_AFE_path(int audio_device);

//close audio path for modem
//return value: 0 if success, or negitive error number
int disable_phone_call_AFE_path(void);

//get default device.
//return value: default device setting
int get_default_device(void);

//set UL analog gain
//return value: 0 if success, or negitive error number
int set_UL_analog_gain(int dB);

//set DL analog gain
//return value: 0 if success, or negitive error number
int set_DL_analog_gain(int dB);

//set BT WB on
//return value: 0 if success, or negitive error number
int set_BT_WB(int on);

//change device when the phone call is enabled
//return value: 0 if success, or negitive error number
int dynamic_switch_phone_call_path(int audio_device);

//set phonecall sample rate
//return value: 0 if success, or negitive error number
int set_phonecall_rate(int rate);


/***********************************************
 * IPC define.                                 *
 ***********************************************/

#define FUNC_ENABLE_PHONE_CALL_AFE_PATH 0
#define FUNC_DISABLE_PHONE_CALL_AFE_PATH 1
#define FUNC_GET_DEFAULT_VALUE 2
#define FUNC_SET_UL_ANALOG_GAIN 3
#define FUNC_SET_DL_ANALOG_GAIN 4
#define FUNC_SET_BT_WB 5
#define FUNC_DYNAMIC_SWITCH_PHONE_CALL_PATH 6
#define FUNC_SET_PHONE_CALL_SAMPLERATE 7

/***********************************************
 * default parameter                           *
 ***********************************************/

#define FUNC_SET_BT_WB_default 0 //default off

/******************************************************************************
 * The API below is for service to call. Please use API defined above if you  *
 * are not service.                                                           *
 ******************************************************************************/
int set_BT_wb(int on);
int set_phonecall_rate_for_service(int rate);

//enable audio path for modem.
//return value: 0 if success, or negitive error number
int enable_phone_call_AFE_path_for_service(int audio_device);

//close audio path for modem
//return value: 0 if success, or negitive error number
int disable_phone_call_AFE_path_for_service(void);

//get default device.
//return value: default device setting
int get_default_device_for_service(void);

//set UL analog gain
int set_UL_analog_gain_for_service(int dB);

//set DL analog gain
int set_DL_analog_gain_for_service(int dB);

//change device when the phone call is enabled
int dynamic_switch_phone_call_path_for_service(int audio_device);

#endif //_MODEM_ALSA_LIB_H_

