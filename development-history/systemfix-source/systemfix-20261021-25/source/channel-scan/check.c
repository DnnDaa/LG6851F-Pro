#define SCAN_CHECK
#include "scan.c"
#include <assert.h>
// Only the host harness supplies ABI constants. Target links the installed library.
#ifdef HOST_CHECK
const char *const IWINFO_OPMODE_NAMES[IWINFO_OPMODE_COUNT] = {"Unknown","Master"};
const char *const ht_secondary_offset[4] = {"no secondary","above","reserved","below"};
const uint16_t ht_chan_width[2] = {20,2040};
const uint16_t vht_chan_width[4] = {40,80,160,8080};
const uint16_t eht_chan_width[5] = {20,40,80,160,320};
uint32_t iwinfo_band2ghz(uint8_t band) {return band==IWINFO_BAND_5?5:2;}
#endif
int main(void)
{
    assert(sizeof(struct iwinfo_scanlist_entry)==76);
    assert(offsetof(struct iwinfo_ops,scanlist)==224);
    assert(offsetof(struct iwinfo_scanlist_entry,ssid)==6 &&
           offsetof(struct iwinfo_scanlist_entry,mode)==40 &&
           offsetof(struct iwinfo_scanlist_entry,band)==44 &&
           offsetof(struct iwinfo_scanlist_entry,channel)==45 &&
           offsetof(struct iwinfo_scanlist_entry,mhz)==48 &&
           offsetof(struct iwinfo_scanlist_entry,signal)==52 &&
           offsetof(struct iwinfo_scanlist_entry,quality)==53 &&
           offsetof(struct iwinfo_scanlist_entry,quality_max)==54 &&
           offsetof(struct iwinfo_scanlist_entry,ht_chan_info)==64);
    assert(offsetof(struct iwinfo_scanlist_entry,vht_chan_info)==67 &&
           offsetof(struct iwinfo_scanlist_entry,he_chan_info)==70 &&
           offsetof(struct iwinfo_scanlist_entry,eht_chan_info)==73);
    struct iwinfo_scanlist_entry e = { .mac={0x50,0xcf,0x14,0,0,1}, .mode=IWINFO_OPMODE_MASTER,
       .band=IWINFO_BAND_5,.channel=36,.mhz=5180,.signal=211,.quality=61,.quality_max=70,
       .vht_chan_info={.chan_width=2,.center_chan_1=50},.eht_chan_info={.chan_width=3,.center_chan_1=50} };
    memset(e.ssid,'A',sizeof(e.ssid)); // Deliberately unterminated driver string.
    json_object *o=entry(&e),*v=NULL,*n=NULL;
    assert(json_object_object_get_ex(o,"signal",&v)&&json_object_get_int(v)==-45);
    assert(json_object_object_get_ex(o,"ssid",&v)&&json_object_get_string_len(v)==32);
    assert(json_object_object_get_ex(o,"eht_operation",&v));
    assert(json_object_object_get_ex(v,"channel_width",&n)&&json_object_get_int(n)==160);
    json_object_put(o);
    for(unsigned i=0;i<256;i++) {
        e.mode=IWINFO_OPMODE_COUNT+1;e.ht_chan_info.primary_chan=36;
        e.ht_chan_info.secondary_chan_off=i;e.ht_chan_info.chan_width=i;
        e.vht_chan_info.chan_width=i;e.he_chan_info.chan_width=i;e.eht_chan_info.chan_width=i;
        o=entry(&e);assert(o);json_object_put(o);
    }
    for(unsigned i=0;i<5000;i++){o=entry(&e);assert(o);json_object_put(o);}
    puts("Channel result formatter/installed ABI passed: 5264 assertions; 5000 allocation/free iterations");
    return 0;
}
