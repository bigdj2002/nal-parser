#include "nal_parse.h"
#include "h264_nal.h"
#include "hevc_nal.h"
#include "vvc_nal.h"

NALParse::NALParse()
{
  nal = new nal_info();
}

NALParse::~NALParse()
{
  if (nal != nullptr)
  {
    delete nal;
    nal = nullptr;
  }
}

void NALParse::nal_parse(unsigned char *nal_bitstream, videoCodecType codecType, int &nextNalPos, int seqSize, parsingLevel level)
{
  nal->codecType = codecType;
  if (codecType == videoCodecType::H264_AVC)
  {
    h264_nal_parse(nal_bitstream, nextNalPos, seqSize, level);
  }
  else if (codecType == videoCodecType::H265_HEVC)
  {
    hevc_nal_parse(nal_bitstream, nextNalPos, seqSize, level);
  }
  else if (codecType == videoCodecType::H266_VVC)
  {
    vvc_nal_parse(nal_bitstream, nextNalPos, seqSize, level);
  }
}

std::vector<uint8_t> NALParse::UnescapeRbsp(const uint8_t *data, size_t length)
{
  std::vector<uint8_t> out;
  out.reserve(length);

  for (size_t i = 0; i < length;)
  {
    // Be careful about over/underflow here. byte_length_ - 3 can underflow, and
    // i + 3 can overflow, but byte_length_ - i can't, because i < byte_length_
    // above, and that expression will produce the number of bytes left in
    // the stream including the byte at i.
    if (length - i >= 3 && data[i] == 0x00 && data[i + 1] == 0x00 &&
        data[i + 2] == 0x03)
    {
      // Two RBSP bytes.
      out.push_back(data[i++]);
      out.push_back(data[i++]);
      // Skip the emulation byte.
      i++;
    }
    else
    {
      // Single rbsp byte.
      out.push_back(data[i++]);
    }
  }
  return out;
}

void NALParse::h264_nal_parse(unsigned char *nal_bitstream, int &nextNalPos, int seqSize, parsingLevel level)
{
  parseNalH264 lib;
  nal_info *nal = this->nal;
  unsigned char *stream = &nal_bitstream[nextNalPos];
  int firstPos = FindStartCode(stream);
  stream += firstPos;

  nal->nal_unit_type = static_cast<int>((*(stream)) & 0x1f);
  nal->sei_type = -1;
  stream++;
  int curLen = FindNextNal(stream, nextNalPos + firstPos, seqSize);
  nextNalPos += (firstPos + 1 + curLen);

  std::vector<uint8_t> streamTmp = UnescapeRbsp(stream, curLen);
  uint8_t* realStream = streamTmp.size() == (uint32_t)curLen ? stream : streamTmp.data();

  if (level > parsingLevel::PARSING_NONE && curLen > 0)
  {
    if (static_cast<avc::h264_nal_type>(nal->nal_unit_type) == avc::h264_nal_type::NALU_TYPE_SEI && level > parsingLevel::PARSING_PARAM_ID)
    {
      lib.sei_parse(realStream, *nal, curLen);
    }
    else if (static_cast<avc::h264_nal_type>(nal->nal_unit_type) == avc::h264_nal_type::NALU_TYPE_SPS)
    {
      if (!nal->mpegParamSet->sps)
        nal->mpegParamSet->sps = new avc::sps;
      lib.sps_parse(realStream, reinterpret_cast<avc::sps *>(nal->mpegParamSet->sps), curLen, level);
    }
    else if (static_cast<avc::h264_nal_type>(nal->nal_unit_type) == avc::h264_nal_type::NALU_TYPE_PPS)
    {
      if (!nal->mpegParamSet->pps)
        nal->mpegParamSet->pps = new avc::pps;
      lib.pps_parse(realStream, reinterpret_cast<avc::pps *>(nal->mpegParamSet->pps), reinterpret_cast<avc::sps *>(nal->mpegParamSet->sps), curLen, level);
    }
  }
}

void NALParse::hevc_nal_parse(unsigned char *nal_bitstream, int &nextNalPos, int seqSize, parsingLevel level)
{
  parseNalH265 lib;
  nal_info *nal = this->nal;
  unsigned char *stream = &nal_bitstream[nextNalPos];
  int firstPos = FindStartCode(stream);
  stream += firstPos;

  nal->nal_unit_type = static_cast<int>((*(stream)) & 0x7e) >> 1;
  nal->sei_type = -1;
  int curLen = FindNextNal(stream, nextNalPos + firstPos, seqSize);
  nextNalPos += (firstPos + curLen);

  std::vector<uint8_t> streamTmp = UnescapeRbsp(stream, curLen);
  uint8_t* realStream = streamTmp.size() == (uint32_t)curLen ? stream : streamTmp.data();

  if (level > parsingLevel::PARSING_NONE && curLen > 0)
  {
    if ((static_cast<hevc::hevc_nal_type>(nal->nal_unit_type) == hevc::hevc_nal_type::NAL_UNIT_PREFIX_SEI ||
         static_cast<hevc::hevc_nal_type>(nal->nal_unit_type) == hevc::hevc_nal_type::NAL_UNIT_SUFFIX_SEI) &&
        level > parsingLevel::PARSING_PARAM_ID)
    {
      lib.sei_parse(realStream, *nal, curLen);
    }
    else if (static_cast<hevc::hevc_nal_type>(nal->nal_unit_type) == hevc::hevc_nal_type::NAL_UNIT_VPS)
    {
      if (!nal->mpegParamSet->vps)
        nal->mpegParamSet->vps = new hevc::vps;
      lib.vps_parse(realStream, reinterpret_cast<hevc::vps *>(nal->mpegParamSet->sps), curLen, level);
    }
    else if (static_cast<hevc::hevc_nal_type>(nal->nal_unit_type) == hevc::hevc_nal_type::NAL_UNIT_SPS)
    {
      if (!nal->mpegParamSet->sps)
        nal->mpegParamSet->sps = new hevc::sps;
      lib.sps_parse(realStream, reinterpret_cast<hevc::sps *>(nal->mpegParamSet->sps), curLen, level);
    }
    else if (static_cast<hevc::hevc_nal_type>(nal->nal_unit_type) == hevc::hevc_nal_type::NAL_UNIT_PPS)
    {
      if (!nal->mpegParamSet->pps)
        nal->mpegParamSet->pps = new hevc::pps;
      lib.pps_parse(realStream, reinterpret_cast<hevc::pps *>(nal->mpegParamSet->pps), reinterpret_cast<hevc::sps *>(nal->mpegParamSet->sps), curLen, level);
    }
  }
}

void NALParse::vvc_nal_parse(unsigned char *nal_bitstream, int &nextNalPos, int seqSize, parsingLevel level)
{
  parseNalH266 lib;
  nal_info *nal = this->nal;
  unsigned char *stream = &nal_bitstream[nextNalPos];
  int firstPos = FindStartCode(stream);
  stream += firstPos;

  nal->nal_unit_type = static_cast<int>(stream[1] & 0xF8) >> 3;
  nal->sei_type = -1;
  int curLen = FindNextNal(stream, nextNalPos + firstPos, seqSize);
  nextNalPos += (firstPos + curLen);
  stream += 2; // length of nal unit header

  std::vector<uint8_t> streamTmp = UnescapeRbsp(stream, curLen);
  uint8_t* realStream = streamTmp.size() == (uint32_t)curLen ? stream : streamTmp.data();

  if (level > parsingLevel::PARSING_NONE && curLen > 0)
  {
    if ((static_cast<vvc::NalUnitType>(nal->nal_unit_type) == vvc::NalUnitType::NAL_UNIT_PREFIX_SEI ||
         static_cast<vvc::NalUnitType>(nal->nal_unit_type) == vvc::NalUnitType::NAL_UNIT_SUFFIX_SEI) &&
        level > parsingLevel::PARSING_PARAM_ID)
    {
      lib.sei_parse(realStream, *nal, curLen);
    }
    else if (static_cast<vvc::NalUnitType>(nal->nal_unit_type) == vvc::NalUnitType::NAL_UNIT_SPS)
    {
      if (!nal->mpegParamSet->sps)
        nal->mpegParamSet->sps = new vvc::SPS;
      lib.sps_parse(realStream, reinterpret_cast<vvc::SPS *>(nal->mpegParamSet->sps), curLen, level);
    }
    else if (static_cast<vvc::NalUnitType>(nal->nal_unit_type) == vvc::NalUnitType::NAL_UNIT_PPS)
    {
      if (!nal->mpegParamSet->pps)
        nal->mpegParamSet->pps = new vvc::PPS;
      lib.pps_parse(realStream, reinterpret_cast<vvc::PPS *>(nal->mpegParamSet->pps), reinterpret_cast<vvc::SPS *>(nal->mpegParamSet->sps), curLen, level);
    }
    else if (static_cast<vvc::NalUnitType>(nal->nal_unit_type) == vvc::NalUnitType::NAL_UNIT_PREFIX_APS ||
             static_cast<vvc::NalUnitType>(nal->nal_unit_type) == vvc::NalUnitType::NAL_UNIT_SUFFIX_APS)
    {
      if (!nal->mpegParamSet->aps)
        nal->mpegParamSet->aps = new vvc::APS;
      lib.aps_parse(realStream, reinterpret_cast<vvc::APS *>(nal->mpegParamSet->aps), curLen, level);
    }
  }
}

int NALParse::FindStartCode(const unsigned char *pData)
{
  const int i = 0;
  if ((pData[i] == 00 && pData[i + 1] == 00 && pData[i + 2] == 00 && pData[i + 3] == 1))
    return 4;
  if ((pData[i] == 00 && pData[i + 1] == 00 && pData[i + 2] == 01))
    return 3;
  return 0;
}

int NALParse::FindNextNal(unsigned char *pData, int nextNalPos, int seqSize)
{
  int i = 0;

  while (nextNalPos + i < seqSize)
  {
    if ((pData[i + 0] == 00 && pData[i + 1] == 00 && pData[i + 2] == 00 && pData[i + 3] == 1) ||
        (pData[i + 0] == 00 && pData[i + 1] == 00 && pData[i + 2] == 01))
    {
      break;
    }
    i++;
  }

  return i;
}