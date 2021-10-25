#pragma once

#include "Image.h"
#include "sommetbloc.h"
#include "d3dx11effect.h"
#include "Objet3D.h"

#include <vector>

namespace PM3D
{
	class CDispositifD3D11;

class CTerrain : public CObjet3D {
public:
	using size_type = unsigned int;

	const size_type width, height;
	const float scaleXY, scaleZ;
	const size_type pointsSize;
	const size_type indexesSize;
	const size_type polygonsCount;

	// Construct a terrain from an image
	CTerrain(const Image& image, float scaleXY, float scaleZ, CDispositifD3D11* dispositif);
	~CTerrain();

	void Anime(float tempsEcoule) override;
	void Draw() override;

	const CSommetBloc* getPoints() const noexcept;
	const size_type* getIndexes() const noexcept;

private:
	std::unique_ptr<CSommetBloc[]> points;
	std::unique_ptr<size_type[]> indexes;

	void CalcIndex() noexcept;
	void CalcNormal();
	void CreateBuffer();
	void InitEffet();

	XMVECTOR getPos(size_type x, size_type y) const;

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
