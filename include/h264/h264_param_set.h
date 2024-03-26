#pragma once

namespace avc
{
  typedef struct
  {
  public:
    unsigned int cpb_cnt_minus1;
    unsigned int bit_rate_scale;
    unsigned int cpb_size_scale;
    unsigned int bit_rate_value_minus1[32];
    unsigned int cpb_size_value_minus1[32];
    unsigned int cbr_flag[32];
    unsigned int initial_cpb_removal_delay_length_minus1;
    unsigned int cpb_removal_delay_length_minus1;
    unsigned int dpb_output_delay_length_minus1;
    unsigned int time_offset_length;
  } hrd_parameters_t;

  typedef struct
  {
  public:
    bool aspect_ratio_info_present_flag;
    unsigned int aspect_ratio_idc;
    unsigned short sar_width;
    unsigned short sar_height;
    bool overscan_info_present_flag;
    bool overscan_appropriate_flag;
    bool video_signal_type_present_flag;
    unsigned int video_format;
    bool video_full_range_flag;
    bool colour_description_present_flag;
    unsigned int colour_primaries;
    unsigned int transfer_characteristics;
    unsigned int matrix_coefficients;
    bool chroma_location_info_present_flag;
    unsigned int chroma_sample_loc_type_top_field;
    unsigned int chroma_sample_loc_type_bottom_field;
    bool timing_info_present_flag;
    unsigned int num_units_in_tick;
    unsigned int time_scale;
    bool fixed_frame_rate_flag;
    bool nal_hrd_parameters_present_flag;
    hrd_parameters_t nal_hrd_parameters;
    bool vcl_hrd_parameters_present_flag;
    hrd_parameters_t vcl_hrd_parameters;
    bool low_delay_hrd_flag;
    bool pic_struct_present_flag;
    bool bitstream_restriction_flag;
    bool motion_vectors_over_pic_boundaries_flag;
    unsigned int max_bytes_per_pic_denom;
    unsigned int max_bits_per_mb_denom;
    unsigned int log2_max_mv_length_vertical;
    unsigned int log2_max_mv_length_horizontal;
    unsigned int num_reorder_frames;
    unsigned int max_dec_frame_buffering;
  } vui_seq_parameters_t;

  typedef struct
  {
  public:
    bool Valid;

    unsigned int profile_idc;
    bool constrained_set0_flag;
    bool constrained_set1_flag;
    bool constrained_set2_flag;
    bool constrained_set3_flag;
    bool constrained_set4_flag;
    bool constrained_set5_flag;
    unsigned int level_idc;
    unsigned int seq_parameter_set_id;
    unsigned int chroma_format_idc;

    bool seq_scaling_matrix_present_flag;
    int seq_scaling_list_present_flag[12];
    int ScalingList4x4[6][16];
    int ScalingList8x8[6][64];
    bool UseDefaultScalingMatrix4x4Flag[6];
    bool UseDefaultScalingMatrix8x8Flag[6];

    unsigned int bit_depth_luma_minus8;
    unsigned int bit_depth_chroma_minus8;
    unsigned int log2_max_frame_num_minus4;
    unsigned int pic_order_cnt_type;
    unsigned int log2_max_pic_order_cnt_lsb_minus4;
    bool delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    unsigned int num_ref_frames_in_pic_order_cnt_cycle;
    int offset_for_ref_frame[256];
    unsigned int num_ref_frames;
    bool gaps_in_frame_num_value_allowed_flag;
    unsigned int pic_width_in_mbs_minus1;
    unsigned int pic_height_in_map_units_minus1;
    bool frame_mbs_only_flag;
    bool mb_adaptive_frame_field_flag;
    bool direct_8x8_inference_flag;
    bool frame_cropping_flag;
    unsigned int frame_crop_left_offset;
    unsigned int frame_crop_right_offset;
    unsigned int frame_crop_top_offset;
    unsigned int frame_crop_bottom_offset;
    bool vui_parameters_present_flag;
    vui_seq_parameters_t vui_seq_parameters;
    unsigned separate_colour_plane_flag;
    int max_dec_frame_buffering;
    int lossless_qpprime_flag;
  } sps;

  typedef struct
  {
  public:
    bool Valid;

    unsigned int pic_parameter_set_id;
    unsigned int seq_parameter_set_id;
    bool entropy_coding_mode_flag;
    bool transform_8x8_mode_flag;

    bool pic_scaling_matrix_present_flag;
    int pic_scaling_list_present_flag[12];
    int ScalingList4x4[6][16];
    int ScalingList8x8[6][64];
    bool UseDefaultScalingMatrix4x4Flag[6];
    bool UseDefaultScalingMatrix8x8Flag[6];

    bool bottom_field_pic_order_in_frame_present_flag;
    unsigned int num_slice_groups_minus1;
    unsigned int slice_group_map_type;
    unsigned int run_length_minus1[8];
    unsigned int top_left[8];
    unsigned int bottom_right[8];
    bool slice_group_change_direction_flag;
    unsigned int slice_group_change_rate_minus1;
    unsigned int pic_size_in_map_units_minus1;
    unsigned char *slice_group_id;

    int num_ref_idx_l0_default_active_minus1;
    int num_ref_idx_l1_default_active_minus1;
    bool weighted_pred_flag;
    unsigned int weighted_bipred_idc;
    int pic_init_qp_minus26;
    int pic_init_qs_minus26;
    int chroma_qp_index_offset;

    int cb_qp_index_offset;
    int cr_qp_index_offset;
    int second_chroma_qp_index_offset;

    bool deblocking_filter_control_present_flag;
    bool constrained_intra_pred_flag;
    bool redundant_pic_cnt_present_flag;
    bool vui_pic_parameters_flag;
  } pps;
}