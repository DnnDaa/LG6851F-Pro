// SPDX-License-Identifier: GPL-2.0-only
// Separate, bounded libiwinfo client: never block rpcd's event loop.
#include <iwinfo.h>
#include <json-c/json.h>
#include <stddef.h>

static void integer(json_object *o, const char *key, int n)
{ json_object_object_add(o, key, json_object_new_int(n)); }
static void string(json_object *o, const char *key, const char *s)
{ json_object_object_add(o, key, json_object_new_string(s)); }

static json_object *operation(const struct iwinfo_scanlist_vht_chan_entry *v,
                             const uint16_t *widths, unsigned count)
{
    if (!v->center_chan_1 || v->chan_width >= count) return NULL;
    json_object *o = json_object_new_object();
    integer(o, "channel_width", widths[v->chan_width]);
    integer(o, "center_freq_1", v->center_chan_1);
    integer(o, "center_freq_2", v->center_chan_2);
    return o;
}

static json_object *entry(const struct iwinfo_scanlist_entry *e)
{
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             e->mac[0], e->mac[1], e->mac[2], e->mac[3], e->mac[4], e->mac[5]);
    json_object *o = json_object_new_object(), *v;
    string(o, "bssid", mac);
    // Bound strings and all driver-provided enum indexes before dereferencing.
    json_object_object_add(o, "ssid", json_object_new_string_len(e->ssid,
        strnlen(e->ssid, IWINFO_ESSID_MAX_SIZE)));
    string(o, "mode", e->mode >= 0 && e->mode < IWINFO_OPMODE_COUNT ?
           IWINFO_OPMODE_NAMES[e->mode] : "Unknown");
    integer(o, "band", iwinfo_band2ghz(e->band));
    integer(o, "channel", e->channel);
    integer(o, "mhz", e->mhz);
    integer(o, "signal", (int)e->signal - 256);
    integer(o, "quality", e->quality);
    integer(o, "quality_max", e->quality_max);
    if (e->ht_chan_info.primary_chan && e->ht_chan_info.secondary_chan_off < 4 &&
        e->ht_chan_info.chan_width < 2) {
        v = json_object_new_object();
        integer(v, "primary_channel", e->ht_chan_info.primary_chan);
        string(v, "secondary_channel_offset", ht_secondary_offset[e->ht_chan_info.secondary_chan_off]);
        integer(v, "channel_width", ht_chan_width[e->ht_chan_info.chan_width]);
        json_object_object_add(o, "ht_operation", v);
    }
    v = operation(&e->vht_chan_info, vht_chan_width, 4);
    if (v) json_object_object_add(o, "vht_operation", v);
    v = operation(&e->he_chan_info, eht_chan_width, 5);
    if (v) json_object_object_add(o, "he_operation", v);
    v = operation(&e->eht_chan_info, eht_chan_width, 5);
    if (v) json_object_object_add(o, "eht_operation", v);
    return o;
}

#ifndef SCAN_CHECK
int main(int argc, char **argv)
{
    if (argc != 2 || !argv[1][0] || strlen(argv[1]) >= IFNAMSIZ) return 2;
    for (const char *p = argv[1]; *p; p++)
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '.' && *p != '-') return 2;
    unsigned flags = 0;
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/net/%s/flags", argv[1]);
    FILE *f = fopen(path, "r");
    if (!f) return 3;
    int got = fscanf(f, "%x", &flags); fclose(f);
    if (got != 1 || !(flags & IFF_UP)) return 3;
    const struct iwinfo_ops *iw = iwinfo_backend(argv[1]);
    int mode = 0, len = 0;
    // Only existing UP APs: avoid libiwinfo's temporary-interface/down-AP fallback.
    if (!iw || !iw->name || strcmp(iw->name, "nl80211") || !iw->mode ||
        iw->mode(argv[1], &mode) || mode != IWINFO_OPMODE_MASTER) {
        iwinfo_finish(); return 4;
    }
    union { max_align_t alignment; char bytes[IWINFO_BUFSIZE]; } buf = {0};
    if (!iw->scanlist || iw->scanlist(argv[1], buf.bytes, &len) || len < 0 ||
        (size_t)len > sizeof(buf.bytes) || len % sizeof(struct iwinfo_scanlist_entry)) {
        iwinfo_finish(); return 5;
    }
    json_object *root = json_object_new_object(), *rows = json_object_new_array();
    for (size_t at = 0; at < (size_t)len; at += sizeof(struct iwinfo_scanlist_entry)) {
        struct iwinfo_scanlist_entry e;
        memcpy(&e, buf.bytes + at, sizeof(e));
        json_object_array_add(rows, entry(&e));
    }
    json_object_object_add(root, "results", rows);
    puts(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN));
    json_object_put(root); iwinfo_finish(); return 0;
}
#endif
