#include "StdAfx.h"
#include "Terrain.h"

#include "util.h"
#include "util/types.h"
#include "DispositifD3D11.h"

#include "resource.h"
#include "MoteurWindows.h"

#include <DirectXMath.h>
#include <fstream>
using namespace DirectX;

namespace PM3D
{

CTerrain::CTerrain(const Image& img, float scaleXY, float scaleZ, CDispositifD3D11* dispositif)
	: width{ img.width() }
	, height{ img.height() }
	, scaleXY{ scaleXY }
	, scaleZ{ scaleZ }
	, pointsSize{ img.width() * img.height() }
	, indexesSize{ (img.width() - 1) * (img.height()-1) * 6 }
	, polygonsCount{ (img.width() - 1) * (img.height() - 1) * 2 }
	, pDispositif(dispositif)
	, matWorld(XMMatrixIdentity())
	, rotation(0.0f)
	, pVertexBuffer(nullptr)
	, pIndexBuffer(nullptr)
	, pConstantBuffer(nullptr)
	, pEffet(nullptr)
	, pTechnique(nullptr)
	, pVertexLayout(nullptr)
{
	points = std::make_unique<CSommetBloc[]>(pointsSize);
	indexes = std::make_unique<size_type[]>(indexesSize);

	const float R = (float) width - 1;
	const float S = (float) height - 1;

	// Set points
	size_type k = 0;
	for (size_type y = 0; y < height; ++y) {
		for (size_type x = 0; x < width; ++x) {
			// Calc the height
			const auto greyScale = img.greyScaleAt(x, y);

			XMFLOAT3 pos{};
			pos.x = (static_cast<float>(x) / R) * scaleXY;
			pos.z = (static_cast<float>(y) / S) * scaleXY;
			pos.y = greyScale/255.f * scaleZ;

			XMFLOAT3 nor{}; // Default normal until calcNormal is called

			points[k++] = CSommetBloc{ pos, nor };
		}
	}

	CalcIndex();
	CalcNormal();
	CreateBuffer();
	InitEffet();

	points.reset();
	indexes.reset();
}

CTerrain::~CTerrain()
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
	pImmediateContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// input layout des sommets
	pImmediateContext->IASetInputLayout(pVertexLayout);

	// Initialiser et sélectionner les «constantes» de l'effet
	ShadersParams sp{};
	const XMMATRIX viewProj = CMoteurWindows::GetInstance().GetMatViewProj();

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

	pImmediateContext->DrawIndexed(indexesSize, 0, 0);
}

void CTerrain::CalcIndex() noexcept
{
	// Set indexes
	size_type k = 0;
	for (size_type y = 0; y < height - 1; ++y) {
		for (size_type x = 0; x < width - 1; ++x) {
			// Triangle 1
			indexes[k++] = y * width + x;
			indexes[k++] = (y + 1) * width + x + 1;
			indexes[k++] = (y + 1) * width + x;
			// Triangle 2
			indexes[k++] = y * width + x;
			indexes[k++] = y * width + x + 1;
			indexes[k++] = (y + 1) * width + x + 1;
		}
	}
}

void CTerrain::CalcNormal()
{
	for (size_type y = 0; y < height; ++y) {
		for (size_type x = 0; x < width; ++x) {
			// Calc the normal
			XMVECTOR v0{}, v1{}, v2{}, v3{}; // vector from point
			XMVECTOR n0{}, n1{}, n2{}, n3{}; // adjacent faces normales

			// default faces normals
			n0 = n1 = n2 = n3 = XMVectorSet( 0, 0, 1, 0 );

			const XMVECTOR pos = getPos(x, y);
			const bool top = y > 0, right = x+1 < width-1, bottom = y + 1 < height - 1, left = x > 0;

			if (top)    v0 = getPos(x, y-1) - pos;
			if (right)  v1 = getPos(x+1, y) - pos;
			if (bottom) v2 = getPos(x, y+1) - pos;
			if (left)   v3 = getPos(x-1, y) - pos;

			if (top && right)	 n0 = XMVector3Cross(v1, v0);
			if (right && bottom) n1 = XMVector3Cross(v2, v1);
			if (bottom && left)	 n2 = XMVector3Cross(v3, v2);
			if (left && top)	 n3 = XMVector3Cross(v0, v3);

			// Update the point normales
			XMVECTOR normal = n0 + n1 + n2 + n3;
			normal = XMVector3Normalize(normal);

			XMStoreFloat3(&points[y * width + x].m_Normal, normal);
		}
	}
}

void CTerrain::CreateBuffer()
{
	// Création du vertex buffer et copie des sommets
	ID3D11Device* pD3DDevice = pDispositif->GetD3DDevice();

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = pointsSize * sizeof(CSommetBloc);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = points.get();
	pVertexBuffer = nullptr;

	DXEssayer(pD3DDevice->CreateBuffer(&bd, &InitData, &pVertexBuffer), DXE_CREATIONVERTEXBUFFER);

	// Création de l'index buffer et copie des indices
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = indexesSize * sizeof(size_type);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.StructureByteStride = sizeof(size_type);

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = indexes.get();
	pIndexBuffer = nullptr;

	DXEssayer(pD3DDevice->CreateBuffer(&bd, &InitData, &pIndexBuffer),
		DXE_CREATIONINDEXBUFFER);
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

	DXEssayer(D3DCompileFromFile(L"MiniPhong.fx", 0, 0, "",
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

const CSommetBloc* CTerrain::getPoints() const noexcept
{
	return points.get();
}

const CTerrain::size_type* CTerrain::getIndexes() const noexcept
{
	return indexes.get();
}

XMVECTOR CTerrain::getPos(size_type x, size_type y) const
{
	return XMLoadFloat3(&points[y * width + x].m_Position);
}

} // namespace PM3D
