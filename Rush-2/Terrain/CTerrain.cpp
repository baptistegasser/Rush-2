#include "stdafx.h"
#include "CTerrain.h"
#include "sommetbloc.h"
#include "resource.h"
#include "util.h"
#include "MoteurWindows.h"
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string>

#include <iostream>
#include <fstream>
#include <algorithm>


namespace PM3D
{
	struct ShadersParams
	{
		XMMATRIX matWorldViewProj;	// la matrice totale 
		XMMATRIX matWorld;			// matrice de transformation dans le monde 
		XMVECTOR vLumiere; 			// la position de la source d'éclairage (Point)
		XMVECTOR vCamera; 			// la position de la caméra
		XMVECTOR vAEcl; 			// la valeur ambiante de l'éclairage
		XMVECTOR vAMat; 			// la valeur ambiante du matériau
		XMVECTOR vDEcl; 			// la valeur diffuse de l'éclairage 
		XMVECTOR vDMat; 			// la valeur diffuse du matériau 
	};

	void CTerrain::LireFichierHeightmap(const char* s)
	{
		int width, height, bpp;

		rgb_image = stbi_load(s, &width, &height, &bpp, 1);

		dx = width;
		dy = height;
		nbSommets = dx * dy;
		nbPolygones = (width - 1) * (height - 1) * 2;

	}

	void CTerrain::ConstruireTerrain(float echelleXY, float echelleZ) {
		for (int y = 0; y < dy; y++) {
			for (int x = 0; x < dx; x++) {
				sommets.push_back(XMFLOAT3(
					(x / static_cast<float>((dx - 1))) * echelleXY,
					(y / static_cast<float>((dy - 1))) * echelleXY,
					(static_cast<float>(rgb_image[y * dx + x]) / 255.0f) * echelleZ)
				);
			}

		}
	}

	void CTerrain::ConstruireIndex()
	{
		pIndices = new unsigned int[nbPolygones * 3];
		int k = 0;

		unsigned int udx = static_cast<unsigned int>(dx);
		unsigned int udy = static_cast<unsigned int>(dy);

		for (unsigned int y = 0; y < udy - 1; ++y)
		{
			for (unsigned int x = 0; x < udx - 1; ++x)
			{
				// L'important ici est d'utiliser la même formule pour identifier 
				// les sommets qu'au moment de leur création 
				pIndices[k++] = y * udx + x;
				pIndices[k++] = (y + 1) * udx + (x + 1);
				pIndices[k++] = y * udx + (x + 1);
				pIndices[k++] = y * udx + x;
				pIndices[k++] = (y + 1) * udx + x;
				pIndices[k++] = (y + 1) * udx + (x + 1);

				// Triangle 1
				//pIndices[k++] = y * udx + x;
				//pIndices[k++] = (y + 1) * udx + x + 1;
				//pIndices[k++] = (y + 1) * udx + x;
				// Triangle 2
				//pIndices[k++] = y * udx + x;
				//pIndices[k++] = y * udx + x + 1;
				//pIndices[k++] = (y + 1) * udx + x + 1;
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
		if (y < dy - 1 && x < dx - 1)
			n1 = XMVector3Cross(v2, v1);
		if (y > 0 && x < dx - 1)
			n2 = XMVector3Cross(v3, v2);
		if (y > 0 && x > 0)
			n3 = XMVector3Cross(v4, v3);
		if (y < dy - 1 && x > 0)
			n4 = XMVector3Cross(v1, v4);


		n1 = n1 + n2 + n3 + n4;

		n1 = XMVector3Normalize(n1);

		XMFLOAT3 resultat;

		XMStoreFloat3(&resultat, n1);

		return resultat;
	}

	XMVECTOR CTerrain::ObtenirPosition(int x, int y)
	{
		XMFLOAT3 s = sommets[y * dx + x];
		return XMVectorSet(s.x, s.y, s.z, 0);
	}

	void CTerrain::InitEffet()
	{
		// Compilation et chargement du vertex shader
		ID3D11Device* pD3DDevice = pDispositif->GetD3DDevice();

		// Création d'un tampon pour les constantes du VS
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(ShadersParams);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		pD3DDevice->CreateBuffer(&bd, nullptr, &pConstantBuffer);

		// Pour l'effet
		ID3DBlob* pFXBlob = nullptr;

		DXEssayer(D3DCompileFromFile(L"MiniPhong.fx", 0, 0, 0,
			"fx_5_0", 0, 0,
			&pFXBlob, nullptr),
			DXE_ERREURCREATION_FX);

		D3DX11CreateEffectFromMemory(pFXBlob->GetBufferPointer(), pFXBlob->GetBufferSize(), 0, pD3DDevice, &pEffet);

		pFXBlob->Release();

		pTechnique = pEffet->GetTechniqueByIndex(0);
		pPasse = pTechnique->GetPassByIndex(0);

		// Créer l'organisation des sommets pour le VS de notre effet
		D3DX11_PASS_SHADER_DESC effectVSDesc;
		pPasse->GetVertexShaderDesc(&effectVSDesc);

		D3DX11_EFFECT_SHADER_DESC effectVSDesc2;
		effectVSDesc.pShaderVariable->GetShaderDesc(effectVSDesc.ShaderIndex, &effectVSDesc2);

		const void* vsCodePtr = effectVSDesc2.pBytecode;
		const unsigned vsCodeLen = effectVSDesc2.BytecodeLength;

		pVertexLayout = nullptr;
		DXEssayer(pD3DDevice->CreateInputLayout(CSommetBloc::layout,
			CSommetBloc::numElements,
			vsCodePtr,
			vsCodeLen,
			&pVertexLayout),
			DXE_CREATIONLAYOUT);
	}

	void CTerrain::CalculerNormales()
	{
		for (int i = 0; i < dy; i++) {
			for (int j = 0; j < dx; j++) {
				normales.push_back(CalculNormale(j, i));
			}
		}
	}

	PM3D::CTerrain::~CTerrain()
	{
		DXRelacher(pVertexBuffer);
		DXRelacher(pIndexBuffer);
		DXRelacher(pConstantBuffer);
		DXRelacher(pEffet);
		DXRelacher(pVertexLayout);
	}

	void CTerrain::Anime(float tempsEcoule)
	{
		rotation = rotation + ((XM_PI * 2.0f) / 10.0f * tempsEcoule);

		// modifier la matrice de l'objet bloc
		matWorld = XMMatrixRotationX(rotation);
	}

	void CTerrain::Draw()
	{
		// Obtenir le contexte
		ID3D11DeviceContext* pImmediateContext = pDispositif->GetImmediateContext();

		// Choisir la topologie des primitives
		pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Source des sommets
		const UINT stride = sizeof(CSommetBloc);
		const UINT offset = 0;
		pImmediateContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);

		// Source des index
		pImmediateContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

		// input layout des sommets
		pImmediateContext->IASetInputLayout(pVertexLayout);

		// Initialiser et sélectionner les «constantes» de l'effet
		ShadersParams sp;
		XMMATRIX viewProj = CMoteurWindows::GetInstance().GetMatViewProj();

		sp.matWorldViewProj = XMMatrixTranspose(matWorld * viewProj);
		sp.matWorld = XMMatrixTranspose(matWorld);

		sp.vLumiere = XMVectorSet(-10.0f, 10.0f, -10.0f, 1.0f);
		sp.vCamera = XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
		sp.vAEcl = XMVectorSet(0.2f, 0.2f, 0.2f, 1.0f);
		sp.vAMat = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
		sp.vDEcl = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
		sp.vDMat = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
		pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &sp, 0, 0);

		ID3DX11EffectConstantBuffer* pCB = pEffet->GetConstantBufferByName("param");  // Nous n'avons qu'un seul CBuffer
		pCB->SetConstantBuffer(pConstantBuffer);

		// **** Rendu de l'objet
		pPasse->Apply(0, pImmediateContext);

		pImmediateContext->DrawIndexed(3 * nbPolygones, 0, 0);
	}

	CTerrain::CTerrain(const char* file, float scaleXY, float scaleZ, CDispositifD3D11* _pDispositif)
		: pDispositif(_pDispositif) // Prendre en note le dispositif
		, matWorld(XMMatrixIdentity())
		, rotation(0.0f)
		, pVertexBuffer(nullptr)
		, pIndexBuffer(nullptr)
		, pConstantBuffer(nullptr)
		, pEffet(nullptr)
		, pTechnique(nullptr)
		, pVertexLayout(nullptr)
	{
		LireFichierHeightmap(file);
		ConstruireTerrain(scaleXY, scaleZ);
		ConstruireIndex();
		CalculerNormales();

		stbi_image_free(rgb_image);

		CSommetBloc* sommetsBloc = new CSommetBloc[sommets.size()];
		for (int i = 0; i < sommets.size(); ++i) {
			sommetsBloc[i] = { sommets[i], normales[i] };
		}

		// Création du vertex buffer et copie des sommets
		ID3D11Device* pD3DDevice = pDispositif->GetD3DDevice();

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = (UINT)sommets.size() * sizeof(CSommetBloc);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = sommetsBloc;
		pVertexBuffer = nullptr;

		DXEssayer(pD3DDevice->CreateBuffer(&bd, &InitData, &pVertexBuffer), DXE_CREATIONVERTEXBUFFER);

		delete[]sommetsBloc;

		// Création de l'index buffer et copie des indices
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = nbPolygones * 3 * sizeof(unsigned int);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = pIndices;
		pIndexBuffer = nullptr;

		DXEssayer(pD3DDevice->CreateBuffer(&bd, &InitData, &pIndexBuffer),
			DXE_CREATIONINDEXBUFFER);

		// Initialisation de l'effet
		InitEffet();
	}

} // namespace PM3D