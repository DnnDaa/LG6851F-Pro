
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t u8;
#define HOSTAPD_PMKID_IN_DRIVER_SUPPORT
#define CONFIG_MTK_IEEE80211BE
#define WPA_KEY_MGMT_SAE 1024
#define wpa_printf(...) ((void)0)
#define wpa_hexdump_key(...) ((void)0)
struct rsn_pmksa_cache_entry { int installed; };
struct rsn_pmksa_cache { int index;struct rsn_pmksa_cache_entry entry; };
struct wpa_authenticator { struct { int disable_pmksa_caching; } conf;void *cb_ctx;struct rsn_pmksa_cache *pmksa; };
struct wpa_ml_link { void *ctx; };
struct wpa_ml_group { int ml_link_num;struct wpa_ml_link links[2]; };
struct hostapd_data { struct wpa_ml_group *ml_group;struct wpa_authenticator *wpa_auth; };
static int reject=-1,calls=0,rollbacks=0;
static u8 peer[6]={2},ap[6]={6},pmk[32]={1},pmkid[16]={3},linkaddr[6]={10};
static struct rsn_pmksa_cache_entry *pmksa_cache_auth_add(struct rsn_pmksa_cache *cache,
 const u8 *key,size_t len,const u8 *id,const u8 *kck,size_t kcklen,const u8 *aa,const u8 *spa,
 int timeout,void *eapol,int akmp,const u8 *link) {
 calls++;assert(!memcmp(aa,ap,6)&&!memcmp(spa,peer,6)&&!memcmp(id,pmkid,16));
 assert(!memcmp(link,linkaddr,6)&&akmp==WPA_KEY_MGMT_SAE);
 if(cache->index==reject)return NULL;
 cache->entry.installed=1;return &cache->entry;
}
static struct rsn_pmksa_cache_entry *pmksa_cache_auth_get(struct rsn_pmksa_cache *cache,const u8 *spa,const u8 *id) {
 assert(!memcmp(spa,peer,6)&&!memcmp(id,pmkid,16));return cache->entry.installed?&cache->entry:NULL;
}
static void pmksa_cache_free_entry(struct rsn_pmksa_cache *cache,struct rsn_pmksa_cache_entry *entry) {
 assert(entry==&cache->entry);entry->installed=0;rollbacks++;
}

int wpa_auth_pmksa_add_sae(struct wpa_authenticator *wpa_auth, const u8 *addr,
			   const u8 *aa, const u8 *pmk, size_t pmk_len,
			   const u8 *pmkid, int akmp
#ifdef HOSTAPD_PMKID_IN_DRIVER_SUPPORT
			   ,const u8 * link_addr
#endif
)
{
	if (wpa_auth->conf.disable_pmksa_caching)
		return -1;

	wpa_hexdump_key(MSG_DEBUG, "RSN: Cache PMK from SAE", pmk, pmk_len);
	if (!akmp)
		akmp = WPA_KEY_MGMT_SAE;

#ifdef CONFIG_MTK_IEEE80211BE
	if (1) {
		struct wpa_ml_group *ml_group = ((struct hostapd_data *)wpa_auth->cb_ctx)->ml_group;
		if (!ml_group || ml_group->ml_link_num == 0) { /*legacy case*/
			// When ml_link_num is zero(dellink or set phymode), Ap cannot generate pmksa for STA 
			wpa_printf(MSG_INFO, "RSN: legacy case to add pmksa");
			if (pmksa_cache_auth_add(wpa_auth->pmksa, pmk, pmk_len, pmkid,
				 NULL, 0, aa, addr, 0, NULL, akmp
#ifdef HOSTAPD_PMKID_IN_DRIVER_SUPPORT
				 ,link_addr
#endif
			))
			return 0;
		} else { /*mlo case*/
			int i = 0;
			struct wpa_ml_link *link = NULL;
			struct wpa_authenticator *auth = NULL;
			wpa_printf(MSG_INFO, "ML: mlo case to add pmksa");
			for (i = 0; i < ml_group->ml_link_num; i++) {
				link = &ml_group->links[i];
				if (link == NULL)
					continue;

				auth = link->ctx ? ((struct hostapd_data *)link->ctx)->wpa_auth : NULL;
				wpa_printf(MSG_INFO, "ML: wpa_auth:%p peer MLD MAC is (" MACSTR ") ", auth, MAC2STR(addr));
				if (!auth || !pmksa_cache_auth_add(auth->pmksa,
					pmk, pmk_len, pmkid, NULL, 0, aa, addr, 0, NULL, akmp
#ifdef HOSTAPD_PMKID_IN_DRIVER_SUPPORT
					,link_addr
#endif
				)) {
					int j;
					wpa_printf(MSG_ERROR, "ML: native PMKSA install failed on link %d", i);
					/* Roll back earlier links in this installation attempt. */
					for (j = 0; j < i; j++) {
						struct hostapd_data *done = ml_group->links[j].ctx;
						struct rsn_pmksa_cache_entry *added;
						if (!done || !done->wpa_auth)
							continue;
						added = pmksa_cache_auth_get(done->wpa_auth->pmksa, addr, pmkid);
						if (added)
							pmksa_cache_free_entry(done->wpa_auth->pmksa, added);
					}
					return -1;
				}
				wpa_printf(MSG_DEBUG, "ML: native PMKSA install acknowledged");
			}
			return 0;
		}
	}
#else

	if (pmksa_cache_auth_add(wpa_auth->pmksa, pmk, pmk_len, pmkid,
		NULL, 0, aa, addr, 0, NULL, akmp
#ifdef HOSTAPD_PMKID_IN_DRIVER_SUPPORT
		,link_addr
#endif
	))
	return 0;
#endif /* CONFIG_MTK_IEEE80211BE */
	return -1;
}


int main(void) {
 struct rsn_pmksa_cache cache[2]={{.index=0},{.index=1}};
 struct wpa_authenticator auth[2]={{.pmksa=&cache[0]},{.pmksa=&cache[1]}};
 struct wpa_ml_group group={.ml_link_num=2};
 struct hostapd_data hapd[2]={{.ml_group=&group,.wpa_auth=&auth[0]},{.ml_group=&group,.wpa_auth=&auth[1]}};
 int i,r,checks=0;
 for(i=0;i<2;i++){auth[i].cb_ctx=&hapd[i];group.links[i].ctx=&hapd[i];}
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==0&&calls==2&&cache[0].entry.installed&&cache[1].entry.installed);checks++;
 for(i=0;i<2;i++)cache[i].entry.installed=0;calls=rollbacks=0;reject=1;
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==-1&&calls==2&&rollbacks==1&&!cache[0].entry.installed&&!cache[1].entry.installed);checks++;
 calls=rollbacks=0;reject=0;
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==-1&&calls==1&&rollbacks==0&&!cache[0].entry.installed&&!cache[1].entry.installed);checks++;
 reject=-1;calls=rollbacks=0;hapd[1].wpa_auth=NULL;
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==-1&&calls==1&&rollbacks==1&&!cache[0].entry.installed);checks++;
 hapd[1].wpa_auth=&auth[1];group.links[1].ctx=NULL;calls=rollbacks=0;
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==-1&&calls==1&&rollbacks==1&&!cache[0].entry.installed);checks++;
 group.links[1].ctx=&hapd[1];hapd[0].ml_group=NULL;calls=0;reject=-1;
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==0&&calls==1&&cache[0].entry.installed);checks++;
 cache[0].entry.installed=0;calls=0;reject=0;
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==-1&&calls==1&&!cache[0].entry.installed);checks++;
 auth[0].conf.disable_pmksa_caching=1;calls=0;
 r=wpa_auth_pmksa_add_sae(&auth[0],peer,ap,pmk,32,pmkid,0,linkaddr);
 assert(r==-1&&calls==0);checks++;
 printf("{\"transaction_cases\":%d,\"actual_source_function\":true,\"driver_ACKs_mocked\":true,\"passed\":true}\n",checks);
 return 0;
}
