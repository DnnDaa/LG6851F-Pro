/* Exact native PMKID wire layout, shared by add/remove and offline checks. */
#ifndef LG6851F_PMKID_ABI_H
#define LG6851F_PMKID_ABI_H
_Static_assert(sizeof(struct wpa_pmkid_entry) == 40, "Native PMKID ABI size");
_Static_assert(offsetof(struct wpa_pmkid_entry, akmp) == 36, "Native PMKID AKM offset");
static inline void lg6851f_fill_pmkid(struct wpa_pmkid_entry *out, u8 add,
 const u8 *aa, const u8 *spa, const u8 *link, const u8 *pmkid, int akmp)
{
 os_memset(out, 0, sizeof(*out)); /* Includes ABI padding, never leak stack. */
 os_memcpy(out->bssid, aa, ETH_ALEN);
 os_memcpy(out->sta, spa, ETH_ALEN);
 os_memcpy(out->link_addr, link, ETH_ALEN);
 os_memcpy(out->pmkid, pmkid, PMKID_LEN);
 out->AddRemove = add;
 out->akmp = akmp;
}
#endif
