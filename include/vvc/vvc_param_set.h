#pragma once

#include <array>
#include <cstring>
#include <list>
#include <map>
#include <vector>
#include <unordered_map>

#include "vvc_type.h"
#include "common_sei.h"

namespace vvc
{
  template <typename T>
  inline T Clip3(const T minVal, const T maxVal, const T a) { return std::min<T>(std::max<T>(minVal, a), maxVal); } ///< general min/max clip

  static constexpr int NOT_VALID = -1;
  static constexpr int MAX_NUM_REF_PICS = 29;          ///< max. number of pictures used for reference
  static constexpr int MAX_NUM_CQP_MAPPING_TABLES = 3; ///< Maximum number of chroma QP mapping tables (Cb, Cr and joint Cb-Cr)
  static constexpr int MAX_VPS_LAYERS = 64;
  static constexpr int MAX_VPS_SUBLAYERS = 7;
  static constexpr int MAX_NUM_OLSS = 256;
  static constexpr int MAX_VPS_OLS_MODE_IDC = 2;
  static constexpr int MAX_QP_OFFSET_LIST_SIZE = 6; ///< Maximum size of QP offset list is 6 entries
  static constexpr int PIC_CODE_CW_BINS = 16;
  static constexpr int MAX_NUM_LONG_TERM_REF_PICS = 33;
  static constexpr int MAX_NUM_REF = 16; ///< max. number of entries in picture reference list
  static constexpr int ALF_CTB_MAX_NUM_APS = 8;
  static constexpr int MAX_NUM_ALF_CLASSES = 25;
  static constexpr int MAX_NUM_ALF_ALTERNATIVES_CHROMA = 8;
  static constexpr int MAX_NUM_ALF_LUMA_COEFF = 13;
  static constexpr int MAX_NUM_ALF_CHROMA_COEFF = 7;
  static constexpr int MAX_NUM_CC_ALF_FILTERS = 4;
  static constexpr int MAX_NUM_CC_ALF_CHROMA_COEFF = 8;
  static constexpr int MAX_NUM_SUB_PICS = (1 << 16);
  static constexpr int MAX_TILE_COLS = 30; // Maximum number of tile columns
  static constexpr int MAX_TILES = 990;    // Maximum number of tiles
  static constexpr int MAX_SLICES = 1000;  // Maximum number of slices per picture
  static constexpr int CCALF_BITS_PER_COEFF_LEVEL = 3;
  static constexpr int MAX_QP = 63;
  static constexpr int MRG_MAX_NUM_CANDS = 6;        ///< MERGE
  static constexpr int AFFINE_MRG_MAX_NUM_CANDS = 5; ///< AFFINE MERGE
  static constexpr int IBC_MRG_MAX_NUM_CANDS = 6;    ///< IBC MERGE
  static constexpr int SCALING_LIST_START_VALUE = 8; ///< start value for dpcm mode
  static constexpr int SCALING_LIST_DC = 16;         ///< default DC value
  static constexpr int MAX_CU_DEPTH = 7;             // log2(CTUSize)
  static constexpr int MAX_CU_SIZE = 1 << MAX_CU_DEPTH;

  struct ScanElement
  {
    uint32_t idx;
    uint16_t x;
    uint16_t y;
  };

  struct OlsHrdParams
  {
    bool m_fixedPicRateGeneralFlag;
    bool m_fixedPicRateWithinCvsFlag;
    uint32_t m_elementDurationInTcMinus1;
    bool m_lowDelayHrdFlag;

    uint32_t m_bitRateValueMinus1[MAX_CPB_CNT][2];
    uint32_t m_cpbSizeValueMinus1[MAX_CPB_CNT][2];
    uint32_t m_ducpbSizeValueMinus1[MAX_CPB_CNT][2];
    uint32_t m_duBitRateValueMinus1[MAX_CPB_CNT][2];
    bool m_cbrFlag[MAX_CPB_CNT][2];
  };
  struct GeneralHrdParams
  {
    uint32_t m_numUnitsInTick;
    uint32_t m_timeScale;
    bool m_generalNalHrdParamsPresentFlag;
    bool m_generalVclHrdParamsPresentFlag;
    bool m_generalSamePicTimingInAllOlsFlag;
    uint32_t m_tickDivisorMinus2;
    bool m_generalDecodingUnitHrdParamsPresentFlag;
    uint32_t m_bitRateScale;
    uint32_t m_cpbSizeScale;
    uint32_t m_cpbSizeDuScale;
    uint32_t m_hrdCpbCntMinus1;
  };
  struct DpbParameters
  {
    int m_maxDecPicBuffering[MAX_TLAYER] = {0};
    int m_maxNumReorderPics[MAX_TLAYER] = {0};
    int m_maxLatencyIncreasePlus1[MAX_TLAYER] = {0};
  };

  struct ReferencePictureList
  {
    int m_numberOfShorttermPictures;
    int m_numberOfLongtermPictures;
    int m_isLongtermRefPic[MAX_NUM_REF_PICS];
    int m_refPicIdentifier[MAX_NUM_REF_PICS]; // This can be delta POC for STRP or POC LSB for LTRP
    int m_POC[MAX_NUM_REF_PICS];
    int m_numberOfActivePictures;
    bool m_deltaPocMSBPresentFlag[MAX_NUM_REF_PICS];
    int m_deltaPOCMSBCycleLT[MAX_NUM_REF_PICS];
    bool m_ltrp_in_slice_header_flag;
    bool m_interLayerPresentFlag;
    bool m_isInterLayerRefPic[MAX_NUM_REF_PICS];
    int m_interLayerRefPicIdx[MAX_NUM_REF_PICS];
    int m_numberOfInterLayerPictures;

    void setRefPicIdentifier(int idx, int identifier, bool isLongterm, bool isInterLayerRefPic, int interLayerIdx);
  };

  struct RPLList
  {
    std::vector<ReferencePictureList> m_referencePictureLists;
    RPLList() {}
    virtual ~RPLList() {}
    void create(int numberOfEntries) { m_referencePictureLists.resize(numberOfEntries); }
    void destroy() {}
    ReferencePictureList *getReferencePictureList(int referencePictureListIdx) { return &m_referencePictureLists[referencePictureListIdx]; }
    const ReferencePictureList *getReferencePictureList(int referencePictureListIdx) const { return &m_referencePictureLists[referencePictureListIdx]; }
    int getNumberOfReferencePictureLists() const { return int(m_referencePictureLists.size()); }
  };

  struct ScalingList
  {
    void outputScalingLists(std::ostream &os) const;
    bool m_scalingListPredModeFlagIsCopy[30]; //!< reference list index
    int m_scalingListDC[30];                  //!< the DC value of the matrix coefficient for 16x16
    uint32_t m_refMatrixId[30];               //!< RefMatrixID
    bool m_scalingListPreditorModeFlag[30];   //!< reference list index
    std::vector<int> m_scalingListCoef[30];   //!< quantization matrix
    bool m_chromaScalingListPresentFlag;

    bool isLumaScalingList(int scalingListId) const;
    int *getScalingListAddress(uint32_t scalingListId) { return &(m_scalingListCoef[scalingListId][0]); }
    int getScalingListDC(uint32_t scalinListId) const { return m_scalingListDC[scalinListId]; }
    void processRefMatrix(uint32_t scalinListId, uint32_t refListId);
    const int* getScalingListDefaultAddress(uint32_t scalingListId);
  };

  struct ConstraintInfo
  {
    bool m_gciPresentFlag;
    bool m_noRprConstraintFlag;
    bool m_noResChangeInClvsConstraintFlag;
    bool m_oneTilePerPicConstraintFlag;
    bool m_picHeaderInSliceHeaderConstraintFlag;
    bool m_oneSlicePerPicConstraintFlag;
    bool m_noIdrRplConstraintFlag;
    bool m_noRectSliceConstraintFlag;
    bool m_oneSlicePerSubpicConstraintFlag;
    bool m_noSubpicInfoConstraintFlag;
    bool m_intraOnlyConstraintFlag;
    uint32_t m_maxBitDepthConstraintIdc;
    int m_maxChromaFormatConstraintIdc;
    bool m_onePictureOnlyConstraintFlag;
    bool m_allLayersIndependentConstraintFlag;
    bool m_noMrlConstraintFlag;
    bool m_noIspConstraintFlag;
    bool m_noMipConstraintFlag;
    bool m_noLfnstConstraintFlag;
    bool m_noMmvdConstraintFlag;
    bool m_noSmvdConstraintFlag;
    bool m_noProfConstraintFlag;
    bool m_noPaletteConstraintFlag;
    bool m_noActConstraintFlag;
    bool m_noLmcsConstraintFlag;
    bool m_noExplicitScaleListConstraintFlag;
    bool m_noVirtualBoundaryConstraintFlag;
    bool m_noMttConstraintFlag;
    bool m_noChromaQpOffsetConstraintFlag;
    bool m_noQtbttDualTreeIntraConstraintFlag;
    int m_maxLog2CtuSizeConstraintIdc;
    bool m_noPartitionConstraintsOverrideConstraintFlag;
    bool m_noSaoConstraintFlag;
    bool m_noAlfConstraintFlag;
    bool m_noCCAlfConstraintFlag;
    bool m_noWeightedPredictionConstraintFlag;
    bool m_noRefWraparoundConstraintFlag;
    bool m_noTemporalMvpConstraintFlag;
    bool m_noSbtmvpConstraintFlag;
    bool m_noAmvrConstraintFlag;
    bool m_noBdofConstraintFlag;
    bool m_noDmvrConstraintFlag;
    bool m_noCclmConstraintFlag;
    bool m_noMtsConstraintFlag;
    bool m_noSbtConstraintFlag;
    bool m_noAffineMotionConstraintFlag;
    bool m_noBcwConstraintFlag;
    bool m_noIbcConstraintFlag;
    bool m_noCiipConstraintFlag;
    bool m_noGeoConstraintFlag;
    bool m_noLadfConstraintFlag;
    bool m_noTransformSkipConstraintFlag;
    bool m_noLumaTransformSize64ConstraintFlag;
    bool m_noBDPCMConstraintFlag;
    bool m_noJointCbCrConstraintFlag;
    bool m_noCuQpDeltaConstraintFlag;
    bool m_noDepQuantConstraintFlag;
    bool m_noSignDataHidingConstraintFlag;
    bool m_noMixedNaluTypesInPicConstraintFlag;
    bool m_noTrailConstraintFlag;
    bool m_noStsaConstraintFlag;
    bool m_noRaslConstraintFlag;
    bool m_noRadlConstraintFlag;
    bool m_noIdrConstraintFlag;
    bool m_noCraConstraintFlag;
    bool m_noGdrConstraintFlag;
    bool m_noApsConstraintFlag;
    bool m_allRapPicturesFlag;
    bool m_noExtendedPrecisionProcessingConstraintFlag;
    bool m_noTsResidualCodingRiceConstraintFlag;
    bool m_noRrcRiceExtensionConstraintFlag;
    bool m_noPersistentRiceAdaptationConstraintFlag;
    bool m_noReverseLastSigCoeffConstraintFlag;
  };

  struct ProfileTierLevel
  {
    Level::Tier m_tierFlag;
    Profile::Name m_profileIdc;
    uint8_t m_numSubProfile;
    std::vector<uint32_t> m_subProfileIdc;
    Level::Name m_levelIdc;
    bool m_frameOnlyConstraintFlag;
    bool m_multiLayerEnabledFlag;
    ConstraintInfo m_constraintInfo;
    std::array<bool, MAX_TLAYER - 1> m_subLayerLevelPresentFlag;
    std::array<Level::Name, MAX_TLAYER> m_subLayerLevelIdc;
  };

  struct LevelTierFeatures
  {
    Level::Name level;
    uint32_t maxLumaPs;
    uint32_t maxCpb[Level::NUMBER_OF_TIERS]; // in units of CpbVclFactor or CpbNalFactor bits
    uint32_t maxSlicesPerAu;
    uint32_t maxTilesPerAu;
    uint32_t maxTileCols;
    uint64_t maxLumaSr;
    uint32_t maxBr[Level::NUMBER_OF_TIERS]; // in units of BrVclFactor or BrNalFactor bits/s
    uint32_t minCrBase[Level::NUMBER_OF_TIERS];
    uint32_t getMaxPicWidthInLumaSamples() const;
    uint32_t getMaxPicHeightInLumaSamples() const;
  };
  struct ProfileFeatures
  {
    Profile::Name profile;
    const char *pNameString;
    uint32_t maxBitDepth;
    ChromaFormat maxChromaFormat;

    bool canUseLevel15p5;
    uint32_t cpbVclFactor;
    uint32_t cpbNalFactor;
    uint32_t formatCapabilityFactorx1000;
    uint32_t minCrScaleFactorx100;
    const LevelTierFeatures *pLevelTiersListInfo;
    bool onePictureOnlyFlagMustBe1;

    static const ProfileFeatures *getProfileFeatures(const Profile::Name p);
  };

  struct SliceReshapeInfo
  {
    bool sliceReshaperEnableFlag;
    bool sliceReshaperModelPresentFlag;
    unsigned enableChromaAdj;
    uint32_t reshaperModelMinBinIdx;
    uint32_t reshaperModelMaxBinIdx;
    int reshaperModelBinCWDelta[PIC_CODE_CW_BINS];
    int maxNbitsNeededDeltaCW;
    int chrResScalingOffset;
  };

  struct ReshapeCW
  {
    std::vector<uint32_t> binCW;
    int updateCtrl;
    int adpOption;
    uint32_t initialCW;
    int rspPicSize;
    int rspFps;
    int rspBaseQP;
    int rspTid;
    int rspSliceQP;
    int rspFpsToIp;
  };

  struct ChromaQpAdj
  {
    union
    {
      struct
      {
        int CbOffset;
        int CrOffset;
        int JointCbCrOffset;
      } comp;
      int offset[3];
    } u;
  };
  struct ChromaQpMappingTableParams
  {
    int m_qpBdOffset;
    bool m_sameCQPTableForAllChromaFlag;
    int m_numQpTables;
    int m_qpTableStartMinus26[MAX_NUM_CQP_MAPPING_TABLES];
    int m_numPtsInCQPTableMinus1[MAX_NUM_CQP_MAPPING_TABLES];
    std::vector<int> m_deltaQpInValMinus1[MAX_NUM_CQP_MAPPING_TABLES];
    std::vector<int> m_deltaQpOutVal[MAX_NUM_CQP_MAPPING_TABLES];

    ChromaQpMappingTableParams()
    {
      m_qpBdOffset = 12;
      m_sameCQPTableForAllChromaFlag = true;
      m_numQpTables = 1;
      m_numPtsInCQPTableMinus1[0] = 0;
      m_qpTableStartMinus26[0] = 0;
      m_deltaQpInValMinus1[0] = {0};
      m_deltaQpOutVal[0] = {0};
    }

    void setSameCQPTableForAllChromaFlag(bool b) { m_sameCQPTableForAllChromaFlag = b; }
    bool getSameCQPTableForAllChromaFlag() const { return m_sameCQPTableForAllChromaFlag; }
    void setNumQpTables(int n) { m_numQpTables = n; }
    int getNumQpTables() const { return m_numQpTables; }
    void setQpTableStartMinus26(int tableIdx, int n) { m_qpTableStartMinus26[tableIdx] = n; }
    int getQpTableStartMinus26(int tableIdx) const { return m_qpTableStartMinus26[tableIdx]; }
    void setNumPtsInCQPTableMinus1(int tableIdx, int n) { m_numPtsInCQPTableMinus1[tableIdx] = n; }
    int getNumPtsInCQPTableMinus1(int tableIdx) const { return m_numPtsInCQPTableMinus1[tableIdx]; }
    void setDeltaQpInValMinus1(int tableIdx, std::vector<int> &inVals) { m_deltaQpInValMinus1[tableIdx] = inVals; }
    void setDeltaQpInValMinus1(int tableIdx, int idx, int n) { m_deltaQpInValMinus1[tableIdx][idx] = n; }
    int getDeltaQpInValMinus1(int tableIdx, int idx) const { return m_deltaQpInValMinus1[tableIdx][idx]; }
    void setDeltaQpOutVal(int tableIdx, std::vector<int> &outVals) { m_deltaQpOutVal[tableIdx] = outVals; }
    void setDeltaQpOutVal(int tableIdx, int idx, int n) { m_deltaQpOutVal[tableIdx][idx] = n; }
    int getDeltaQpOutVal(int tableIdx, int idx) const { return m_deltaQpOutVal[tableIdx][idx]; }
  };

  struct ChromaQpMappingTable : ChromaQpMappingTableParams
  {
    std::map<int, int> m_chromaQpMappingTables[MAX_NUM_CQP_MAPPING_TABLES];

    int getMappedChromaQpValue(ComponentID compID, const int qpVal) const { return m_chromaQpMappingTables[m_sameCQPTableForAllChromaFlag ? 0 : (int)compID - 1].at(qpVal); }
    void derivedChromaQPMappingTables();
    void setParams(const ChromaQpMappingTableParams &params, const int qpBdOffset);
  };

  struct SliceMap
  {
    uint32_t m_sliceID;                     //!< slice identifier (slice index for rectangular slices, slice address for raser-scan slices)
    uint32_t m_numTilesInSlice;             //!< number of tiles in slice (raster-scan slices only)
    uint32_t m_numCtuInSlice;               //!< number of CTUs in the slice
    std::vector<uint32_t> m_ctuAddrInSlice; //!< raster-scan addresses of all the CTUs in the slice
  };

  struct RectSlice
  {
    uint32_t m_tileIdx;            //!< tile index corresponding to the first CTU in the slice
    uint32_t m_sliceWidthInTiles;  //!< slice width in units of tiles
    uint32_t m_sliceHeightInTiles; //!< slice height in units of tiles
    uint32_t m_numSlicesInTile;    //!< number of slices in current tile for the special case of multiple slices inside a single tile
    uint32_t m_sliceHeightInCtu;   //!< slice height in units of CTUs for the special case of multiple slices inside a single tile
  };

  struct SubPic
  {
    uint32_t m_subPicID;                     //!< ID of subpicture
    uint32_t m_subPicIdx;                    //!< Index of subpicture
    uint32_t m_numCTUsInSubPic;              //!< number of CTUs contained in this sub-picture
    uint32_t m_subPicCtuTopLeftX;            //!< horizontal position of top left CTU of the subpicture in unit of CTU
    uint32_t m_subPicCtuTopLeftY;            //!< vertical position of top left CTU of the subpicture in unit of CTU
    uint32_t m_subPicWidth;                  //!< the width of subpicture in units of CTU
    uint32_t m_subPicHeight;                 //!< the height of subpicture in units of CTU
    uint32_t m_subPicWidthInLumaSample;      //!< the width of subpicture in units of luma sample
    uint32_t m_subPicHeightInLumaSample;     //!< the height of subpicture in units of luma sample
    uint32_t m_firstCtuInSubPic;             //!< the raster scan index of the first CTU in a subpicture
    uint32_t m_lastCtuInSubPic;              //!< the raster scan index of the last CTU in a subpicture
    uint32_t m_subPicLeft;                   //!< the position of left boundary
    uint32_t m_subPicRight;                  //!< the position of right boundary
    uint32_t m_subPicTop;                    //!< the position of top boundary
    uint32_t m_subPicBottom;                 //!< the position of bottom boundary
    std::vector<uint32_t> m_ctuAddrInSubPic; //!< raster scan addresses of all the CTUs in the slice

    bool m_treatedAsPicFlag;                  //!< whether the subpicture is treated as a picture in the decoding process excluding in-loop filtering operations
    bool m_loopFilterAcrossSubPicEnabledFlag; //!< whether in-loop filtering operations may be performed across the boundaries of the subpicture
    uint32_t m_numSlicesInSubPic;             //!< Number of slices contained in this subpicture
  };

  struct DCI
  {
    int m_maxSubLayersMinus1;
    std::vector<ProfileTierLevel> m_profileTierLevel;
  };

  struct OPI
  {
    bool m_olsinfopresentflag;
    bool m_htidinfopresentflag;
    uint32_t m_opiolsidx;
    uint32_t m_opihtidplus1;
  };

  struct VPS
  {
    int m_VPSId;
    uint32_t m_maxLayers;

    uint32_t m_vpsMaxSubLayers;
    uint32_t m_vpsLayerId[MAX_VPS_LAYERS];
    bool m_vpsDefaultPtlDpbHrdMaxTidFlag;
    bool m_vpsAllIndependentLayersFlag;
    uint32_t m_vpsCfgPredDirection[MAX_VPS_SUBLAYERS];
    bool m_vpsIndependentLayerFlag[MAX_VPS_LAYERS];
    bool m_vpsDirectRefLayerFlag[MAX_VPS_LAYERS][MAX_VPS_LAYERS];
    std::vector<std::vector<uint32_t>> m_vpsMaxTidIlRefPicsPlus1;
    bool m_vpsEachLayerIsAnOlsFlag;
    uint32_t m_vpsOlsModeIdc;
    uint32_t m_vpsNumOutputLayerSets;
    bool m_vpsOlsOutputLayerFlag[MAX_NUM_OLSS][MAX_VPS_LAYERS];
    uint32_t m_directRefLayerIdx[MAX_VPS_LAYERS][MAX_VPS_LAYERS];
    uint32_t m_generalLayerIdx[MAX_VPS_LAYERS];

    uint32_t m_vpsNumPtls;
    bool m_ptPresentFlag[MAX_NUM_OLSS];
    uint32_t m_ptlMaxTemporalId[MAX_NUM_OLSS];
    std::vector<ProfileTierLevel> m_vpsProfileTierLevel;
    uint32_t m_olsPtlIdx[MAX_NUM_OLSS];

    // stores index ( ilrp_idx within 0 .. NumDirectRefLayers ) of the dependent reference layers
    uint32_t m_interLayerRefIdx[MAX_VPS_LAYERS][MAX_VPS_LAYERS];
    bool m_vpsExtensionFlag;
    bool m_vpsGeneralHrdParamsPresentFlag;
    bool m_vpsSublayerCpbParamsPresentFlag;
    uint32_t m_numOlsTimingHrdParamsMinus1;
    uint32_t m_hrdMaxTid[MAX_NUM_OLSS];
    uint32_t m_olsTimingHrdIdx[MAX_NUM_OLSS];
    GeneralHrdParams m_generalHrdParams;
    std::vector<Size> m_olsDpbPicSize;
    std::vector<int> m_olsDpbParamsIdx;
    std::vector<std::vector<int>> m_outputLayerIdInOls;
    std::vector<std::vector<int>> m_numSubLayersInLayerInOLS;

    std::vector<int> m_multiLayerOlsIdxToOlsIdx; // mapping from multi-layer OLS index to OLS index. Initialized in deriveOutputLayerSets()
                                                 // m_multiLayerOlsIdxToOlsIdx[n] is the OLSidx of the n-th multi-layer OLS.
    std::vector<std::vector<OlsHrdParams>> m_olsHrdParams;
    int m_totalNumOLSs;
    int m_numMultiLayeredOlss;
    uint32_t m_multiLayerOlsIdx[MAX_NUM_OLSS];
    int m_numDpbParams;
    std::vector<DpbParameters> m_dpbParameters;
    bool m_sublayerDpbParamsPresentFlag;
    std::vector<int> m_dpbMaxTemporalId;
    std::vector<int> m_targetOutputLayerIdSet; ///< set of LayerIds to be outputted
    std::vector<int> m_targetLayerIdSet;       ///< set of LayerIds to be included in the sub-bitstream extraction process.
    int m_targetOlsIdx;
    std::vector<int> m_numOutputLayersInOls;
    std::vector<int> m_numLayersInOls;
    std::vector<std::vector<int>> m_layerIdInOls;
    std::vector<int> m_olsDpbChromaFormatIdc;
    std::vector<int> m_olsDpbBitDepthMinus8;

    void deriveOutputLayerSets();
    void checkVPS();

    uint32_t getOlsTimingHrdIdx(int olsIdx) const { return m_olsTimingHrdIdx[olsIdx]; }
    uint32_t getOlsPtlIdx(int idx) const { return m_olsPtlIdx[idx]; }
    uint32_t getHrdMaxTid(int olsIdx) const { return m_hrdMaxTid[olsIdx]; }
    uint32_t getPtlMaxTemporalId(int idx) const { return m_ptlMaxTemporalId[idx]; }
    int getOlsDpbParamsIdx(int olsIdx) const { return m_olsDpbParamsIdx[olsIdx]; }
  };

  struct Window
  {
    bool m_enabledFlag;
    int m_winLeftOffset;
    int m_winRightOffset;
    int m_winTopOffset;
    int m_winBottomOffset;
  };

  struct VUI
  {
    bool m_progressiveSourceFlag;
    bool m_interlacedSourceFlag;
    bool m_nonPackedFlag;
    bool m_nonProjectedFlag;
    bool m_aspectRatioInfoPresentFlag;
    bool m_aspectRatioConstantFlag;
    int m_aspectRatioIdc;
    int m_sarWidth;
    int m_sarHeight;
    bool m_overscanInfoPresentFlag;
    bool m_overscanAppropriateFlag;
    bool m_colourDescriptionPresentFlag;
    int m_colourPrimaries;
    int m_transferCharacteristics;
    int m_matrixCoefficients;
    bool m_videoFullRangeFlag;
    bool m_chromaLocInfoPresentFlag;
    int m_chromaSampleLocTypeTopField;
    int m_chromaSampleLocTypeBottomField;
    int m_chromaSampleLocType;
  };

  struct SPSRExt // Names aligned to text specification
  {
    bool m_transformSkipRotationEnabledFlag;
    bool m_transformSkipContextEnabledFlag;
    bool m_extendedPrecisionProcessingFlag;
    bool m_tsrcRicePresentFlag;
    bool m_highPrecisionOffsetsEnabledFlag;
    bool m_rrcRiceExtensionEnableFlag;
    bool m_persistentRiceAdaptationEnabledFlag;
    bool m_reverseLastSigCoeffEnabledFlag;
    bool m_cabacBypassAlignmentEnabledFlag;
  };

  struct SPS
  {
    int m_SPSId;
    int m_VPSId;
    int m_layerId;
    bool m_affineAmvrEnabledFlag;
    bool m_DMVR;
    bool m_MMVD;
    bool m_SBT;
    bool m_ISP;
    ChromaFormat m_chromaFormatIdc;

    uint32_t m_uiMaxTLayers; // maximum number of temporal layers

    bool m_ptlDpbHrdParamsPresentFlag;
    bool m_SubLayerDpbParamsFlag;

    // Structure
    uint32_t m_maxWidthInLumaSamples;
    uint32_t m_maxHeightInLumaSamples;
    Window m_conformanceWindow;
    bool m_subPicInfoPresentFlag; // indicates the presence of sub-picture info
    uint32_t m_numSubPics;        //!< number of sub-pictures used
    bool m_independentSubPicsFlag;
    bool m_subPicSameSizeFlag;
    std::vector<uint32_t> m_subPicCtuTopLeftX;
    std::vector<uint32_t> m_subPicCtuTopLeftY;
    std::vector<uint32_t> m_subPicWidth;
    std::vector<uint32_t> m_subPicHeight;
    std::vector<bool> m_subPicTreatedAsPicFlag;
    std::vector<bool> m_loopFilterAcrossSubpicEnabledFlag;
    bool m_subPicIdMappingExplicitlySignalledFlag;
    bool m_subPicIdMappingPresentFlag;
    uint32_t m_subPicIdLen;           //!< sub-picture ID length in bits
    std::vector<uint16_t> m_subPicId; //!< sub-picture ID for each sub-picture in the sequence

    int m_log2MinCodingBlockSize;
    unsigned m_CTUSize;
    unsigned m_partitionOverrideEnalbed; // enable partition constraints override function
    unsigned m_minQT[3];                 // 0: I slice luma; 1: P/B slice; 2: I slice chroma
    unsigned m_maxMTTHierarchyDepth[3];
    unsigned m_maxBTSize[3];
    unsigned m_maxTTSize[3];
    bool m_idrRefParamList;
    unsigned m_dualITree;
    uint32_t m_uiMaxCUWidth;
    uint32_t m_uiMaxCUHeight;

    RPLList m_RPLList0;
    RPLList m_RPLList1;
    uint32_t m_numRPL0;
    uint32_t m_numRPL1;

    bool m_rpl1CopyFromRpl0Flag;
    bool m_rpl1IdxPresentFlag;
    bool m_allRplEntriesHasSameSignFlag;
    bool m_bLongTermRefsPresent;
    bool m_SPSTemporalMVPEnabledFlag;
    int m_maxNumReorderPics[MAX_TLAYER];

    // Tool list

    bool m_transformSkipEnabledFlag;
    int m_log2MaxTransformSkipBlockSize;
    bool m_BDPCMEnabledFlag;
    bool m_JointCbCrEnabledFlag;
    // Parameter
    BitDepths m_bitDepths;
    bool m_entropyCodingSyncEnabledFlag; //!< Flag for enabling WPP
    bool m_entryPointPresentFlag;        //!< Flag for indicating the presence of entry points
    int m_qpBDOffset[vvc::MAX_NUM_CHANNEL_TYPE];
    int m_internalMinusInputBitDepth[vvc::MAX_NUM_CHANNEL_TYPE]; //  max(0, internal bitdepth - input bitdepth);                                          }

    bool m_sbtmvpEnabledFlag;
    bool m_bdofEnabledFlag;
    bool m_fpelMmvdEnabledFlag;
    bool m_BdofControlPresentInPhFlag;
    bool m_DmvrControlPresentInPhFlag;
    bool m_ProfControlPresentInPhFlag;
    uint32_t m_uiBitsForPOC;
    bool m_pocMsbCycleFlag;
    uint32_t m_pocMsbCycleLen;
    int m_numExtraPHBytes;
    int m_numExtraSHBytes;

    std::vector<bool> m_extraPHBitPresentFlag;
    std::vector<bool> m_extraSHBitPresentFlag;
    uint32_t m_numLongTermRefPicSPS;
    uint32_t m_ltRefPicPocLsbSps[MAX_NUM_LONG_TERM_REF_PICS];
    bool m_usedByCurrPicLtSPSFlag[MAX_NUM_LONG_TERM_REF_PICS];
    uint32_t m_log2MaxTbSize;
    bool m_useWeightPred;     //!< Use of Weighting Prediction (P_SLICE)
    bool m_useWeightedBiPred; //!< Use of Weighting Bi-Prediction (B_SLICE)

    bool m_saoEnabledFlag;

    bool m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

    bool m_scalingListEnabledFlag;
    bool m_depQuantEnabledFlag;          //!< dependent quantization enabled flag
    bool m_signDataHidingEnabledFlag;    //!< sign data hiding enabled flag
    bool m_virtualBoundariesEnabledFlag; //!< Enable virtual boundaries tool
    bool m_virtualBoundariesPresentFlag; //!< disable loop filtering across virtual boundaries
    unsigned m_numVerVirtualBoundaries;  //!< number of vertical virtual boundaries
    unsigned m_numHorVirtualBoundaries;  //!< number of horizontal virtual boundaries
    unsigned m_virtualBoundariesPosX[3]; //!< horizontal position of each vertical virtual boundary
    unsigned m_virtualBoundariesPosY[3]; //!< vertical position of each horizontal virtual boundary
    uint32_t m_maxDecPicBuffering[MAX_TLAYER];
    uint32_t m_maxLatencyIncreasePlus1[MAX_TLAYER];

    bool m_generalHrdParametersPresentFlag;
    GeneralHrdParams m_generalHrdParams;
    OlsHrdParams m_olsHrdParams[MAX_TLAYER];

    bool m_fieldSeqFlag;
    bool m_vuiParametersPresentFlag;
    unsigned m_vuiPayloadSize;
    VUI m_vuiParameters;

    SPSRExt m_spsRangeExtension;

    static const int m_winUnitX[vvc::NUM_CHROMA_FORMAT];
    static const int m_winUnitY[vvc::NUM_CHROMA_FORMAT];
    ProfileTierLevel m_profileTierLevel;

    bool m_alfEnabledFlag;
    bool m_ccalfEnabledFlag;
    bool m_wrapAroundEnabledFlag;
    unsigned m_IBCFlag;
    bool m_useColorTrans;
    unsigned m_PLTMode;

    bool m_lmcsEnabled;
    bool m_AMVREnabledFlag;
    bool m_LMChroma;
    bool m_horCollocatedChromaFlag;
    bool m_verCollocatedChromaFlag;
    bool m_mtsEnabled{false};
    bool m_explicitMtsIntra{false};
    bool m_explicitMtsInter{false};
    bool m_LFNST;
    bool m_SMVD;
    bool m_Affine;
    bool m_AffineType;
    bool m_PROF;
    bool m_bcw;
    bool m_ciip;
    bool m_Geo;
    bool m_MRL;
    bool m_MIP;
    ChromaQpMappingTable m_chromaQpMappingTable;
    bool m_GDREnabledFlag;
    bool m_SubLayerCbpParametersPresentFlag;

    bool m_rprEnabledFlag;
    bool m_resChangeInClvsEnabledFlag;
    bool m_interLayerPresentFlag;

    uint32_t m_log2ParallelMergeLevelMinus2;
    bool m_ppsValidFlag[64];
    Size m_scalingWindowSizeInPPS[64];
    uint32_t m_maxNumMergeCand;
    uint32_t m_maxNumAffineMergeCand;
    uint32_t m_maxNumIBCMergeCand;
    uint32_t m_maxNumGeoCand;
    bool m_scalingMatrixAlternativeColourSpaceDisabledFlag;
    bool m_scalingMatrixDesignatedColourSpaceFlag;

    bool m_disableScalingMatrixForLfnstBlks;

    void setChromaQpMappingTableFromParams(const ChromaQpMappingTableParams &params, const int qpBdOffset) { m_chromaQpMappingTable.setParams(params, qpBdOffset); }
    void derivedChromaQPMappingTables() { m_chromaQpMappingTable.derivedChromaQPMappingTables(); }
    void createRPLList0(int numRPL);
    void createRPLList1(int numRPL);
  };

  struct PPS
  {
    int m_PPSId; // pic_parameter_set_id
    int m_SPSId; // seq_parameter_set_id
    int m_picInitQPMinus26;
    bool m_useDQP;
    bool m_usePPSChromaTool;
    bool m_bSliceChromaQpFlag; // slicelevel_chroma_qp_flag

    int m_layerId;
    int m_temporalId;
    int m_puCounter;

    // access channel

    int m_chromaCbQpOffset;
    int m_chromaCrQpOffset;
    bool m_chromaJointCbCrQpOffsetPresentFlag;
    int m_chromaCbCrQpOffset;

    // Chroma QP Adjustments
    int m_chromaQpOffsetListLen;                                                   // size (excludes the null entry used in the following array).
    ChromaQpAdj m_ChromaQpAdjTableIncludingNullEntry[1 + MAX_QP_OFFSET_LIST_SIZE]; //!< Array includes entry [0] for the null offset used when cu_chroma_qp_offset_flag=0, and entries [cu_chroma_qp_offset_idx+1...] otherwise

    uint32_t m_numRefIdxL0DefaultActive;
    uint32_t m_numRefIdxL1DefaultActive;

    bool m_rpl1IdxPresentFlag;

    bool m_bUseWeightPred;        //!< Use of Weighting Prediction (P_SLICE)
    bool m_useWeightedBiPred;     //!< Use of Weighting Bi-Prediction (B_SLICE)
    bool m_OutputFlagPresentFlag; //!< Indicates the presence of output_flag in slice header
    uint32_t m_numSubPics;        //!< number of sub-pictures used - must match SPS
    bool m_subPicIdMappingInPpsFlag;
    uint32_t m_subPicIdLen;                   //!< sub-picture ID length in bits
    std::vector<uint16_t> m_subPicId;         //!< sub-picture ID for each sub-picture in the sequence
    bool m_noPicPartitionFlag;                //!< no picture partitioning flag - single slice, single tile
    uint8_t m_log2CtuSize;                    //!< log2 of the CTU size - required to match corresponding value in SPS
    uint8_t m_ctuSize;                        //!< CTU size
    uint32_t m_picWidthInCtu;                 //!< picture width in units of CTUs
    uint32_t m_picHeightInCtu;                //!< picture height in units of CTUs
    uint32_t m_numExpTileCols;                //!< number of explicitly specified tile columns
    uint32_t m_numExpTileRows;                //!< number of explicitly specified tile rows
    uint32_t m_numTileCols;                   //!< number of tile columns
    uint32_t m_numTileRows;                   //!< number of tile rows
    std::vector<uint32_t> m_tileColWidth;     //!< tile column widths in units of CTUs
    std::vector<uint32_t> m_tileRowHeight;    //!< tile row heights in units of CTUs
    std::vector<uint32_t> m_tileColBd;        //!< tile column left-boundaries in units of CTUs
    std::vector<uint32_t> m_tileRowBd;        //!< tile row top-boundaries in units of CTUs
    std::vector<uint32_t> m_ctuToTileCol;     //!< mapping between CTU horizontal address and tile column index
    std::vector<uint32_t> m_ctuToTileRow;     //!< mapping between CTU vertical address and tile row index
    bool m_rectSliceFlag;                     //!< rectangular slice flag
    bool m_singleSlicePerSubPicFlag;          //!< single slice per sub-picture flag
    std::vector<uint32_t> m_ctuToSubPicIdx;   //!< mapping between CTU and Sub-picture index
    uint32_t m_numSlicesInPic;                //!< number of rectangular slices in the picture (raster-scan slice specified at slice level)
    bool m_tileIdxDeltaPresentFlag;           //!< tile index delta present flag
    std::vector<RectSlice> m_rectSlices;      //!< list of rectangular slice signalling parameters
    std::vector<SliceMap> m_sliceMap;         //!< list of CTU maps for each slice in the picture
    std::vector<SubPic> m_subPics;            //!< list of subpictures in the picture
    bool m_loopFilterAcrossTilesEnabledFlag;  //!< loop filtering applied across tiles flag
    bool m_loopFilterAcrossSlicesEnabledFlag; //!< loop filtering applied across slices flag

    bool m_cabacInitPresentFlag;

    bool m_pictureHeaderExtensionPresentFlag; //< picture header extension flags present in picture headers or not
    bool m_sliceHeaderExtensionPresentFlag;
    bool m_deblockingFilterControlPresentFlag;
    bool m_deblockingFilterOverrideEnabledFlag;
    bool m_ppsDeblockingFilterDisabledFlag;
    int m_deblockingFilterBetaOffsetDiv2;   //< beta offset for deblocking filter
    int m_deblockingFilterTcOffsetDiv2;     //< tc offset for deblocking filter
    int m_deblockingFilterCbBetaOffsetDiv2; //< beta offset for Cb deblocking filter
    int m_deblockingFilterCbTcOffsetDiv2;   //< tc offset for Cb deblocking filter
    int m_deblockingFilterCrBetaOffsetDiv2; //< beta offset for Cr deblocking filter
    int m_deblockingFilterCrTcOffsetDiv2;   //< tc offset for Cr deblocking filter
    bool m_listsModificationPresentFlag;

    bool m_rplInfoInPhFlag;
    bool m_dbfInfoInPhFlag;
    bool m_saoInfoInPhFlag;
    bool m_alfInfoInPhFlag;
    bool m_wpInfoInPhFlag;
    bool m_qpDeltaInfoInPhFlag;
    bool m_mixedNaluTypesInPicFlag;

    bool m_conformanceWindowFlag;
    uint32_t m_picWidthInLumaSamples;
    uint32_t m_picHeightInLumaSamples;
    Window m_conformanceWindow;
    bool m_explicitScalingWindowFlag;
    Window m_scalingWindow;

    bool m_wrapAroundEnabledFlag;             //< reference wrap around enabled or not
    unsigned m_picWidthMinusWrapAroundOffset; // <pic_width_in_minCbSizeY - wraparound_offset_in_minCbSizeY
    unsigned m_wrapAroundOffset;              //< reference wrap around offset in luma samples

    void initTiles();
    void resetTileSliceInfo();
    void initRectSlices();
    void setChromaQpOffsetListEntry(int cuChromaQpOffsetIdxPlus1, int cbOffset, int crOffset, int jointCbCrOffset);
  };

  enum AlfFilterType
  {
    ALF_FILTER_5,
    ALF_FILTER_7,
    CC_ALF,
    ALF_NUM_OF_FILTER_TYPES
  };
  
  static const int size_CC_ALF = -1;
  struct AlfFilterShape
  {
    AlfFilterShape(int size) : filterLength(size), numCoeff(size * size / 4 + 1)
    {
      if (size == 5)
      {
        pattern = {
            0,
            1, 2, 3,
            4, 5, 6, 5, 4,
            3, 2, 1,
            0};

        filterType = ALF_FILTER_5;
      }
      else if (size == 7)
      {
        pattern = {
            0,
            1, 2, 3,
            4, 5, 6, 7, 8,
            9, 10, 11, 12, 11, 10, 9,
            8, 7, 6, 5, 4,
            3, 2, 1,
            0};

        filterType = ALF_FILTER_7;
      }
      else if (size == size_CC_ALF)
      {
        size = 4;
        filterLength = 8;
        numCoeff = 8;
        filterType = CC_ALF;
      }
      else
      {
        filterType = ALF_NUM_OF_FILTER_TYPES;
      }
    }

    AlfFilterType filterType;
    int filterLength;
    int numCoeff;
    std::vector<int> pattern;
  };

  struct AlfParam
  {
    bool enabledFlag[MAX_NUM_COMPONENT];                                             // alf_slice_enable_flag, alf_chroma_idc
    bool nonLinearFlag[MAX_NUM_CHANNEL_TYPE];                                        // alf_[luma/chroma]_clip_flag
    short lumaCoeff[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF];                   // alf_coeff_luma_delta[i][j]
    uint16_t lumaClipp[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF];                // alf_clipp_luma_[i][j]
    int numAlternativesChroma;                                                       // alf_chroma_num_alts_minus_one + 1
    short chromaCoeff[MAX_NUM_ALF_ALTERNATIVES_CHROMA][MAX_NUM_ALF_CHROMA_COEFF];    // alf_coeff_chroma[i]
    uint16_t chromaClipp[MAX_NUM_ALF_ALTERNATIVES_CHROMA][MAX_NUM_ALF_CHROMA_COEFF]; // alf_clipp_chroma[i]
    short filterCoeffDeltaIdx[MAX_NUM_ALF_CLASSES];                                  // filter_coeff_delta[i]
    bool alfLumaCoeffFlag[MAX_NUM_ALF_CLASSES];                                      // alf_luma_coeff_flag[i]
    int numLumaFilters;                                                              // number_of_filters_minus1 + 1
    bool alfLumaCoeffDeltaFlag;                                                      // alf_luma_coeff_delta_flag
    std::vector<AlfFilterShape> *filterShapes;
    bool newFilterFlag[MAX_NUM_CHANNEL_TYPE];

    void reset();
  };

  struct CcAlfFilterParam
  {
    bool ccAlfFilterEnabled[2];
    bool ccAlfFilterIdxEnabled[2][MAX_NUM_CC_ALF_FILTERS];
    uint8_t ccAlfFilterCount[2];
    short ccAlfCoeff[2][MAX_NUM_CC_ALF_FILTERS][MAX_NUM_CC_ALF_CHROMA_COEFF];
    int newCcAlfFilter[2];
    int numberValidComponents;
  };
  struct APS
  {
    int m_APSId; // adaptation_parameter_set_id
    int m_temporalId;
    int m_puCounter;
    int m_layerId;
    ApsType m_APSType; // aps_params_type
    AlfParam m_alfAPSParam;
    SliceReshapeInfo m_reshapeAPSInfo;
    ScalingList m_scalingListApsInfo;
    CcAlfFilterParam m_ccAlfAPSParam;
    bool m_hasPrefixNalUnitType;
    bool chromaPresentFlag;
  };

  struct WPScalingParam
  {
    // Explicit weighted prediction parameters parsed in slice header,
    // or Implicit weighted prediction parameters (8 bits depth values).
    bool presentFlag;
    uint32_t log2WeightDenom;
    int codedWeight;
    int codedOffset;

    // Weighted prediction scaling values built from above parameters (bitdepth scaled):
    int w;
    int o;
    int offset;
    int shift;
    int round;
  };

  struct HRD
  {
    GeneralHrdParams m_generalHrdParams;
    OlsHrdParams m_olsHrdParams[MAX_TLAYER];
    bool m_bufferingPeriodInitialized;
    SEIBufferingPeriod m_bufferingPeriodSEI;
    bool m_pictureTimingAvailable;
    SEIPictureTimingH266 m_pictureTimingSEI;
  };
}