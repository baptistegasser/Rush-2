#include "stdafx.h"
#include "CTerrain.h"
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string>

#include <iostream>
#include <fstream>
#include <algorithm>



void CTerrain::LireFichierHeightmap(const char* s)
{
	int width, height, bpp;

	rgb_image = stbi_load(s, &width, &height, &bpp, 1);

	dx = width;
	dy = height;
	nbSommets = dx * dy;
	nbPolygones = (width - 1)* (height - 1) * 2;

}

void CTerrain::ConstruireTerrain(float echelleXY, float echelleZ) {
	for (int i = 0; i < dy; i++) {
		for (int j = 0; j < dx; j++) {
			sommets.push_back(CSommetTerrain((j / static_cast<float>((dx-1)))*echelleXY, (i / static_cast<float>((dy-1))) * echelleXY, (static_cast<float>( rgb_image[i * dx + j]) / 255.0f)* echelleZ));
		}

	}
}

void CTerrain::ConstruireIndex()
{
	pIndices = new unsigned int[nbPolygones * 3];
	int k = 0;

	for (int y = 0; y < dy - 1; ++y)
	{
		for (int x = 0; x < dx - 1; ++x)
		{
			// L'important ici est d'utiliser la même formule pour identifier 
			// les sommets qu'au moment de leur création 
			pIndices[k++] = static_cast<unsigned int>(y * dx + x);
			pIndices[k++] = static_cast<unsigned int>((y + 1) * dx + (x + 1));
			pIndices[k++] = static_cast<unsigned int>(y * dx + (x + 1));
			pIndices[k++] = static_cast<unsigned int>(y * dx + x);
			pIndices[k++] = static_cast<unsigned int>((y + 1) * dx + x);
			pIndices[k++] = static_cast<unsigned int>((y + 1) * dx + (x + 1));

		}
	}
}

XMFLOAT3 CTerrain::CalculNormale(int x, int y)
{
	XMVECTOR n1;
	XMVECTOR n2;
	XMVECTOR n3;
	XMVECTOR n4;

	XMVECTOR v1;
	XMVECTOR v2;  
	XMVECTOR v3;
	XMVECTOR v4;

	n1 = n2 = n3 = n4 = XMVectorSet(0, 0, 1, 0);  // Le Z est le haut

	// v1 = p1 – p0, etc... 
	if (y < dy - 1) 
		v1 = ObtenirPosition(x, y + 1) - ObtenirPosition(x, y);
	if (x < dx - 1)
		v2 = ObtenirPosition(x + 1, y) - ObtenirPosition(x, y);
	if (y > 0)
		v3 = ObtenirPosition(x, y - 1) - ObtenirPosition(x, y);
	if (x > 0)
		v4 = ObtenirPosition(x - 1, y) - ObtenirPosition(x, y);

	// les produits vectoriels
	if (y < dy-1 && x < dx-1 )
		n1 = XMVector3Cross(v2, v1);
	if (y > 0   && x < dx-1 )
		n2 = XMVector3Cross(v3, v2);
	if (y > 0   && x > 0 )
		n3 = XMVector3Cross(v4, v3);
	if (y < dy-1 && x > 0 )
		n4 = XMVector3Cross(v1, v4);    

	
	n1 = n1 + n2 + n3 +n4;
	
	n1 = XMVector3Normalize(n1);
	
	XMFLOAT3 resultat; 

	XMStoreFloat3(&resultat, n1);
	
	return resultat;
}

XMVECTOR CTerrain::ObtenirPosition(int x, int y)
{
	CSommetTerrain s = sommets[y * dx + x];
	return XMVectorSet(s.getX(), s.getY(), s.getZ(), 0);
}

void CTerrain::CalculerNormales()
{
	for (int i = 0; i < dy; i++) {
		for (int j = 0; j < dx; j++) {
			normales.push_back(CalculNormale(j, i));
		}
	}
}

void CTerrain::EnregistrerTout(const char* s)
{
	ofstream out{ s };
	out << "dx" << endl << dx << endl;
	out << "dy" << endl << dy << endl;
	out << "\n";
	out << "nbSommets" << endl << nbSommets << endl;
	out << "nbPolygones" << endl << nbPolygones << endl;
	out << "\n";
	out << "sommets" << endl;
	for_each(begin(sommets), end(sommets), [&out](CSommetTerrain s) { out << s.getX() << " " << s.getY() << " " << s.getZ() << endl;});
	out << "\n";
	out << "indices" << endl;
	for (auto i = 0; i < nbPolygones*3; i = i+3) {
		out << pIndices[i] << " ";
		out << pIndices[i+1] << " ";
		out << pIndices[i+2] << endl;
	}
	out << "\n";
	out << "normales" << endl;
	for_each(begin(normales), end(normales), [&out](XMFLOAT3 s) { out << s.x << " " << s.y << " " << s.z << endl; });

}


void CTerrain::RecupererTout(const char* s) {
	ifstream in{ s };
	string delimiteur = " ";
	string ligne;

	while (getline(in, ligne))
	{
		if (ligne == "dx") {
			getline(in, ligne);
			dx = stoi(ligne);
		}
		else if (ligne == "dy") {
			getline(in, ligne);
			dy = stoi(ligne);
		}
		else if (ligne == "nbSommets") {
			getline(in, ligne);
			nbSommets = stoi(ligne);
		}
		else if (ligne == "nbPolygones") {
			getline(in, ligne);
			nbPolygones = stoi(ligne);
		}
		else if (ligne == "sommets") {
			sommets.clear();
			float x, y, z;
			for (int i = 0; i < nbSommets; ++i) {
				getline(in, ligne);
				x = stof(ligne.substr(0, ligne.find(delimiteur)));
				ligne.erase(0, ligne.find(delimiteur) + delimiteur.length());
				y = stof(ligne.substr(0, ligne.find(delimiteur)));
				ligne.erase(0, ligne.find(delimiteur) + delimiteur.length());
				z = stof(ligne.substr(0, ligne.find(delimiteur)));
				ligne.erase(0, ligne.find(delimiteur) + delimiteur.length());
				sommets.push_back(CSommetTerrain{ x,y,z});
			}
		}
		else if (ligne == "indices") {
			pIndices = new unsigned int[nbPolygones * 3];
			for (int i = 0; i < nbPolygones * 3; i += 3) {
				getline(in, ligne);
				pIndices[i] = stoi(ligne.substr(0, ligne.find(delimiteur)));
				ligne.erase(0, ligne.find(delimiteur) + delimiteur.length());
				pIndices[i + 1] = stoi(ligne.substr(0, ligne.find(delimiteur)));
				ligne.erase(0, ligne.find(delimiteur) + delimiteur.length());
				pIndices[i + 2] = stoi(ligne.substr(0, ligne.find(delimiteur)));
			}

		}
		else if (ligne == "normales") {
			normales.clear();
			float nx, ny, nz;
			for (int i = 0; i < nbSommets; ++i) {
				getline(in, ligne);
				nx = stof(ligne.substr(0, ligne.find(delimiteur)));
				ligne.erase(0, ligne.find(delimiteur) + delimiteur.length());
				ny = stof(ligne.substr(0, ligne.find(delimiteur)));
				ligne.erase(0, ligne.find(delimiteur) + delimiteur.length());
				nz = stof(ligne.substr(0, ligne.find(delimiteur)));
				normales.push_back(XMFLOAT3{nx,ny,nz });
			}
		}
	}
}