#pragma once

#include "common_sei.h"

#include <vector>
#include <algorithm>
#include <cstring>
#include <cassert>

namespace hevc
{
  static const unsigned int REF_PIC_LIST_NUM_IDX = 32;
  static const int MAX_NUM_REF_PICS = 16;
  static const int MAX_NUM_LONG_TERM_REF_PICS = 33;
  static const int SCALING_LIST_NUM = 6;
  static const int MAX_MATRIX_COEF_NUM = 64;
  static const int SCALING_LIST_START_VALUE = 8;
  static const int MAX_CU_DEPTH = 6;
  static const int MAX_TLAYER = 7;
  static const int MAX_CPB_CNT = 32;
  static const int MAX_QP_OFFSET_LIST_SIZE = 6;
  static const int MAX_VPS_OP_SETS_PLUS1 = 1024;
  static const int MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 = 1;
  namespace Level
  {
    enum Tier
    {
      MAIN = 0,
      HIGH = 1,
      NUMBER_OF_TIERS = 2
    };

    enum Name
    {
      NONE = 0,
      LEVEL1 = 30,
      LEVEL2 = 60,
      LEVEL2_1 = 63,
      LEVEL3 = 90,
      LEVEL3_1 = 93,
      LEVEL4 = 120,
      LEVEL4_1 = 123,
      LEVEL5 = 150,
      LEVEL5_1 = 153,
      LEVEL5_2 = 156,
      LEVEL6 = 180,
      LEVEL6_1 = 183,
      LEVEL6_2 = 186,
      LEVEL8_5 = 255,
    };
  }

  namespace Profile
  {
    enum Name
    {
      NONE = 0,
      MAIN = 1,
      MAIN10 = 2,
      MAINSTILLPICTURE = 3,
      MAINREXT = 4,
      HIGHTHROUGHPUTREXT = 5
    };
  }

  enum ChromaFormat
  {
    CHROMA_400 = 0,
    CHROMA_420 = 1,
    CHROMA_422 = 2,
    CHROMA_444 = 3,
    NUM_CHROMA_FORMAT = 4
  };

  enum ChannelType
  {
    CHANNEL_TYPE_LUMA = 0,
    CHANNEL_TYPE_CHROMA = 1,
    MAX_NUM_CHANNEL_TYPE = 2
  };

  enum RDPCMSignallingMode
  {
    RDPCM_SIGNAL_IMPLICIT = 0,
    RDPCM_SIGNAL_EXPLICIT = 1,
    NUMBER_OF_RDPCM_SIGNALLING_MODES = 2
  };

  enum ScalingListSize
  {
    SCALING_LIST_4x4 = 0,
    SCALING_LIST_8x8,
    SCALING_LIST_16x16,
    SCALING_LIST_32x32,
    SCALING_LIST_SIZE_NUM
  };

  const int g_quantTSDefault4x4[4 * 4] =
      {
          16, 16, 16, 16,
          16, 16, 16, 16,
          16, 16, 16, 16,
          16, 16, 16, 16};

  const int g_quantIntraDefault8x8[8 * 8] =
      {
          16, 16, 16, 16, 17, 18, 21, 24,
          16, 16, 16, 16, 17, 19, 22, 25,
          16, 16, 17, 18, 20, 22, 25, 29,
          16, 16, 18, 21, 24, 27, 31, 36,
          17, 17, 20, 24, 30, 35, 41, 47,
          18, 19, 22, 27, 35, 44, 54, 65,
          21, 22, 25, 31, 41, 54, 70, 88,
          24, 25, 29, 36, 47, 65, 88, 115};

  const int g_quantInterDefault8x8[8 * 8] =
      {
          16, 16, 16, 16, 17, 18, 20, 24,
          16, 16, 16, 17, 18, 20, 24, 25,
          16, 16, 17, 18, 20, 24, 25, 28,
          16, 17, 18, 20, 24, 25, 28, 33,
          17, 18, 20, 24, 25, 28, 33, 41,
          18, 20, 24, 25, 28, 33, 41, 54,
          20, 24, 25, 28, 33, 41, 54, 71,
          24, 25, 28, 33, 41, 54, 71, 91};

  const unsigned int g_scalingListSize[SCALING_LIST_SIZE_NUM] = {16, 64, 256, 1024};
  const unsigned int g_scalingListSizeX[SCALING_LIST_SIZE_NUM] = {4, 8, 16, 32};

  enum PredMode
  {
    MODE_INTER = 0,
    MODE_INTRA = 1,
    NUMBER_OF_PREDICTION_MODES = 2,
  };

  struct TComReferencePictureSet
  {
    int m_numberOfPictures;
    int m_numberOfNegativePictures;
    int m_numberOfPositivePictures;
    int m_numberOfLongtermPictures;
    int m_deltaPOC[MAX_NUM_REF_PICS];
    int m_POC[MAX_NUM_REF_PICS];
    bool m_used[MAX_NUM_REF_PICS];
    bool m_interRPSPrediction;
    int m_deltaRIdxMinus1;
    int m_deltaRPS;
    int m_numRefIdc;
    int m_refIdc[MAX_NUM_REF_PICS + 1];
    bool m_bCheckLTMSB[MAX_NUM_REF_PICS];
    int m_pocLSBLT[MAX_NUM_REF_PICS];
    int m_deltaPOCMSBCycleLT[MAX_NUM_REF_PICS];
    bool m_deltaPocMSBPresentFlag[MAX_NUM_REF_PICS];
  };

  struct TComRPSList
  {
    std::vector<TComReferencePictureSet> m_referencePictureSets;
  };

  struct ProfileTierLevel
  {
    int m_profileSpace;
    Level::Tier m_tierFlag;
    Profile::Name m_profileIdc;
    bool m_profileCompatibilityFlag[32];
    Level::Name m_levelIdc;

    bool m_progressiveSourceFlag;
    bool m_interlacedSourceFlag;
    bool m_nonPackedConstraintFlag;
    bool m_frameOnlyConstraintFlag;
    unsigned int m_bitDepthConstraintValue;
    ChromaFormat m_chromaFormatConstraintValue;
    bool m_intraConstraintFlag;
    bool m_onePictureOnlyConstraintFlag;
    bool m_lowerBitRateConstraintFlag;
  };

  struct TComPTL
  {
    ProfileTierLevel m_generalPTL;
    ProfileTierLevel m_subLayerPTL[MAX_TLAYER - 1];
    bool m_subLayerProfilePresentFlag[MAX_TLAYER - 1];
    bool m_subLayerLevelPresentFlag[MAX_TLAYER - 1];
  };

  struct TComScalingList
  {
    bool m_scalingListPredModeFlagIsDPCM[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
    int m_scalingListDC[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
    unsigned int m_refMatrixId[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
    // std::vector<int> m_scalingListCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
    std::vector<int> *m_scalingListCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];

    TComScalingList()
    {
      for (unsigned int sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
      {
        for (unsigned int listId = 0; listId < SCALING_LIST_NUM; listId++)
        {
          m_scalingListCoef[sizeId][listId] = new std::vector<int>(g_scalingListSize[sizeId]);
        }
      }
    }
    ~TComScalingList()
    {
      for (unsigned int sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
      {
        for (unsigned int listId = 0; listId < SCALING_LIST_NUM; listId++)
        {
          delete m_scalingListCoef[sizeId][listId];
        }
      }
    }

  public:
    const int *getScalingListDefaultAddress(unsigned int sizeId, unsigned int listId)
    {
      const int *src = 0;
      switch (sizeId)
      {
      case SCALING_LIST_4x4:
        src = g_quantTSDefault4x4;
        break;
      case SCALING_LIST_8x8:
      case SCALING_LIST_16x16:
      case SCALING_LIST_32x32:
        src = (listId < (SCALING_LIST_NUM / NUMBER_OF_PREDICTION_MODES)) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
        break;
      default:
        assert(0);
        src = NULL;
        break;
      }
      return src;
    }
    int *getScalingListAddress(unsigned int sizeId, unsigned int listId)
    {
      if (m_scalingListCoef[sizeId][listId] && !m_scalingListCoef[sizeId][listId]->empty())
      {
        return &((*m_scalingListCoef[sizeId][listId])[0]);
      }
      return nullptr;
    }

    const int *getScalingListAddress(unsigned int sizeId, unsigned int listId) const
    {
      if (m_scalingListCoef[sizeId][listId] && !m_scalingListCoef[sizeId][listId]->empty())
      {
        return &((*m_scalingListCoef[sizeId][listId])[0]);
      }
      return nullptr;
    }
    void setScalingListDC(unsigned int sizeId, unsigned int listId, unsigned int u)
    {
      m_scalingListDC[sizeId][listId] = u;
    }
    int getScalingListDC(unsigned int sizeId, unsigned int listId) const
    {
      return m_scalingListDC[sizeId][listId];
    }

    void processRefMatrix(unsigned int sizeId, unsigned int listId, unsigned int refListId)
    {
      std::memcpy(getScalingListAddress(sizeId, listId), ((listId == refListId) ? getScalingListDefaultAddress(sizeId, refListId) : getScalingListAddress(sizeId, refListId)), sizeof(int) * std::min(hevc::MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]));
    }
  };

  struct HrdSubLayerInfo
  {
    bool fixedPicRateFlag;
    bool fixedPicRateWithinCvsFlag;
    unsigned int picDurationInTcMinus1;
    bool lowDelayHrdFlag;
    unsigned int cpbCntMinus1;
    unsigned int bitRateValueMinus1[MAX_CPB_CNT][2];
    unsigned int cpbSizeValue[MAX_CPB_CNT][2];
    unsigned int ducpbSizeValue[MAX_CPB_CNT][2];
    bool cbrFlag[MAX_CPB_CNT][2];
    unsigned int duBitRateValue[MAX_CPB_CNT][2];
  };

  struct TComHRD
  {
    bool m_nalHrdParametersPresentFlag;
    bool m_vclHrdParametersPresentFlag;
    bool m_subPicCpbParamsPresentFlag;
    unsigned int m_tickDivisorMinus2;
    unsigned int m_duCpbRemovalDelayLengthMinus1;
    bool m_subPicCpbParamsInPicTimingSEIFlag;
    unsigned int m_dpbOutputDelayDuLengthMinus1;
    unsigned int m_bitRateScale;
    unsigned int m_cpbSizeScale;
    unsigned int m_ducpbSizeScale;
    unsigned int m_initialCpbRemovalDelayLengthMinus1;
    unsigned int m_cpbRemovalDelayLengthMinus1;
    unsigned int m_dpbOutputDelayLengthMinus1;
    HrdSubLayerInfo m_HRD[MAX_TLAYER];
  };

  struct TimingInfo
  {
    bool m_timingInfoPresentFlag;
    unsigned int m_numUnitsInTick;
    unsigned int m_timeScale;
    bool m_pocProportionalToTimingFlag;
    int m_numTicksPocDiffOneMinus1;
  };

  struct ChromaQpAdj
  {
    union
    {
      struct
      {
        int CbOffset;
        int CrOffset;
      } comp;
      int offset[2];
    } u;
  };

  struct Window
  {
    bool m_enabledFlag;
    int m_winLeftOffset;
    int m_winRightOffset;
    int m_winTopOffset;
    int m_winBottomOffset;
  };

  struct TComVUI
  {
    bool m_aspectRatioInfoPresentFlag;
    int m_aspectRatioIdc;
    int m_sarWidth;
    int m_sarHeight;
    bool m_overscanInfoPresentFlag;
    bool m_overscanAppropriateFlag;
    bool m_videoSignalTypePresentFlag;
    int m_videoFormat;
    bool m_videoFullRangeFlag;
    bool m_colourDescriptionPresentFlag;
    int m_colourPrimaries;
    int m_transferCharacteristics;
    int m_matrixCoefficients;
    bool m_chromaLocInfoPresentFlag;
    int m_chromaSampleLocTypeTopField;
    int m_chromaSampleLocTypeBottomField;
    bool m_neutralChromaIndicationFlag;
    bool m_fieldSeqFlag;
    Window m_defaultDisplayWindow;
    bool m_frameFieldInfoPresentFlag;
    bool m_hrdParametersPresentFlag;
    bool m_bitstreamRestrictionFlag;
    bool m_tilesFixedStructureFlag;
    bool m_motionVectorsOverPicBoundariesFlag;
    bool m_restrictedRefPicListsFlag;
    int m_minSpatialSegmentationIdc;
    int m_maxBytesPerPicDenom;
    int m_maxBitsPerMinCuDenom;
    int m_log2MaxMvLengthHorizontal;
    int m_log2MaxMvLengthVertical;
    TComHRD m_hrdParameters;
    TimingInfo m_timingInfo;
  };

  struct BitDepths
  {
    int recon[MAX_NUM_CHANNEL_TYPE];
  };

  struct TComSPSRExt
  {
    bool m_transformSkipRotationEnabledFlag;
    bool m_transformSkipContextEnabledFlag;
    bool m_rdpcmEnabledFlag[NUMBER_OF_RDPCM_SIGNALLING_MODES];
    bool m_extendedPrecisionProcessingFlag;
    bool m_intraSmoothingDisabledFlag;
    bool m_highPrecisionOffsetsEnabledFlag;
    bool m_persistentRiceAdaptationEnabledFlag;
    bool m_cabacBypassAlignmentEnabledFlag;
  };

  struct vps
  {
    int m_VPSId;
    unsigned int m_uiMaxTLayers;
    unsigned int m_uiMaxLayers;
    bool m_bTemporalIdNestingFlag;

    unsigned int m_numReorderPics[MAX_TLAYER];
    unsigned int m_uiMaxDecPicBuffering[MAX_TLAYER];
    unsigned int m_uiMaxLatencyIncrease[MAX_TLAYER]; // Really max latency increase plus 1 (value 0 expresses no limit)

    unsigned int m_numHrdParameters;
    unsigned int m_maxNuhReservedZeroLayerId;
    std::vector<TComHRD> m_hrdParameters;
    std::vector<unsigned int> m_hrdOpSetIdx;
    std::vector<bool> m_cprmsPresentFlag;
    unsigned int m_numOpSets;
    bool m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1];

    TComPTL m_pcPTL;
    TimingInfo m_timingInfo;
  };

  struct sps
  {
    int m_SPSId;
    int m_VPSId;
    ChromaFormat m_chromaFormatIdc;

    unsigned int m_uiMaxTLayers;

    unsigned int m_picWidthInLumaSamples;
    unsigned int m_picHeightInLumaSamples;

    int m_log2MinCodingBlockSize;
    int m_log2DiffMaxMinCodingBlockSize;
    unsigned int m_uiMaxCUWidth;
    unsigned int m_uiMaxCUHeight;
    unsigned int m_uiMaxTotalCUDepth;

    Window m_conformanceWindow;

    TComRPSList m_RPSList;
    bool m_bLongTermRefsPresent;
    bool m_SPSTemporalMVPEnabledFlag;
    int m_numReorderPics[MAX_TLAYER];

    unsigned int m_uiQuadtreeTULog2MaxSize;
    unsigned int m_uiQuadtreeTULog2MinSize;
    unsigned int m_uiQuadtreeTUMaxDepthInter;
    unsigned int m_uiQuadtreeTUMaxDepthIntra;
    bool m_usePCM;
    unsigned int m_pcmLog2MaxSize;
    unsigned int m_uiPCMLog2MinSize;
    bool m_useAMP;

    // Parameter
    BitDepths m_bitDepths;
    int m_qpBDOffset[MAX_NUM_CHANNEL_TYPE];
    int m_pcmBitDepths[MAX_NUM_CHANNEL_TYPE];
    bool m_bPCMFilterDisableFlag;

    unsigned int m_uiBitsForPOC;
    unsigned int m_numLongTermRefPicSPS;
    unsigned int m_ltRefPicPocLsbSps[MAX_NUM_LONG_TERM_REF_PICS];
    bool m_usedByCurrPicLtSPSFlag[MAX_NUM_LONG_TERM_REF_PICS];
    unsigned int m_uiMaxTrSize;

    bool m_bUseSAO;

    bool m_bTemporalIdNestingFlag;

    bool m_scalingListEnabledFlag;
    bool m_scalingListPresentFlag;
    TComScalingList m_scalingList;
    unsigned int m_uiMaxDecPicBuffering[MAX_TLAYER];
    unsigned int m_uiMaxLatencyIncreasePlus1[MAX_TLAYER];

    bool m_useStrongIntraSmoothing;

    bool m_vuiParametersPresentFlag;
    TComVUI m_vuiParameters;

    TComSPSRExt m_spsRangeExtension;

    int m_winUnitX[NUM_CHROMA_FORMAT];
    int m_winUnitY[NUM_CHROMA_FORMAT];
    TComPTL m_pcPTL;

    unsigned int m_forceDecodeBitDepth;
  };

  struct TComPPSRExt
  {
    int m_log2MaxTransformSkipBlockSize;
    bool m_crossComponentPredictionEnabledFlag;

    int m_diffCuChromaQpOffsetDepth;
    int m_chromaQpOffsetListLen;
    ChromaQpAdj m_ChromaQpAdjTableIncludingNullEntry[1 + MAX_QP_OFFSET_LIST_SIZE];

    unsigned int m_log2SaoOffsetScale[MAX_NUM_CHANNEL_TYPE];
  };

  struct pps
  {
    int m_PPSId;
    int m_SPSId;
    int m_picInitQPMinus26;
    bool m_useDQP;
    bool m_bConstrainedIntraPred;
    bool m_bSliceChromaQpFlag;

    unsigned int m_uiMaxCuDQPDepth;

    int m_chromaCbQpOffset;
    int m_chromaCrQpOffset;

    unsigned int m_numRefIdxL0DefaultActive;
    unsigned int m_numRefIdxL1DefaultActive;

    bool m_bUseWeightPred;
    bool m_useWeightedBiPred;
    bool m_OutputFlagPresentFlag;
    bool m_TransquantBypassEnabledFlag;
    bool m_useTransformSkip;
    bool m_dependentSliceSegmentsEnabledFlag;
    bool m_tilesEnabledFlag;
    bool m_entropyCodingSyncEnabledFlag;

    bool m_loopFilterAcrossTilesEnabledFlag;
    bool m_uniformSpacingFlag;
    int m_numTileColumnsMinus1;
    int m_numTileRowsMinus1;
    std::vector<int> m_tileColumnWidth;
    std::vector<int> m_tileRowHeight;

    bool m_signDataHidingEnabledFlag;

    bool m_cabacInitPresentFlag;

    bool m_sliceHeaderExtensionPresentFlag;
    bool m_loopFilterAcrossSlicesEnabledFlag;
    bool m_deblockingFilterControlPresentFlag;
    bool m_deblockingFilterOverrideEnabledFlag;
    bool m_ppsDeblockingFilterDisabledFlag;
    int m_deblockingFilterBetaOffsetDiv2;
    int m_deblockingFilterTcOffsetDiv2;
    bool m_scalingListPresentFlag;
    TComScalingList m_scalingList;
    bool m_listsModificationPresentFlag;
    unsigned int m_log2ParallelMergeLevelMinus2;
    int m_numExtraSliceHeaderBits;

    TComPPSRExt m_ppsRangeExtension;
  };

  struct TComRefPicListModification
  {
    bool m_refPicListModificationFlagL0;
    bool m_refPicListModificationFlagL1;
    unsigned int m_RefPicSetIdxL0[REF_PIC_LIST_NUM_IDX];
    unsigned int m_RefPicSetIdxL1[REF_PIC_LIST_NUM_IDX];
  };
}