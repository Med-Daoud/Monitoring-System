#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "../commun/protocol.h"

/* ════════════════════════════════════════
   Lecture du CPU depuis /proc/stat
   On lit deux fois avec 1 sec d'écart
   ════════════════════════════════════════ */
float get_cpu_usage() {

    long user1, nice1, sys1, idle1;
    long user2, nice2, sys2, idle2;

    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1.0;
    fscanf(f, "cpu %ld %ld %ld %ld", &user1, &nice1, &sys1, &idle1);
    fclose(f);

    sleep(1);

    f = fopen("/proc/stat", "r");
    if (!f) return -1.0;
    fscanf(f, "cpu %ld %ld %ld %ld", &user2, &nice2, &sys2, &idle2);
    fclose(f);

    long total1 = user1 + nice1 + sys1 + idle1;
    long total2 = user2 + nice2 + sys2 + idle2;

    return (float)((total2 - total1) - (idle2 - idle1))
           / (total2 - total1) * 100.0;
}

/* ════════════════════════════════════════
   Lecture de la RAM depuis /proc/meminfo
   ════════════════════════════════════════ */
void get_ram_usage(float *used_mb, float *total_mb) {

    long total = 0, free = 0, buffers = 0, cached = 0;
    char line[128];

    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) { *used_mb = *total_mb = -1; return; }

    while (fgets(line, sizeof(line), f)) {
        if      (strncmp(line, "MemTotal:",  9) == 0)
            sscanf(line, "MemTotal: %ld",  &total);
        else if (strncmp(line, "MemFree:",   8) == 0)
            sscanf(line, "MemFree: %ld",   &free);
        else if (strncmp(line, "Buffers:",   8) == 0)
            sscanf(line, "Buffers: %ld",   &buffers);
        else if (strncmp(line, "Cached:",    7) == 0)
            sscanf(line, "Cached: %ld",    &cached);
    }
    fclose(f);

    *total_mb = total / 1024.0;
    *used_mb  = (total - free - buffers - cached) / 1024.0;
}

/* ════════════════════════════════════════
   Température CPU depuis /sys
   Retourne -1 si non disponible (VM)
   ════════════════════════════════════════ */
float get_cpu_temp() {

    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return -1.0;

    int temp_raw;
    fscanf(f, "%d", &temp_raw);
    fclose(f);

    return temp_raw / 1000.0;
}

/* ════════════════════════════════════════
   Uptime depuis /proc/uptime
   ════════════════════════════════════════ */
long get_uptime() {

    FILE *f = fopen("/proc/uptime", "r");
    if (!f) return -1;

    float uptime;
    fscanf(f, "%f", &uptime);
    fclose(f);

    return (long)uptime;
}

/* ════════════════════════════════════════
   Infos réseau depuis /proc/net/dev
   ════════════════════════════════════════ */
void get_net_info(NetInfo *net) {

    char line[256];
    char iface[IFACE_LEN];
    long recv, sent;

    strncpy(net->name, "N/A", IFACE_LEN);
    strncpy(net->ip,   "N/A", IP_LEN);
    net->bytes_sent = 0;
    net->bytes_recv = 0;

    FILE *f = fopen("/proc/net/dev", "r");
    if (!f) return;

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f)) {

        if (sscanf(line,
            " %15[^:]: %ld %*d %*d %*d %*d %*d %*d %*d %ld",
            iface, &recv, &sent) != 3) continue;

        if (strcmp(iface, "lo") == 0) continue;

        strncpy(net->name, iface, IFACE_LEN);
        net->bytes_recv = recv;
        net->bytes_sent = sent;

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock >= 0) {
            struct ifreq ifr;
            strncpy(ifr.ifr_name, iface, IFNAMSIZ);
            if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
                strncpy(net->ip,
                    inet_ntoa(((struct sockaddr_in *)
                        &ifr.ifr_addr)->sin_addr), IP_LEN);
            }
            close(sock);
        }
        break;
    }
    fclose(f);
}

/* ════════════════════════════════════════
   Tentatives SSH échouées
   depuis /var/log/auth.log
   ════════════════════════════════════════ */
int get_ssh_failed() {

    FILE *f = popen(
        "grep 'Failed password' /var/log/auth.log 2>/dev/null | wc -l",
        "r");
    if (!f) return 0;

    int count = 0;
    fscanf(f, "%d", &count);
    pclose(f);

    return count;
}

/* ════════════════════════════════════════
   Utilisateurs connectés via who
   ════════════════════════════════════════ */
void get_logged_users(char users[][USER_LEN], int *count) {

    *count = 0;

    FILE *f = popen("who | awk '{print $1}' | sort -u", "r");
    if (!f) return;

    char line[USER_LEN];
    while (fgets(line, sizeof(line), f) && *count < MAX_USERS) {
        line[strcspn(line, "\n")] = 0;
        strncpy(users[*count], line, USER_LEN);
        (*count)++;
    }
    pclose(f);
}

/* ════════════════════════════════════════
   Connexions TCP actives via ss
   ════════════════════════════════════════ */
void get_active_connections(char conns[][CONN_LEN], int *count) {

    *count = 0;

    FILE *f = popen(
        "ss -tn state established 2>/dev/null | "
        "awk 'NR>1 {print $4 \" -> \" $5}' | head -10",
        "r");
    if (!f) return;

    char line[CONN_LEN];
    while (fgets(line, sizeof(line), f) && *count < MAX_CONNECTIONS) {
        line[strcspn(line, "\n")] = 0;
        strncpy(conns[*count], line, CONN_LEN);
        (*count)++;
    }
    pclose(f);
}

/* ════════════════════════════════════════
   Nombre de processus actifs
   ════════════════════════════════════════ */
int get_process_count() {

    int count = 0;
    FILE *f = popen(
        "ls /proc | grep -E '^[0-9]+$' | wc -l", "r");
    if (!f) return -1;

    fscanf(f, "%d", &count);
    pclose(f);

    return count;
}

/* ════════════════════════════════════════
   Fonction principale : remplit toute
   la structure RemoteData
   ════════════════════════════════════════ */
void collect_data(RemoteData *data) {

    gethostname(data->hostname, HOSTNAME_LEN);

    data->cpu_usage      = get_cpu_usage();
    data->cpu_temp       = get_cpu_temp();
    data->uptime_seconds = get_uptime();
    data->process_count  = get_process_count();

    get_ram_usage(&data->ram_used_mb, &data->ram_total_mb);
    get_net_info(&data->net);

    data->ssh_failed = get_ssh_failed();
    get_logged_users(data->users,            &data->user_count);
    get_active_connections(data->connections, &data->connection_count);
}
