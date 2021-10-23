#include "StdAfx.h"

#include "BlocEffet1.h"
#include "sommetbloc.h"
#include "util.h"
#include "DispositifD3D11.h"

#include "resource.h"
#include "MoteurWindows.h"

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

//  FONCTION : CBlocEffet1, constructeur paramètré 
//  BUT :	Constructeur d'une classe de bloc avec effet voir 6.5
//  PARAMÈTRES:		
//		dx, dy, dz:	dimension en x, y, et z
//		pDispositif: pointeur sur notre objet dispositif
CBlocEffet1::CBlocEffet1(const float dx, const float dy, const float dz,
	CDispositifD3D11* pDispositif_)
	: pDispositif(pDispositif_) // Prendre en note le dispositif
	, matWorld(XMMatrixIdentity())
	, rotation(0.0f)
	, pVertexBuffer(nullptr)
	, pIndexBuffer(nullptr)
	, pConstantBuffer(nullptr)
	, pEffet(nullptr)
	, pTechnique(nullptr)
	, pVertexLayout(nullptr)
{
	// Les points
	XMFLOAT3 point[8] =
	{
		XMFLOAT3(-dx / 2, dy / 2, -dz / 2),
		XMFLOAT3(dx / 2, dy / 2, -dz / 2),
		XMFLOAT3(dx / 2, -dy / 2, -dz / 2),
		XMFLOAT3(-dx / 2, -dy / 2, -dz / 2),
		XMFLOAT3(-dx / 2, dy / 2, dz / 2),
		XMFLOAT3(-dx / 2, -dy / 2, dz / 2),
		XMFLOAT3(dx / 2, -dy / 2, dz / 2),
		XMFLOAT3(dx / 2, dy / 2, dz / 2)
	};

	// Calculer les normales
	const XMFLOAT3 n0(0.0f, 0.0f, -1.0f); // devant
	const XMFLOAT3 n1(0.0f, 0.0f, 1.0f); // arrière
	const XMFLOAT3 n2(0.0f, -1.0f, 0.0f); // dessous
	const XMFLOAT3 n3(0.0f, 1.0f, 0.0f); // dessus
	const XMFLOAT3 n4(-1.0f, 0.0f, 0.0f); // face gauche
	const XMFLOAT3 n5(1.0f, 0.0f, 0.0f); // face droite

	CSommetBloc sommets[24] =
	{
		// Le devant du bloc
		CSommetBloc(point[0], n0),
		CSommetBloc(point[1], n0),
		CSommetBloc(point[2], n0),
		CSommetBloc(point[3], n0),

		// L'arrière du bloc
		CSommetBloc(point[4], n1),
		CSommetBloc(point[5], n1),
		CSommetBloc(point[6], n1),
		CSommetBloc(point[7], n1),

		// Le dessous du bloc
		CSommetBloc(point[3], n2),
		CSommetBloc(point[2], n2),
		CSommetBloc(point[6], n2),
		CSommetBloc(point[5], n2),

		// Le dessus du bloc
		CSommetBloc(point[0], n3),
		CSommetBloc(point[4], n3),
		CSommetBloc(point[7], n3),
		CSommetBloc(point[1], n3),

		// La face gauche
		CSommetBloc(point[0], n4),
		CSommetBloc(point[3], n4),
		CSommetBloc(point[5], n4),
		CSommetBloc(point[4], n4),

		// La face droite
		CSommetBloc(point[1], n5),
		CSommetBloc(point[7], n5),
		CSommetBloc(point[6], n5),
		CSommetBloc(point[2], n5)
	};

	// Création du vertex buffer et copie des sommets
	ID3D11Device* pD3DDevice = pDispositif->GetD3DDevice();

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(sommets);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = sommets;
	pVertexBuffer = nullptr;

	DXEssayer(pD3DDevice->CreateBuffer(&bd, &InitData, &pVertexBuffer), DXE_CREATIONVERTEXBUFFER);

	// Création de l'index buffer et copie des indices
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(index_bloc);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = index_bloc;
	pIndexBuffer = nullptr;

	DXEssayer(pD3DDevice->CreateBuffer(&bd, &InitData, &pIndexBuffer),
		DXE_CREATIONINDEXBUFFER);

	// Initialisation de l'effet
	InitEffet();
}

void CBlocEffet1::Anime(float tempsEcoule)
{
	rotation = rotation + ((XM_PI * 2.0f) / 10.0f * tempsEcoule);

	// modifier la matrice de l'objet bloc
	matWorld = XMMatrixRotationX(rotation);
}

void CBlocEffet1::Draw()
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

	pImmediateContext->DrawIndexed(ARRAYSIZE(index_bloc), 0, 0);
}

CBlocEffet1::~CBlocEffet1()
{
	DXRelacher(pVertexBuffer);
	DXRelacher(pIndexBuffer);
	DXRelacher(pConstantBuffer);
	DXRelacher(pEffet);
	DXRelacher(pVertexLayout);
}

void CBlocEffet1::InitEffet()
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

	const void *vsCodePtr = effectVSDesc2.pBytecode;
	const unsigned vsCodeLen = effectVSDesc2.BytecodeLength;

	pVertexLayout = nullptr;
	DXEssayer(pD3DDevice->CreateInputLayout(CSommetBloc::layout,
		CSommetBloc::numElements,
		vsCodePtr,
		vsCodeLen,
		&pVertexLayout),
		DXE_CREATIONLAYOUT);
}

} // namespace PM3D
