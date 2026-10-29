
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
typedef uint8_t u8;
#define ETH_ALEN 6
#define ML_MAX_LINK_NUM 3
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(a) (a)[0],(a)[1],(a)[2],(a)[3],(a)[4],(a)[5]
#define os_snprintf snprintf
static int is_zero_ether_addr(const u8 *a){return !(a[0]|a[1]|a[2]|a[3]|a[4]|a[5]);}
static int is_multicast_ether_addr(const u8 *a){return a[0]&1;}
struct profile{u8 addr[6];unsigned int mac_addr_present;};
struct wpa_ml_ie_parse{u8 ml_addr[6];int valid;size_t prof_num;struct profile profiles[3];};
struct hostapd_data{u8 own_addr[6];};
struct wpa_authenticator{void *cb_ctx;};
struct wpa_state_machine{struct wpa_authenticator *wpa_auth;u8 addr[6];int dot11MultiLinkActivated;struct wpa_ml_ie_parse *sta_ml_ie;};
#define LG6851F_IDENTITY_DIR "/tmp/lg6851f-wifi-identities"

static int ml_identity_path(struct wpa_state_machine *sm, char *path, size_t len)
{
    struct hostapd_data *hapd;
    if (!sm || !sm->wpa_auth || !sm->wpa_auth->cb_ctx)
        return -1;
    hapd = sm->wpa_auth->cb_ctx;
    return os_snprintf(path, len, LG6851F_IDENTITY_DIR "/"
                       MACSTR "-" MACSTR ".json",
                       MAC2STR(hapd->own_addr), MAC2STR(sm->addr));
}

void ml_identity_remove(struct wpa_state_machine *sm)
{
    char path[160];
    int n = ml_identity_path(sm, path, sizeof(path));
    if (n > 0 && n < (int) sizeof(path))
        unlink(path);
}

static void ml_identity_publish(struct wpa_state_machine *sm)
{
    char path[160], temporary[164];
    FILE *f;
    size_t i;
    int n, failed;
    struct wpa_ml_ie_parse *ml;

    if (!sm || !sm->dot11MultiLinkActivated || !(ml = sm->sta_ml_ie) ||
        !ml->valid || ml->prof_num > ML_MAX_LINK_NUM ||
        is_zero_ether_addr(ml->ml_addr) || is_multicast_ether_addr(ml->ml_addr))
        return;
    n = ml_identity_path(sm, path, sizeof(path));
    if (n <= 0 || n >= (int) sizeof(path))
        return;
    if (mkdir(LG6851F_IDENTITY_DIR, 0700) && errno != EEXIST)
        return;
    os_snprintf(temporary, sizeof(temporary), "%s.new", path);
    f = fopen(temporary, "w");
    if (!f)
        return;
    /* Only addresses and process ownership are exported. Never export IEs,
     * passwords, SAE material, PMKs, or key data. Atomic replace avoids tears. */
    fprintf(f, "{\"pid\":%ld,\"mld_mac\":\"" MACSTR "\",\"links\":[\""
            MACSTR "\"", (long) getpid(), MAC2STR(ml->ml_addr), MAC2STR(sm->addr));
    for (i = 0; i < ml->prof_num; i++) {
        const u8 *addr = ml->profiles[i].addr;
        if (!ml->profiles[i].mac_addr_present || is_zero_ether_addr(addr) ||
            is_multicast_ether_addr(addr))
            continue;
        fprintf(f, ",\"" MACSTR "\"", MAC2STR(addr));
    }
    fputs("]}\n", f);
    failed = ferror(f);
    if (fclose(f))
        failed = 1;
    if (failed || rename(temporary, path))
        unlink(temporary);
}


int main(void){
 struct hostapd_data h={{0x52,0xcf,0x14,0x1a,0x5f,0xb7}};
 struct wpa_authenticator a={&h};
 struct wpa_ml_ie_parse ml={{0x6e,0xc8,0x1e,0x3e,0x5d,0x25},1,1,{{{0xae,0xb0,0xba,0x95,0x1d,0x50},1}}};
 struct wpa_state_machine s={&a,{0xbe,0x69,0x59,0x3d,0xad,0xea},1,&ml};
 char path[160],data[2048];FILE *f;size_t n;
 ml_identity_path(&s,path,sizeof(path));ml_identity_remove(&s);
 ml_identity_publish(&s);f=fopen(path,"r");assert(f);n=fread(data,1,sizeof(data)-1,f);data[n]=0;fclose(f);
 assert(strstr(data,"6e:c8:1e:3e:5d:25")&&strstr(data,"be:69:59:3d:ad:ea")&&strstr(data,"ae:b0:ba:95:1d:50"));
 assert(!strstr(data,"password")&&!strstr(data,"pmk"));
 ml_identity_remove(&s);assert(access(path,F_OK)!=0);
 ml.valid=0;ml_identity_publish(&s);assert(access(path,F_OK)!=0);
 ml.valid=1;ml.prof_num=4;ml_identity_publish(&s);assert(access(path,F_OK)!=0);
 ml.prof_num=1;ml.ml_addr[0]|=1;ml_identity_publish(&s);assert(access(path,F_OK)!=0);
 ml.ml_addr[0]&=~1;s.dot11MultiLinkActivated=0;ml_identity_publish(&s);assert(access(path,F_OK)!=0);
 ml_identity_remove(NULL);ml_identity_publish(NULL);
 puts("{\"passed\":true,\"actual_identity_helper_checks\":8,\"target_aarch64\":true}");return 0;
}
