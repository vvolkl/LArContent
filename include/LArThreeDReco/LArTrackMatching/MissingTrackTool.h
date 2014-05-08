/**
 *  @file   LArContent/include/LArThreeDReco/LArTrackMatching/MissingTrackTool.h
 * 
 *  @brief  Header file for the missing track tool class.
 * 
 *  $Log: $
 */
#ifndef MISSING_TRACK_TOOL_H
#define MISSING_TRACK_TOOL_H 1

#include "LArThreeDReco/LArTrackMatching/ThreeDTransverseTracksAlgorithm.h"

namespace lar
{

/**
 *  @brief  MissingTrackTool class
 */
class MissingTrackTool : public TensorManipulationTool
{
public:
    /**
     *  @brief  Factory class for instantiating algorithm tool
     */
    class Factory : public pandora::AlgorithmToolFactory
    {
    public:
        pandora::AlgorithmTool *CreateAlgorithmTool() const;
    };

    bool Run(ThreeDTransverseTracksAlgorithm *pAlgorithm, TensorType &overlapTensor);

private:
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    /**
     *  @brief  Find missing tracks, due to merging of multiple particle deposits into single hits during hit creation
     * 
     *  @param  pAlgorithm address of the calling algorithm
     *  @param  overlapTensor the overlap tensor
     *  @param  protoParticleVector to receive the list of proto particles
     */
    void FindMissingTracks(ThreeDTransverseTracksAlgorithm *pAlgorithm, const TensorType &overlapTensor, ProtoParticleVector &protoParticleVector) const;
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::AlgorithmTool *MissingTrackTool::Factory::CreateAlgorithmTool() const
{
    return new MissingTrackTool();
}

} // namespace lar

#endif // #ifndef MISSING_TRACK_TOOL_H
