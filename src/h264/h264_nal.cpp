#include "nal_parse.h"
#include "h264_nal.h"
#include "h264_vlc.h"

static const int ReadHRDParameters(Bitstream *s, avc::hrd_parameters_t *hrd, DecoderParams *p_Dec)
{
  unsigned int SchedSelIdx;

  hrd->cpb_cnt_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);
  hrd->bit_rate_scale = s->read_u_v(4, s, &p_Dec->UsedBits);
  hrd->cpb_size_scale = s->read_u_v(4, s, &p_Dec->UsedBits);

  for (SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++)
  {
    hrd->bit_rate_value_minus1[SchedSelIdx] = s->read_ue_v(s, &p_Dec->UsedBits);
    hrd->cpb_size_value_minus1[SchedSelIdx] = s->read_ue_v(s, &p_Dec->UsedBits);
    hrd->cbr_flag[SchedSelIdx] = s->read_u_1(s, &p_Dec->UsedBits);
  }

  hrd->initial_cpb_removal_delay_length_minus1 = s->read_u_v(5, s, &p_Dec->UsedBits);
  hrd->cpb_removal_delay_length_minus1 = s->read_u_v(5, s, &p_Dec->UsedBits);
  hrd->dpb_output_delay_length_minus1 = s->read_u_v(5, s, &p_Dec->UsedBits);
  hrd->time_offset_length = s->read_u_v(5, s, &p_Dec->UsedBits);

  return 0;
}

static const void Scaling_List(Bitstream *s, int *scalingList, int sizeOfScalingList, bool *UseDefaultScalingMatrix, DecoderParams *p_Dec)
{
  int j, scanj;
  int delta_scale, lastScale, nextScale;

  lastScale = 8;
  nextScale = 8;

  for (j = 0; j < sizeOfScalingList; j++)
  {
    scanj = (sizeOfScalingList == 16) ? avc::ZZ_SCAN[j] : avc::ZZ_SCAN8[j];

    if (nextScale != 0)
    {
      delta_scale = s->read_se_v(s, &p_Dec->UsedBits);
      nextScale = (lastScale + delta_scale + 256) % 256;
      *UseDefaultScalingMatrix = (bool)(scanj == 0 && nextScale == 0);
    }

    scalingList[scanj] = (nextScale == 0) ? lastScale : nextScale;
    lastScale = scalingList[scanj];
  }
}

static const void InitVUI(avc::sps *sps)
{
  sps->vui_seq_parameters.matrix_coefficients = 2;
}

static const int ReadVUI(Bitstream *s, avc::sps *sps, DecoderParams *p_Dec)
{
  if (sps->vui_parameters_present_flag)
  {
    sps->vui_seq_parameters.aspect_ratio_info_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.aspect_ratio_info_present_flag)
    {
      sps->vui_seq_parameters.aspect_ratio_idc = s->read_u_v(8, s, &p_Dec->UsedBits);
      if (255 == sps->vui_seq_parameters.aspect_ratio_idc)
      {
        sps->vui_seq_parameters.sar_width = (unsigned short)s->read_u_v(16, s, &p_Dec->UsedBits);
        sps->vui_seq_parameters.sar_height = (unsigned short)s->read_u_v(16, s, &p_Dec->UsedBits);
      }
    }

    sps->vui_seq_parameters.overscan_info_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.overscan_info_present_flag)
    {
      sps->vui_seq_parameters.overscan_appropriate_flag = s->read_u_1(s, &p_Dec->UsedBits);
    }

    sps->vui_seq_parameters.video_signal_type_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.video_signal_type_present_flag)
    {
      sps->vui_seq_parameters.video_format = s->read_u_v(3, s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.video_full_range_flag = s->read_u_1(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.colour_description_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
      if (sps->vui_seq_parameters.colour_description_present_flag)
      {
        sps->vui_seq_parameters.colour_primaries = s->read_u_v(8, s, &p_Dec->UsedBits);
        sps->vui_seq_parameters.transfer_characteristics = s->read_u_v(8, s, &p_Dec->UsedBits);
        sps->vui_seq_parameters.matrix_coefficients = s->read_u_v(8, s, &p_Dec->UsedBits);
      }
    }
    sps->vui_seq_parameters.chroma_location_info_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.chroma_location_info_present_flag)
    {
      sps->vui_seq_parameters.chroma_sample_loc_type_top_field = s->read_ue_v(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.chroma_sample_loc_type_bottom_field = s->read_ue_v(s, &p_Dec->UsedBits);
    }
    sps->vui_seq_parameters.timing_info_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.timing_info_present_flag)
    {
      sps->vui_seq_parameters.num_units_in_tick = s->read_u_v(32, s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.time_scale = s->read_u_v(32, s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.fixed_frame_rate_flag = s->read_u_1(s, &p_Dec->UsedBits);
    }
    sps->vui_seq_parameters.nal_hrd_parameters_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
    {
      ReadHRDParameters(s, &(sps->vui_seq_parameters.nal_hrd_parameters), p_Dec);
    }
    sps->vui_seq_parameters.vcl_hrd_parameters_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
      ReadHRDParameters(s, &(sps->vui_seq_parameters.vcl_hrd_parameters), p_Dec);
    }
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag || sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
      sps->vui_seq_parameters.low_delay_hrd_flag = s->read_u_1(s, &p_Dec->UsedBits);
    }
    sps->vui_seq_parameters.pic_struct_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
    sps->vui_seq_parameters.bitstream_restriction_flag = s->read_u_1(s, &p_Dec->UsedBits);
    if (sps->vui_seq_parameters.bitstream_restriction_flag)
    {
      sps->vui_seq_parameters.motion_vectors_over_pic_boundaries_flag = s->read_u_1(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.max_bytes_per_pic_denom = s->read_ue_v(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.max_bits_per_mb_denom = s->read_ue_v(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.log2_max_mv_length_horizontal = s->read_ue_v(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.log2_max_mv_length_vertical = s->read_ue_v(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.num_reorder_frames = s->read_ue_v(s, &p_Dec->UsedBits);
      sps->vui_seq_parameters.max_dec_frame_buffering = s->read_ue_v(s, &p_Dec->UsedBits);
    }
  }

  return 0;
}

void parseNalH264::vps_parse(unsigned char *nal_bitstream, avc::sps *sps, int curLen, parsingLevel level)
{
  // Meaningless method to prevent compilation error
}

void parseNalH264::sps_parse(unsigned char *nal_bitstream, avc::sps *sps, int curLen, parsingLevel level)
{
  unsigned i;
  unsigned n_ScalingList;
  int reserved_zero;
  Bitstream *s = m_bits;
  s->streamBuffer = nal_bitstream;
  s->code_len = s->bitstream_length = curLen;
  s->ei_flag = 0;
  s->read_len = s->frame_bitoffset = 0;
  DecoderParams *p_Dec = m_pDec;
  p_Dec->UsedBits = 0;

  sps->profile_idc = s->read_u_v(8, s, &p_Dec->UsedBits);

  if ((sps->profile_idc != avc::BASELINE) &&
      (sps->profile_idc != avc::MAIN) &&
      (sps->profile_idc != avc::EXTENDED) &&
      (sps->profile_idc != avc::FREXT_HP) &&
      (sps->profile_idc != avc::FREXT_Hi10P) &&
      (sps->profile_idc != avc::FREXT_Hi422) &&
      (sps->profile_idc != avc::FREXT_Hi444) &&
      (sps->profile_idc != avc::FREXT_CAVLC444) && (sps->profile_idc != avc::MVC_HIGH) && (sps->profile_idc != avc::STEREO_HIGH))
  {
    printf("Invalid Profile IDC (%d) encountered. \n", sps->profile_idc);
  }

  sps->constrained_set0_flag = s->read_u_1(s, &p_Dec->UsedBits);
  sps->constrained_set1_flag = s->read_u_1(s, &p_Dec->UsedBits);
  sps->constrained_set2_flag = s->read_u_1(s, &p_Dec->UsedBits);
  sps->constrained_set3_flag = s->read_u_1(s, &p_Dec->UsedBits);
  sps->constrained_set4_flag = s->read_u_1(s, &p_Dec->UsedBits);
  sps->constrained_set5_flag = s->read_u_1(s, &p_Dec->UsedBits);
  reserved_zero = s->read_u_v(2, s, &p_Dec->UsedBits);

  if (reserved_zero != 0)
  {
    printf("Warning, reserved_zero flag not equal to 0. Possibly new constrained_setX flag introduced.\n");
  }

  sps->level_idc = s->read_u_v(8, s, &p_Dec->UsedBits);

  sps->seq_parameter_set_id = s->read_ue_v(s, &p_Dec->UsedBits);

  if (level == parsingLevel::PARSING_PARAM_ID)
  {
    return;
  }

  sps->chroma_format_idc = 1;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->lossless_qpprime_flag = 0;
  sps->separate_colour_plane_flag = 0;

  if ((sps->profile_idc == avc::FREXT_HP) ||
      (sps->profile_idc == avc::FREXT_Hi10P) ||
      (sps->profile_idc == avc::FREXT_Hi422) ||
      (sps->profile_idc == avc::FREXT_Hi444) ||
      (sps->profile_idc == avc::FREXT_CAVLC444) ||
      (sps->profile_idc == avc::MVC_HIGH) ||
      (sps->profile_idc == avc::STEREO_HIGH))
  {
    sps->chroma_format_idc = s->read_ue_v(s, &p_Dec->UsedBits);

    if (sps->chroma_format_idc == avc::YUV444)
    {
      sps->separate_colour_plane_flag = s->read_u_1(s, &p_Dec->UsedBits);
    }

    sps->bit_depth_luma_minus8 = s->read_ue_v(s, &p_Dec->UsedBits);
    sps->bit_depth_chroma_minus8 = s->read_ue_v(s, &p_Dec->UsedBits);
    sps->lossless_qpprime_flag = s->read_u_1(s, &p_Dec->UsedBits);

    sps->seq_scaling_matrix_present_flag = s->read_u_1(s, &p_Dec->UsedBits);

    if (sps->seq_scaling_matrix_present_flag)
    {
      n_ScalingList = (sps->chroma_format_idc != avc::YUV444) ? 8 : 12;
      for (i = 0; i < n_ScalingList; i++)
      {
        sps->seq_scaling_list_present_flag[i] = s->read_u_1(s, &p_Dec->UsedBits);
        if (sps->seq_scaling_list_present_flag[i])
        {
          if (i < 6)
            Scaling_List(s, sps->ScalingList4x4[i], 16, &sps->UseDefaultScalingMatrix4x4Flag[i], p_Dec);
          else
            Scaling_List(s, sps->ScalingList8x8[i - 6], 64, &sps->UseDefaultScalingMatrix8x8Flag[i - 6], p_Dec);
        }
      }
    }
  }

  sps->log2_max_frame_num_minus4 = s->read_ue_v(s, &p_Dec->UsedBits);
  sps->pic_order_cnt_type = s->read_ue_v(s, &p_Dec->UsedBits);

  if (sps->pic_order_cnt_type == 0)
    sps->log2_max_pic_order_cnt_lsb_minus4 = s->read_ue_v(s, &p_Dec->UsedBits);
  else if (sps->pic_order_cnt_type == 1)
  {
    sps->delta_pic_order_always_zero_flag = s->read_u_1(s, &p_Dec->UsedBits);
    sps->offset_for_non_ref_pic = s->read_se_v(s, &p_Dec->UsedBits);
    sps->offset_for_top_to_bottom_field = s->read_se_v(s, &p_Dec->UsedBits);
    sps->num_ref_frames_in_pic_order_cnt_cycle = s->read_ue_v(s, &p_Dec->UsedBits);
    for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      sps->offset_for_ref_frame[i] = s->read_se_v(s, &p_Dec->UsedBits);
  }
  sps->num_ref_frames = s->read_ue_v(s, &p_Dec->UsedBits);
  sps->gaps_in_frame_num_value_allowed_flag = s->read_u_1(s, &p_Dec->UsedBits);
  sps->pic_width_in_mbs_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);
  sps->pic_height_in_map_units_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);
  sps->frame_mbs_only_flag = s->read_u_1(s, &p_Dec->UsedBits);
  if (!sps->frame_mbs_only_flag)
  {
    sps->mb_adaptive_frame_field_flag = s->read_u_1(s, &p_Dec->UsedBits);
  }
  sps->direct_8x8_inference_flag = s->read_u_1(s, &p_Dec->UsedBits);
  sps->frame_cropping_flag = s->read_u_1(s, &p_Dec->UsedBits);

  if (sps->frame_cropping_flag)
  {
    sps->frame_crop_left_offset = s->read_ue_v(s, &p_Dec->UsedBits);
    sps->frame_crop_right_offset = s->read_ue_v(s, &p_Dec->UsedBits);
    sps->frame_crop_top_offset = s->read_ue_v(s, &p_Dec->UsedBits);
    sps->frame_crop_bottom_offset = s->read_ue_v(s, &p_Dec->UsedBits);
  }
  sps->vui_parameters_present_flag = (bool)s->read_u_1(s, &p_Dec->UsedBits);

  InitVUI(sps);
  ReadVUI(s, sps, p_Dec);

  sps->Valid = true;
}

void parseNalH264::pps_parse(unsigned char *nal_bitstream, avc::pps *pps, avc::sps *sps, int curLen, parsingLevel level)
{
  unsigned i;
  unsigned n_ScalingList;
  int chroma_format_idc;
  int NumberBitsPerSliceGroupId;
  Bitstream *s = m_bits;
  s->streamBuffer = nal_bitstream;
  s->code_len = s->bitstream_length = curLen;
  s->ei_flag = 0;
  s->read_len = s->frame_bitoffset = 0;
  DecoderParams *p_Dec = m_pDec;
  p_Dec->UsedBits = 0;

  pps->pic_parameter_set_id = s->read_ue_v(s, &p_Dec->UsedBits);
  pps->seq_parameter_set_id = s->read_ue_v(s, &p_Dec->UsedBits);

  if (level == parsingLevel::PARSING_PARAM_ID)
  {
    return;
  }

  pps->entropy_coding_mode_flag = s->read_u_1(s, &p_Dec->UsedBits);
  pps->bottom_field_pic_order_in_frame_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
  pps->num_slice_groups_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);

  if (pps->num_slice_groups_minus1 > 0)
  {
    pps->slice_group_map_type = s->read_ue_v(s, &p_Dec->UsedBits);
    if (pps->slice_group_map_type == 0)
    {
      for (i = 0; i <= pps->num_slice_groups_minus1; i++)
        pps->run_length_minus1[i] = s->read_ue_v(s, &p_Dec->UsedBits);
    }
    else if (pps->slice_group_map_type == 2)
    {
      for (i = 0; i < pps->num_slice_groups_minus1; i++)
      {
        pps->top_left[i] = s->read_ue_v(s, &p_Dec->UsedBits);
        pps->bottom_right[i] = s->read_ue_v(s, &p_Dec->UsedBits);
      }
    }
    else if (pps->slice_group_map_type == 3 ||
             pps->slice_group_map_type == 4 ||
             pps->slice_group_map_type == 5)
    {
      pps->slice_group_change_direction_flag = s->read_u_1(s, &p_Dec->UsedBits);
      pps->slice_group_change_rate_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);
    }
    else if (pps->slice_group_map_type == 6)
    {
      if (pps->num_slice_groups_minus1 + 1 > 4)
        NumberBitsPerSliceGroupId = 3;
      else if (pps->num_slice_groups_minus1 + 1 > 2)
        NumberBitsPerSliceGroupId = 2;
      else
        NumberBitsPerSliceGroupId = 1;
      pps->pic_size_in_map_units_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);
      for (i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
        pps->slice_group_id[i] = (unsigned char)s->read_u_v(NumberBitsPerSliceGroupId, s, &p_Dec->UsedBits);
    }
  }

  pps->num_ref_idx_l0_default_active_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);
  pps->num_ref_idx_l1_default_active_minus1 = s->read_ue_v(s, &p_Dec->UsedBits);
  pps->weighted_pred_flag = s->read_u_1(s, &p_Dec->UsedBits);
  pps->weighted_bipred_idc = s->read_u_v(2, s, &p_Dec->UsedBits);
  pps->pic_init_qp_minus26 = s->read_se_v(s, &p_Dec->UsedBits);
  pps->pic_init_qs_minus26 = s->read_se_v(s, &p_Dec->UsedBits);

  pps->chroma_qp_index_offset = s->read_se_v(s, &p_Dec->UsedBits);

  pps->deblocking_filter_control_present_flag = s->read_u_1(s, &p_Dec->UsedBits);
  pps->constrained_intra_pred_flag = s->read_u_1(s, &p_Dec->UsedBits);
  pps->redundant_pic_cnt_present_flag = s->read_u_1(s, &p_Dec->UsedBits);

  if (s->more_rbsp_data(s->streamBuffer, s->frame_bitoffset, s->bitstream_length))
  {
    pps->transform_8x8_mode_flag = s->read_u_1(s, &p_Dec->UsedBits);
    pps->pic_scaling_matrix_present_flag = s->read_u_1(s, &p_Dec->UsedBits);

    if (pps->pic_scaling_matrix_present_flag)
    {
      chroma_format_idc = sps->chroma_format_idc; // FIXME: May be in potentail bugs (sps would not be indicated from current pps)
      n_ScalingList = 6 + ((chroma_format_idc != avc::YUV444) ? 2 : 6) * pps->transform_8x8_mode_flag;
      for (i = 0; i < n_ScalingList; i++)
      {
        pps->pic_scaling_list_present_flag[i] = s->read_u_1(s, &p_Dec->UsedBits);

        if (pps->pic_scaling_list_present_flag[i])
        {
          if (i < 6)
            Scaling_List(s, pps->ScalingList4x4[i], 16, &pps->UseDefaultScalingMatrix4x4Flag[i], p_Dec);
          else
            Scaling_List(s, pps->ScalingList8x8[i - 6], 64, &pps->UseDefaultScalingMatrix8x8Flag[i - 6], p_Dec);
        }
      }
    }
    pps->second_chroma_qp_index_offset = s->read_se_v(s, &p_Dec->UsedBits);
  }
  else
  {
    pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
  }

  pps->Valid = true;
}

void parseNalH264::sei_parse(unsigned char *msg, nal_info &nal, int curLen)
{
  avc::sps *sps = reinterpret_cast<avc::sps *>(nal.mpegParamSet->sps);
  parseSeiH264 fCallobj;

  int payload_type = 0;
  int payload_size = 0;
  int offset = 0;
  unsigned char tmp_byte;

  do
  {
    payload_type = 0;
    tmp_byte = msg[offset++];
    while (tmp_byte == 0xFF)
    {
      payload_type += 255;
      tmp_byte = msg[offset++];
    }
    payload_type += tmp_byte;

    payload_size = 0;
    tmp_byte = msg[offset++];
    while (tmp_byte == 0xFF)
    {
      payload_size += 255;
      tmp_byte = msg[offset++];
    }
    payload_size += tmp_byte;

    nal.sei_type = payload_type;
    nal.sei_length = payload_size;

    switch (payload_type)
    {
    case avc::SEI_BUFFERING_PERIOD:
      fCallobj.interpret_buffering_period_info(msg + offset, payload_size, sps, nal.h264SEI->h264_sei_bp);
      break;
    case avc::SEI_PIC_TIMING:
      fCallobj.interpret_picture_timing_info(msg + offset, payload_size, sps, nal.h264SEI->h264_sei_pt);
      break;
    case avc::SEI_PAN_SCAN_RECT:
      // interpret_pan_scan_rect_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_FILLER_PAYLOAD:
      // interpret_filler_payload_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_USER_DATA_REGISTERED_ITU_T_T35:
      fCallobj.interpret_user_data_registered_itu_t_t35_info(msg + offset, payload_size, nal.mpegCommonSEI->common_sei_dr);
      break;
    case avc::SEI_USER_DATA_UNREGISTERED:
      fCallobj.interpret_user_data_unregistered_info(msg + offset, payload_size, nal.mpegCommonSEI->common_sei_du);
      break;
    case avc::SEI_RECOVERY_POINT:
      // interpret_recovery_point_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_DEC_REF_PIC_MARKING_REPETITION:
      // interpret_dec_ref_pic_marking_repetition_info( msg+offset, payload_size, sps, pSlice ); // pSlice → Cannot parse
      break;
    case avc::SEI_SPARE_PIC:
      // interpret_spare_pic( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_SCENE_INFO:
      // interpret_scene_information( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_SUB_SEQ_INFO:
      // interpret_subsequence_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
      // interpret_subsequence_layer_characteristics_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_SUB_SEQ_CHARACTERISTICS:
      // interpret_subsequence_characteristics_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_FULL_FRAME_FREEZE:
      // interpret_full_frame_freeze_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_FULL_FRAME_FREEZE_RELEASE:
      // interpret_full_frame_freeze_release_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_FULL_FRAME_SNAPSHOT:
      // interpret_full_frame_snapshot_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
      // interpret_progressive_refinement_start_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
      // interpret_progressive_refinement_end_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
      // interpret_motion_constrained_slice_group_set_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
    case avc::SEI_FILM_GRAIN_CHARACTERISTICS:
      // interpret_film_grain_characteristics_info ( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
      // interpret_deblocking_filter_display_preference_info ( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_STEREO_VIDEO_INFO:
      // interpret_stereo_video_info_info ( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_TONE_MAPPING:
      // interpret_tone_mapping( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_POST_FILTER_HINTS:
      // interpret_post_filter_hints_info ( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_FRAME_PACKING_ARRANGEMENT:
      // interpret_frame_packing_arrangement_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    case avc::SEI_GREEN_METADATA:
      // interpret_green_metadata_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    default:
      // interpret_reserved_info( msg+offset, payload_size, sps ); // TODO: Not implemented yet
      break;
    }
    offset += payload_size;

  } while (msg[offset] != 0x80);
  assert(msg[offset] == 0x80);
}