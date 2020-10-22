/**
 *  @file   larpandoracontent/LArTrackShowerId/MvaPfoCharacterisationAlgorithm.cc
 *
 *  @brief  Implementation of the mva pfo characterisation algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArFileHelper.h"
#include "larpandoracontent/LArHelpers/LArPfoHelper.h"

#include "larpandoracontent/LArObjects/LArTwoDSlidingFitResult.h"

#include "larpandoracontent/LArTrackShowerId/MvaPfoCharacterisationAlgorithm.h"

using namespace pandora;

namespace lar_content
{

template<typename T>
MvaPfoCharacterisationAlgorithm<T>::MvaPfoCharacterisationAlgorithm() :
    m_trainingSetMode(false),
    m_testBeamMode(false),
    m_enableProbability(true),
    m_useThreeDInformation(true),
    m_minProbabilityCut(0.5f),
    m_minCaloHitsCut(5),
    m_applyFiducialCut(false),
    m_fiducialMinX(-std::numeric_limits<float>::max()),
    m_fiducialMaxX(std::numeric_limits<float>::max()),
    m_fiducialMinY(-std::numeric_limits<float>::max()),
    m_fiducialMaxY(std::numeric_limits<float>::max()),
    m_fiducialMinZ(-std::numeric_limits<float>::max()),
    m_fiducialMaxZ(std::numeric_limits<float>::max()),
    m_applyReconstructabilityChecks(false),
    m_filePathEnvironmentVariable("FW_SEARCH_PATH")
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
bool MvaPfoCharacterisationAlgorithm<T>::IsClearTrack(const Cluster *const pCluster) const
{
    if (pCluster->GetNCaloHits() < m_minCaloHitsCut)
        return false;

    const LArMvaHelper::MvaFeatureVector featureVector(LArMvaHelper::CalculateFeatures(m_featureToolVector, this, pCluster));

    if (m_trainingSetMode)
    {
        bool isTrueTrack(false);

        try
        {
            const MCParticle *const pMCParticle(MCParticleHelper::GetMainMCParticle(pCluster));
            isTrueTrack = ((PHOTON != pMCParticle->GetParticleId()) && (E_MINUS != std::abs(pMCParticle->GetParticleId())));
        }
        catch (const StatusCodeException &) {}

        LArMvaHelper::ProduceTrainingExample(m_trainingOutputFile, isTrueTrack, featureVector);
        return isTrueTrack;
    }

    if (!m_enableProbability)
    {
        return LArMvaHelper::Classify(m_mva, featureVector);
    }
    else
    {
        return (LArMvaHelper::CalculateProbability(m_mva, featureVector) > m_minProbabilityCut);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
bool MvaPfoCharacterisationAlgorithm<T>::IsClearTrack(const pandora::ParticleFlowObject *const pPfo) const
{
    if (!LArPfoHelper::IsThreeD(pPfo))
    {
        if (m_enableProbability)
        {
            object_creation::ParticleFlowObject::Metadata metadata;
            metadata.m_propertiesToAdd["TrackScore"] = -1.f;
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::AlterMetadata(*this, pPfo, metadata));
        }
        return (pPfo->GetParticleId() == MU_MINUS);
    }

    // Charge related features are only calculated using hits in W view
    ClusterList wClusterList;
    LArPfoHelper::GetClusters(pPfo, TPC_VIEW_W, wClusterList);

    // TODO Investigate what to do here if use 3D info is set to false
    const PfoCharacterisationFeatureTool::FeatureToolVector &chosenFeatureToolVector(wClusterList.empty() ? m_featureToolVectorNoChargeInfo : m_featureToolVectorThreeD);
    const LArMvaHelper::MvaFeatureVector featureVector(LArMvaHelper::CalculateFeatures(chosenFeatureToolVector, this, pPfo));

    if (m_trainingSetMode && m_applyReconstructabilityChecks)
    {
        const PfoList myPfoList(1, pPfo);

        const MCParticleList *pMCParticleList(nullptr);
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_mcParticleListName, pMCParticleList));

        const CaloHitList *pCaloHitList(nullptr);
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_caloHitListName, pCaloHitList));

        // Mapping target MCParticles -> truth associated Hits
        LArMCParticleHelper::MCContributionMap targetMCParticleToHitsMap;
        if (!m_testBeamMode)
            LArMCParticleHelper::SelectReconstructableMCParticles(pMCParticleList, pCaloHitList, m_primaryParameters, LArMCParticleHelper::IsBeamNeutrinoFinalState, targetMCParticleToHitsMap);
        else
            LArMCParticleHelper::SelectReconstructableMCParticles(pMCParticleList, pCaloHitList, m_primaryParameters, LArMCParticleHelper::IsBeamParticle, targetMCParticleToHitsMap);

        LArMCParticleHelper::MCContributionMapVector mcParticlesToGoodHitsMaps({targetMCParticleToHitsMap});

        LArMCParticleHelper::PfoContributionMap pfoToReconstructable2DHitsMap;
        LArMCParticleHelper::GetPfoToReconstructable2DHitsMap(myPfoList, mcParticlesToGoodHitsMaps, pfoToReconstructable2DHitsMap, m_primaryParameters.m_foldBackHierarchy);
        if (pfoToReconstructable2DHitsMap.empty())
            return false;

        LArMCParticleHelper::PfoToMCParticleHitSharingMap pfoToMCParticleHitSharingMap;
        LArMCParticleHelper::MCParticleToPfoHitSharingMap mcParticleToPfoHitSharingMap;
        LArMCParticleHelper::GetPfoMCParticleHitSharingMaps(pfoToReconstructable2DHitsMap, mcParticlesToGoodHitsMaps, pfoToMCParticleHitSharingMap, mcParticleToPfoHitSharingMap);
        if (pfoToMCParticleHitSharingMap.empty())
            return false;

        const CaloHitList &allHitsInPfo(pfoToReconstructable2DHitsMap.at(pPfo));
        const int nHitsInPfoTotal(allHitsInPfo.size());
        unsigned int nHitsInBestMCParticleTotal(0);
        unsigned int nHitsSharedWithBestMCParticleTotal(0);
        int bestMCParticlePdgCode(0);
        CartesianVector threeDVertexPosition(0.f, 0.f, 0.f);
        float hitsShower = 0;
        float hitsTrack = 0;
        const LArMCParticleHelper::MCParticleToSharedHitsVector &mcParticleToSharedHitsVector(pfoToMCParticleHitSharingMap.at(pPfo));

        for (const LArMCParticleHelper::MCParticleCaloHitListPair &mcParticleCaloHitListPair : mcParticleToSharedHitsVector)
        {
            const pandora::MCParticle *const pAssociatedMCParticle(mcParticleCaloHitListPair.first);
            const CaloHitList &allMCHits(targetMCParticleToHitsMap.at(pAssociatedMCParticle));
            const CaloHitList &associatedMCHits(mcParticleCaloHitListPair.second);

            if ((std::abs(pAssociatedMCParticle->GetParticleId()) == 11) || (pAssociatedMCParticle->GetParticleId()) == 22)
            {
                hitsShower = hitsShower + associatedMCHits.size();
            }
            else
            {
                hitsTrack = hitsTrack + associatedMCHits.size();
            }

            if (static_cast<int>(associatedMCHits.size()) > nHitsSharedWithBestMCParticleTotal)
            {
                nHitsSharedWithBestMCParticleTotal = associatedMCHits.size();
                nHitsInBestMCParticleTotal = allMCHits.size();
                bestMCParticlePdgCode = pAssociatedMCParticle->GetParticleId();
                threeDVertexPosition = pAssociatedMCParticle->GetVertex();
            }
        }

        float trackShowerHitsRatio((hitsTrack + hitsShower) > 0 ? hitsTrack / (hitsTrack + hitsShower) : 0.f);
        const bool isTrueTrack = trackShowerHitsRatio >= 0.5;

        const float completeness((nHitsInBestMCParticleTotal > 0) ? static_cast<float>(nHitsSharedWithBestMCParticleTotal) / static_cast<float>(nHitsInBestMCParticleTotal) : 0.f);
        const float purity((nHitsInPfoTotal > 0) ? static_cast<float>(nHitsSharedWithBestMCParticleTotal) / static_cast<float>(nHitsInPfoTotal) : 0.f);
        int pdgCode = bestMCParticlePdgCode;

        CaloHitList checkHitListW;
        LArPfoHelper::GetCaloHits(pPfo, TPC_VIEW_W, checkHitListW);
        CaloHitList checkHitListU;
        LArPfoHelper::GetCaloHits(pPfo, TPC_VIEW_U, checkHitListU);
        CaloHitList checkHitListV;
        LArPfoHelper::GetCaloHits(pPfo, TPC_VIEW_V, checkHitListV);
        CaloHitList checkHitListAll;
        checkHitListAll.splice(checkHitListAll.end(), checkHitListW);
        checkHitListAll.splice(checkHitListAll.end(), checkHitListU);
        checkHitListAll.splice(checkHitListAll.end(), checkHitListV);

        LArMCParticleHelper::MCRelationMap mcPrimaryMap;
        LArMCParticleHelper::GetMCPrimaryMap(pMCParticleList, mcPrimaryMap);

        LArMCParticleHelper::MCContributionMap mcToTrueHitListMap;
        LArMCParticleHelper::CaloHitToMCMap hitToMCMap;        
        LArMCParticleHelper::GetMCParticleToCaloHitMatches(&checkHitListAll, mcPrimaryMap, hitToMCMap, mcToTrueHitListMap);

        int showerCount(0);
        for (const CaloHit *pHit : checkHitListAll)
        {
            const MCParticle *pHitMCParticle(nullptr);

            try
            {
                pHitMCParticle = hitToMCMap.at(pHit);
            }
            catch (...) {continue;}

            if ((PHOTON == pHitMCParticle->GetParticleId()) || (E_MINUS == std::abs(pHitMCParticle->GetParticleId())))
            {
                ++showerCount;
            }
        }

        float showerProbability(hitToMCMap.size() > 0 ? static_cast<float>(showerCount) / static_cast<float>(hitToMCMap.size()) : 1.f);
        const bool mischaracterisedPfo((showerProbability < 0.5f && !isTrueTrack) || (showerProbability > 0.5 && isTrueTrack) ? true : false);
        const bool isMainMCParticleSet(0 != pdgCode);

        if (isMainMCParticleSet)
        {
            if (completeness >= 0.f && purity >= 0.f && !mischaracterisedPfo &&
                (!m_applyFiducialCut || this->PassesFiducialCut(threeDVertexPosition)))
            {
                std::string outputFile;
                outputFile.append(m_trainingOutputFile);
                const std::string end=((wClusterList.empty()) ? "noChargeInfo.txt" : ".txt");
                outputFile.append(end);
                LArMvaHelper::ProduceTrainingExample(outputFile, isTrueTrack, featureVector);
            }
        }

        return isTrueTrack;
    }
    else if (m_trainingSetMode)
    {
        bool isTrueTrack(false);
        bool isMainMCParticleSet(false);

        try
        {
            const MCParticle *const pMCParticle(LArMCParticleHelper::GetMainMCParticle(pPfo));
            isTrueTrack = ((PHOTON != pMCParticle->GetParticleId()) && (E_MINUS != std::abs(pMCParticle->GetParticleId())));
            isMainMCParticleSet = (pMCParticle->GetParticleId() != 0);
        }
        catch (const StatusCodeException &) {}

        if (isMainMCParticleSet)
        {
            std::string outputFile;
            outputFile.append(m_trainingOutputFile);
            const std::string end=((wClusterList.empty()) ? "noChargeInfo.txt" : ".txt");
            outputFile.append(end);
            LArMvaHelper::ProduceTrainingExample(outputFile, isTrueTrack, featureVector);
        }
 
        return isTrueTrack;
    }

    for (const LArMvaHelper::MvaFeature &featureValue : featureVector)
    {
        if (!featureValue.IsInitialized())
        {
            if (m_enableProbability)
            {
                object_creation::ParticleFlowObject::Metadata metadata;
                metadata.m_propertiesToAdd["TrackScore"] = -1.f;
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::AlterMetadata(*this, pPfo, metadata));
            }
            return (pPfo->GetParticleId() == MU_MINUS);
        }
    }

    // If no failures, proceed with MvaPfoCharacterisationAlgorithm classification
    if (!m_enableProbability)
    {
        return LArMvaHelper::Classify((wClusterList.empty() ? m_mvaNoChargeInfo : m_mva), featureVector);
    }
    else
    {
        const double score(LArMvaHelper::CalculateProbability((wClusterList.empty() ? m_mvaNoChargeInfo : m_mva), featureVector));
        object_creation::ParticleFlowObject::Metadata metadata;
        metadata.m_propertiesToAdd["TrackScore"] = score;
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::AlterMetadata(*this, pPfo, metadata));
        return (m_minProbabilityCut <= score);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
StatusCode MvaPfoCharacterisationAlgorithm<T>::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinPrimaryGoodHits", m_primaryParameters.m_minPrimaryGoodHits));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinHitsForGoodView", m_primaryParameters.m_minHitsForGoodView));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinPrimaryGoodViews", m_primaryParameters.m_minPrimaryGoodViews));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "SelectInputHits", m_primaryParameters.m_selectInputHits));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinHitSharingFraction", m_primaryParameters.m_minHitSharingFraction));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxPhotonPropagation", m_primaryParameters.m_maxPhotonPropagation));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FoldToPrimaries", m_primaryParameters.m_foldBackHierarchy));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "TrainingSetMode", m_trainingSetMode));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCaloHitsCut", m_minCaloHitsCut));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "UseThreeDInformation", m_useThreeDInformation));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FilePathEnvironmentVariable", m_filePathEnvironmentVariable));

    // ATTN Support legacy XML configurations (note an order of precedence of XML keys exists)
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "BdtFileName", m_mvaFileName));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "SvmFileName", m_mvaFileName));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MvaFileName", m_mvaFileName));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "BdtName", m_mvaName));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "SvmName", m_mvaName));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MvaName", m_mvaName));

    if (m_useThreeDInformation)
    {
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "BdtFileNameNoChargeInfo", m_mvaFileNameNoChargeInfo));
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "SvmFileNameNoChargeInfo", m_mvaFileNameNoChargeInfo));
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MvaFileNameNoChargeInfo", m_mvaFileNameNoChargeInfo));

        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "BdtNameNoChargeInfo", m_mvaNameNoChargeInfo));
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "SvmNameNoChargeInfo", m_mvaNameNoChargeInfo));
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MvaNameNoChargeInfo", m_mvaNameNoChargeInfo));
    }

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "EnableProbability",  m_enableProbability));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinProbabilityCut", m_minProbabilityCut));

    if (m_trainingSetMode)
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "CaloHitListName", m_caloHitListName));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "MCParticleListName", m_mcParticleListName));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "TrainingOutputFileName", m_trainingOutputFile));
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "TestBeamMode", m_testBeamMode));
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "ApplyFiducialCut", m_applyFiducialCut));
        if (m_applyFiducialCut)
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "FiducialCutMinX", m_fiducialMinX));
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "FiducialCutMaxX", m_fiducialMaxX));
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "FiducialCutMinY", m_fiducialMinY));
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "FiducialCutMaxY", m_fiducialMaxY));
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "FiducialCutMinZ", m_fiducialMinZ));
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "FiducialCutMaxZ", m_fiducialMaxZ));
        }
        PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "ApplyReconstructabilityChecks", m_applyReconstructabilityChecks));
    }
    else
    {
        if (m_mvaFileName.empty() || m_mvaName.empty())
        {
            std::cout << "MvaPfoCharacterisationAlgorithm: MvaFileName and MvaName must be set if in classification mode " << std::endl;
            return STATUS_CODE_INVALID_PARAMETER;
        }

        const std::string fullMvaFileName(LArFileHelper::FindFileInPath(m_mvaFileName, m_filePathEnvironmentVariable));
        m_mva.Initialize(fullMvaFileName, m_mvaName);

        if (m_useThreeDInformation)
        {
            if (m_mvaFileNameNoChargeInfo.empty() || m_mvaNameNoChargeInfo.empty())
            {
                std::cout << "MvaPfoCharacterisationAlgorithm: MvaFileNameNoChargeInfo and MvaNameNoChargeInfo must be set if in classification mode for no charge info in 3D mode " << std::endl;
                return STATUS_CODE_INVALID_PARAMETER;
            }
            const std::string fullMvaFileNameNoChargeInfo(LArFileHelper::FindFileInPath(m_mvaFileNameNoChargeInfo, m_filePathEnvironmentVariable));
            m_mvaNoChargeInfo.Initialize(fullMvaFileNameNoChargeInfo, m_mvaNameNoChargeInfo);
        }
    }

    AlgorithmToolVector algorithmToolVector;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithmToolList(*this, xmlHandle, "FeatureTools", algorithmToolVector));

    if (m_useThreeDInformation)
    {
        AlgorithmToolVector algorithmToolVectorNoChargeInfo;
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithmToolList(*this, xmlHandle, "FeatureToolsNoChargeInfo", algorithmToolVectorNoChargeInfo));

        for (AlgorithmTool *const pAlgorithmTool : algorithmToolVector)
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, LArMvaHelper::AddFeatureToolToVector(pAlgorithmTool, m_featureToolVectorThreeD));

        for (AlgorithmTool *const pAlgorithmTool : algorithmToolVectorNoChargeInfo)
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, LArMvaHelper::AddFeatureToolToVector(pAlgorithmTool, m_featureToolVectorNoChargeInfo));
    }
    else
    {
        for (AlgorithmTool *const pAlgorithmTool : algorithmToolVector)
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, LArMvaHelper::AddFeatureToolToVector(pAlgorithmTool, m_featureToolVector));
    }

    return PfoCharacterisationBaseAlgorithm::ReadSettings(xmlHandle);
}

//------------------------------------------------------------------------------------------------------------------------------------------

template <typename T>
bool MvaPfoCharacterisationAlgorithm<T>::PassesFiducialCut(const CartesianVector &vertex) const
{
    const float vx(vertex.GetX()), vy(vertex.GetY()), vz(vertex.GetZ());
    return m_fiducialMinX <= vx && vx <= m_fiducialMaxX && m_fiducialMinY <= vy && vy <= m_fiducialMaxY && m_fiducialMinZ <= vz && vz <= m_fiducialMaxZ;
}

template class MvaPfoCharacterisationAlgorithm<AdaBoostDecisionTree>;
template class MvaPfoCharacterisationAlgorithm<SupportVectorMachine>;

} // namespace lar_content
