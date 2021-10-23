#include "stdafx.h"
#include <iostream>
#include "CTerrain.h"

using namespace std;


int main()
{

    const char* nomImage = "carreau.jpg";
    const char* nomFichier = "result.txt"; // ne pas commenter si on veut tester de recup les donnees du fichier txt
    float echelleXY = 1.0f;
    float echelleZ = 1.0f;

    CTerrain terrain;
    terrain.LireFichierHeightmap(nomImage);
    terrain.ConstruireTerrain(echelleXY, echelleZ);
    terrain.ConstruireIndex();
    terrain.CalculerNormales();
    terrain.EnregistrerTout(nomFichier);

    /*CTerrain terrain;
    const char* nomFichier2 = "result_lecture.txt";
    terrain.RecupererTout(nomFichier);
    terrain.EnregistrerTout(nomFichier2);*/

    return 0;
}