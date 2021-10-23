#pragma once
#include "CSommetTerrain.h"
#include <vector>
#include <DirectXMath.h>
using namespace DirectX;
using namespace std;

class CTerrain
{
	uint8_t* rgb_image;
	int dx, dy;
	vector<CSommetTerrain> sommets;
	int nbSommets;
	int nbPolygones;
	unsigned int* pIndices;
	vector<XMFLOAT3> normales;
public :
	void LireFichierHeightmap(const char* s);

	void ConstruireTerrain(float echelleXY, float echelleZ);

	void ConstruireIndex();

	void CalculerNormales();

	void EnregistrerTout(const char* s);

	void RecupererTout(const char* s);
	//CTerrain(const char* s);

private :
	XMFLOAT3 CalculNormale(int x, int y);
	XMVECTOR ObtenirPosition(int x, int y);
};

