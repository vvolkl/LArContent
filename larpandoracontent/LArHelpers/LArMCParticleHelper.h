/**
 *  @file   larpandoracontent/LArHelpers/LArMCParticleHelper.h
 *
 *  @brief  Header file for the lar monte carlo particle helper helper class.
 *
 *  $Log: $
 */
#ifndef LAR_MC_PARTICLE_HELPER_H
#define LAR_MC_PARTICLE_HELPER_H 1

#include "Pandora/PandoraInternal.h"

#include "larpandoracontent/LArObjects/LArMCParticle.h"

#include <unordered_map>
#include <functional>

namespace lar_content
{

/**
 *  @brief  LArMCParticleHelper class
 */
class LArMCParticleHelper
{
public:
    typedef std::unordered_map<const pandora::MCParticle*, const pandora::MCParticle*> MCRelationMap;

    typedef std::unordered_map<const pandora::MCParticle*, const pandora::ParticleFlowObject*> MCToPfoMap;

    typedef std::unordered_map<const pandora::CaloHit*, const pandora::MCParticle*> CaloHitToMCMap;
    typedef std::unordered_map<const pandora::CaloHit*, const pandora::ParticleFlowObject*> CaloHitToPfoMap;

    typedef std::unordered_map<const pandora::MCParticle*, pandora::CaloHitList> MCContributionMap;
    typedef std::vector<MCContributionMap> MCContributionMapVector;

    typedef std::unordered_map<const pandora::ParticleFlowObject*, pandora::CaloHitList> PfoContributionMap;
    typedef std::unordered_map<const pandora::MCParticle*, PfoContributionMap> MCToPfoMatchingMap;

    typedef std::pair<const pandora::MCParticle*, pandora::CaloHitList > MCParticleCaloHitListPair;
    typedef std::pair<const pandora::ParticleFlowObject*, pandora::CaloHitList > PfoCaloHitListPair;

    typedef std::vector<MCParticleCaloHitListPair> MCParticleToSharedHitsVector;
    typedef std::vector<PfoCaloHitListPair> PfoToSharedHitsVector;

    typedef std::map<const pandora::ParticleFlowObject*, MCParticleToSharedHitsVector> PfoToMCParticleHitSharingMap;
    typedef std::map<const pandora::MCParticle*, PfoToSharedHitsVector> MCParticleToPfoHitSharingMap;

    /**
     *  @brief   PrimaryParameters class
     */
    class PrimaryParameters
    {
    public:
        /**
         *  @brief  Constructor
         */
        PrimaryParameters();

        unsigned int  m_minPrimaryGoodHits;       ///< the minimum number of primary good Hits
        unsigned int  m_minHitsForGoodView;       ///< the minimum number of Hits for a good view
        unsigned int  m_minPrimaryGoodViews;      ///< the minimum number of primary good views
        bool          m_selectInputHits;          ///< whether to select input hits
        float         m_maxPhotonPropagation;     ///< the maximum photon propagation length
        float         m_minHitSharingFraction;    ///< the minimum Hit sharing fraction
    };

    /**
     *  @brief  Whether a mc particle is a final-state particle from a neutrino or antineutrino interaction
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return boolean
     */
    static bool IsNeutrinoFinalState(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Whether a mc particle is produced by the interation of a neutrino or antineutrino
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return boolean
     */
    static bool IsNeutrinoInduced(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Whether a mc particle is a neutrino or antineutrino
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return boolean
     */
    static bool IsNeutrino(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Whether a mc particle is visible (i.e. long-lived charged particle)
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return boolean
     */
    static bool IsVisible(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Get neutrino MC particles from an input MC particle list
     *
     *  @param  pMCParticleList the input MC particle list
     *  @param  trueNeutrinos to receive the vector of neutrino MC particles
     */
    static void GetTrueNeutrinos(const pandora::MCParticleList *const pMCParticleList, pandora::MCParticleVector &trueNeutrinos);

    /**
     *  @brief  Get the primary parent mc particle
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return address of the primary parent mc particle
     */
    static const pandora::MCParticle *GetPrimaryMCParticle(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Get the parent mc particle
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return address of the parent mc particle
     */
    static const pandora::MCParticle *GetParentMCParticle(const pandora::MCParticle *const pMCParticle);

     /**
     *  @brief  Get parent neutrino or antineutrino
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return address of primary neutrino mc particle
     */
    static const pandora::MCParticle *GetParentNeutrino(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Get parent neutrino or antineutrino pdg code
     *
     *  @param  pMCParticle the input mc particle
     *
     *  @return pdg code of neutrino (or zero, otherwise)
     */
    static int GetParentNeutrinoId(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Whether a cluster is the product of a neutrino interaction
     *
     *  @param  pCluster the input cluster
     *  @param  minFraction minimum threshold for the combined fraction of neutrino-induced particles associated with the cluster
     *
     *  @return boolean
     */
    static bool IsNeutrinoInduced(const pandora::Cluster *const pCluster, const float minFraction = 0.f);

    /**
     *  @brief  Whether a hit is the product of a neutrino interaction
     *
     *  @param  pCaloHit the input calo hit
     *  @param  minFraction minimum threshold for the combined fraction of neutrino-induced particles associated with the hit
     *
     *  @return boolean
     */
    static bool IsNeutrinoInduced(const pandora::CaloHit *const pCaloHit, const float minFraction = 0.f);

    /**
     *  @brief  Calculate the fractional weight of neutrino-induced particles associated with a given object
     *
     *  @param  pT the input object
     *
     *  @return the fractional weight of neutrino-induced particles associated with the object
     */
    template <typename T>
    static float GetNeutrinoFraction(const T *const pT);

    /**
     *  @brief  Calculate the weight of neutrino-induced particles (and, separately, of all particles) associated with a given object
     *
     *  @param  pT the input object
     *  @param  neutrinoWeight to receive the neutrino weight
     *  @param  totalWeight to receive the total weight
     */
    template <typename T>
    static void GetNeutrinoWeight(const T *const pT, float &neutrinoWeight, float &totalWeight);

    /**
     *  @brief  Whether a provided mc particle matches the implemented definition of being primary
     *
     *  @param  pMCParticle the address of the mc particle
     *
     *  @return boolean
     */
    static bool IsPrimary(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Get mapping from individual mc particles (in a provided list) and their primary parent mc particles
     *
     *  @param  pMCParticleList the input mc particle list
     *  @param  mcPrimaryMap the output mapping between mc particles and their parents
     */
    static void GetMCPrimaryMap(const pandora::MCParticleList *const pMCParticleList, MCRelationMap &mcPrimaryMap);

    /**
     *  @brief  Get vector of primary MC particles from an input list of MC particles
     *
     *  @param  pMCParticleList the input mc particle list
     *  @param  mcPrimaryVector the output mc particle vector
     */
    static void GetPrimaryMCParticleList(const pandora::MCParticleList *const pMCParticleList, pandora::MCParticleVector &mcPrimaryVector);

    /**
     *  @brief  Get vector of primary neutrinos from an input list of MC particles
     *
     *  @param  pMCParticleList the input mc particle list
     *  @param  mcNeutrinoVector the output mc particle vector
     */
    static void GetNeutrinoMCParticleList(const pandora::MCParticleList *const pMCParticleList, pandora::MCParticleVector &mcNeutrinoVector);

    /**
     *  @brief  Sort mc particles by their momentum
     *
     *  @param  pLhs address of first mc particle
     *  @param  pRhs address of second mc particle
     */
    static bool SortByMomentum(const pandora::MCParticle *const pLhs, const pandora::MCParticle *const pRhs);

    /**
     *  @brief  Match calo hits to their parent particles
     *
     *  @param  pCaloHitList the input list of calo hits
     *  @param  mcToPrimaryMCMap input mapping between mc particles and their primaries
     *  @param  hitToPrimaryMCMap output mapping between calo hits and their main MC particle (primary MC particle if mcToPrimaryMCMap provided)
     *  @param  mcToTrueHitListMap output mapping between MC particles and their associated hits
     */
    static void GetMCParticleToCaloHitMatches(const pandora::CaloHitList *const pCaloHitList, const MCRelationMap &mcToPrimaryMCMap,
        CaloHitToMCMap &hitToMCMap, MCContributionMap &mcToTrueHitListMap);

    /**
     *  @brief  Select a subset of true neutrinos representing those that should be used in performance metrics
     *
     *  @param  pAllMCParticleList address of the input mc particle list
     *  @param  selectedMCNeutrinoVector to receive the populated selected true neutrino vector
     */
    static void SelectTrueNeutrinos(const pandora::MCParticleList *const pAllMCParticleList, pandora::MCParticleVector &selectedMCNeutrinoVector);

    /**
     *  @brief  Select primary, reconstructable mc particles that match given criteria.
     *
     *  @param  pMCParticleList the address of the list of MCParticles
     *  @param  pCaloHitList the address of the list of CaloHits
     *  @param  parameters validation parameters to decide when an MCParticle is considered reconstructable
     *  @param  fCriteria a function which returns a bool (= shouldSelect) for a given input MCParticle
     *  @param  selectedMCParticlesToHitsMap the output mapping from selected mcparticles to their hits
     */
    static void SelectReconstructableMCParticles(const pandora::MCParticleList *pMCParticleList, const pandora::CaloHitList *pCaloHitList,
        const PrimaryParameters &parameters, std::function<bool(const pandora::MCParticle *const)> fCriteria, MCContributionMap &selectedMCParticlesToHitsMap);

    /**
     *  @brief  Returns true if passed a primary neutrino final state MCParticle
     */
    static bool IsBeamNeutrinoFinalState(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Returns true if passed a primary beam MCParticle
     */
    static bool IsBeamParticle(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Return true if passed a primary cosmic ray MCParticle
     */
    static bool IsCosmicRay(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Get the nuance code of an MCParticle
     */
    static unsigned int GetNuanceCode(const pandora::MCParticle *const pMCParticle);

    /**
     *  @brief  Get mapping from Pfo to reconstructable 2D hits (=good hits belonging to a selected reconstructable MCParticle)
     *
     *  @param  pfoList the input list of Pfos
     *  @param  selectedMCParticleToHitsMap the input mapping from selected reconstructable MCParticles to their hits
     *  @param  pfoToReconstructable2DHitsMap the output mapping from Pfos to their reconstructable 2D hits
     */
    static void GetPfoToReconstructable2DHitsMap(const pandora::PfoList &pfoList, const MCContributionMap &selectedMCParticleToHitsMap,
        PfoContributionMap &pfoToReconstructable2DHitsMap);

    /**
     *  @brief  Get mapping from Pfo to reconstructable 2D hits (=good hits belonging to a selected reconstructable MCParticle)
     *
     *  @param  pfoList the input list of Pfos
     *  @param  selectedMCParticleToHitsMaps the input vector of mappings from selected reconstructable MCParticles to their hits
     *  @param  pfoToReconstructable2DHitsMap the output mapping from Pfos to their reconstructable 2D hits
     */
    static void GetPfoToReconstructable2DHitsMap(const pandora::PfoList &pfoList, const MCContributionMapVector &selectedMCParticleToHitsMaps,
        PfoContributionMap &pfoToReconstructable2DHitsMap);

    /**
     *  @brief  Get the mappings from Pfo -> pair (reconstructable MCparticles, number of reconstructable 2D hits shared with Pfo)
     *                                reconstructable MCParticle -> pair (Pfo, number of reconstructable 2D hits shared with MCParticle)
     *
     *  @param  pfoToReconstructable2DHitsMap the input mapping from Pfos to reconstructable 2D hits
     *  @param  selectedMCParticleToHitsMaps the input mappings from selected reconstructable MCParticles to hits
     *  @param  pfoToMCParticleHitSharingMap the output mapping from Pfos to selected reconstructable MCParticles and the number hits shared
     *  @param  mcParticleToPfoHitSharingMap the output mapping from selected reconstructable MCParticles to Pfos and the number hits shared
     */
    static void GetPfoMCParticleHitSharingMaps(const PfoContributionMap &pfoToReconstructable2DHitsMap, const MCContributionMapVector &selectedMCParticleToHitsMaps,
        PfoToMCParticleHitSharingMap &pfoToMCParticleHitSharingMap, MCParticleToPfoHitSharingMap &mcParticleToPfoHitSharingMap);

private:
    /**
     *  @brief  For a given Pfo, collect the hits which are reconstructable (=good hits belonging to a selected reconstructable MCParticle)
     *
     *  @param  pPfo the input pfo
     *  @param  selectedMCParticleToHitsMaps the input mappings from selected reconstructable MCParticles to hits
     *  @param  reconstructableCaloHitList2D the output list of reconstructable 2D calo hits in the input pfo
     */
    static void CollectReconstructable2DHits(const pandora::ParticleFlowObject *const pPfo, const MCContributionMapVector &selectedMCParticleToHitsMaps,
        pandora::CaloHitList &reconstructableCaloHitList2D);

    /**
     *  @brief  Select a subset of calo hits representing those that represent "reconstructable" regions of the event
     *
     *  @param  pCaloHitList the address of the input calo hit list
     *  @param  mcToPrimaryMCMap the mc particle to primary mc particle map
     *  @param  selectedCaloHitList to receive the populated selected calo hit list
     *  @param  selectInputHits whether to select input hits
     *  @param  maxPhotonPropagation the maximum photon propagation length
     */
    static void SelectCaloHits(const pandora::CaloHitList *const pCaloHitList, const MCRelationMap &mcToPrimaryMCMap,
        pandora::CaloHitList &selectedCaloHitList, const bool selectInputHits, const float maxPhotonPropagation);

    /**
     *  @brief  Apply further selection criteria to end up with a collection of "good" calo hits that can be use to define whether
     *          a target mc particle is reconstructable.
     *
     *  @param  pSelectedCaloHitList the address of the calo hit list (typically already been through some selection procedure)
     *  @param  mcToPrimaryMCMap the mc particle to primary mc particle map
     *  @param  selectedGoodCaloHitList to receive the populated good selected calo hit list
     *  @param  selectInputHits whether to select input hits
     *  @param  minHitSharingFraction the minimum Hit sharing fraction
     */
    static void SelectGoodCaloHits(const pandora::CaloHitList *const pSelectedCaloHitList, const MCRelationMap &mcToPrimaryMCMap,
        pandora::CaloHitList &selectedGoodCaloHitList, const bool selectInputHits, const float minHitSharingFraction);

    /**
     *  @brief  Select mc particles matching given criteria from an input list
     *
     *  @param  inputMCParticles input vector of MCParticles
     *  @param  fCriteria a function which returns a bool (= shouldSelect) for a given input MCParticle
     *  @param  selectedParticles the output vector of particles selected
     */
    static void SelectParticlesMatchingCriteria(const pandora::MCParticleVector &inputMCParticles, std::function<bool(const pandora::MCParticle *const)> fCriteria,
        pandora::MCParticleVector &selectedParticles);

    /**
     *  @brief  Filter an input vector of MCParticles to ensure they have sufficient good hits to be reconstructable
     *
     *  @param  candidateTargets candidate recontructable MCParticles
     *  @param  mcToTrueHitListMap mapping from candidates reconstructable MCParticles to their true hits
     *  @param  mcToPrimaryMCMap the mc particle to primary mc particle map
     *  @param  parameters validation parameters to decide when an MCParticle is considered reconstructable
     *  @param  selectedMCParticlesToHitsMap the output mapping from selected mcparticles to their hits
     */
    static void SelectParticlesByHitCount(const pandora::MCParticleVector &candidateTargets, const MCContributionMap &mcToTrueHitListMap,
        const MCRelationMap &mcToPrimaryMCMap, const PrimaryParameters &parameters, MCContributionMap &selectedMCParticlesToHitsMap);

    /**
     *  @brief  Whether it is possible to navigate from a primary mc particle to a downstream mc particle without "passing through" a neutron
     *
     *  @param  pOriginalPrimary the address of the original primary mc particle
     *  @param  pThisMCParticle the address of the current mc particle in the primary decay chain
     *  @param  pHitMCParticle the address of the mc particle associated to a calo hit
     *  @param  maxPhotonPropagation the maximum photon propagation length
     *
     *  @return boolean
     */
    static bool PassMCParticleChecks(const pandora::MCParticle *const pOriginalPrimary, const pandora::MCParticle *const pThisMCParticle,
        const pandora::MCParticle *const pHitMCParticle, const float maxPhotonPropagation);

    /**
     *  @brief  Get the hits in the intersection of two hit lists
     *
     *  @param  hitListA an input hit list
     *  @param  hitListB another input hit list
     *
     *  @return The hits that are found in both hitListA and hitListB
     */
    static pandora::CaloHitList GetSharedHits(const pandora::CaloHitList &hitListA, const pandora::CaloHitList &hitListB);
};

} // namespace lar_content

#endif // #ifndef LAR_MC_PARTICLE_HELPER_H
