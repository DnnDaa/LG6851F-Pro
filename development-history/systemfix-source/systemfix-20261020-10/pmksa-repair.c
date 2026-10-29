#include "rt_config.h"
#include <net/cfg80211.h>

/* The native PMKID list stores validation metadata. hostapd still performs
 * SAE and the four-way handshake and owns the PMK/expiry lifecycle. */
extern struct cfg80211_ops CFG80211_Ops;

static struct wifi_dev *lg9_ap(struct wiphy *wiphy, struct net_device *dev,
                            RTMP_ADAPTER **adapter)
{
 int index;
 RTMP_ADAPTER *ad;
 if (!wiphy || !dev || !dev->ieee80211_ptr ||
     dev->ieee80211_ptr->iftype != NL80211_IFTYPE_AP)
  return NULL;
 ad = (RTMP_ADAPTER *)(*(ULONG *)wiphy_priv(wiphy));
 if (!ad || !ad->physical_dev)
  return NULL;
 index = CFG80211_FindMbssApIdxByNetDevice(ad, dev);
 if (index < 0 || index >= ad->ApCfg.BssidNum)
  return NULL;
 *adapter = ad;
 return &ad->ApCfg.MBSSID[index].wdev;
}

static bool lg9_same_owner(PAP_BSSID_INFO item, struct wifi_dev *wdev)
{
 if (MAC_ADDR_EQUAL(item->own_mac, wdev->bssid))
  return true;
#ifdef DOT11_EHT_BE
 if (!MAC_ADDR_EQUAL(wdev->bss_info_argument.mld_info.mld_addr, ZERO_MAC_ADDR) &&
     MAC_ADDR_EQUAL(item->own_mac, wdev->bss_info_argument.mld_info.mld_addr))
  return true;
#endif
 return false;
}

static int lg9_ap_set(struct wiphy *wiphy, struct net_device *dev,
                     struct cfg80211_pmksa *pmksa)
{
 RTMP_ADAPTER *ad = NULL;
 struct wifi_dev *wdev = lg9_ap(wiphy, dev, &ad);
 NDIS_AP_802_11_PMKID *cache;
 MAC_TABLE_ENTRY *entry;
 UCHAR zero_pmk[LEN_MAX_PMK] = {0};
 UCHAR *own, *peer;
 bool mlo = false;
 int index;
 if (!wdev || !pmksa || !pmksa->bssid || !pmksa->pmkid ||
     !is_valid_ether_addr(pmksa->bssid))
  return -EINVAL;
 own = wdev->bssid;
 peer = (UCHAR *)pmksa->bssid;
 entry = MacTableLookupForTx(ad, peer, NULL);
 /* Do not borrow a station entry from another BSS. A PMKSA insertion may
  * precede creation of its station entry, so no entry is also valid. */
 if (entry && entry->wdev != wdev)
  entry = NULL;
#ifdef DOT11_EHT_BE
 if (entry && (entry->mlo.mlo_en || entry->mlo_join) &&
     is_valid_ether_addr(entry->mlo.mld_addr) &&
     is_valid_ether_addr(wdev->bss_info_argument.mld_info.mld_addr)) {
  mlo = true;
  own = wdev->bss_info_argument.mld_info.mld_addr;
  peer = entry->mlo.mld_addr;
 }
#endif
 cache = PD_GET_PMKID_PTR(ad->physical_dev);
 if (!cache)
  return -ENODEV;
 OS_SEM_LOCK(&cache->pmkid_lock);
 index = RTMPAddPMKIDCache(cache, own, peer, (UCHAR *)pmksa->pmkid,
                         zero_pmk, FALSE, mlo, LEN_PMK, 0);
 OS_SEM_UNLOCK(&cache->pmkid_lock);
 if (index < 0 || index >= MAX_PMKID_COUNT)
  return -ENOSPC;
 if (entry)
  NdisCopyMemory(entry->PmkidByHostapd, pmksa->pmkid, LEN_PMKID);
 return 0;
}

int lg9_pmksa_del(struct wiphy *wiphy, struct net_device *dev,
                 struct cfg80211_pmksa *pmksa)
{
 RTMP_ADAPTER *ad = NULL;
 struct wifi_dev *wdev = lg9_ap(wiphy, dev, &ad);
 NDIS_AP_802_11_PMKID *cache;
 MAC_TABLE_ENTRY *entry;
 int i;
 if (!wdev || !pmksa || !pmksa->bssid || !pmksa->pmkid)
  return -EINVAL;
 cache = PD_GET_PMKID_PTR(ad->physical_dev);
 if (!cache)
  return -ENODEV;
 entry = MacTableLookupForTx(ad, (UCHAR *)pmksa->bssid, NULL);
 OS_SEM_LOCK(&cache->pmkid_lock);
 for (i = 0; i < MAX_PMKID_COUNT; i++) {
  PAP_BSSID_INFO item = &cache->BSSIDInfo[i];
  if (item->Valid && lg9_same_owner(item, wdev) &&
      (MAC_ADDR_EQUAL(item->peer_mac, pmksa->bssid)
#ifdef DOT11_EHT_BE
       || (entry && entry->wdev == wdev && (entry->mlo.mlo_en || entry->mlo_join) &&
           MAC_ADDR_EQUAL(item->peer_mac, entry->mlo.mld_addr))
#endif
      ) && RTMPEqualMemory(item->PMKID, pmksa->pmkid, LEN_PMKID))
   NdisZeroMemory(item, sizeof(*item));
 }
 OS_SEM_UNLOCK(&cache->pmkid_lock);
 if (entry && entry->wdev == wdev &&
     RTMPEqualMemory(entry->PmkidByHostapd, pmksa->pmkid, LEN_PMKID))
  NdisZeroMemory(entry->PmkidByHostapd, LEN_PMKID);
 return 0;
}

int lg9_pmksa_flush(struct wiphy *wiphy, struct net_device *dev)
{
 RTMP_ADAPTER *ad = NULL;
 struct wifi_dev *wdev = lg9_ap(wiphy, dev, &ad);
 NDIS_AP_802_11_PMKID *cache;
 int i;
 /* The old callback was a no-op in this APCLI build. Preserve non-AP use. */
 if (!wdev)
  return 0;
 cache = PD_GET_PMKID_PTR(ad->physical_dev);
 if (!cache)
  return -ENODEV;
 OS_SEM_LOCK(&cache->pmkid_lock);
 for (i = 0; i < MAX_PMKID_COUNT; i++)
  if (cache->BSSIDInfo[i].Valid && lg9_same_owner(&cache->BSSIDInfo[i], wdev))
   NdisZeroMemory(&cache->BSSIDInfo[i], sizeof(cache->BSSIDInfo[i]));
 OS_SEM_UNLOCK(&cache->pmkid_lock);
 return 0;
}

int lg9_pmksa_set(struct wiphy *wiphy, struct net_device *dev,
                 struct cfg80211_pmksa *pmksa)
{
 RTMP_ADAPTER *ad;
 if (!wiphy || !dev || !dev->ieee80211_ptr || !pmksa ||
     !pmksa->bssid || !pmksa->pmkid)
  return -EINVAL;
 if (dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP)
  return lg9_ap_set(wiphy, dev, pmksa);
 ad = (RTMP_ADAPTER *)(*(ULONG *)wiphy_priv(wiphy));
 if (!ad)
  return -ENODEV;
#if defined(CONFIG_STA_SUPPORT) || defined(APCLI_CFG80211_SUPPORT)
 if (dev->ieee80211_ptr->iftype == NL80211_IFTYPE_STATION) {
  RT_CMD_STA_IOCTL_PMA_SA request;
  NdisZeroMemory(&request, sizeof(request));
  request.Cmd = RT_CMD_STA_IOCTL_PMA_SA_ADD;
  request.pBssid = (UCHAR *)pmksa->bssid;
  request.pPmkid = (UCHAR *)pmksa->pmkid;
  request.pNetDev = dev;
  RTMP_DRIVER_80211_PMKID_CTRL(ad, &request);
 }
#endif
 return 0;
}

/* hostapd's internal-AP-SME SAE completion uses EXTERNAL_AUTH, not
 * SET_PMKSA. The factory callback accepts only STATION and silently discards
 * AP notifications. Keep that callback for STA and consume the authenticated
 * PMKID on AP before the following association request is processed. */
static int (*lg10_original_external_auth)(struct wiphy *, struct net_device *,
                                        struct cfg80211_external_auth_params *);

static int lg10_external_auth(struct wiphy *wiphy, struct net_device *dev,
                             struct cfg80211_external_auth_params *params)
{
 struct cfg80211_pmksa pmksa;
 if (!wiphy || !dev || !dev->ieee80211_ptr || !params)
  return -EINVAL;
 if (dev->ieee80211_ptr->iftype != NL80211_IFTYPE_AP)
  return lg10_original_external_auth ?
   lg10_original_external_auth(wiphy, dev, params) : -EOPNOTSUPP;
 /* Never create a cache entry from an unsuccessful authentication or a
  * notification without a PMKID (e.g. when PMKSA caching is disabled). */
 if (params->status || !params->pmkid)
  return 0;
 NdisZeroMemory(&pmksa, sizeof(pmksa));
 pmksa.bssid = params->bssid;
 pmksa.pmkid = params->pmkid;
 return lg9_ap_set(wiphy, dev, &pmksa);
}

int lg9_wiphy_register(struct wiphy *wiphy)
{
 /* Expose the standard interface only with its complete add/delete/flush
  * implementation installed. The unmodified vendor path remains usable. */
 if (wiphy && (wiphy->interface_modes & BIT(NL80211_IFTYPE_AP))) {
  if (CFG80211_Ops.external_auth != lg10_external_auth) {
   lg10_original_external_auth = CFG80211_Ops.external_auth;
   CFG80211_Ops.external_auth = lg10_external_auth;
  }
  CFG80211_Ops.del_pmksa = lg9_pmksa_del;
  wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_AP_PMKSA_CACHING);
  wiphy->max_num_pmkids = MAX_PMKID_COUNT;
 }
 return wiphy_register(wiphy);
}

void lg9_get_rate_flag(UCHAR mode, UINT8 *flags)
{
 UINT8 short_gi;
 if (!flags)
  return;
 short_gi = *flags & RATE_INFO_FLAGS_SHORT_GI;
 *flags = 0;
#ifdef DOT11_EHT_BE
 if (mode == MODE_EHT || mode == MODE_EHT_ER_SU ||
     mode == MODE_EHT_TB || mode == MODE_EHT_MU) {
  *flags = RATE_INFO_FLAGS_EHT_MCS;
  return;
 }
#endif
 if (mode == MODE_UNKNOWN)
  return;
 if (mode == MODE_HE || mode >= MODE_HE_24G)
  *flags = RATE_INFO_FLAGS_HE_MCS;
 else if (mode >= MODE_VHT)
  *flags = RATE_INFO_FLAGS_VHT_MCS | short_gi;
 else if (mode >= MODE_HTMIX)
  *flags = RATE_INFO_FLAGS_MCS | short_gi;
}
