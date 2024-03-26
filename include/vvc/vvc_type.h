#pragma once

#include <vector>
#include <utility>
#include <sstream>
#include <cstddef>
#include <cstring>
#include <assert.h>
#include <cassert>
#include <stdarg.h>
namespace vvc
{
  const int g_quantTSDefault4x4[4 * 4] =
      {
          16, 16, 16, 16,
          16, 16, 16, 16,
          16, 16, 16, 16,
          16, 16, 16, 16};

  const int g_quantIntraDefault8x8[8 * 8] =
      {
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16};

  const int g_quantInterDefault8x8[8 * 8] =
      {
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16,
          16, 16, 16, 16, 16, 16, 16, 16};

  enum ScalingListSize
  {
    SCALING_LIST_1x1 = 0,
    SCALING_LIST_2x2,
    SCALING_LIST_4x4,
    SCALING_LIST_8x8,
    SCALING_LIST_16x16,
    SCALING_LIST_32x32,
    SCALING_LIST_64x64,
    SCALING_LIST_128x128,
    SCALING_LIST_SIZE_NUM,
    // for user define matrix
    SCALING_LIST_FIRST_CODED = SCALING_LIST_2x2,
    SCALING_LIST_LAST_CODED = SCALING_LIST_64x64
  };

  enum ScalingList1dStartIdx
  {
    SCALING_LIST_1D_START_2x2 = 0,
    SCALING_LIST_1D_START_4x4 = 2,
    SCALING_LIST_1D_START_8x8 = 8,
    SCALING_LIST_1D_START_16x16 = 14,
    SCALING_LIST_1D_START_32x32 = 20,
    SCALING_LIST_1D_START_64x64 = 26,
  };

  enum CoeffScanType
  {
    SCAN_DIAG = 0, ///< up-right diagonal scan
    SCAN_TRAV_HOR = 1,
    SCAN_TRAV_VER = 2,
    SCAN_NUMBER_OF_TYPES
  };

  enum CoeffScanGroupType
  {
    SCAN_UNGROUPED = 0,
    SCAN_GROUPED_4x4 = 1,
    SCAN_NUMBER_OF_GROUP_TYPES = 2
  };

  enum MsgLevel
  {
    SILENT = 0,
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    NOTICE = 4,
    VERBOSE = 5,
    DETAILS = 6
  };

  inline void msg(MsgLevel level, const char *fmt, ...)
  {
    va_list args;
    va_start(args, fmt);
    vfprintf(level == ERROR ? stderr : stdout, fmt, args);
    va_end(args);
  }

  enum SPSExtensionFlagIndex
  {
    SPS_EXT__REXT = 0,
    NUM_SPS_EXTENSION_FLAGS = 8
  };

  enum ApsType
  {
    ALF_APS = 0,
    LMCS_APS = 1,
    SCALING_LIST_APS = 2,
  };

  enum SliceType
  {
    B_SLICE = 0,
    P_SLICE = 1,
    I_SLICE = 2,
    NUMBER_OF_SLICE_TYPES = 3
  };

  enum ChromaFormat
  {
    CHROMA_400 = 0,
    CHROMA_420 = 1,
    CHROMA_422 = 2,
    CHROMA_444 = 3,
    NUM_CHROMA_FORMAT = 4
  };

  enum ComponentID
  {
    COMPONENT_Y = 0,
    COMPONENT_Cb = 1,
    COMPONENT_Cr = 2,
    MAX_NUM_COMPONENT = 3,
    JOINT_CbCr = MAX_NUM_COMPONENT,
    MAX_NUM_TBLOCKS = MAX_NUM_COMPONENT
  };

  enum ChannelType
  {
    CHANNEL_TYPE_LUMA = 0,
    CHANNEL_TYPE_CHROMA = 1,
    MAX_NUM_CHANNEL_TYPE = 2
  };

  struct Size
  {
    uint32_t width;
    uint32_t height;
    Size() : width(0), height(0) {}
    Size(const uint32_t _width, const uint32_t _height) : width(_width), height(_height) {}
  };

  namespace Profile
  {
    enum Name
    {
      NONE = 0,
      INTRA = 8,
      STILL_PICTURE = 64,
      MAIN_10 = 1,
      MAIN_10_STILL_PICTURE = MAIN_10 | STILL_PICTURE,
      MULTILAYER_MAIN_10 = 17,
      MULTILAYER_MAIN_10_STILL_PICTURE = MULTILAYER_MAIN_10 | STILL_PICTURE,
      MAIN_10_444 = 33,
      MAIN_10_444_STILL_PICTURE = MAIN_10_444 | STILL_PICTURE,
      MULTILAYER_MAIN_10_444 = 49,
      MULTILAYER_MAIN_10_444_STILL_PICTURE = MULTILAYER_MAIN_10_444 | STILL_PICTURE,
      MAIN_12 = 2,
      MAIN_12_444 = 34,
      MAIN_16_444 = 35,
      MAIN_12_INTRA = MAIN_12 | INTRA,
      MAIN_12_444_INTRA = MAIN_12_444 | INTRA,
      MAIN_16_444_INTRA = MAIN_16_444 | INTRA,
      MAIN_12_STILL_PICTURE = MAIN_12 | STILL_PICTURE,
      MAIN_12_444_STILL_PICTURE = MAIN_12_444 | STILL_PICTURE,
      MAIN_16_444_STILL_PICTURE = MAIN_16_444 | STILL_PICTURE,
    };
  }

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
      // code = (major_level * 16 + minor_level * 3)
      NONE = 0,
      LEVEL1 = 16,
      LEVEL2 = 32,
      LEVEL2_1 = 35,
      LEVEL3 = 48,
      LEVEL3_1 = 51,
      LEVEL4 = 64,
      LEVEL4_1 = 67,
      LEVEL5 = 80,
      LEVEL5_1 = 83,
      LEVEL5_2 = 86,
      LEVEL6 = 96,
      LEVEL6_1 = 99,
      LEVEL6_2 = 102,
      LEVEL6_3 = 105,
      LEVEL15_5 = 255,
    };
  }

  enum NalUnitType
  {
    NAL_UNIT_CODED_SLICE_TRAIL = 0, // 0
    NAL_UNIT_CODED_SLICE_STSA,      // 1
    NAL_UNIT_CODED_SLICE_RADL,      // 2
    NAL_UNIT_CODED_SLICE_RASL,      // 3

    NAL_UNIT_RESERVED_VCL_4,
    NAL_UNIT_RESERVED_VCL_5,
    NAL_UNIT_RESERVED_VCL_6,

    NAL_UNIT_CODED_SLICE_IDR_W_RADL, // 7
    NAL_UNIT_CODED_SLICE_IDR_N_LP,   // 8
    NAL_UNIT_CODED_SLICE_CRA,        // 9
    NAL_UNIT_CODED_SLICE_GDR,        // 10

    NAL_UNIT_RESERVED_IRAP_VCL_11,
    NAL_UNIT_OPI,                   // 12
    NAL_UNIT_DCI,                   // 13
    NAL_UNIT_VPS,                   // 14
    NAL_UNIT_SPS,                   // 15
    NAL_UNIT_PPS,                   // 16
    NAL_UNIT_PREFIX_APS,            // 17
    NAL_UNIT_SUFFIX_APS,            // 18
    NAL_UNIT_PH,                    // 19
    NAL_UNIT_ACCESS_UNIT_DELIMITER, // 20
    NAL_UNIT_EOS,                   // 21
    NAL_UNIT_EOB,                   // 22
    NAL_UNIT_PREFIX_SEI,            // 23
    NAL_UNIT_SUFFIX_SEI,            // 24
    NAL_UNIT_FD,                    // 25

    NAL_UNIT_RESERVED_NVCL_26,
    NAL_UNIT_RESERVED_NVCL_27,

    NAL_UNIT_UNSPECIFIED_28,
    NAL_UNIT_UNSPECIFIED_29,
    NAL_UNIT_UNSPECIFIED_30,
    NAL_UNIT_UNSPECIFIED_31,
    NAL_UNIT_INVALID
  };

  struct BitDepths
  {
    const int &operator[](const ChannelType ch) const { return recon[ch]; }
    int recon[MAX_NUM_CHANNEL_TYPE]; ///< the bit depth as indicated in the SPS
  };

  struct SEITimeSet
  {
    bool clockTimeStampFlag;
    bool numUnitFieldBasedFlag;
    int countingType;
    bool fullTimeStampFlag;
    bool discontinuityFlag;
    bool cntDroppedFlag;
    int numberOfFrames;
    int secondsValue;
    int minutesValue;
    int hoursValue;
    bool secondsFlag;
    bool minutesFlag;
    bool hoursFlag;
    int timeOffsetLength;
    int timeOffsetValue;
  };

  struct SEIMasteringDisplay
  {
    bool colourVolumeSEIEnabled;
    uint32_t maxLuminance;
    uint32_t minLuminance;
    uint16_t primaries[3][2];
    uint16_t whitePoint[2];
  };

  enum SEIMessageType
  {
    BUFFERING_PERIOD = 0,
    PICTURE_TIMING = 1,
    FILLER_PAYLOAD = 3,
    USER_DATA_REGISTERED_ITU_T_T35 = 4,
    USER_DATA_UNREGISTERED = 5,
    FILM_GRAIN_CHARACTERISTICS = 19,
    FRAME_PACKING = 45,
    DISPLAY_ORIENTATION = 47,
    GREEN_METADATA = 56,
    PARAMETER_SETS_INCLUSION_INDICATION = 129,
    DECODING_UNIT_INFO = 130,
    DECODED_PICTURE_HASH = 132,
    SCALABLE_NESTING = 133,
    MASTERING_DISPLAY_COLOUR_VOLUME = 137,
    COLOUR_TRANSFORM_INFO = 142,
    DEPENDENT_RAP_INDICATION = 145,
    EQUIRECTANGULAR_PROJECTION = 150,
    SPHERE_ROTATION = 154,
    REGION_WISE_PACKING = 155,
    OMNI_VIEWPORT = 156,
    GENERALIZED_CUBEMAP_PROJECTION = 153,
    ALPHA_CHANNEL_INFO = 165,
    FRAME_FIELD_INFO = 168,
    DEPTH_REPRESENTATION_INFO = 177,
    MULTIVIEW_ACQUISITION_INFO = 179,
    MULTIVIEW_VIEW_POSITION = 180,
    SUBPICTURE_LEVEL_INFO = 203,
    SAMPLE_ASPECT_RATIO_INFO = 204,
    CONTENT_LIGHT_LEVEL_INFO = 144,
    ALTERNATIVE_TRANSFER_CHARACTERISTICS = 147,
    AMBIENT_VIEWING_ENVIRONMENT = 148,
    CONTENT_COLOUR_VOLUME = 149,
    ANNOTATED_REGIONS = 202,
    SCALABILITY_DIMENSION_INFO = 205,
    EXTENDED_DRAP_INDICATION = 206,
    CONSTRAINED_RASL_ENCODING = 207,
    VDI_SEI_ENVELOPE = 208,
    SHUTTER_INTERVAL_INFO = 209,
    NEURAL_NETWORK_POST_FILTER_CHARACTERISTICS = 210,
    NEURAL_NETWORK_POST_FILTER_ACTIVATION = 211,
  };
}