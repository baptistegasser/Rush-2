#pragma once
#include "Singleton.h"
#include "dispositif.h"

#include <vector>
#include "Objet3D.h"
#include "Bloc.h"
#include "BlocEffet1.h"

namespace PM3D
{

	const int IMAGESPARSECONDE = 60;
	const double EcartTemps = 1.0 / static_cast<double>(IMAGESPARSECONDE);

	//
	//   TEMPLATE�: CMoteur
	//
	//   BUT�: Template servant � construire un objet Moteur qui implantera les
	//         aspects "g�n�riques" du moteur de jeu
	//
	//   COMMENTAIRES�:
	//
	//        Comme plusieurs de nos objets repr�senteront des �l�ments uniques 
	//        du syst�me (ex: le moteur lui-m�me, le lien vers 
	//        le dispositif Direct3D), l'utilisation d'un singleton 
	//        nous simplifiera plusieurs aspects.
	//
	template <class T, class TClasseDispositif> class CMoteur :public CSingleton<T>
	{
	public:

		virtual void Run()
		{
			bool bBoucle = true;

			while (bBoucle)
			{
				// Propre � la plateforme - (Conditions d'arr�t, interface, messages)
				bBoucle = RunSpecific();

				// appeler la fonction d'animation
				if (bBoucle)
				{
					bBoucle = Animation();
				}
			}
		}

		virtual int Initialisations()
		{
			// Propre � la plateforme
			InitialisationsSpecific();

			// * Initialisation du dispositif de rendu
			pDispositif = CreationDispositifSpecific(CDS_FENETRE);

			// * Initialisation de la sc�ne
			InitScene();

			// * Initialisation des param�tres de l'animation et 
			//   pr�paration de la premi�re image
			InitAnimation();

			return 0;
		}

		virtual bool Animation()
		{
			// m�thode pour lire l'heure et calculer le 
			// temps �coul�
			const int64_t TempsCompteurCourant = GetTimeSpecific();
			const double TempsEcoule = GetTimeIntervalsInSec(TempsCompteurPrecedent, TempsCompteurCourant);

			// Est-il temps de rendre l'image?
			if (TempsEcoule > EcartTemps)
			{
				// Affichage optimis�
				pDispositif->Present(); // On enlevera �//� plus tard

										// On pr�pare la prochaine image
				AnimeScene(static_cast<float>(TempsEcoule));

				// On rend l'image sur la surface de travail
				// (tampon d'arri�re plan)
				RenderScene();

				// Calcul du temps du prochain affichage
				TempsCompteurPrecedent = TempsCompteurCourant;
			}

			return true;
		}

		const XMMATRIX& GetMatView() const { return m_MatView; }
		const XMMATRIX& GetMatProj() const { return m_MatProj; }
		const XMMATRIX& GetMatViewProj() const { return m_MatViewProj; }

	protected:

		virtual ~CMoteur()
		{
			Cleanup();
		}

		// Sp�cifiques - Doivent �tre implant�s
		virtual bool RunSpecific() = 0;
		virtual int InitialisationsSpecific() = 0;

		virtual int64_t GetTimeSpecific() const = 0;
		virtual double GetTimeIntervalsInSec(int64_t start, int64_t stop) const = 0;

		virtual TClasseDispositif* CreationDispositifSpecific(const CDS_MODE cdsMode) = 0;
		virtual void BeginRenderSceneSpecific() = 0;
		virtual void EndRenderSceneSpecific() = 0;

		// Autres fonctions
		virtual int InitAnimation()
		{
			TempsSuivant = GetTimeSpecific();
			TempsCompteurPrecedent = TempsSuivant;

			// premi�re Image
			RenderScene();

			return true;
		}

		// Fonctions de rendu et de pr�sentation de la sc�ne
		virtual bool RenderScene()
		{
			BeginRenderSceneSpecific();

			// Appeler les fonctions de dessin de chaque objet de la sc�ne
			for (auto& object3D : ListeScene)
			{
				object3D->Draw();
			}

			EndRenderSceneSpecific();
			return true;
		}


		virtual void Cleanup()
		{
			// d�truire les objets
			ListeScene.clear();

			// D�truire le dispositif
			if (pDispositif)
			{
				delete pDispositif;
				pDispositif = nullptr;
			}
		}

		virtual int InitScene()
		{
			// Initialisation des objets 3D - cr�ation et/ou chargement
			if (!InitObjets())
			{
				return 1;
			}

			// Initialisation des matrices View et Proj
			// Dans notre cas, ces matrices sont fixes
			m_MatView = XMMatrixLookAtLH(
				XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f),
				XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));

			const float champDeVision = XM_PI / 4; 	// 45 degr�s
			const float ratioDAspect = static_cast<float>(pDispositif->GetLargeur()) / static_cast<float>(pDispositif->GetHauteur());
			const float planRapproche = 2.0;
			const float planEloigne = 20.0;

			m_MatProj = XMMatrixPerspectiveFovLH(
				champDeVision,
				ratioDAspect,
				planRapproche,
				planEloigne);

			// Calcul de VP � l'avance
			m_MatViewProj = m_MatView * m_MatProj;

			return 0;
		}

		bool InitObjets()
		{
			// Puis, il est ajout� � la sc�ne
			ListeScene.emplace_back(std::make_unique<CBlocEffet1>(2.0f, 2.0f, 2.0f, pDispositif));

			return true;
		}

		bool AnimeScene(float tempsEcoule)
		{
			for (auto& object3D : ListeScene)
			{
				object3D->Anime(tempsEcoule);
			}

			return true;
		}

	protected:
		// Variables pour le temps de l'animation
		int64_t TempsSuivant;
		int64_t TempsCompteurPrecedent;

		// Le dispositif de rendu
		TClasseDispositif* pDispositif;

		// La seule sc�ne
		std::vector<std::unique_ptr<CObjet3D>> ListeScene;

		// Les matrices
		XMMATRIX m_MatView;
		XMMATRIX m_MatProj;
		XMMATRIX m_MatViewProj;
	};

} // namespace PM3D
