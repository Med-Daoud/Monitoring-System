CC      = gcc
FLAGS   = -Wall -Wextra -pthread

all: agent serveur

agent:
	$(CC) $(FLAGS) agent/agent.c agent/collector.c -o agent/agent
	@echo "✅ Agent compilé"

serveur:
	$(CC) $(FLAGS) serveur/serveur.c serveur/worker.c -o serveur/serveur
	@echo "✅ Serveur compilé"

clean:
	rm -f agent/agent serveur/serveur agent/test_collector
	@echo "🧹 Nettoyage terminé"

re: clean all

.PHONY: all agent serveur clean re

⚠️  IMPORTANT : Les lignes de commande dans le Makefile doivent
    être indentées avec une TABULATION (touche Tab), pas des espaces.
