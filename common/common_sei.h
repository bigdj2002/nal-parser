#pragma once

#include <cstdint>
#include <vector>

static constexpr int ISO_IEC_11578_LEN = 16;
static constexpr int MAX_TIMECODE_SEI_SETS = 3;
static constexpr int MAX_CPB_CNT = 32;
static constexpr int MAX_TLAYER = 7;

struct SEI
{
  enum PayloadType
  {
    BUFFERING_PERIOD = 0,
    PICTURE_TIMING = 1,
    USER_DATA_REGISTERED_ITU_T_T35 = 4,
    USER_DATA_UNREGISTERED = 5,
    TIME_CODE = 136,
    MASTERING_DISPLAY_COLOUR_VOLUME = 137,
    CONTENT_LIGHT_LEVEL_INFO = 144,
  };

  virtual PayloadType payloadType() const = 0;
};

struct SEIBufferingPeriod : public SEI
{
  PayloadType payloadType() const override { return BUFFERING_PERIOD; }

  uint32_t bpSeqParameterSetId;
  bool rapCpbParamsPresentFlag;
  uint32_t cpbDelayOffset;
  uint32_t dpbDelayOffset;
  uint32_t initialCpbRemovalDelay[MAX_CPB_CNT][2];
  uint32_t initialCpbRemovalDelayOffset[MAX_CPB_CNT][2];
  uint32_t initialAltCpbRemovalDelay[MAX_CPB_CNT][2];
  uint32_t initialAltCpbRemovalDelayOffset[MAX_CPB_CNT][2];
  bool concatenationFlag;
  uint32_t auCpbRemovalDelayDelta;
};

struct SEIPictureTimingH264 : public SEI
{
  PayloadType payloadType() const override { return PICTURE_TIMING; }

  uint32_t cpb_removal_delay;
  uint32_t dpb_output_delay;
  uint8_t pic_struct_present_flag;
  uint8_t pic_struct;
  int clock_timestamp_flag[3];
  int ct_type[3];
  int nuit_field_based_flag[3];
  int counting_type[3];
  int full_timestamp_flag[3];
  int discontinuity_flag[3];
  int cnt_dropped_flag[3];
  int nframes[3];
  int seconds_value[3];
  int minutes_value[3];
  int hours_value[3];
  int seconds_flag[3];
  int minutes_flag[3];
  int hours_flag[3];
  int time_offset[3];
};

struct SEIPictureTimingH265 : public SEI
{
  PayloadType payloadType() const override { return PICTURE_TIMING; }

  uint8_t picStruct;
  uint8_t sourceScanType;
  uint8_t duplicateFlag;

  uint32_t auCpbRemovalDelay;
  uint32_t picDpbOutputDelay;
  uint32_t picDpbOutputDuDelay;
  uint32_t numDecodingUnitsMinus1;
  uint8_t duCommonCpbRemovalDelayFlag;
  uint32_t duCommonCpbRemovalDelayMinus1;
  std::vector<uint32_t> numNalusInDuMinus1;
  std::vector<uint32_t> duCpbRemovalDelayMinus1;
};

struct SEIPictureTimingH266 : public SEI
{
  PayloadType payloadType() const { return PICTURE_TIMING; }

  bool m_ptSubLayerDelaysPresentFlag[MAX_TLAYER];
  bool m_cpbRemovalDelayDeltaEnabledFlag[MAX_TLAYER];
  uint32_t m_cpbRemovalDelayDeltaIdx[MAX_TLAYER];
  uint32_t m_auCpbRemovalDelay[MAX_TLAYER];
  uint32_t m_picDpbOutputDelay;
  uint32_t m_picDpbOutputDuDelay;
  uint32_t m_numDecodingUnitsMinus1;
  bool m_duCommonCpbRemovalDelayFlag;
  uint32_t m_duCommonCpbRemovalDelayMinus1[MAX_TLAYER];
  std::vector<uint32_t> m_numNalusInDuMinus1;
  std::vector<uint32_t> m_duCpbRemovalDelayMinus1;
  bool m_cpbAltTimingInfoPresentFlag;
  std::vector<std::vector<uint32_t>> m_nalCpbAltInitialRemovalDelayDelta;
  std::vector<std::vector<uint32_t>> m_nalCpbAltInitialRemovalOffsetDelta;
  std::vector<uint32_t> m_nalCpbDelayOffset;
  std::vector<uint32_t> m_nalDpbDelayOffset;
  std::vector<std::vector<uint32_t>> m_vclCpbAltInitialRemovalDelayDelta;
  std::vector<std::vector<uint32_t>> m_vclCpbAltInitialRemovalOffsetDelta;
  std::vector<uint32_t> m_vclCpbDelayOffset;
  std::vector<uint32_t> m_vclDpbDelayOffset;
  int m_ptDisplayElementalPeriodsMinus1;
};

struct SEIUserDataRegistered : public SEI
{
  PayloadType payloadType() const override { return USER_DATA_REGISTERED_ITU_T_T35; }

  uint16_t ituCountryCode;
  std::vector<uint8_t> userData;
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

struct SEIUserDataUnregistered : public SEI
{
  PayloadType payloadType() const override { return USER_DATA_UNREGISTERED; }

  uint8_t uuid_iso_iec_11578[ISO_IEC_11578_LEN];
  std::vector<uint8_t> userData;
};

struct SEITimeCode : public SEI
{
  PayloadType payloadType() const override { return TIME_CODE; }

  uint32_t numClockTs;
  SEITimeSet timeSetArray[MAX_TIMECODE_SEI_SETS];
};

// Mastering display colour volume (SMTPE ST2086)
struct SEIMasteringDisplayColourVolume : public SEI
{
  PayloadType payloadType() const override { return MASTERING_DISPLAY_COLOUR_VOLUME; }

  uint16_t primaries[3][2]; // [0] : x, [1] : y
  uint16_t white_point[2];
  uint32_t max_luminance;
  uint32_t min_luminance;
};

// Content light level information (CTA 861.3)
struct SEIContentLightLevelInfo : public SEI
{
  PayloadType payloadType() const override { return CONTENT_LIGHT_LEVEL_INFO; }

  uint16_t max_content_light_level;     // MaxCLL
  uint16_t max_pic_average_light_level; // MaxFALL
};
