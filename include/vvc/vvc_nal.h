#pragma once

#include "vvc_vlc.h"
#include "nal_parse.h"

class parseNalH266 : public parseLib<vvc::VPS, vvc::SPS, vvc::PPS>, public VLCReader, public InputBitstream
{
public:
  parseNalH266(){};
  virtual ~parseNalH266()
  {
    if (m_bits)
    {
      delete m_bits;
      m_bits = nullptr;
    }
  };

protected:
  void copyRefPicList(vvc::SPS *pcSPS, vvc::ReferencePictureList *source_rpl, vvc::ReferencePictureList *dest_rpl);
  void parseRefPicList(vvc::SPS *pcSPS, vvc::ReferencePictureList *rpl, int rplIdx);

protected:
  void setBitstream(InputBitstream *p) { m_pcBitstream = p; }
  void parseOPI(vvc::OPI *opi);
  void parseVPS(vvc::VPS *pcVPS);
  void parseDCI(vvc::DCI *dci);
  void parseVUI(vvc::VUI *pcVUI, vvc::SPS *pcSPS);
  void parseConstraintInfo(vvc::ConstraintInfo *cinfo, const vvc::ProfileTierLevel *ptl);
  void parseProfileTierLevel(vvc::ProfileTierLevel *ptl, bool profileTierPresentFlag, int maxNumSubLayersMinus1);
  void parseOlsHrdParameters(vvc::GeneralHrdParams *generalHrd, vvc::OlsHrdParams *olsHrd, uint32_t firstSubLayer, uint32_t tempLevelHigh);
  void parseGeneralHrdParameters(vvc::GeneralHrdParams *generalHrd);
  void parseTerminatingBit(uint32_t &ruiBit);
  void parseRemainingBytes(bool noTrailingBytesExpected);
  void parseScalingList(vvc::ScalingList *scalingList, bool aps_chromaPresentFlag);
  void decodeScalingList(vvc::ScalingList *scalingList, uint32_t scalingListId, bool isPredictor);
  void dpb_parameters(int maxSubLayersMinus1, bool subLayerInfoFlag, vvc::SPS *pcSPS);
  void alfFilter(vvc::AlfParam &alfParam, const bool isChroma, const int altIdx);

public:
  void vps_parse(unsigned char *nal_bitstream, vvc::VPS *pcVPS, int curLen, parsingLevel level) override;
  void sps_parse(unsigned char *nal_bitstream, vvc::SPS *pcSPS, int curLen, parsingLevel level) override;
  void pps_parse(unsigned char *nal_bitstream, vvc::PPS *pcPPS, vvc::SPS *pcSPS, int curLen, parsingLevel level) override;
  void aps_parse(unsigned char *nal_bitstream, vvc::APS *aps, int curLen, parsingLevel level);
  void sei_parse(unsigned char *nal_bitstream, nal_info &nal, int curLen) override;
  void alf_aps_parse(vvc::APS *aps);
  void lmcs_aps_parse(vvc::APS *aps);
  void scalinglist_aps_parse(vvc::APS *aps);
protected:
  bool xMoreRbspData();

private:  
  InputBitstream *m_bits{new InputBitstream};
};

class parseSeiH266 : public VLCReader, public InputBitstream
{
public:
  parseSeiH266(){};
  virtual ~parseSeiH266(){};
  // void parseSEImessage(InputBitstream *bs, SEIMessages &seis, const NalUnitType nalUnitType, const uint32_t nuh_layer_id, const uint32_t temporalId, const VPS *vps, const SPS *sps, HRD &hrd, );
  // void parseAndExtractSEIScalableNesting(InputBitstream *bs, const NalUnitType nalUnitType, const uint32_t nuh_layer_id, const VPS *vps, const SPS *sps, HRD &hrd, uint32_t payloadSize, std::vector<std::tuple<int, int, bool, uint32_t, uint8_t *, int, int>> *seiList);
  // void getSEIDecodingUnitInfoDuiIdx(InputBitstream *bs, const NalUnitType nalUnitType, const uint32_t nuh_layer_id, HRD &hrd, uint32_t payloadSize, int &duiIdx);

protected:
  // void xParseSEIuserDataUnregistered          (SEIuserDataUnregistered &sei,          uint32_t payloadSize,                      );
  // void xParseSEIDecodingUnitInfo              (SEIDecodingUnitInfo& sei,              uint32_t payloadSize, const SEIBufferingPeriod& bp, const uint32_t temporalId,  );
  // void xParseSEIDecodedPictureHash            (SEIDecodedPictureHash& sei,            uint32_t payloadSize,                      );
  void xParseSEIBufferingPeriod(SEIBufferingPeriod &sei, uint32_t payloadSize);
  void xParseSEIPictureTiming(SEIPictureTimingH266 &sei, uint32_t payloadSize, const uint32_t temporalId, const SEIBufferingPeriod &bp);
  // void xParseSEIScalableNesting               (SEIScalableNesting& sei, const NalUnitType nalUnitType, const uint32_t nuhLayerId, uint32_t payloadSize, const VPS* vps, const SPS* sps, HRD &hrd, std::ostream* decodedMessageOutputStream);
  // void xParseSEIScalableNestingBinary         (SEIScalableNesting& sei, const NalUnitType nalUnitType, const uint32_t nuhLayerId, uint32_t payloadSize, const VPS* vps, const SPS* sps, HRD &hrd, std::ostream* decodedMessageOutputStream, std::vector<std::tuple<int, int, bool, uint32_t, uint8_t*, int, int>> *seiList);
  // void xCheckScalableNestingConstraints       (const SEIScalableNesting& sei, const NalUnitType nalUnitType, const VPS* vps);
  // void xParseSEIFrameFieldinfo                (SEIFrameFieldInfo& sei,                uint32_t payloadSize,  );
  // void xParseSEIGreenMetadataInfo             (SEIGreenMetadataInfo& sei,             uint32_t payLoadSize,                      );
  // void xParseSEIDependentRAPIndication        (SEIDependentRAPIndication& sei,        uint32_t payLoadSize,                      );
  // void xParseSEIFramePacking                  (SEIFramePacking& sei,                  uint32_t payloadSize,                      );
  // void xParseSEIDisplayOrientation            (SEIDisplayOrientation& sei,            uint32_t payloadSize,                     std::ostream* pDecodedMessageOutputStream);
  // void xParseSEIParameterSetsInclusionIndication(SEIParameterSetsInclusionIndication& sei, uint32_t payloadSize,                std::ostream* pDecodedMessageOutputStream);
  void xParseSEIMasteringDisplayColourVolume(SEIMasteringDisplayColourVolume &sei, uint32_t payloadSize);
  // void xParseSEIAnnotatedRegions              (SEIAnnotatedRegions& sei,              uint32_t payloadSize,                      );
  // void xParseSEIEquirectangularProjection     (SEIEquirectangularProjection &sei,     uint32_t payloadSize,                      );
  // void xParseSEISphereRotation                (SEISphereRotation &sei,                uint32_t payloadSize,                      );
  // void xParseSEIOmniViewport                  (SEIOmniViewport& sei,                  uint32_t payloadSize,                      );
  // void xParseSEIRegionWisePacking             (SEIRegionWisePacking& sei,             uint32_t payloadSize,                      );
  // void xParseSEIGeneralizedCubemapProjection  (SEIGeneralizedCubemapProjection &sei,  uint32_t payloadSize,                      );
  // void xParseSEIScalabilityDimensionInfo      (SEIScalabilityDimensionInfo& sei,      uint32_t payloadSize,  );
  // void xParseSEIMultiviewAcquisitionInfo      (SEIMultiviewAcquisitionInfo& sei,      uint32_t payloadSize,  );
  // void xParseSEIMultiviewViewPosition         (SEIMultiviewViewPosition& sei,         uint32_t payloadSize,  );
  // void xParseSEIAlphaChannelInfo              (SEIAlphaChannelInfo& sei,              uint32_t payloadSize,                      );
  // void xParseSEIDepthRepresentationInfo       (SEIDepthRepresentationInfo& sei,       uint32_t payloadSize,  );
  void xParseSEIDepthRepInfoElement(double &f);
  // void xParseSEISubpictureLevelInfo           (SEISubpicureLevelInfo& sei,            uint32_t payloadSize,                      );
  // void xParseSEISampleAspectRatioInfo         (SEISampleAspectRatioInfo& sei,         uint32_t payloadSize,                      );
  void xParseSEIUserDataRegistered(SEIUserDataRegistered &sei, uint32_t payloadSize);
  // void xParseSEIFilmGrainCharacteristics      (SEIFilmGrainCharacteristics& sei,      uint32_t payloadSize,                      );
  void xParseSEIContentLightLevelInfo(SEIContentLightLevelInfo &sei, uint32_t payloadSize);
  // void xParseSEIAmbientViewingEnvironment     (SEIAmbientViewingEnvironment& sei,     uint32_t payloadSize,                      );
  // void xParseSEIContentColourVolume           (SEIContentColourVolume& sei,           uint32_t payloadSize,                      );
  // void xParseSEIExtendedDrapIndication        (SEIExtendedDrapIndication& sei,        uint32_t payloadSize,                      );
  // void xParseSEIColourTransformInfo           (SEIColourTransformInfo& sei, uint32_t payloadSize, std::ostream* pDecodedMessageOutputStream);
  // void xParseSEIConstrainedRaslIndication     (SEIConstrainedRaslIndication& sei,     uint32_t payLoadSize,                      );
  // void xParseSEIShutterInterval(SEIShutterIntervalInfo& sei, uint32_t payloadSize,  );
  // void xParseSEINNPostFilterCharacteristics(SEINeuralNetworkPostFilterCharacteristics& sei, uint32_t payloadSize,  );
  // void xParseSEINNPostFilterActivation(SEINeuralNetworkPostFilterActivation& sei, uint32_t payloadSize,  );

  void sei_read_scode(uint32_t length, int &code, const char *pSymbolName);
  void sei_read_code(uint32_t length, uint32_t &ruiCode, const char *pSymbolName);
  void sei_read_uvlc(uint32_t &ruiCode, const char *pSymbolName);
  void sei_read_svlc(int &ruiCode, const char *pSymbolName);
  void sei_read_flag(uint32_t &ruiCode, const char *pSymbolName);
};