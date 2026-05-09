#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../commun/protocol.h"

/* déclaration de la fonction définie dans collector.c */
void collect_data(RemoteData *data);

int main(int argc, char *argv[]) {

    /* vérifier que l'IP du serveur est passée en argument */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <IP_SERVEUR>\n", argv[0]);
        fprintf(stderr, "Exemple: %s 192.168.182.128\n", argv[0]);
        exit(1);
    }

    const char         *server_ip = argv[1];
    int                 sock;
    struct sockaddr_in  server_addr;
    RemoteData          data;

    printf("[AGENT] Demarrage...\n");
    printf("[AGENT] Cible serveur : %s:%d\n", server_ip, PORT);

    /* Boucle principale avec reconnexion automatique */
    while (1) {

        /* 1. Créer la socket TCP */
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("[AGENT] Erreur creation socket");
            sleep(3);
            continue;
        }

        /* 2. Configurer l'adresse du serveur */
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port   = htons(PORT);

        if (inet_pton(AF_INET, server_ip,
                      &server_addr.sin_addr) <= 0) {
            perror("[AGENT] Adresse IP invalide");
            close(sock);
            sleep(3);
            continue;
        }

        /* 3. Se connecter au serveur */
        if (connect(sock,
                    (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) < 0) {
            perror("[AGENT] Connexion echouee, on reessaie...");
            close(sock);
            sleep(3);
            continue;
        }

        printf("[AGENT] Connecte au serveur !\n");

        /* 4. Boucle d'envoi toutes les 3 secondes */
        while (1) {

            collect_data(&data);

            ssize_t sent = send(sock, &data, sizeof(RemoteData), 0);

            if (sent <= 0) {
                printf("[AGENT] Connexion perdue, reconnexion...\n");
                break;
            }

            printf("[AGENT] Donnees envoyees -> CPU: %.2f%%"
                   " | RAM: %.1f/%.1fMB | SSH fail: %d\n",
                   data.cpu_usage,
                   data.ram_used_mb,
                   data.ram_total_mb,
                   data.ssh_failed);

            sleep(3);
        }

        close(sock);
    }

    return 0;
}
   
