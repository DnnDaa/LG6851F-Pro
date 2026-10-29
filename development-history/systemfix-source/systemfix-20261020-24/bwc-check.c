#define main original_bwc_main
#include "fixture-bwc.c"
#undef main
#include <assert.h>

static unsigned int checks;
#define CHECK(x) do { checks++; assert(x); } while (0)
static void put_file(const char *path, const char *text)
{
    char parents[1024];
    snprintf(parents,sizeof(parents),"%s",path);
    CHECK(!init_directory(parents));
    FILE *file = fopen(path, "w");
    CHECK(file != NULL);
    CHECK(fputs(text, file) >= 0);
    CHECK(!fclose(file));
}
static void put_snapshot(int age, const char *boot)
{
    char data[4096];
    snprintf(data,sizeof(data),"{\"boot_id\":\"%s\",\"mono\":%lld,\"wifi\":{\"available\":true,\"radios\":[{\"ifname\":\"rai1\",\"rate_kbps\":2402000},{\"ifname\":\"ra1\",\"rate_kbps\":229400}],\"clients\":[{\"ifname\":\"rai1\",\"signal\":-40},{\"ifname\":\"rai1\",\"signal\":-50},{\"ifname\":\"ra1\",\"signal\":-90}]}}",boot,(long long)monotonic_seconds()-age);
    put_file(WIFI_SNAPSHOT,data);
}
int main(void)
{
    char demand_parent[] = DEMAND_PATH "/.parent";
    char path[256], data[256];
    struct file_map map;
    struct traffic_entry *traffic;
    struct radio_entry *radio;
    uint64_t expires;
    FILE *file;
    struct json_object *object;
    CHECK(!init_directory(demand_parent));
    CHECK(valid_name("Guest-W7"));
    CHECK(!valid_name(NULL)); CHECK(!valid_name(""));
    CHECK(!valid_name("../oops")); CHECK(!valid_name(".."));
    CHECK(!valid_name("too-long-netdev-name"));
    CHECK(!any_demand('l') && !any_demand('i') && !any_demand('r'));
    CHECK(request_demand('l',"all")==0);
    CHECK(any_demand('l') && !any_demand('i') && !any_demand('r'));
    CHECK(request_demand('i',"ra1")==0);
    CHECK(demanded('i',"ra1") && !demanded('i',"ra0"));
    put_file(TEST_ROOT "/proc/net/dev", "Inter-| header\n ra0: 1 2 0 0 0 0 0 0 3 4 0 0 0 0 0 0\n ra1: 1234567890123 42 0 0 0 0 0 0 8765432109876 99 0 0 0 0 0 0\n");
    update_requested_interfaces();
    snprintf(path,sizeof(path),DB_IF_FILE,"ra0"); CHECK(access(path,F_OK)!=0);
    snprintf(path,sizeof(path),DB_IF_FILE,"ra1");
    CHECK(!mmap_file(path,sizeof(struct traffic_entry),&map));
    traffic=(struct traffic_entry *)(map.mmap+map.size-sizeof(*traffic));
    CHECK(be64toh(traffic->rxb)==1234567890123ULL);
    CHECK(be64toh(traffic->txb)==8765432109876ULL);
    CHECK(be64toh(traffic->rxp)==42 && be64toh(traffic->txp)==99);
    umap_file(&map);
    CHECK(request_demand('r',"rai1")==0);
    CHECK(any_demand('r'));
    put_file(TEST_ROOT "/proc/sys/kernel/random/boot_id","test-boot\n");
    put_file(TEST_ROOT "/sys/class/net/rai1/flags","0x1303\n");
    snprintf(data,sizeof(data),"%ld 0",(long)getpid()); put_file(WIFI_OWNER,data);
    snprintf(path,sizeof(path),TEST_ROOT "/proc/%ld/cmdline",(long)getpid());
    put_file(path,WIFI_SAMPLER);
    put_snapshot(0,"test-boot");
    update_requested_radios();
    snprintf(path,sizeof(path),DB_RD_FILE,"rai1");
    CHECK(!mmap_file(path,sizeof(struct radio_entry),&map));
    radio=(struct radio_entry *)(map.mmap+map.size-sizeof(*radio));
    CHECK(be32toh(radio->rate)==2402000);
    CHECK((int8_t)radio->rssi==-45);
    CHECK(radio->noise==0);
    umap_file(&map);
    snprintf(path,sizeof(path),DB_RD_FILE,"ra1");CHECK(access(path,F_OK)!=0);
    object=load_wireless_snapshot();CHECK(object!=NULL);json_object_put(object);
    put_snapshot(31,"test-boot");CHECK(load_wireless_snapshot()==NULL);
    put_snapshot(-3,"test-boot");CHECK(load_wireless_snapshot()==NULL);
    put_snapshot(0,"other-boot");CHECK(load_wireless_snapshot()==NULL);
    put_file(WIFI_SNAPSHOT,"{broken");CHECK(load_wireless_snapshot()==NULL);
    put_snapshot(0,"test-boot");
    for (int i=0;i<5000;i++) { object=load_wireless_snapshot();CHECK(object!=NULL);json_object_put(object); }
    CHECK(!demand_path(path,sizeof(path),'r',"rai1"));
    expires=monotonic_seconds()-1;file=fopen(path,"w");CHECK(file!=NULL);
    CHECK(fwrite(&expires,1,sizeof(expires),file)==sizeof(expires));fclose(file);
    CHECK(!any_demand('r'));
    expires=monotonic_seconds()+4000;file=fopen(path,"w");CHECK(file!=NULL);
    CHECK(fwrite(&expires,1,sizeof(expires),file)==sizeof(expires));fclose(file);
    CHECK(!demanded('r',"rai1"));
    file=fopen(WIFI_SNAPSHOT,"w");CHECK(file!=NULL);CHECK(!ftruncate(fileno(file),1048577));fclose(file);
    CHECK(load_wireless_snapshot()==NULL);
    printf("BWC actual-source cache/demand checks passed: %u assertions, 5000 parse/free iterations\n",checks);
    return 0;
}
