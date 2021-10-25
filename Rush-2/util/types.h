#pragma once

#include <stdafx.h>

namespace PM3D
{
	struct ShadersParams
	{
		DirectX::XMMATRIX matWorldViewProj;	// la matrice totale 
		DirectX::XMMATRIX matWorld;			// matrice de transformation dans le monde 
		DirectX::XMVECTOR vLumiere; 		// la position de la source d'�clairage (Point)
		DirectX::XMVECTOR vCamera; 			// la position de la cam�ra
		DirectX::XMVECTOR vAEcl; 			// la valeur ambiante de l'�clairage
		DirectX::XMVECTOR vAMat; 			// la valeur ambiante du mat�riau
		DirectX::XMVECTOR vDEcl; 			// la valeur diffuse de l'�clairage 
		DirectX::XMVECTOR vDMat; 			// la valeur diffuse du mat�riau 
	};

} // namespace PM3D
