#include <linux/netdevice.h>
#include <net/cfg80211.h>
#include <linux/errno.h>
_Static_assert(offsetof(struct net_device, ieee80211_ptr) == 976, "original net_device ABI");
_Static_assert(offsetof(struct wireless_dev, iftype) == 8, "original wireless_dev ABI");
int lg63_change_type_noop(struct wiphy *wiphy, struct net_device *dev,
                         enum nl80211_iftype type, struct vif_params *params)
{
 unsigned char nonzero = 0; int i;
 if (!dev || !dev->ieee80211_ptr) return -EINVAL;
 if (type != dev->ieee80211_ptr->iftype) return -EOPNOTSUPP;
 if (!params) return 0;
 if (params->use_4addr > 0 || params->flags || params->vht_mumimo_groups || params->vht_mumimo_follow_addr)
  return -EOPNOTSUPP;
 for (i=0;i<6;i++) nonzero |= params->macaddr[i];
 return nonzero ? -EOPNOTSUPP : 0;
}
const unsigned long lg63_vif_layout[] = {sizeof(struct vif_params),offsetof(struct vif_params,flags),offsetof(struct vif_params,use_4addr),offsetof(struct vif_params,macaddr),offsetof(struct vif_params,vht_mumimo_groups),offsetof(struct vif_params,vht_mumimo_follow_addr)};
