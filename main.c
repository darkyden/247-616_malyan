//main:
// 2018-10-11, Yves Roy, creation (247-637 S-0003)

// INCLUSIONS
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "main.h"
#include "piloteSerieUSB.h"
#include "interfaceTouche.h"
#include "interfaceMalyan.h"

// Definitions privees
#define MAIN_LONGUEUR_MAXIMALE 99

// Declarations de fonctions privees:
int main_initialise(void);
void main_termine(void);

// Definitions de fonctions privees:
int main_initialise(void) {
    if (piloteSerieUSB_initialise() != 0) {
        return -1;
    }
    if (interfaceTouche_initialise() != 0) {
        return -1;
    }
    if (interfaceMalyan_initialise() != 0) {
        return -1;
    }
    return 0;
}

void main_termine(void) {
    piloteSerieUSB_termine();
    interfaceTouche_termine();
    interfaceMalyan_termine();
}

// Definitions de fonctions publiques:
int main(int argc, char** argv) {
    unsigned char toucheLue = 'D';
    char reponse[MAIN_LONGUEUR_MAXIMALE + 1];
    int pipe_pere_fils[2];
    int pipe_fils_pere[2];
    pid_t pid;

    if (pipe(pipe_pere_fils) == -1 || pipe(pipe_fils_pere) == -1) {
        perror("Erreur dans la création du pipe");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid < 0) {
        perror("Erreur de fork");
        return EXIT_FAILURE;
    }

    if (pid > 0) {  // Processus père
        close(pipe_pere_fils[0]); // Ferme la lecture du pipe père -> fils
        close(pipe_fils_pere[1]); // Ferme l'écriture du pipe fils -> père

        if (main_initialise()) {
            printf("main_initialise: erreur\n");
            return EXIT_FAILURE;
        }

        fprintf(stdout, "Tapez:\n");
        fprintf(stdout, "Q\" : pour terminer.\n");
        fprintf(stdout, "6\" : pour démarrer le ventilateur.\n");
        fprintf(stdout, "7\" : pour arrêter le ventilateur.\n");
        fprintf(stdout, "8\" : donner la position actuelle.\n");
        fprintf(stdout, "P\" : aller à la position x=20, y=20, z=20.\n");
        fprintf(stdout, "H\" : positionner la tete d'impression à l'origine (home).\n");
        fprintf(stdout, "S\": Initialise un deplacement en S.\n\r");
        fprintf(stdout, "autre chose pour générer une erreur.\n\r");
		fflush(stdout);

        while (toucheLue != 'Q') {
            printf("Entrez une commande\n");
            toucheLue = interfaceTouche_lit();
            printf("Caractère lu = '%c'\n", toucheLue);
            char *input_str;
            switch (toucheLue) {	
                case '6':
		    	input_str = "START_FAN\n";
                    	write(pipe_pere_fils[1],input_str , strlen(input_str)+1);
                break;
                case '7':
		    	input_str = "STOP_FAN\n";
                    	write(pipe_pere_fils[1],input_str , strlen(input_str)+1);
                break;
                case '8':
			input_str = "GET_POSITION\n";
                    	write(pipe_pere_fils[1],input_str , strlen(input_str)+1);
                break;
                case 'P':
			input_str = "MOVE_TO_20_20_20\n";
                    	write(pipe_pere_fils[1],input_str , strlen(input_str)+1);
                break;
                case 'H':
			input_str = "HOME\n";
                	write(pipe_pere_fils[1],input_str , strlen(input_str)+1);
                break;
		case 'S':
		    	input_str = "S_MOVE\n";
                    	write(pipe_pere_fils[1],input_str , strlen(input_str)+1);
		break;			
                case 'Q':
			input_str = "STOP\n";
                    	write(pipe_pere_fils[1],input_str , strlen(input_str)+1);
                break;
                default:
                    printf("Commande invalide.\n");
                continue;
            }

            // Attendre la réponse du fils
            int bytes_read = read(pipe_fils_pere[0], reponse, MAIN_LONGUEUR_MAXIMALE);
            if (bytes_read > 0) {
                reponse[bytes_read] = '\0';
                printf("Réponse du fils : %s\n", reponse);
            }
        }

        close(pipe_pere_fils[1]); // Ferme l'écriture
        close(pipe_fils_pere[0]); // Ferme la lecture
		kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);  // Attend que le fils se termine
        main_termine();
        printf("Processus père terminé.\n");

    } else {  // Processus fils
        close(pipe_pere_fils[1]); // Ferme l'écriture du pipe père -> fils
        close(pipe_fils_pere[0]); // Ferme la lecture du pipe fils -> père

        int serial_port = piloteSerieUSB_initialise();
        if (serial_port == -1) {
            exit(EXIT_FAILURE);
        }

        int running = 1;
        char commande[MAIN_LONGUEUR_MAXIMALE + 1];
        while (running) {
            // Lire la commande du père
            int bytes_read = read(pipe_pere_fils[0], commande, MAIN_LONGUEUR_MAXIMALE);
            if (bytes_read <= 0) {
                break;
            }
            commande[bytes_read] = '\0';
            
            // Traiter la commande reçue
            if (strncmp(commande, "STOP", 4) == 0) {
                running = 0;
                write(pipe_fils_pere[1], "Fils arrêté\n", 13);
            } 
			else if (strncmp(commande,"START_FAN",9) == 0)
			{
				if (interfaceMalyan_demarreLeVentilateur() < 0)
				{
					printf("Erreur : Impossible de demarrer le ventilateur\n");
				}
			}
			else if (strncmp(commande,"STOP_FAN",8) == 0)
			{
			    if (interfaceMalyan_arreteLeVentilateur() < 0)
			    {
				   printf("Erreur : Impossible d'arreter le ventilateur\n");
				}
			}
			else if (strncmp(commande,"GET_POSITION",12) == 0)
			{
				if (interfaceMalyan_donneLaPosition() < 0)
				{
				   printf("Erreur : Impossible d'obtenir la position\n");				   
				}
			}
			else if (strncmp(commande,"MOVE_TO_20_20_20",16) == 0)
			{
				if (interfaceMalyan_vaALaPosition(20, 20, 20) < 0)
				{
				   printf("Erreur : Impossible d'aller à la position x=20, y=20, z=20\n");
				}
			}
			else if (strncmp(commande,"HOME",4) == 0)
			{
				if (interfaceMalyan_retourneALaMaison() < 0)
				{
				  printf("Erreur : Impossible de retourner à la maison\n"); 				  
				}
			}
			else if (strncmp(commande,"S_MOVE",6) == 0)
			{
				if (interfaceMalyan_deplacementEnS() < 0)
				{
                  printf("Erreur : Impossible d'effectuer le test de deplacement en S");
				}
			}

                
                usleep(100000);  // Délai pour attendre la réponse

                // Recevoir la réponse de l'imprimante
                int nombre = interfaceMalyan_recoitUneReponse(reponse, MAIN_LONGUEUR_MAXIMALE);
                if (nombre > 0) {
                    reponse[nombre] = '\0';
                    printf("Réponse de l'imprimante : %s\n", reponse);

                    // Vérifier si la réponse contient "ok"
                    if (strstr(reponse, "ok") != NULL) {
                        printf("La réponse contient 'ok'\n");
                    }

                    // Envoyer un accusé de réception au père
                    write(pipe_fils_pere[1], "Réponse reçue\n", 14);
					write(STDOUT_FILENO, "R", 1); // Écrire 'R' dans la sortie standard
                } else {
                    perror("Erreur lors de la réception de la réponse");
                    write(pipe_fils_pere[1], "Erreur de réception\n", 21);
                }
            }

        close(pipe_pere_fils[0]); // Ferme la lecture
        close(pipe_fils_pere[1]); // Ferme l'écriture
        piloteSerieUSB_termine();
        exit(EXIT_SUCCESS);
    }
    return EXIT_SUCCESS;
}

