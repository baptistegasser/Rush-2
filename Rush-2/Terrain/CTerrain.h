#pragma once
#include "CSommetTerrain.h"
#include "d3dx11effect.h"
#include "Objet3D.h"

#include <vector>
#include <DirectXMath.h>
using namespace DirectX;
using namespace std;

namespace PM3D
{
	class CDispositifD3D11;

	class CTerrain : public CObjet3D
	{
		uint8_t* rgb_image;
		int dx, dy;
		vector<XMFLOAT3> sommets;
		int nbSommets;
		int nbPolygones;
		unsigned int* pIndices;
		vector<XMFLOAT3> normales;

		void LireFichierHeightmap(const char* s);

		void ConstruireTerrain(float echelleXY, float echelleZ);

		void ConstruireIndex();

		void CalculerNormales();

	public:
		CTerrain(const char* file, float scaleXY, float scaleZ, CDispositifD3D11* pDispositif);
		virtual ~CTerrain();

		virtual void Anime(float tempsEcoule) override;
		virtual void Draw() override;

	private:
		XMFLOAT3 CalculNormale(int x, int y);
		XMVECTOR ObtenirPosition(int x, int y);

		void InitEffet();

		CDispositifD3D11* pDispositif;

		ID3D11Buffer* pVertexBuffer;
		ID3D11Buffer* pIndexBuffer;

		// Définitions des valeurs d'animation
		ID3D11Buffer* pConstantBuffer;
		XMMATRIX matWorld;
		float rotation;

		// Pour les effets
		ID3DX11Effect* pEffet;
		ID3DX11EffectTechnique* pTechnique;
		ID3DX11EffectPass* pPasse;
		ID3D11InputLayout* pVertexLayout;
	};

} // namespace PM3D
