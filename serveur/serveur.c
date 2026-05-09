#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "../commun/protocol.h"

/* déclarations depuis worker.c */
void *handle_agent(void *arg);

int main() {

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    printf("[SERVEUR] Démarrage sur le port %d...\n", PORT);

    /* 1. Créer la socket serveur */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("socket"); exit(1); }

    /* réutiliser le port immédiatement après arrêt */
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Bind — attacher la socket au port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; /* accepte toutes les IPs */
    server_addr.sin_port        = htons(PORT);

    if (bind(server_sock,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }

    /* 3. Listen — mise en écoute */
    if (listen(server_sock, MAX_AGENTS) < 0) {
        perror("listen"); exit(1);
    }

    printf("[SERVEUR] ✓ En attente de connexions...\n");

    /* 4. Boucle principale : accept() + création d'un thread par client */
    while (1) {

        client_sock = accept(server_sock,
                             (struct sockaddr *)&client_addr,
                             &client_len);
        if (client_sock < 0) {
            perror("[SERVEUR] accept échoué");
            continue;
        }

        printf("[SERVEUR] Nouvelle connexion : %s\n",
               inet_ntoa(client_addr.sin_addr));

        /* allouer le socket pour le thread */
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client_sock;

        /* créer un thread dédié à ce client */
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_agent, sock_ptr) != 0) {
            perror("pthread_create");
            free(sock_ptr);
            close(client_sock);
            continue;
        }

        pthread_detach(tid); /* libère automatiquement à la fin */
    }

    close(server_sock);
    return 0;
}
