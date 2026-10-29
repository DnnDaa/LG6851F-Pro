/*******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef WPA_ML_SUPPLICANT_H
#define WPA_ML_SUPPLICANT_H

int ml_set_sae_auth_commit_req_ml_ie(struct sae_data * sae,const u8 * ies,size_t ies_len);
int ml_set_assoc_req_ml_ie(struct wpa_sm *sm, const u8 *ies, size_t ies_len);
int ml_set_assoc_resp_ml_ie(struct wpa_sm *sm, const u8 *ies, size_t ies_len, u8 *bssid);
size_t ml_add_m2_kde(struct wpa_sm *sm, u8 *pos);
int ml_validate_m3_kde(struct wpa_sm *sm, const struct wpa_eapol_key *key,
	struct wpa_eapol_ie_parse *ie);
int ml_process_m1_kde(struct wpa_sm *sm, struct wpa_eapol_ie_parse *ie);
int ml_process_m3_kde(struct wpa_sm *sm, const struct wpa_eapol_key *key,
	struct wpa_eapol_ie_parse *ie);
size_t ml_add_m4_kde(struct wpa_sm *sm, u8 *pos);
size_t ml_add_key_request_kde(struct wpa_sm *sm, u8 *pos);
size_t ml_add_2_of_2_kde(struct wpa_sm *sm, u8 *pos);
int ml_process_1_of_2(struct wpa_sm *sm, const struct wpa_eapol_key *key,
		const u8 *key_data, size_t key_data_len, u16 key_info);
const u8 * ml_get_ie(const u8 *ies, size_t ie_len, u32 ml_ie_type);

#ifdef CONFIG_MTK_IEEE80211BE
const u8 * ml_sm_spa(struct wpa_sm *sm, const u8 *own_addr);
const u8 * ml_sm_aa(struct wpa_sm *sm, const u8 *bssid);
#else
#define ml_sm_spa(__sm, __addr) __addr
#define ml_sm_aa(__sm, __addr) __addr
#endif

#endif /* WPA_ML_SUPPLICANT_H */
