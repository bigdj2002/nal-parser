#include "nal_parse.h"
#include "h264_nal.h"

void parseSeiH264::interpret_buffering_period_info(unsigned char *payload, int size, avc::sps *sps, SEIBufferingPeriod &sei_bp)
{
  avc::sps *active_sps = sps;
  DecoderParams *p_Dec = m_pDec;
  unsigned int k;

  Bitstream *buf;

  if (NULL == active_sps)
  {
    fprintf(stderr, "Warning: no active SPS, buffering SEI cannot be parsed\n");
    return;
  }

  buf = m_bits;
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  p_Dec->UsedBits = 0;

  sei_bp.bpSeqParameterSetId = buf->read_ue_v(buf, &p_Dec->UsedBits);

  // Note: NalHrdBpPresentFlag and CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  if (sps->vui_parameters_present_flag)
  {
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
    {
      for (k = 0; k < sps->vui_seq_parameters.nal_hrd_parameters.cpb_cnt_minus1 + 1; k++)
      {
        sei_bp.initialCpbRemovalDelay[k][0] = buf->read_u_v(sps->vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1, buf, &p_Dec->UsedBits);
        sei_bp.initialCpbRemovalDelayOffset[k][0] = buf->read_u_v(sps->vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1, buf, &p_Dec->UsedBits);
      }
    }

    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
      for (k = 0; k < sps->vui_seq_parameters.vcl_hrd_parameters.cpb_cnt_minus1 + 1; k++)
      {
        sei_bp.initialCpbRemovalDelay[k][0] = buf->read_u_v(sps->vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1, buf, &p_Dec->UsedBits);
        sei_bp.initialCpbRemovalDelayOffset[k][0] = buf->read_u_v(sps->vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1, buf, &p_Dec->UsedBits);
      }
    }
  }
}

void parseSeiH264::interpret_picture_timing_info(unsigned char *payload, int size, avc::sps *sps, SEIPictureTimingH264 &sei_pt)
{
  avc::sps *active_sps = sps;
  DecoderParams *p_Dec = m_pDec;

  int NumClockTs = 0;
  int i;

  int cpb_removal_len = 24;
  int dpb_output_len = 24;

  bool CpbDpbDelaysPresentFlag;

  Bitstream *buf;

  if (NULL == active_sps)
  {
    fprintf(stderr, "Warning: no active SPS, timing SEI cannot be parsed\n");
    return;
  }

  buf = m_bits;
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  p_Dec->UsedBits = 0;

  // CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  CpbDpbDelaysPresentFlag = (bool)(active_sps->vui_parameters_present_flag && ((active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag != 0) || (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag != 0)));

  if (CpbDpbDelaysPresentFlag)
  {
    if (active_sps->vui_parameters_present_flag)
    {
      if (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
      {
        cpb_removal_len = active_sps->vui_seq_parameters.nal_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
        dpb_output_len = active_sps->vui_seq_parameters.nal_hrd_parameters.dpb_output_delay_length_minus1 + 1;
      }
      else if (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      {
        cpb_removal_len = active_sps->vui_seq_parameters.vcl_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
        dpb_output_len = active_sps->vui_seq_parameters.vcl_hrd_parameters.dpb_output_delay_length_minus1 + 1;
      }
    }

    if ((active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag) ||
        (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag))
    {
      sei_pt.cpb_removal_delay = (uint32_t)buf->read_u_v(cpb_removal_len, buf, &p_Dec->UsedBits);
      sei_pt.dpb_output_delay = (uint32_t)buf->read_u_v(dpb_output_len, buf, &p_Dec->UsedBits);
    }
  }

  if (!active_sps->vui_parameters_present_flag)
  {
    sei_pt.pic_struct_present_flag = 0;
  }
  else
  {
    sei_pt.pic_struct_present_flag = (uint8_t)active_sps->vui_seq_parameters.pic_struct_present_flag;
  }

  if (sei_pt.pic_struct_present_flag)
  {
    sei_pt.pic_struct = (uint8_t)buf->read_u_v(4, buf, &p_Dec->UsedBits);
    switch (sei_pt.pic_struct)
    {
    case 0:
    case 1:
    case 2:
      NumClockTs = 1;
      break;
    case 3:
    case 4:
    case 7:
      NumClockTs = 2;
      break;
    case 5:
    case 6:
    case 8:
      NumClockTs = 3;
      break;
    default:
      assert(0);
    }
    for (i = 0; i < NumClockTs; i++)
    {
      sei_pt.clock_timestamp_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
      if (sei_pt.clock_timestamp_flag[i])
      {
        sei_pt.ct_type[i] = buf->read_u_v(2, buf, &p_Dec->UsedBits);
        sei_pt.nuit_field_based_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
        sei_pt.counting_type[i] = buf->read_u_v(5, buf, &p_Dec->UsedBits);
        sei_pt.full_timestamp_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
        sei_pt.discontinuity_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
        sei_pt.cnt_dropped_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
        sei_pt.nframes[i] = buf->read_u_v(8, buf, &p_Dec->UsedBits);

        if (sei_pt.full_timestamp_flag)
        {
          sei_pt.seconds_value[i] = buf->read_u_v(6, buf, &p_Dec->UsedBits);
          sei_pt.minutes_value[i] = buf->read_u_v(6, buf, &p_Dec->UsedBits);
          sei_pt.hours_value[i] = buf->read_u_v(5, buf, &p_Dec->UsedBits);
        }
        else
        {
          sei_pt.seconds_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
          if (sei_pt.seconds_flag)
          {
            sei_pt.seconds_value[i] = buf->read_u_v(6, buf, &p_Dec->UsedBits);
            sei_pt.minutes_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
            if (sei_pt.minutes_flag[i])
            {
              sei_pt.minutes_value[i] = buf->read_u_v(6, buf, &p_Dec->UsedBits);
              sei_pt.hours_flag[i] = buf->read_u_1(buf, &p_Dec->UsedBits);
              if (sei_pt.hours_flag[i])
              {
                sei_pt.hours_value[i] = buf->read_u_v(5, buf, &p_Dec->UsedBits);
              }
            }
          }
        }
        {
          int time_offset_length;
          if (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.vcl_hrd_parameters.time_offset_length;
          else if (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.nal_hrd_parameters.time_offset_length;
          else
            time_offset_length = 24;
          if (time_offset_length)
            sei_pt.time_offset[i] = buf->read_i_v(time_offset_length, buf, &p_Dec->UsedBits);
          else
            sei_pt.time_offset[i] = 0;
        }
      }
    }
  }
}

void parseSeiH264::interpret_user_data_registered_itu_t_t35_info(unsigned char *payload, int size, SEIUserDataRegistered &sei_dr_itu_t35)
{
  int offset = 0;

  sei_dr_itu_t35.ituCountryCode = payload[offset];
  offset++;
  if (sei_dr_itu_t35.ituCountryCode == 0xFF)
  {
    sei_dr_itu_t35.ituCountryCode += payload[offset];
    offset++;
  }
  while (offset < size)
  {
    sei_dr_itu_t35.userData.push_back(payload[offset]);
    offset++;
  }
}

void parseSeiH264::interpret_user_data_unregistered_info(unsigned char *payload, int size, SEIUserDataUnregistered &sei_du)
{
  int offset = 0;

  while (offset < ISO_IEC_11578_LEN)
  {
    sei_du.uuid_iso_iec_11578[offset] = payload[offset];
    offset++;
  }

  assert(size >= 16);

  while (offset < size)
  {
    sei_du.userData.push_back(payload[offset]);
    offset++;
  }
}