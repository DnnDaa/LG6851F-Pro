typedef struct __RT_CMD_AP_IOCTL_UPDATE_PMKID {
	UCHAR bssid[ETH_ALEN]; /* AP's MLD addr */
	UCHAR sta[ETH_ALEN]; /* STA's MLD addr */
	UCHAR link_addr[ETH_ALEN]; /* Add: STA's link addr, Remove: AP's link addr */
	UCHAR pmkid[LEN_PMKID];
	UCHAR AddRemove;	/*1- Add, 0- Remove*/
	int akmp;		/* AKM protocol */
} RT_CMD_AP_IOCTL_UPDATE_PMKID;
