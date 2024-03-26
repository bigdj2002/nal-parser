#pragma once

#include "nal_parse.h"
#include "h264_type.h"
#include "h264_vlc.h"

struct parseNalH264 : public parseLib<avc::sps, avc::pps>, public Bitstream, public DecoderParams
{
  parseNalH264(){};
  virtual ~parseNalH264()
  {
    if (m_bits)
    {
      delete m_bits;
      m_bits = nullptr;
    }
    if (m_pDec)
    {
      delete m_pDec;
      m_pDec = nullptr;
    }
  };

public:
  void sps_parse(unsigned char *nal_bitstream, avc::sps *sps, int curLen, parsingLevel level) override;
  void pps_parse(unsigned char *nal_bitstream, avc::pps *pps, avc::sps *sps, int curLen, parsingLevel level) override;
  void sei_parse(unsigned char *msg, nal_info &nal, int curLen) override;

protected:
  Bitstream *m_bits{new Bitstream};
  DecoderParams *m_pDec{new DecoderParams};
};

struct parseSeiH264 : public DataPartition, public parseNalH264
{
public:
  void interpret_buffering_period_info(unsigned char *payload, int size, avc::sps *sps, SEIBufferingPeriod &sei_bp); // SEI Type = 0
  void interpret_picture_timing_info(unsigned char *payload, int size, avc::sps *sps, SEIPictureTimingH264 &sei);    // SEI Type = 1
  void interpret_user_data_registered_itu_t_t35_info(unsigned char *payload, int size, SEIUserDataRegistered &sei);                       // SEI Type = 4
  void interpret_user_data_unregistered_info(unsigned char *payload, int size, SEIUserDataUnregistered &sei);                             // SEI Type = 5
};