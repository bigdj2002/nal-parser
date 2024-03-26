#pragma once

/** \author      Dongjae Won
    \interface   nal_parse()
    \brief       Call 'nal_parse()' function to parse NAL bitstream in accordance with the rules of MPEG's video codec standard (H264/AVC, H265/HEVC, H266/VVC)
    \warning     nal_parse API cannot support parsing for VCL layer bitstream (Only support in non-VCL layer)
 */

#include "h264_param_set.h"
#include "hevc_param_set.h"
#include "vvc_param_set.h"
#include "h264_type.h"
#include "hevc_type.h"
#include "vvc_type.h"
#include "common_sei.h"

#include <stdio.h>
#include <assert.h>

enum class videoCodecType
{
  UNDEFINED = -1,
  H264_AVC = 1,
  H265_HEVC,
  H266_VVC
};

enum class parsingLevel
{
  PARSING_NONE = 0,
  PARSING_PARAM_ID,
  PARSING_FULL
};

struct h264_seis
{
  struct SEIBufferingPeriod h264_sei_bp;
  struct SEIPictureTimingH264 h264_sei_pt;
};

struct hevc_seis
{
  struct SEIBufferingPeriod hevc_sei_bp;
  struct SEIPictureTimingH265 hevc_sei_pt;
  struct SEITimeCode hevc_sei_tc;
};

struct mpeg_common_seis
{
  struct SEIUserDataRegistered common_sei_dr;
  struct SEIUserDataUnregistered common_sei_du;
};

struct param_set
{
  void *vps; // Caution: H264/AVC standard has not VPS
  void *sps;
  void *pps;
  void *aps; // Caution: Only available in VVC
};

struct nal_info
{
  videoCodecType codecType;
  int nal_unit_type;
  int sei_type;
  size_t sei_length;
  void *sei;

  param_set *mpegParamSet;

  h264_seis *h264SEI;
  hevc_seis *hevcSEI;
  mpeg_common_seis *mpegCommonSEI;

  nal_info()
  {
    codecType = videoCodecType::UNDEFINED;
    nal_unit_type = -1;
    sei_type = -1;
    sei = nullptr;
    sei_length = 0;
    mpegParamSet = new param_set{};
    h264SEI = new h264_seis{};
    hevcSEI = new hevc_seis{};
    mpegCommonSEI = new mpeg_common_seis{};
  }
  ~nal_info()
  {
    if (codecType == videoCodecType::H264_AVC)
    {
      if (mpegParamSet->sps)
      {
        delete static_cast<avc::sps *>(mpegParamSet->sps);
        mpegParamSet->sps = NULL;
      }
      if (mpegParamSet->pps)
      {
        delete static_cast<avc::pps *>(mpegParamSet->pps);
        mpegParamSet->pps = NULL;
      }
    }
    if (codecType == videoCodecType::H265_HEVC)
    {
      if (mpegParamSet->sps)
      {
        delete static_cast<hevc::sps *>(mpegParamSet->sps);
        mpegParamSet->sps = NULL;
      }
      if (mpegParamSet->pps)
      {
        delete static_cast<hevc::pps *>(mpegParamSet->pps);
        mpegParamSet->pps = NULL;
      }
    }
    if (codecType == videoCodecType::H266_VVC)
    {
      if (mpegParamSet->aps)
      {
        delete static_cast<vvc::APS *>(mpegParamSet->aps);
        mpegParamSet->aps = NULL;
      }
      if (mpegParamSet->vps)
      {
        delete static_cast<vvc::VPS *>(mpegParamSet->vps);
        mpegParamSet->vps = NULL;
      }
      if (mpegParamSet->sps)
      {
        delete static_cast<vvc::SPS *>(mpegParamSet->sps);
        mpegParamSet->sps = NULL;
      }
      if (mpegParamSet->pps)
      {
        delete static_cast<vvc::PPS *>(mpegParamSet->pps);
        mpegParamSet->pps = NULL;
      }
    }

    delete mpegParamSet;
    mpegParamSet = NULL;
    delete h264SEI;
    h264SEI = NULL;
    delete hevcSEI;
    hevcSEI = NULL;
    delete mpegCommonSEI;
    mpegCommonSEI = NULL;
  }
};

class NALParse
{
public:
  NALParse();
  virtual ~NALParse();

public:
  /**
   * \brief Extern API to parse NAL unit of h264/AVC, h265/HEVC and h266/VVC
   * \param nal_bitstream    Pointer to indicate buffer for NAL stream (Started with Byte stream NAL Unit, ex) 0 0 1, 0 0 0 1 ~)
   * \param codecType        Video codec type
   * \param nextNalPos       Position of next NAL stream in the currnent packet
   * \param seqSize          NAL size (Just needed to find end of NAL stream)
   * \param level            Parsing level (Refer to enum parsingLevel structure)
   */
  void nal_parse(unsigned char *nal_bitstream, videoCodecType codecType, int &nextNalPos, int seqSize, parsingLevel level);
  nal_info *nal;

private:
  void h264_nal_parse(unsigned char *nal_bitstream, int &nextNalPos, int seqSize, parsingLevel level);
  void hevc_nal_parse(unsigned char *nal_bitstream, int &nextNalPos, int seqSize, parsingLevel level);
  void vvc_nal_parse(unsigned char *nal_bitstream, int &nextNalPos, int seqSize, parsingLevel level);

private:
  int FindStartCode(const unsigned char *nal_bitstream);
  int FindNextNal(unsigned char *nal_bitstream, int nextNalPos, int seqSize);
  std::vector<uint8_t> UnescapeRbsp(const uint8_t *data, size_t length);
};

template <typename T1, typename T2>
class parseLib
{
public:
  parseLib(){};
  virtual ~parseLib(){};

public:
  virtual void sps_parse(unsigned char *nal_bitstream, T1 *pcSPS, int curLen, parsingLevel level) = 0;
  virtual void pps_parse(unsigned char *nal_bitstream, T2 *pcPPS, T1 *pcSPS, int curLen, parsingLevel level) = 0;
  virtual void sei_parse(unsigned char *nal_bitstream, nal_info &nal, int curLen) = 0;
};