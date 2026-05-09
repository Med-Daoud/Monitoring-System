#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include "../commun/protocol.h"

/* ─────────────────────────────────────────
   Données partagées entre tous les threads
   ───────────────────────────────────────── */
RemoteData      agents[MAX_AGENTS];
int             agent_count = 0;
pthread_mutex_t data_mutex  = PTHREAD_MUTEX_INITIALIZER;

/* ─────────────────────────────────────────
   Affichage du dashboard terminal
   ───────────────────────────────────────── */
void display_dashboard() {

    printf("\033[2J\033[H"); /* effacer le terminal */
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║        MONITORING DASHBOARD - %d machine(s)              ║\n",
           agent_count);

    for (int i = 0; i < agent_count; i++) {

        /* convertir uptime en jours/heures/minutes */
        long u      = agents[i].uptime_seconds;
        int jours   = u / 86400;
        int heures  = (u % 86400) / 3600;
        int minutes = (u % 3600) / 60;

        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║  SYSTEME : %-45s ║\n", agents[i].hostname);
        printf("╠══════════════════════════════════════════════════════════╣\n");

        /* Performances */
        printf("║  [PERF]                                                  ║\n");
        printf("║  CPU      : %-6.2f%%                                    ║\n",
               agents[i].cpu_usage);

        if (agents[i].cpu_temp > 0)
            printf("║  Temp     : %-5.1f C                                    ║\n",
                   agents[i].cpu_temp);
        else
            printf("║  Temp     : N/A (VM)                                   ║\n");

        printf("║  RAM      : %-6.1f / %-6.1f MB                         ║\n",
               agents[i].ram_used_mb, agents[i].ram_total_mb);
        printf("║  PID      : %-4d                                       ║\n",
               agents[i].process_count);
        printf("║  Uptime   : %dj %dh %dm                                 ║\n",
               jours, heures, minutes);

        /* Réseau */
        printf("║  [RESEAU]                                                ║\n");
        printf("║  Interface: %-6s  IP: %-15s              ║\n",
               agents[i].net.name, agents[i].net.ip);
        printf("║  Envoye   : %-10ld octets                          ║\n",
               agents[i].net.bytes_sent);
        printf("║  Recu     : %-10ld octets                          ║\n",
               agents[i].net.bytes_recv);

        /* Sécurité */
        printf("║  [SECURITE]                                              ║\n");

        if (agents[i].ssh_failed > 0)
            printf("║  SSH fail : %-4d  !! ALERTE BRUTE FORCE !!            ║\n",
                   agents[i].ssh_failed);
        else
            printf("║  SSH fail : 0    OK                                    ║\n");

        if (agents[i].user_count == 0) {
            printf("║  Users    : aucun                                      ║\n");
        } else {
            for (int u = 0; u < agents[i].user_count; u++)
                printf("║  User     : %-44s ║\n", agents[i].users[u]);
        }

        printf("║  Connexions actives : %-4d                             ║\n",
               agents[i].connection_count);
        for (int c = 0; c < agents[i].connection_count; c++)
            printf("║    > %-50s ║\n", agents[i].connections[c]);
    }

    printf("╚══════════════════════════════════════════════════════════╝\n");
}

/* ─────────────────────────────────────────
   Trouver ou créer un slot pour cet agent
   ───────────────────────────────────────── */
int find_or_create_slot(const char *hostname) {

    for (int i = 0; i < agent_count; i++) {
        if (strcmp(agents[i].hostname, hostname) == 0)
            return i; /* machine déjà connue */
    }
    if (agent_count < MAX_AGENTS)
        return agent_count++; /* nouveau slot */
    return -1; /* tableau plein */
}

/* ─────────────────────────────────────────
   Fonction exécutée par chaque thread
   Un thread par agent connecté
   ───────────────────────────────────────── */
void *handle_agent(void *arg) {

    int client_sock = *(int *)arg;
    free(arg);

    RemoteData data;
    ssize_t received;

    printf("[WORKER] Nouveau thread démarre\n");

    while (1) {

        /* recevoir les données de l'agent */
        received = recv(client_sock, &data, sizeof(RemoteData), 0);

        if (received <= 0) {
            printf("[WORKER] Agent déconnecté\n");
            break;
        }

        /* ── ZONE CRITIQUE : protégée par mutex ── */
        pthread_mutex_lock(&data_mutex);

        int slot = find_or_create_slot(data.hostname);
        if (slot >= 0) {
            agents[slot] = data;
            display_dashboard();
        }

        pthread_mutex_unlock(&data_mutex);
        /* ── FIN ZONE CRITIQUE ── */
    }

    close(client_sock);
    pthread_exit(NULL);
}
