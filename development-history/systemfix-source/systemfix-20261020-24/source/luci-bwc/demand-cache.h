/* LG6851F: demand-driven LuCI graphs. All state stays in RAM. */
#define DEMAND_PATH "/var/run/luci-bwc-demand"
#define WIFI_SNAPSHOT "/tmp/lg6851f-wireless.snapshot.json"
#define WIFI_WATCH "/tmp/lg6851f-wireless.watch"
#define WIFI_OWNER "/tmp/lg6851f-wireless.owner"
#define WIFI_SAMPLER "/usr/sbin/lg6851f-wireless-sampler"

static uint64_t monotonic_seconds(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_BOOTTIME, &now))
        return 0;
    return now.tv_sec;
}

static int valid_name(const char *name)
{
    const unsigned char *p = (const unsigned char *)name;
    if (!name || !*name || strlen(name) >= IFNAMSIZ ||
        !strcmp(name, ".") || !strcmp(name, ".."))
        return 0;
    for (; *p; p++)
        if (*p < 32 || *p == 127 || *p == '/' || *p == '\\')
            return 0;
    return 1;
}

static int demand_path(char *path, size_t len, char kind, const char *name)
{
    if (!valid_name(name))
        return -1;
    return snprintf(path, len, DEMAND_PATH "/%c-%s", kind, name) >= (int)len ? -1 : 0;
}

static int request_demand(char kind, const char *name)
{
    char path[128];
    uint64_t deadline;
    int fd, result;
    if (demand_path(path, sizeof(path), kind, name) ||
        (mkdir(DEMAND_PATH, 0700) && errno != EEXIST))
        return -1;
    /* Preserve the public CLI timeout with a finite upper bound. */
    deadline = monotonic_seconds() + (timeout > 0 && timeout <= 3600 ? timeout : TIMEOUT);
    fd = open(path, O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0)
        return -1;
    result = write(fd, &deadline, sizeof(deadline)) == sizeof(deadline) ? 0 : -1;
    close(fd);
    return result;
}

static int demanded(char kind, const char *name)
{
    char path[128];
    uint64_t deadline = 0, now = monotonic_seconds();
    int fd;
    if (demand_path(path, sizeof(path), kind, name))
        return 0;
    fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        return 0;
    if (read(fd, &deadline, sizeof(deadline)) != sizeof(deadline))
        deadline = 0;
    close(fd);
    return deadline >= now && deadline <= now + 3600;
}

static int any_demand(char kind)
{
    struct dirent *entry;
    DIR *dir = opendir(DEMAND_PATH);
    int active = 0;
    if (!dir)
        return 0;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == kind && entry->d_name[1] == '-' &&
            demanded(kind, entry->d_name + 2)) {
            active = 1;
            break;
        }
    }
    closedir(dir);
    return active;
}

static void keep_wireless_sampler(void)
{
    static uint64_t last_start;
    uint64_t now = monotonic_seconds();
    char line[96], cmdline[512];
    FILE *f;
    int owner = 0, fd, count;
    pid_t child;
    /* Renew the existing single sampler's lease, without a driver/RPC query. */
    f = fopen(WIFI_WATCH, "w");
    if (f) { fprintf(f, "%" PRIu64, now); fclose(f); }
    f = fopen(WIFI_OWNER, "r");
    if (f) { if (fscanf(f, "%d", &owner) != 1) owner = 0; fclose(f); }
    if (owner > 1 && !kill(owner, 0)) {
        snprintf(line, sizeof(line), "/proc/%d/cmdline", owner);
        fd = open(line, O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            count = read(fd, cmdline, sizeof(cmdline));
            close(fd);
            if (count > 0 && memmem(cmdline, count, WIFI_SAMPLER, strlen(WIFI_SAMPLER)))
                return;
        }
    }
    if (now - last_start < 10)
        return;
    last_start = now;
    child = fork();
    if (!child) {
        setsid();
        fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, 0); dup2(fd, 1); dup2(fd, 2);
            if (fd > 2) close(fd);
        }
        execl("/bin/sh", "sh", WIFI_SAMPLER, (char *)NULL);
        _exit(127);
    }
    /* The daemon ignores SIGCHLD, so short lock-contending children are reaped. */
}

static struct json_object *json_member(struct json_object *obj, const char *key)
{
    struct json_object *value = NULL;
    return obj && json_object_object_get_ex(obj, key, &value) ? value : NULL;
}

static const char *json_string_member(struct json_object *obj, const char *key)
{
    struct json_object *value = json_member(obj, key);
    return value && json_object_is_type(value, json_type_string) ? json_object_get_string(value) : NULL;
}

static struct json_object *load_wireless_snapshot(void)
{
    struct stat st;
    struct json_object *snapshot = NULL;
    struct json_tokener *parser;
    char *data, boot[80];
    const char *saved_boot;
    int fd;
    ssize_t bytes;
    FILE *f;
    double stamp, age;
    fd = open(WIFI_SNAPSHOT, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        return NULL;
    if (fstat(fd, &st) || st.st_size <= 0 || st.st_size > 1024 * 1024) {
        close(fd);
        return NULL;
    }
    data = malloc(st.st_size + 1);
    if (!data) { close(fd); return NULL; }
    bytes = read(fd, data, st.st_size);
    close(fd);
    if (bytes != st.st_size) { free(data); return NULL; }
    data[bytes] = 0;
    parser = json_tokener_new();
    if (parser) {
        snapshot = json_tokener_parse_ex(parser, data, bytes);
        if (json_tokener_get_error(parser) != json_tokener_success) {
            json_object_put(snapshot); snapshot = NULL;
        }
        json_tokener_free(parser);
    }
    free(data);
    if (!snapshot)
        return NULL;
    f = fopen("/proc/sys/kernel/random/boot_id", "r");
    if (!f || !fgets(boot, sizeof(boot), f)) {
        if (f) fclose(f);
        goto invalid;
    }
    fclose(f);
    boot[strcspn(boot, "\r\n")] = 0;
    saved_boot = json_string_member(snapshot, "boot_id");
    if (!saved_boot || strcmp(saved_boot, boot) ||
        !json_member(snapshot, "mono"))
        goto invalid;
    stamp = json_object_get_double(json_member(snapshot, "mono"));
    age = (double)monotonic_seconds() - stamp;
    if (age < -1.0 || age > 30.0)
        goto invalid;
    return snapshot;
invalid:
    json_object_put(snapshot);
    return NULL;
}

static void update_requested_radios(void)
{
    struct json_object *snapshot, *wifi, *radios, *clients, *radio, *station;
    const char *dev, *station_dev;
    uint32_t rate;
    int signal, count, sum, flags;
    char path[128];
    FILE *f;
    size_t i, j;
    keep_wireless_sampler();
    snapshot = load_wireless_snapshot();
    if (!snapshot)
        return;
    wifi = json_member(snapshot, "wifi");
    radios = json_member(wifi, "radios");
    clients = json_member(wifi, "clients");
    if (!json_object_get_boolean(json_member(wifi, "available")) ||
        json_object_get_boolean(json_member(wifi, "confirmed_off")) ||
        !json_object_is_type(radios, json_type_array) ||
        !json_object_is_type(clients, json_type_array))
        goto done;
    for (i = 0; i < json_object_array_length(radios); i++) {
        radio = json_object_array_get_idx(radios, i);
        dev = json_string_member(radio, "ifname");
        if (!dev || !demanded('r', dev))
            continue;
        snprintf(path, sizeof(path), "/sys/class/net/%s/flags", dev);
        f = fopen(path, "r");
        flags = 0;
        if (f) { if (fscanf(f, "%x", &flags) != 1) flags = 0; fclose(f); }
        if (!(flags & IFF_UP))
            continue;
        rate = json_object_get_int64(json_member(radio, "rate_kbps")) > 0 &&
            json_object_get_int64(json_member(radio, "rate_kbps")) <= UINT32_MAX ?
            (uint32_t)json_object_get_int64(json_member(radio, "rate_kbps")) : 0;
        sum = count = 0;
        for (j = 0; j < json_object_array_length(clients); j++) {
            station = json_object_array_get_idx(clients, j);
            station_dev = json_string_member(station, "ifname");
            if (!station_dev || strcmp(dev, station_dev) ||
                !json_member(station, "signal"))
                continue;
            signal = json_object_get_int(json_member(station, "signal"));
            if (signal >= -127 && signal < 0) { sum += signal; count++; }
        }
        /* Missing noise keeps the pre-existing unknown(0) wire convention;
         * never invent a noise-floor measurement or issue another driver dump. */
        update_radiostat(dev, rate, count ? (uint8_t)(sum / count) : 0, 0);
    }
done:
    json_object_put(snapshot);
}

static void update_requested_interfaces(void)
{
    FILE *f = fopen("/proc/net/dev", "r");
    char line[1024], name[IFNAMSIZ], *colon, *dev, *end;
    uint64_t fields[16];
    int i;
    if (!f)
        return;
    while (fgets(line, sizeof(line), f)) {
        colon = strrchr(line, ':');
        if (!colon)
            continue;
        *colon = 0;
        dev = line;
        while (*dev == ' ' || *dev == '\t') dev++;
        end = colon;
        while (end > dev && (end[-1] == ' ' || end[-1] == '\t')) *--end = 0;
        if (!demanded('i', dev))
            continue;
        memcpy(name, dev, strlen(dev) + 1);
        end = colon + 1;
        for (i = 0; i < 16; i++) {
            char *next;
            errno = 0;
            fields[i] = strtoull(end, &next, 10);
            if (next == end || errno) break;
            end = next;
        }
        if (i == 16)
            update_ifstat(name, fields[0], fields[1], fields[8], fields[9]);
    }
    fclose(f);
}
