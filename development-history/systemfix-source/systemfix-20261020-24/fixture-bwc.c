/*
 * luci-bwc - Very simple bandwidth collector cache for LuCI realtime graphs
 *
 *   Copyright (C) 2010 Jo-Philipp Wich <jow@openwrt.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <endian.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <net/if.h>
#include <sys/file.h>
#include <json-c/json.h>

#define STEP_COUNT	60
#define STEP_TIME	1
#define TIMEOUT		10

#define PID_PATH	TEST_ROOT "/var/run/luci-bwc.pid"

#define DB_PATH		TEST_ROOT "/var/lib/luci-bwc"
#define DB_IF_FILE	DB_PATH "/if/%s"
#define DB_RD_FILE	DB_PATH "/radio32/%s"
#define DB_CN_FILE	DB_PATH "/connections"
#define DB_LD_FILE	DB_PATH "/load"

#define LD_SCAN_PATTERN \
	"%f %f %f"


struct file_map {
	int fd;
	int size;
	char *mmap;
};

struct traffic_entry {
	uint32_t time;
	uint64_t rxb;
	uint64_t rxp;
	uint64_t txb;
	uint64_t txp;
};

struct conn_entry {
	uint32_t time;
	uint32_t udp;
	uint32_t tcp;
	uint32_t other;
};

struct load_entry {
	uint32_t time;
	uint16_t load1;
	uint16_t load5;
	uint16_t load15;
};

struct radio_entry {
	uint32_t time;
	uint32_t rate;
	uint8_t  rssi;
	uint8_t  noise;
};

static int readpid(void)
{
	int fd;
	int pid = -1;
	char buf[9] = { 0 };

	if ((fd = open(PID_PATH, O_RDONLY)) > -1)
	{
		if (read(fd, buf, sizeof(buf)))
		{
			buf[8] = 0;
			pid = atoi(buf);
		}

		close(fd);
	}

	return pid;
}

static int writepid(void)
{
	int fd;
	int wlen;
	char buf[9] = { 0 };

	if ((fd = open(PID_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0600)) > -1)
	{
		wlen = snprintf(buf, sizeof(buf), "%i", getpid());
		write(fd, buf, wlen);
		close(fd);

		return 0;
	}

	return -1;
}

static int timeout = TIMEOUT;
static int countdown = -1;

static void reset_countdown(int sig)
{
	(void)sig;
	countdown = timeout;

}


static char *progname;
static int prognamelen;

/* Wireless metrics come exclusively from the shared serial sampler. */


static int init_directory(char *path)
{
	char *p = path;

	for (p = &path[1]; *p; p++)
	{
		if (*p == '/')
		{
			*p = 0;

			if (mkdir(path, 0700) && (errno != EEXIST))
				return -1;

			*p = '/';
		}
	}

	return 0;
}

static int init_file(char *path, int esize)
{
	int i, file;
	char buf[sizeof(struct traffic_entry)] = { 0 };

	if (init_directory(path))
		return -1;

	if ((file = open(path, O_WRONLY | O_CREAT, 0600)) >= 0)
	{
		for (i = 0; i < STEP_COUNT; i++)
		{
			if (write(file, buf, esize) < 0)
				break;
		}

		close(file);

		return 0;
	}

	return -1;
}

static inline uint32_t timeof(void *entry)
{
	uint32_t stamp;
	memcpy(&stamp, entry, sizeof(stamp));
	return be32toh(stamp);
}

static int update_file(const char *path, void *entry, int esize)
{
	int rv = -1;
	int file;
	char *map;

	if ((file = open(path, O_RDWR)) >= 0)
	{
		map = mmap(NULL, esize * STEP_COUNT, PROT_READ | PROT_WRITE,
				   MAP_SHARED | MAP_LOCKED, file, 0);

		if ((map != NULL) && (map != MAP_FAILED))
		{
			if (timeof(entry) > timeof(map + esize * (STEP_COUNT-1)))
			{
				memmove(map, map + esize, esize * (STEP_COUNT-1));
				memcpy(map + esize * (STEP_COUNT-1), entry, esize);
			}

			munmap(map, esize * STEP_COUNT);

			rv = 0;
		}

		close(file);
	}

	return rv;
}

static int mmap_file(const char *path, int esize, struct file_map *m)
{
	m->fd   = -1;
	m->size = -1;
	m->mmap = NULL;

	if ((m->fd = open(path, O_RDONLY)) >= 0)
	{
		m->size = STEP_COUNT * esize;
		m->mmap = mmap(NULL, m->size, PROT_READ,
					   MAP_SHARED | MAP_LOCKED, m->fd, 0);

		if ((m->mmap != NULL) && (m->mmap != MAP_FAILED))
			return 0;
	}

	return -1;
}

static void umap_file(struct file_map *m)
{
	if ((m->mmap != NULL) && (m->mmap != MAP_FAILED))
		munmap(m->mmap, m->size);

	if (m->fd > -1)
		close(m->fd);
}

static int update_ifstat(
	const char *ifname, uint64_t rxb, uint64_t rxp, uint64_t txb, uint64_t txp
) {
	char path[1024];

	struct stat s;
	struct traffic_entry e = {0};

	snprintf(path, sizeof(path), DB_IF_FILE, ifname);

	if (stat(path, &s))
	{
		if (init_file(path, sizeof(struct traffic_entry)))
		{
			fprintf(stderr, "Failed to init %s: %s\n",
					path, strerror(errno));

			return -1;
		}
	}

	e.time = htobe32(time(NULL));
	e.rxb  = htobe64(rxb);
	e.rxp  = htobe64(rxp);
	e.txb  = htobe64(txb);
	e.txp  = htobe64(txp);

	return update_file(path, &e, sizeof(struct traffic_entry));
}

static int update_radiostat(
	const char *ifname, uint32_t rate, uint8_t rssi, uint8_t noise
) {
	char path[1024];

	struct stat s;
	struct radio_entry e = {0};

	snprintf(path, sizeof(path), DB_RD_FILE, ifname);

	if (stat(path, &s))
	{
		if (init_file(path, sizeof(struct radio_entry)))
		{
			fprintf(stderr, "Failed to init %s: %s\n",
					path, strerror(errno));

			return -1;
		}
	}

	e.time  = htobe32(time(NULL));
	e.rate  = htobe32(rate);
	e.rssi  = rssi;
	e.noise = noise;

	return update_file(path, &e, sizeof(struct radio_entry));
}

static int update_cnstat(uint32_t udp, uint32_t tcp, uint32_t other)
{
	char path[1024];

	struct stat s;
	struct conn_entry e = {0};

	snprintf(path, sizeof(path), DB_CN_FILE);

	if (stat(path, &s))
	{
		if (init_file(path, sizeof(struct conn_entry)))
		{
			fprintf(stderr, "Failed to init %s: %s\n",
					path, strerror(errno));

			return -1;
		}
	}

	e.time  = htobe32(time(NULL));
	e.udp   = htobe32(udp);
	e.tcp   = htobe32(tcp);
	e.other = htobe32(other);

	return update_file(path, &e, sizeof(struct conn_entry));
}

static int update_ldstat(uint16_t load1, uint16_t load5, uint16_t load15)
{
	char path[1024];

	struct stat s;
	struct load_entry e = {0};

	snprintf(path, sizeof(path), DB_LD_FILE);

	if (stat(path, &s))
	{
		if (init_file(path, sizeof(struct load_entry)))
		{
			fprintf(stderr, "Failed to init %s: %s\n",
					path, strerror(errno));

			return -1;
		}
	}

	e.time   = htobe32(time(NULL));
	e.load1  = htobe16(load1);
	e.load5  = htobe16(load5);
	e.load15 = htobe16(load15);

	return update_file(path, &e, sizeof(struct load_entry));
}

/* LG6851F: demand-driven LuCI graphs. All state stays in RAM. */
#define DEMAND_PATH TEST_ROOT "/var/run/luci-bwc-demand"
#define WIFI_SNAPSHOT TEST_ROOT "/tmp/lg6851f-wireless.snapshot.json"
#define WIFI_WATCH TEST_ROOT "/tmp/lg6851f-wireless.watch"
#define WIFI_OWNER TEST_ROOT "/tmp/lg6851f-wireless.owner"
#define WIFI_SAMPLER TEST_ROOT "/usr/sbin/lg6851f-wireless-sampler"

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
        snprintf(line, sizeof(line), TEST_ROOT "/proc/%d/cmdline", owner);
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
    f = fopen(TEST_ROOT "/proc/sys/kernel/random/boot_id", "r");
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
        snprintf(path, sizeof(path), TEST_ROOT "/sys/class/net/%s/flags", dev);
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
    FILE *f = fopen(TEST_ROOT "/proc/net/dev", "r");
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


static int run_daemon(void)
{
	FILE *info;
	uint32_t udp, tcp, other;
	float lf1, lf5, lf15;
	char line[1024];
	char buf[32];
	struct sigaction sa;

	struct stat s;
	char *ipc = NULL;
	char *ipc_command = NULL;
	if(! stat(TEST_ROOT "/proc/net/nf_conntrack", &s))
		ipc = TEST_ROOT "/proc/net/nf_conntrack";
	else if(! stat(TEST_ROOT "/proc/net/ip_conntrack", &s))
		ipc = TEST_ROOT "/proc/net/ip_conntrack";
	else if(! stat(TEST_ROOT "/usr/sbin/conntrack" , &s))
		ipc_command = TEST_ROOT "/usr/sbin/conntrack -L -o extended";

	switch (fork())
	{
		case -1:
			perror("fork()");
			return -1;

		case 0:
			if (chdir("/") < 0)
			{
				perror("chdir()");
				exit(1);
			}

			close(0);
			close(1);
			close(2);
			break;

		default:
			return 0;
	}

	/* setup USR1 signal handler to reset timer */
	sa.sa_handler = reset_countdown;
	sa.sa_flags   = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1, &sa, NULL);


	/* Serialize concurrent first graph requests; keep the lock until exit. */
	int daemon_lock = open(TEST_ROOT "/var/run/luci-bwc.lock", O_CREAT | O_RDWR | O_CLOEXEC, 0600);
	if (daemon_lock < 0 || flock(daemon_lock, LOCK_EX | LOCK_NB))
		return 0;
	/* Only the lock owner publishes its PID. */
	if (writepid())
		return 1;
	signal(SIGCHLD, SIG_IGN);

	/* go */
	for (reset_countdown(0); countdown >= 0; countdown--)
	{
		/* alter progname for ps, top */
		memset(progname, 0, prognamelen);
		snprintf(progname, prognamelen, "luci-bwc %d", countdown);

		/* One netdev snapshot; no repeated per-field sysfs statistics reads. */
		if (any_demand('i'))
			update_requested_interfaces();
		if (any_demand('r'))
			update_requested_radios();

		if (demanded('c', "all") &&
		    ((ipc && ((info = fopen(ipc, "r")) != NULL)) ||
		     (ipc_command && ((info = popen(ipc_command, "r")) != NULL))))
		{
			udp   = 0;
			tcp   = 0;
			other = 0;

			while (fgets(line, sizeof(line), info))
			{
				if (strstr(line, "TIME_WAIT"))
					continue;

				if ((strstr(line, "src=127.0.0.1 ") && strstr(line, "dst=127.0.0.1 "))
				|| (strstr(line, "src=::1 ") && strstr(line, "dst=::1 ")))
					continue;

				if (sscanf(line, "%*s %*d %s", buf) || sscanf(line, "%s %*d", buf))
				{
					if (!strcmp(buf, "tcp"))
						tcp++;
					else if (!strcmp(buf, "udp"))
						udp++;
					else
						other++;
				}
			}

			update_cnstat(udp, tcp, other);

			if (ipc)
				fclose(info);
			else
				pclose(info);
		}

		if (demanded('l', "all") && (info = fopen(TEST_ROOT "/proc/loadavg", "r")) != NULL)
		{
			if (fscanf(info, LD_SCAN_PATTERN, &lf1, &lf5, &lf15))
			{
				update_ldstat((uint16_t)(lf1  * 100),
							  (uint16_t)(lf5  * 100),
							  (uint16_t)(lf15 * 100));
			}

			fclose(info);
		}

		sleep(STEP_TIME);
	}

	unlink(PID_PATH);


	return 0;
}

static void check_daemon(void)
{
	int pid;

	if ((pid = readpid()) < 0 || kill(pid, 0) < 0)
	{
		/* daemon ping failed, try to start it up */
		if (run_daemon())
		{
			fprintf(stderr,
				"Failed to ping daemon and unable to start it up: %s\n",
				strerror(errno));

			exit(1);
		}
	}
	else if (kill(pid, SIGUSR1))
	{
		fprintf(stderr, "Failed to send signal: %s\n", strerror(errno));
		exit(2);
	}
}

static int run_dump_ifname(const char *ifname)
{
	int i;
	char path[1024];
	struct file_map m;
	struct traffic_entry *e;

	if (request_demand('i', ifname))
		return 1;
	check_daemon();
	snprintf(path, sizeof(path), DB_IF_FILE, ifname);

	if (mmap_file(path, sizeof(struct traffic_entry), &m))
	{
		fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
		return 1;
	}

	for (i = 0; i < m.size; i += sizeof(struct traffic_entry))
	{
		e = (struct traffic_entry *) &m.mmap[i];

		if (!e->time)
			continue;

		printf("[ %" PRIu32 ", %" PRIu64 ", %" PRIu64
			   ", %" PRIu64 ", %" PRIu64 " ]%s\n",
			be32toh(e->time),
			be64toh(e->rxb), be64toh(e->rxp),
			be64toh(e->txb), be64toh(e->txp),
			((i + sizeof(struct traffic_entry)) < (size_t)m.size) ? "," : "");
	}

	umap_file(&m);

	return 0;
}

static int run_dump_radio(const char *ifname)
{
	int i;
	char path[1024];
	struct file_map m;
	struct radio_entry *e;

	if (request_demand('r', ifname))
		return 1;
	check_daemon();
	snprintf(path, sizeof(path), DB_RD_FILE, ifname);

	if (mmap_file(path, sizeof(struct radio_entry), &m))
	{
		fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
		return 1;
	}

	for (i = 0; i < m.size; i += sizeof(struct radio_entry))
	{
		e = (struct radio_entry *) &m.mmap[i];

		if (!e->time)
			continue;

		printf("[ %" PRIu32 ", %" PRIu32 ", %" PRIu8 ", %" PRIu8 " ]%s\n",
			be32toh(e->time),
			be32toh(e->rate), e->rssi, e->noise,
			((i + sizeof(struct radio_entry)) < (size_t)m.size) ? "," : "");
	}

	umap_file(&m);

	return 0;
}

static int run_dump_conns(void)
{
	int i;
	char path[1024];
	struct file_map m;
	struct conn_entry *e;

	if (request_demand('c', "all"))
		return 1;
	check_daemon();
	snprintf(path, sizeof(path), DB_CN_FILE);

	if (mmap_file(path, sizeof(struct conn_entry), &m))
	{
		fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
		return 1;
	}

	for (i = 0; i < m.size; i += sizeof(struct conn_entry))
	{
		e = (struct conn_entry *) &m.mmap[i];

		if (!e->time)
			continue;

		printf("[ %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 " ]%s\n",
			be32toh(e->time), be32toh(e->udp),
			be32toh(e->tcp), be32toh(e->other),
			((i + sizeof(struct conn_entry)) < (size_t)m.size) ? "," : "");
	}

	umap_file(&m);

	return 0;
}

static int run_dump_load(void)
{
	int i;
	char path[1024];
	struct file_map m;
	struct load_entry *e;

	if (request_demand('l', "all"))
		return 1;
	check_daemon();
	snprintf(path, sizeof(path), DB_LD_FILE);

	if (mmap_file(path, sizeof(struct load_entry), &m))
	{
		fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
		return 1;
	}

	for (i = 0; i < m.size; i += sizeof(struct load_entry))
	{
		e = (struct load_entry *) &m.mmap[i];

		if (!e->time)
			continue;

		printf("[ %" PRIu32 ", %" PRIu16 ", %" PRIu16 ", %" PRIu16 " ]%s\n",
			be32toh(e->time),
			be16toh(e->load1), be16toh(e->load5), be16toh(e->load15),
			((i + sizeof(struct load_entry)) < (size_t)m.size) ? "," : "");
	}

	umap_file(&m);

	return 0;
}


int main(int argc, char *argv[])
{
	int opt;

	progname = argv[0];
	prognamelen = -1;

	for (opt = 0; opt < argc; opt++)
		prognamelen += 1 + strlen(argv[opt]);

	while ((opt = getopt(argc, argv, "t:i:r:cl")) > -1)
	{
		switch (opt)
		{
			case 't':
				timeout = atoi(optarg);
				break;

			case 'i':
				if (optarg)
					return run_dump_ifname(optarg);
				break;

			case 'r':
				if (optarg)
					return run_dump_radio(optarg);
				break;

			case 'c':
				return run_dump_conns();

			case 'l':
				return run_dump_load();

			default:
				break;
		}
	}

	fprintf(stderr,
		"Usage:\n"
		"	%s [-t timeout] -i ifname\n"
		"	%s [-t timeout] -r radiodev\n"
		"	%s [-t timeout] -c\n"
		"	%s [-t timeout] -l\n",
			argv[0], argv[0], argv[0], argv[0]
	);

	return 1;
}
