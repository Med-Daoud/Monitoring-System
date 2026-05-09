#ifndef PROTOCOL_H
#define PROTOCOL_H

/* Paramètres réseau */
#define PORT             8080
#define MAX_AGENTS       50

/* Tailles des champs */
#define HOSTNAME_LEN     64
#define IFACE_LEN        16
#define IP_LEN           16
#define MAX_USERS        8
#define USER_LEN         32
#define MAX_CONNECTIONS  10
#define CONN_LEN         64

/* Infos interface réseau */
typedef struct {
    char  name[IFACE_LEN];   /* nom de l'interface : eth0, wlan0 */
    char  ip[IP_LEN];        /* adresse IP de la machine         */
    long  bytes_sent;        /* octets envoyés depuis démarrage  */
    long  bytes_recv;        /* octets reçus depuis démarrage    */
} NetInfo;

/* Structure principale envoyée au serveur */
typedef struct {

    /* Identité */
    char    hostname[HOSTNAME_LEN];

    /* Performances */
    float   cpu_usage;
    float   ram_used_mb;
    float   ram_total_mb;
    int     process_count;
    float   cpu_temp;
    long    uptime_seconds;

    /* Réseau */
    NetInfo net;

    /* Sécurité */
    int     ssh_failed;
    char    users[MAX_USERS][USER_LEN];
    int     user_count;
    char    connections[MAX_CONNECTIONS][CONN_LEN];
    int     connection_count;

} RemoteData;

#endif
