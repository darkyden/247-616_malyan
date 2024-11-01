//interfaceMalyan
//Historique: 
//2018-11-12, Yves Roy, creation

//INCLUSIONS
#include "main.h"
#include "piloteSerieUSB.h"
#include "interfaceMalyan.h"

//Definitions privees
#define INTERFACEMALYAN_LONGUEUR_MAXIMALE_DES_COMMANDES 64
#define INTERFACEMALYAN_LONGUEUR_MAXIMALE_DES_REPONSES 64
#define X_DEPART 0
#define Y_DEPART 0
#define Z_DEPART 0
#define PAS 100

//Declarations de fonctions privees:
int interfaceMalyan_ecritUneCommande(char *Commande, unsigned char Longueur);
int interfaceMalyan_recoitUneReponse(char *Reponse, unsigned char LongueurMaximale);

//Definitions de variables privees
unsigned char interfaceMalyan_commande[INTERFACEMALYAN_LONGUEUR_MAXIMALE_DES_COMMANDES];
unsigned char interfaceMalyan_reponse[INTERFACEMALYAN_LONGUEUR_MAXIMALE_DES_REPONSES];

//Definitions de fonctions privees:
int interfaceMalyan_ecritUneCommande(char *Commande, unsigned char Longueur)
{
int retour;
  retour = piloteSerieUSB_ecrit(Commande, Longueur);
  if (retour != (int)Longueur)
  {
    return -1;
  }
  piloteSerieUSB_attendLaFinDeLEcriture();
  return retour;
}

int interfaceMalyan_recoitUneReponse(char *Reponse, unsigned char LongueurMaximale)
{
  return piloteSerieUSB_lit(Reponse, LongueurMaximale);  
}

//Definitions de variables publiques:
//pas de variables publiques

//Definitions de fonctions publiques:
int interfaceMalyan_initialise(void)
{
  return 0;
}

int interfaceMalyan_termine(void)
{
  return 0;
}

int interfaceMalyan_demarreLeVentilateur(void)
{
  return interfaceMalyan_ecritUneCommande("M106\n", 5);  
}

int interfaceMalyan_arreteLeVentilateur(void)
{
  return interfaceMalyan_ecritUneCommande("M107\n", 5);
}

int interfaceMalyan_genereUneErreur(void)
{
  return interfaceMalyan_ecritUneCommande("x000\n", 5);
}

int interfaceMalyan_donneLaPosition(void)
{
  return interfaceMalyan_ecritUneCommande("M114\n", 5);
}

int interfaceMalyan_vaALaPosition(int x, int y, int z)
{
  char commande[INTERFACEMALYAN_LONGUEUR_MAXIMALE_DES_COMMANDES];
  sprintf(commande, "G0 X%d Y%d Z%d\n",x,y,z);
  return interfaceMalyan_ecritUneCommande(commande, 15);
}

int interfaceMalyan_retourneALaMaison(void)
{
  return interfaceMalyan_ecritUneCommande("G28\n", 4);
}

int interfaceMalyan_deplacementEnS(void)
{
  int x = X_DEPART, y = Y_DEPART;  // Réinitialise aux coordonnées de départ
  int result = 0;

  result = interfaceMalyan_retourneALaMaison();

  for (int repetition = 0; repetition < 2; repetition++) {

        // Trois déplacements vers la droite
        for (int i = 0; i < 3; i++) {
            x += PAS;
            result = interfaceMalyan_vaALaPosition(x, y, Z_DEPART);
            sleep(1);  // Pause d'une seconde à chaque point d'arrêt
        }

        // Un déplacement vers le haut
        y += PAS;
        result = interfaceMalyan_vaALaPosition(x, y, Z_DEPART);
        sleep(1);

        // Trois déplacements vers la gauche
        for (int i = 0; i < 3; i++) {
            x -= PAS;
            result = interfaceMalyan_vaALaPosition(x, y, Z_DEPART);
            sleep(1);
        }

        // Montée d'un niveau pour la seconde partie du "S"
        if(repetition < 1 ){y += PAS;}
    }

  // Retour au point de départ
  result = interfaceMalyan_retourneALaMaison();

  return result;
}
