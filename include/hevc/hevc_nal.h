#pragma once

#include "hevc_vlc.h"
#include "nal_parse.h"

class parseNalH265 : public parseLib<hevc::sps, hevc::pps>, public SyntaxElementParser, public TComInputBitstream
{
public:
  parseNalH265(){};
  virtual ~parseNalH265()
  {
    if (m_bits)
    {
      delete m_bits;
      m_bits = nullptr;
    }
  };

protected:
  void parseShortTermRefPicSet(hevc::sps *pcSPS, hevc::TComReferencePictureSet *pcRPS, int idx);
  void parseVUI(hevc::TComVUI *pcVUI, hevc::sps *pcSPS);
  void parsePTL(hevc::TComPTL *rpcPTL, bool profilePresentFlag, int maxNumSubLayersMinus1);
  void parseProfileTier(hevc::ProfileTierLevel *ptl, const bool bIsSubLayer);
  void parseHrdParameters(hevc::TComHRD *hrd, bool cprms_present_flag, unsigned int tempLevelHigh);
  void parseScalingList(hevc::TComScalingList *scalingList);

public:
  void setBitstream(TComInputBitstream *p) { m_pcBitstream = p; }
  void sps_parse(unsigned char *nal_bitstream, hevc::sps *pcSPS, int curLen, parsingLevel level) override;
  void pps_parse(unsigned char *nal_bitstream, hevc::pps *pcPPS, hevc::sps *pcSPS, int curLen, parsingLevel level) override;
  void sei_parse(unsigned char *nal_bitstream, nal_info &nal, int curLen) override;

private:
  void sortDeltaPOC();
  void xDecodeScalingList(hevc::TComScalingList *scalingList, unsigned int sizeId, unsigned int listId);
  TComInputBitstream *m_bits{new TComInputBitstream};

protected:
  bool xMoreRbspData();
};

class parseSeiH265 : public SyntaxElementParser, public TComInputBitstream
{
public:
  parseSeiH265(){};
  virtual ~parseSeiH265(){};

public:
  void xReadSEIPayloadData(int payloadType, int payloadSize, nal_info &nal, hevc::hevc_nal_type nalUnitType, hevc::sps *sps, TComInputBitstream *bits);

  void xParseSEIBufferingPeriod(SEIBufferingPeriod &sei, unsigned int payloadSize, const hevc::sps *sps);
  void xParseSEIPictureTiming(SEIPictureTimingH265 &sei, unsigned int payloadSize, const hevc::sps *sps);
  // void xParseSEIPanScanRect(SEIPanScanRect &sei, unsigned int payloadSize);
  // void xParseSEIFillerPayload(SEIFillerPayload &sei, unsigned int payloadSize);
  void xParseSEIUserDataRegistered(SEIUserDataRegistered &sei, unsigned int payloadSize);
  void xParseSEIUserDataUnregistered(SEIUserDataUnregistered &sei, unsigned int payloadSize);
  // void xParseSEIRecoveryPoint(SEIRecoveryPoint &sei, unsigned int payloadSize);
  // void xParseSEISceneInfo(SEISceneInfo &sei, unsigned int payloadSize);
  // void xParseSEIPictureSnapshot(SEIPictureSnapshot &sei, unsigned int payloadSize);
  // void xParseSEIProgressiveRefinementSegmentStart(SEIProgressiveRefinementSegmentStart &sei, unsigned int payloadSize);
  // void xParseSEIProgressiveRefinementSegmentEnd(SEIProgressiveRefinementSegmentEnd &sei, unsigned int payloadSize);
  // void xParseSEIFilmGrainCharacteristics(SEIFilmGrainCharacteristics &sei, unsigned int payloadSize);
  // void xParseSEIPostFilterHint(SEIPostFilterHint &sei, unsigned int payloadSize, const sps *sps);
  // void xParseSEIToneMappingInfo(SEIToneMappingInfo &sei, unsigned int payloadSize);
  // void xParseSEIFramePacking(SEIFramePacking &sei, unsigned int payloadSize);
  // void xParseSEIDisplayOrientation(SEIDisplayOrientation &sei, unsigned int payloadSize);
  // void xParseSEIGreenMetadataInfo(SEIGreenMetadataInfo &sei, unsigned int payLoadSize);
  // void xParseSEISOPDescription(SEISOPDescription &sei, unsigned int payloadSize);
  // void xParseSEIActiveParameterSets(SEIActiveParameterSets &sei, unsigned int payloadSize);
  // void xParseSEIDecodingUnitInfo(SEIDecodingUnitInfo &sei, unsigned int payloadSize, const sps *sps);
  // void xParseSEITemporalLevel0Index(SEITemporalLevel0Index &sei, unsigned int payloadSize);
  // void xParseSEIDecodedPictureHash(SEIDecodedPictureHash &sei, unsigned int payloadSize);
  // void xParseSEIScalableNesting(SEIScalableNesting &sei, const NalUnitType nalUnitType, unsigned int payloadSize, const sps *sps);
  // void xParseSEIRegionRefreshInfo(SEIRegionRefreshInfo &sei, unsigned int payloadSize);
  // void xParseSEINoDisplay(SEINoDisplay &sei, unsigned int payloadSize);
  void xParseSEITimeCode(SEITimeCode &sei, unsigned int payloadSize);
  // void xParseSEIMasteringDisplayColourVolume(SEIMasteringDisplayColourVolume &sei, unsigned int payloadSize);
  // void xParseSEISegmentedRectFramePacking(SEISegmentedRectFramePacking &sei, unsigned int payloadSize);
  // void xParseSEITempMotionConstraintsTileSets(SEITempMotionConstrainedTileSets &sei, unsigned int payloadSize);
  // void xParseSEIMCTSExtractionInfoSet(SEIMCTSExtractionInfoSet &sei, unsigned int payloadSize);
  // void xParseSEIChromaResamplingFilterHint(SEIChromaResamplingFilterHint &sei, unsigned int payloadSize);
  // void xParseSEIKneeFunctionInfo(SEIKneeFunctionInfo &sei, unsigned int payloadSize);
  // void xParseSEIContentColourVolume(SEIContentColourVolume &sei, unsigned int payloadSize);
  // void xParseSEIEquirectangularProjection(SEIEquirectangularProjection &sei, unsigned int payloadSize);
  // void xParseSEISphereRotation(SEISphereRotation &sei, unsigned int payloadSize);
  // void xParseSEIOmniViewport(SEIOmniViewport &sei, unsigned int payloadSize);
  // void xParseSEICubemapProjection(SEICubemapProjection &sei, unsigned int payloadSize);
  // void xParseSEIAnnotatedRegions(SEIAnnotatedRegions &sei, unsigned int payloadSize);
  // void xParseSEIRegionWisePacking(SEIRegionWisePacking &sei, unsigned int payloadSize);
  // void xParseSEIFisheyeVideoInfo(SEIFisheyeVideoInfo &sei, unsigned int payloadSize);
  // void xParseSEIColourRemappingInfo(SEIColourRemappingInfo &sei, unsigned int payloadSize);
  // void xParseSEIDeinterlaceFieldIdentification(SEIDeinterlaceFieldIdentification &sei, unsigned int payLoadSize);
  // void xParseSEIContentLightLevelInfo(SEIContentLightLevelInfo &sei, unsigned int payLoadSize);
  // void xParseSEIDependentRAPIndication(SEIDependentRAPIndication &sei, unsigned int payLoadSize);
  // void xParseSEICodedRegionCompletion(SEICodedRegionCompletion &sei, unsigned int payLoadSize);
  // void xParseSEIAlternativeTransferCharacteristics(SEIAlternativeTransferCharacteristics &sei, unsigned int payLoadSize);
  // void xParseSEIAmbientViewingEnvironment(SEIAmbientViewingEnvironment &sei, unsigned int payLoadSize);
  // void xParseSEIRegionalNesting(SEIRegionalNesting &sei, unsigned int payloadSize, const sps *sps);
  // void xParseSEIShutterInterval(SEIShutterIntervalInfo &sei, unsigned int payloadSize);
};