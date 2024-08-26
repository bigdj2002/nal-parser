#include "nal_parse.h"
#include "hevc_nal.h"
#include "hevc_vlc.h"

void parseNalH265::xDecodeScalingList(hevc::TComScalingList *scalingList, unsigned int sizeId, unsigned int listId)
{
  int i, coefNum = std::min(hevc::MAX_MATRIX_COEF_NUM, (int)hevc::g_scalingListSize[sizeId]);
  int data;
  int scalingListDcCoefMinus8 = 0;
  int nextCoef = hevc::SCALING_LIST_START_VALUE;
  // unsigned int *scan = g_scanOrder[SCAN_UNGROUPED][SCAN_DIAG][sizeId == 0 ? 2 : 3][sizeId == 0 ? 2 : 3];
  [[maybe_unused]] int *dst = scalingList->getScalingListAddress(sizeId, listId);

  if (sizeId > hevc::SCALING_LIST_8x8)
  {
    xReadSvlc(scalingListDcCoefMinus8, "scaling_list_dc_coef_minus8");
    scalingList->setScalingListDC(sizeId, listId, scalingListDcCoefMinus8 + 8);
    nextCoef = scalingList->getScalingListDC(sizeId, listId);
  }

  for (i = 0; i < coefNum; i++)
  {
    xReadSvlc(data, "scaling_list_delta_coef");
    nextCoef = (nextCoef + data + 256) % 256;
    // dst[scan[i]] = nextCoef;
  }
}

bool parseNalH265::xMoreRbspData()
{
  int bitsLeft = m_pcBitstream->getNumBitsLeft();

  if (bitsLeft > 8)
  {
    return true;
  }

  unsigned char lastByte = m_pcBitstream->peekBits(bitsLeft);
  int cnt = bitsLeft;

  while ((cnt > 0) && ((lastByte & 1) == 0))
  {
    lastByte >>= 1;
    cnt--;
  }
  cnt--;

  assert(cnt >= 0);

  return (cnt > 0);
}

void parseNalH265::parseHrdParameters(hevc::TComHRD *hrd, bool commonInfPresentFlag, unsigned int maxNumSubLayersMinus1)
{
  unsigned int uiCode;
  if (commonInfPresentFlag)
  {
    xReadFlag(uiCode, "nal_hrd_parameters_present_flag");
    hrd->m_nalHrdParametersPresentFlag = uiCode == 1 ? true : false;
    xReadFlag(uiCode, "vcl_hrd_parameters_present_flag");
    hrd->m_vclHrdParametersPresentFlag = uiCode == 1 ? true : false;
    if (hrd->m_nalHrdParametersPresentFlag || hrd->m_vclHrdParametersPresentFlag)
    {
      xReadFlag(uiCode, "sub_pic_hrd_params_present_flag");
      hrd->m_subPicCpbParamsPresentFlag = uiCode == 1 ? true : false;
      if (hrd->m_subPicCpbParamsPresentFlag)
      {
        xReadCode(8, uiCode, "tick_divisor_minus2");
        hrd->m_tickDivisorMinus2 = uiCode;
        xReadCode(5, uiCode, "du_cpb_removal_delay_increment_length_minus1");
        hrd->m_duCpbRemovalDelayLengthMinus1 = uiCode;
        xReadFlag(uiCode, "sub_pic_cpb_params_in_pic_timing_sei_flag");
        hrd->m_subPicCpbParamsInPicTimingSEIFlag = uiCode == 1 ? true : false;
        xReadCode(5, uiCode, "dpb_output_delay_du_length_minus1");
        hrd->m_dpbOutputDelayDuLengthMinus1 = uiCode;
      }
      xReadCode(4, uiCode, "bit_rate_scale");
      hrd->m_bitRateScale = uiCode;
      xReadCode(4, uiCode, "cpb_size_scale");
      hrd->m_cpbSizeScale = uiCode;
      if (hrd->m_subPicCpbParamsPresentFlag)
      {
        xReadCode(4, uiCode, "cpb_size_du_scale");
        hrd->m_ducpbSizeScale = uiCode;
      }
      xReadCode(5, uiCode, "initial_cpb_removal_delay_length_minus1");
      hrd->m_initialCpbRemovalDelayLengthMinus1 = uiCode;
      xReadCode(5, uiCode, "au_cpb_removal_delay_length_minus1");
      hrd->m_cpbRemovalDelayLengthMinus1 = uiCode;
      xReadCode(5, uiCode, "dpb_output_delay_length_minus1");
      hrd->m_dpbOutputDelayLengthMinus1 = uiCode;
    }
  }
  unsigned int i, j, nalOrVcl;
  for (i = 0; i <= maxNumSubLayersMinus1; i++)
  {
    xReadFlag(uiCode, "fixed_pic_rate_general_flag");
    hrd->m_HRD[i].fixedPicRateFlag = uiCode == 1 ? true : false;
    if (!hrd->m_HRD[i].fixedPicRateFlag)
    {
      xReadFlag(uiCode, "fixed_pic_rate_within_cvs_flag");
      hrd->m_HRD[i].fixedPicRateWithinCvsFlag = uiCode == 1 ? true : false;
    }
    else
    {
      hrd->m_HRD[i].fixedPicRateWithinCvsFlag = true;
    }

    hrd->m_HRD[i].lowDelayHrdFlag = 0;
    hrd->m_HRD[i].cpbCntMinus1 = 0;

    if (hrd->m_HRD[i].fixedPicRateWithinCvsFlag)
    {
      xReadUvlc(uiCode, "elemental_duration_in_tc_minus1");
      hrd->m_HRD[i].picDurationInTcMinus1 = uiCode;
    }
    else
    {
      xReadFlag(uiCode, "low_delay_hrd_flag");
      hrd->m_HRD[i].lowDelayHrdFlag = uiCode == 1 ? true : false;
    }
    if (!hrd->m_HRD[i].lowDelayHrdFlag)
    {
      xReadUvlc(uiCode, "cpb_cnt_minus1");
      hrd->m_HRD[i].cpbCntMinus1 = uiCode;
    }

    for (nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
    {
      if (((nalOrVcl == 0) && (hrd->m_nalHrdParametersPresentFlag)) ||
          ((nalOrVcl == 1) && (hrd->m_vclHrdParametersPresentFlag)))
      {
        for (j = 0; j <= (hrd->m_HRD[i].cpbCntMinus1); j++)
        {
          xReadUvlc(uiCode, "bit_rate_value_minus1");
          hrd->m_HRD[i].bitRateValueMinus1[j][nalOrVcl] = uiCode;
          xReadUvlc(uiCode, "cpb_size_value_minus1");
          hrd->m_HRD[i].cpbSizeValue[j][nalOrVcl] = uiCode;
          if (hrd->m_subPicCpbParamsPresentFlag)
          {
            xReadUvlc(uiCode, "cpb_size_du_value_minus1");
            hrd->m_HRD[i].ducpbSizeValue[j][nalOrVcl] = uiCode;
            xReadUvlc(uiCode, "bit_rate_du_value_minus1");
            hrd->m_HRD[i].duBitRateValue[j][nalOrVcl] = uiCode;
          }
          xReadFlag(uiCode, "cbr_flag");
          hrd->m_HRD[i].cbrFlag[j][nalOrVcl] = uiCode == 1 ? true : false;
        }
      }
    }
  }
}

void parseNalH265::parseVUI(hevc::TComVUI *pcVUI, hevc::sps *pcSPS)
{
  unsigned int uiCode;

  xReadFlag(uiCode, "aspect_ratio_info_present_flag");
  pcVUI->m_aspectRatioInfoPresentFlag = uiCode;
  if (pcVUI->m_aspectRatioInfoPresentFlag)
  {
    xReadCode(8, uiCode, "aspect_ratio_idc");
    pcVUI->m_aspectRatioIdc = uiCode;
    if (pcVUI->m_aspectRatioIdc == 255)
    {
      xReadCode(16, uiCode, "sar_width");
      pcVUI->m_sarWidth = uiCode;
      xReadCode(16, uiCode, "sar_height");
      pcVUI->m_sarHeight = uiCode;
    }
  }

  xReadFlag(uiCode, "overscan_info_present_flag");
  pcVUI->m_overscanInfoPresentFlag = uiCode;
  if (pcVUI->m_overscanInfoPresentFlag)
  {
    xReadFlag(uiCode, "overscan_appropriate_flag");
    pcVUI->m_overscanAppropriateFlag = uiCode;
  }

  xReadFlag(uiCode, "video_signal_type_present_flag");
  pcVUI->m_videoSignalTypePresentFlag = uiCode;
  if (pcVUI->m_videoSignalTypePresentFlag)
  {
    xReadCode(3, uiCode, "video_format");
    pcVUI->m_videoFormat = uiCode;
    xReadFlag(uiCode, "video_full_range_flag");
    pcVUI->m_videoFullRangeFlag = uiCode;
    xReadFlag(uiCode, "colour_description_present_flag");
    pcVUI->m_colourDescriptionPresentFlag = uiCode;
    if (pcVUI->m_colourDescriptionPresentFlag)
    {
      xReadCode(8, uiCode, "colour_primaries");
      pcVUI->m_colourPrimaries = uiCode;
      xReadCode(8, uiCode, "transfer_characteristics");
      pcVUI->m_transferCharacteristics = uiCode;
      xReadCode(8, uiCode, "matrix_coeffs");
      pcVUI->m_matrixCoefficients = uiCode;
    }
  }

  xReadFlag(uiCode, "chroma_loc_info_present_flag");
  pcVUI->m_chromaLocInfoPresentFlag = uiCode;
  if (pcVUI->m_chromaLocInfoPresentFlag)
  {
    xReadUvlc(uiCode, "chroma_sample_loc_type_top_field");
    pcVUI->m_chromaSampleLocTypeTopField = uiCode;
    xReadUvlc(uiCode, "chroma_sample_loc_type_bottom_field");
    pcVUI->m_chromaSampleLocTypeBottomField = uiCode;
  }

  xReadFlag(uiCode, "neutral_chroma_indication_flag");
  pcVUI->m_neutralChromaIndicationFlag = uiCode;

  xReadFlag(uiCode, "field_seq_flag");
  pcVUI->m_fieldSeqFlag = uiCode;

  xReadFlag(uiCode, "frame_field_info_present_flag");
  pcVUI->m_frameFieldInfoPresentFlag = uiCode;

  xReadFlag(uiCode, "default_display_window_flag");
  if (uiCode != 0)
  {
    hevc::Window &defDisp = pcVUI->m_defaultDisplayWindow;
    xReadUvlc(uiCode, "def_disp_win_left_offset");
    defDisp.m_winLeftOffset = uiCode * pcSPS->m_winUnitX[pcSPS->m_chromaFormatIdc];
    xReadUvlc(uiCode, "def_disp_win_right_offset");
    defDisp.m_winRightOffset = uiCode * pcSPS->m_winUnitX[pcSPS->m_chromaFormatIdc];
    xReadUvlc(uiCode, "def_disp_win_top_offset");
    defDisp.m_winTopOffset = uiCode * pcSPS->m_winUnitX[pcSPS->m_chromaFormatIdc];
    xReadUvlc(uiCode, "def_disp_win_bottom_offset");
    defDisp.m_winBottomOffset = uiCode * pcSPS->m_winUnitX[pcSPS->m_chromaFormatIdc];
  }

  hevc::TimingInfo *timingInfo = &(pcVUI->m_timingInfo);
  xReadFlag(uiCode, "vui_timing_info_present_flag");
  timingInfo->m_timingInfoPresentFlag = uiCode ? true : false;
  if (timingInfo->m_timingInfoPresentFlag)
  {
    xReadCode(32, uiCode, "vui_num_units_in_tick");
    timingInfo->m_numUnitsInTick = uiCode;
    xReadCode(32, uiCode, "vui_time_scale");
    timingInfo->m_timeScale = uiCode;
    xReadFlag(uiCode, "vui_poc_proportional_to_timing_flag");
    timingInfo->m_pocProportionalToTimingFlag = uiCode ? true : false;
    if (timingInfo->m_pocProportionalToTimingFlag)
    {
      xReadUvlc(uiCode, "vui_num_ticks_poc_diff_one_minus1");
      timingInfo->m_numTicksPocDiffOneMinus1 = uiCode;
    }

    xReadFlag(uiCode, "vui_hrd_parameters_present_flag");
    pcVUI->m_hrdParametersPresentFlag = uiCode;
    if (pcVUI->m_hrdParametersPresentFlag)
    {
      parseHrdParameters(&(pcVUI->m_hrdParameters), 1, pcSPS->m_uiMaxTLayers - 1);
    }
  }

  xReadFlag(uiCode, "bitstream_restriction_flag");
  pcVUI->m_bitstreamRestrictionFlag = uiCode;
  if (pcVUI->m_bitstreamRestrictionFlag)
  {
    xReadFlag(uiCode, "tiles_fixed_structure_flag");
    pcVUI->m_tilesFixedStructureFlag = uiCode;
    xReadFlag(uiCode, "motion_vectors_over_pic_boundaries_flag");
    pcVUI->m_motionVectorsOverPicBoundariesFlag = uiCode;
    xReadFlag(uiCode, "restricted_ref_pic_lists_flag");
    pcVUI->m_restrictedRefPicListsFlag = uiCode;
    xReadUvlc(uiCode, "min_spatial_segmentation_idc");
    pcVUI->m_minSpatialSegmentationIdc = uiCode;
    assert(uiCode < 4096);
    xReadUvlc(uiCode, "max_bytes_per_pic_denom");
    pcVUI->m_maxBytesPerPicDenom = uiCode;
    xReadUvlc(uiCode, "max_bits_per_min_cu_denom");
    pcVUI->m_maxBitsPerMinCuDenom = uiCode;
    xReadUvlc(uiCode, "log2_max_mv_length_horizontal");
    pcVUI->m_log2MaxMvLengthHorizontal = uiCode;
    xReadUvlc(uiCode, "log2_max_mv_length_vertical");
    pcVUI->m_log2MaxMvLengthHorizontal = uiCode;
  }
}

void parseNalH265::parseShortTermRefPicSet(hevc::sps *sps, hevc::TComReferencePictureSet *rps, int idx)
{
  unsigned int code;
  unsigned int interRPSPred;
  if (idx > 0)
  {
    xReadFlag(interRPSPred, "inter_ref_pic_set_prediction_flag");
    rps->m_interRPSPrediction = interRPSPred;
  }
  else
  {
    interRPSPred = false;
    rps->m_interRPSPrediction = false;
  }

  if (interRPSPred)
  {
    unsigned int bit;
    if (idx == (int)(sps->m_RPSList.m_referencePictureSets.size()))
    {
      xReadUvlc(code, "delta_idx_minus1"); // delta index of the Reference Picture Set used for prediction minus 1
    }
    else
    {
      code = 0;
    }
    rps->m_deltaRIdxMinus1 = code;
    assert((int)code <= idx - 1); // delta_idx_minus1 shall not be larger than idx-1, otherwise we will predict from a negative row position that does not exist. When idx equals 0 there is no legal value and interRPSPred must be zero. See J0185-r2
    int rIdx = idx - 1 - code;
    assert(rIdx <= idx - 1 && rIdx >= 0); // Made assert tighter; if rIdx = idx then prediction is done from itself. rIdx must belong to range 0, idx-1, inclusive, see J0185-r2
    hevc::TComReferencePictureSet *rpsRef = &(sps->m_RPSList.m_referencePictureSets[rIdx]);
    int k = 0, k0 = 0, k1 = 0;
    xReadCode(1, bit, "delta_rps_sign");       // delta_RPS_sign
    xReadUvlc(code, "abs_delta_rps_minus1");   // absolute delta RPS minus 1
    int deltaRPS = (1 - 2 * bit) * (code + 1); // delta_RPS
    rps->m_deltaRPS = deltaRPS;
    for (int j = 0; j <= rpsRef->m_numberOfPictures; j++)
    {
      xReadCode(1, bit, "used_by_curr_pic_flag"); // first bit is "1" if Idc is 1
      int refIdc = bit;
      if (refIdc == 0)
      {
        xReadCode(1, bit, "use_delta_flag"); // second bit is "1" if Idc is 2, "0" otherwise.
        refIdc = bit << 1;                   // second bit is "1" if refIdc is 2, "0" if refIdc = 0.
      }
      if (refIdc == 1 || refIdc == 2)
      {
        int deltaPOC = deltaRPS + ((j < rpsRef->m_numberOfPictures) ? rpsRef->m_deltaPOC[j] : 0);
        rps->m_deltaPOC[k] = deltaPOC;
        rps->m_used[k] = (refIdc == 1);

        if (deltaPOC < 0)
        {
          k0++;
        }
        else
        {
          k1++;
        }
        k++;
      }
      rps->m_refIdc[j] = refIdc;
    }
    rps->m_numRefIdc = rpsRef->m_numberOfPictures + 1;
    rps->m_numberOfPictures = k;
    rps->m_numberOfNegativePictures = k0;
    rps->m_numberOfPositivePictures = k1;
    // rps->sortDeltaPOC(); // TODO: Not implemented
  }
  else
  {
    xReadUvlc(code, "num_negative_pics");
    rps->m_numberOfNegativePictures = code;
    xReadUvlc(code, "num_positive_pics");
    rps->m_numberOfPositivePictures = code;
    int prev = 0;
    int poc;
    for (int j = 0; j < rps->m_numberOfNegativePictures; j++)
    {
      xReadUvlc(code, "delta_poc_s0_minus1");
      poc = prev - code - 1;
      prev = poc;
      rps->m_deltaPOC[j] = poc;
      xReadFlag(code, "used_by_curr_pic_s0_flag");
      rps->m_used[j] = code;
    }
    prev = 0;
    for (int j = rps->m_numberOfNegativePictures; j < rps->m_numberOfNegativePictures + rps->m_numberOfPositivePictures; j++)
    {
      xReadUvlc(code, "delta_poc_s1_minus1");
      poc = prev + code + 1;
      prev = poc;
      rps->m_deltaPOC[j] = poc;
      xReadFlag(code, "used_by_curr_pic_s1_flag");
      rps->m_used[j] = code;
    }
    rps->m_numberOfPictures = rps->m_numberOfNegativePictures + rps->m_numberOfPositivePictures;
  }
}

void parseNalH265::parseScalingList(hevc::TComScalingList *scalingList)
{
  unsigned int code, sizeId, listId;
  bool scalingListPredModeFlag;
  // for each size
  for (sizeId = 0; sizeId < hevc::SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for (listId = 0; listId < hevc::SCALING_LIST_NUM; listId++)
    {
      if ((sizeId == hevc::SCALING_LIST_32x32) && (listId % (hevc::SCALING_LIST_NUM / hevc::NUMBER_OF_PREDICTION_MODES) != 0))
      {
        int *src = scalingList->m_scalingListCoef[sizeId][listId]->data();
        const int size = std::min(hevc::MAX_MATRIX_COEF_NUM, (int)hevc::g_scalingListSize[sizeId]);
        const int *srcNextSmallerSize = scalingList->m_scalingListCoef[sizeId - 1][listId]->data();
        for (int i = 0; i < size; i++)
        {
          src[i] = srcNextSmallerSize[i];
        }
        scalingList->m_scalingListDC[sizeId][listId] = (sizeId > hevc::SCALING_LIST_8x8) ? scalingList->m_scalingListDC[sizeId - 1][listId] : src[0];
      }
      else
      {
        xReadFlag(code, "scaling_list_pred_mode_flag");
        scalingListPredModeFlag = (code) ? true : false;
        scalingList->m_scalingListPredModeFlagIsDPCM[sizeId][listId] = scalingListPredModeFlag;
        if (!scalingListPredModeFlag) // Copy Mode
        {
          xReadUvlc(code, "scaling_list_pred_matrix_id_delta");

          if (sizeId == hevc::SCALING_LIST_32x32)
          {
            code *= (hevc::SCALING_LIST_NUM / hevc::NUMBER_OF_PREDICTION_MODES); // Adjust the decoded code for this size, to cope with the missing 32x32 chroma entries.
          }

          scalingList->m_refMatrixId[sizeId][listId] = (unsigned int)((int)(listId) - (code));
          if (sizeId > hevc::SCALING_LIST_8x8)
          {
            scalingList->m_scalingListDC[sizeId][listId] = ((listId == scalingList->m_refMatrixId[sizeId][listId]) ? 16 : scalingList->m_scalingListDC[sizeId][scalingList->m_refMatrixId[sizeId][listId]]);
          }
          // scalingList->processRefMatrix(sizeId, listId, scalingList->m_refMatrixId[sizeId][listId]); // TODO: Not implemented
        }
        else // DPCM Mode
        {
          xDecodeScalingList(scalingList, sizeId, listId);
        }
      }
    }
  }

  return;
}

void parseNalH265::parseProfileTier(hevc::ProfileTierLevel *ptl, const bool bIsSubLayer)
{
  unsigned int uiCode;
  xReadCode(2, uiCode, ("profile_space"));
  ptl->m_profileSpace = uiCode;
  xReadFlag(uiCode, ("tier_flag"));
  ptl->m_tierFlag = uiCode ? hevc::Level::HIGH : hevc::Level::MAIN;
  xReadCode(5, uiCode, ("profile_idc"));
  ptl->m_profileIdc = hevc::Profile::Name(uiCode);
  for (int j = 0; j < 32; j++)
  {
    xReadFlag(uiCode, ("profile_compatibility_flag[][j]"));
    ptl->m_profileCompatibilityFlag[j] = uiCode ? 1 : 0;
  }
  xReadFlag(uiCode, ("progressive_source_flag"));
  ptl->m_progressiveSourceFlag = uiCode ? true : false;

  xReadFlag(uiCode, ("interlaced_source_flag"));
  ptl->m_interlacedSourceFlag = uiCode ? true : false;

  xReadFlag(uiCode, ("non_packed_constraint_flag"));
  ptl->m_nonPackedConstraintFlag = uiCode ? true : false;

  xReadFlag(uiCode, ("frame_only_constraint_flag"));
  ptl->m_frameOnlyConstraintFlag = uiCode ? true : false;

  if (ptl->m_profileIdc == hevc::Profile::MAINREXT || ptl->m_profileCompatibilityFlag[hevc::Profile::MAINREXT] ||
      ptl->m_profileIdc == hevc::Profile::HIGHTHROUGHPUTREXT || ptl->m_profileCompatibilityFlag[hevc::Profile::HIGHTHROUGHPUTREXT])
  {
    unsigned int maxBitDepth = 16;
    xReadFlag(uiCode, ("max_12bit_constraint_flag"));
    if (uiCode)
      maxBitDepth = 12;
    xReadFlag(uiCode, ("max_10bit_constraint_flag"));
    if (uiCode)
      maxBitDepth = 10;
    xReadFlag(uiCode, ("max_8bit_constraint_flag"));
    if (uiCode)
      maxBitDepth = 8;
    ptl->m_bitDepthConstraintValue = maxBitDepth;
    hevc::ChromaFormat chromaFmtConstraint = hevc::CHROMA_444;
    xReadFlag(uiCode, ("max_422chroma_constraint_flag"));
    if (uiCode)
      chromaFmtConstraint = hevc::CHROMA_422;
    xReadFlag(uiCode, ("max_420chroma_constraint_flag"));
    if (uiCode)
      chromaFmtConstraint = hevc::CHROMA_420;
    xReadFlag(uiCode, ("max_monochrome_constraint_flag"));
    if (uiCode)
      chromaFmtConstraint = hevc::CHROMA_400;
    ptl->m_chromaFormatConstraintValue = chromaFmtConstraint;
    xReadFlag(uiCode, ("intra_constraint_flag"));
    ptl->m_intraConstraintFlag = uiCode != 0;
    xReadFlag(uiCode, ("one_picture_only_constraint_flag"));
    ptl->m_onePictureOnlyConstraintFlag = uiCode != 0;
    xReadFlag(uiCode, ("lower_bit_rate_constraint_flag"));
    ptl->m_lowerBitRateConstraintFlag = uiCode != 0;
    xReadCode(16, uiCode, ("reserved_zero_34bits[0..15]"));
    xReadCode(16, uiCode, ("reserved_zero_34bits[16..31]"));
    xReadCode(2, uiCode, ("reserved_zero_34bits[32..33]"));
  }
  else
  {
    ptl->m_bitDepthConstraintValue = (ptl->m_profileIdc == hevc::Profile::MAIN10) ? 10 : 8;
    ptl->m_chromaFormatConstraintValue = hevc::CHROMA_420;
    ptl->m_intraConstraintFlag = false;
    ptl->m_lowerBitRateConstraintFlag = true;
    if (ptl->m_profileIdc == hevc::Profile::MAIN10 || ptl->m_profileCompatibilityFlag[hevc::Profile::MAIN10])
    {
      xReadCode(7, uiCode, ("reserved_zero_7bits"));
      xReadFlag(uiCode, ("one_picture_only_constraint_flag"));
      ptl->m_onePictureOnlyConstraintFlag = uiCode != 0;
      xReadCode(16, uiCode, ("reserved_zero_35bits[0..15]"));
      xReadCode(16, uiCode, ("reserved_zero_35bits[16..31]"));
      xReadCode(3, uiCode, ("reserved_zero_35bits[32..34]"));
    }
    else
    {
      xReadCode(16, uiCode, ("reserved_zero_43bits[0..15]"));
      xReadCode(16, uiCode, ("reserved_zero_43bits[16..31]"));
      xReadCode(11, uiCode, ("reserved_zero_43bits[32..42]"));
    }
  }

  if ((ptl->m_profileIdc >= hevc::Profile::MAIN && ptl->m_profileIdc <= hevc::Profile::HIGHTHROUGHPUTREXT) ||
      ptl->m_profileCompatibilityFlag[hevc::Profile::MAIN] ||
      ptl->m_profileCompatibilityFlag[hevc::Profile::MAIN10] ||
      ptl->m_profileCompatibilityFlag[hevc::Profile::MAINSTILLPICTURE] ||
      ptl->m_profileCompatibilityFlag[hevc::Profile::MAINREXT] ||
      ptl->m_profileCompatibilityFlag[hevc::Profile::HIGHTHROUGHPUTREXT])
  {
    xReadFlag(uiCode, ("inbld_flag"));
    assert(uiCode == 0);
  }
  else
  {
    xReadFlag(uiCode, ("reserved_zero_bit"));
  }
}

void parseNalH265::parsePTL(hevc::TComPTL *rpcPTL, bool profilePresentFlag, int maxNumSubLayersMinus1)
{
  unsigned int uiCode;
  if (profilePresentFlag)
  {
    parseProfileTier(&(rpcPTL->m_generalPTL), false);
  }
  xReadCode(8, uiCode, "general_level_idc");
  rpcPTL->m_generalPTL.m_levelIdc = hevc::Level::Name(uiCode);

  for (int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    xReadFlag(uiCode, "sub_layer_profile_present_flag[i]");
    rpcPTL->m_subLayerProfilePresentFlag[i] = uiCode;
    xReadFlag(uiCode, "sub_layer_level_present_flag[i]");
    rpcPTL->m_subLayerLevelPresentFlag[i] = uiCode;
  }

  if (maxNumSubLayersMinus1 > 0)
  {
    for (int i = maxNumSubLayersMinus1; i < 8; i++)
    {
      xReadCode(2, uiCode, "reserved_zero_2bits");
      assert(uiCode == 0);
    }
  }

  for (int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    if (rpcPTL->m_subLayerProfilePresentFlag[i])
    {
      parseProfileTier(&(rpcPTL->m_subLayerPTL[i]), true);
    }
    if (rpcPTL->m_subLayerLevelPresentFlag[i])
    {
      xReadCode(8, uiCode, "sub_layer_level_idc[i]");
      rpcPTL->m_subLayerPTL[i].m_levelIdc = hevc::Level::Name(uiCode);
    }
  }
}

const void convertPayloadToRBSP(std::vector<unsigned char> &nalUnitBuf, TComInputBitstream *bitstream, bool isVclNalUnit)
{
  unsigned int zeroCount = 0;
  std::vector<unsigned char>::iterator it_read, it_write;

  unsigned int pos = 0;
  bitstream->clearEmulationPreventionByteLocation();
  for (it_read = it_write = nalUnitBuf.begin(); it_read != nalUnitBuf.end(); it_read++, it_write++, pos++)
  {
    assert(zeroCount < 2 || *it_read >= 0x03);
    if (zeroCount == 2 && *it_read == 0x03)
    {
      bitstream->pushEmulationPreventionByteLocation(pos);
      pos++;
      it_read++;
      zeroCount = 0;
      if (it_read == nalUnitBuf.end())
      {
        break;
      }
      assert(*it_read <= 0x03);
    }
    zeroCount = (*it_read == 0x00) ? zeroCount + 1 : 0;
    *it_write = *it_read;
  }
  assert(zeroCount == 0);

  if (isVclNalUnit)
  {
    // Remove cabac_zero_word from payload if present
    int n = 0;

    while (it_write[-1] == 0x00)
    {
      it_write--;
      n++;
    }

    if (n > 0)
    {
      printf("\nDetected %d instances of cabac_zero_word\n", n / 2);
    }
  }

  nalUnitBuf.resize(it_write - nalUnitBuf.begin());
}

const unsigned int getMinLog2CtbSize(const hevc::TComPTL &ptl, unsigned int layerPlus1)
{
  hevc::ProfileTierLevel pPTL = (layerPlus1 == 0) ? (ptl.m_generalPTL) : (ptl.m_subLayerPTL[layerPlus1 - 1]);
  return (pPTL.m_levelIdc < hevc::Level::LEVEL5) ? 4 : 5;
}

const unsigned int getMaxLog2CtbSize(const hevc::TComPTL & /*ptl*/, unsigned int /*layerPlus1*/)
{
  return hevc::MAX_CU_DEPTH;
}

static inline unsigned int getMaxCUDepthOffset(const hevc::ChromaFormat chFmt, const unsigned int quadtreeTULog2MinSize)
{
  return (chFmt == hevc::CHROMA_422 && quadtreeTULog2MinSize > 2) ? 1 : 0;
}

void parseNalH265::vps_parse(unsigned char *nal_bitstream, hevc::vps *pcVPS, int curLen, parsingLevel level)
{
  for (int i = 0; i < curLen; i++)
    m_bits->m_fifo.push_back(nal_bitstream[i]);
  setBitstream(m_bits);

  // bool isVclNalUnit = (m_pcBitstream->m_fifo[0] & 64) == 0;
  // convertPayloadToRBSP(m_pcBitstream->m_fifo, m_pcBitstream, isVclNalUnit);
  m_pcBitstream->m_fifo_idx = 2;
  m_pcBitstream->m_num_held_bits = 0;
  m_pcBitstream->m_held_bits = 1;
  m_pcBitstream->m_numBitsRead = 16;

  unsigned int uiCode;

  xReadCode(4, uiCode, "vps_video_parameter_set_id");
  pcVPS->m_VPSId = uiCode;
  xReadFlag(uiCode, "vps_base_layer_internal_flag");
  assert(uiCode == 1);
  xReadFlag(uiCode, "vps_base_layer_available_flag");
  assert(uiCode == 1);
  xReadCode(6, uiCode, "vps_max_layers_minus1");
  xReadCode(3, uiCode, "vps_max_sub_layers_minus1");
  pcVPS->m_uiMaxTLayers = uiCode + 1;
  assert(uiCode + 1 <= MAX_TLAYER);
  xReadFlag(uiCode, "vps_temporal_id_nesting_flag");
  pcVPS->m_bTemporalIdNestingFlag = uiCode ? true : false;
  assert(pcVPS->m_uiMaxTLayers > 1 || pcVPS->m_bTemporalIdNestingFlag);
  xReadCode(16, uiCode, "vps_reserved_0xffff_16bits");
  assert(uiCode == 0xffff);
  parsePTL(&pcVPS->m_pcPTL, true, pcVPS->m_uiMaxTLayers - 1);
  unsigned int subLayerOrderingInfoPresentFlag;
  xReadFlag(subLayerOrderingInfoPresentFlag, "vps_sub_layer_ordering_info_present_flag");
  for (unsigned int i = 0; i <= pcVPS->m_uiMaxTLayers - 1; i++)
  {
    xReadUvlc(uiCode, "vps_max_dec_pic_buffering_minus1[i]");
    pcVPS->m_uiMaxDecPicBuffering[i] = uiCode + 1;
    xReadUvlc(uiCode, "vps_max_num_reorder_pics[i]");
    pcVPS->m_numReorderPics[i] = uiCode;
    xReadUvlc(uiCode, "vps_max_latency_increase_plus1[i]");
    pcVPS->m_uiMaxLatencyIncrease[i] = uiCode;

    if (!subLayerOrderingInfoPresentFlag)
    {
      for (i++; i <= pcVPS->m_uiMaxTLayers - 1; i++)
      {
        pcVPS->m_uiMaxDecPicBuffering[i] = pcVPS->m_uiMaxDecPicBuffering[0] + 1;
        pcVPS->m_numReorderPics[i] = pcVPS->m_numReorderPics[0];
        pcVPS->m_uiMaxLatencyIncrease[i] = pcVPS->m_uiMaxLatencyIncrease[0];
      }
      break;
    }
  }

  assert(pcVPS->m_numHrdParameters < hevc::MAX_VPS_OP_SETS_PLUS1);
  assert(pcVPS->m_maxNuhReservedZeroLayerId < hevc::MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1);
  xReadCode(6, uiCode, "vps_max_layer_id");
  pcVPS->m_maxNuhReservedZeroLayerId = uiCode;
  xReadUvlc(uiCode, "vps_num_layer_sets_minus1");
  pcVPS->m_numOpSets = uiCode + 1;
  for (unsigned int opsIdx = 1; opsIdx <= (pcVPS->m_numOpSets - 1); opsIdx++)
  {
    for (unsigned int i = 0; i <= pcVPS->m_maxNuhReservedZeroLayerId; i++)
    {
      xReadFlag(uiCode, "layer_id_included_flag[opsIdx][i]");
      pcVPS->m_layerIdIncludedFlag[opsIdx][i] = uiCode == 1 ? true : false;
    }
  }

  hevc::TimingInfo *timingInfo = &(pcVPS->m_timingInfo);
  xReadFlag(uiCode, "vps_timing_info_present_flag");
  timingInfo->m_timingInfoPresentFlag = uiCode ? true : false;
  if (timingInfo->m_timingInfoPresentFlag)
  {
    xReadCode(32, uiCode, "vps_num_units_in_tick");
    timingInfo->m_numUnitsInTick = uiCode;
    xReadCode(32, uiCode, "vps_time_scale");
    timingInfo->m_timeScale = uiCode;
    xReadFlag(uiCode, "vps_poc_proportional_to_timing_flag");
    timingInfo->m_pocProportionalToTimingFlag = uiCode ? true : false;
    if (timingInfo->m_pocProportionalToTimingFlag)
    {
      xReadUvlc(uiCode, "vps_num_ticks_poc_diff_one_minus1");
      timingInfo->m_numTicksPocDiffOneMinus1 = uiCode;
    }

    xReadUvlc(uiCode, "vps_num_hrd_parameters");
    pcVPS->m_numHrdParameters = uiCode;

    if (pcVPS->m_numHrdParameters > 0)
    {
      // pcVPS->createHrdParamBuffer();
      pcVPS->m_hrdParameters.resize(pcVPS->m_numHrdParameters);
      pcVPS->m_hrdOpSetIdx.resize(pcVPS->m_numHrdParameters);
      pcVPS->m_cprmsPresentFlag.resize(pcVPS->m_numHrdParameters);
    }
    for (unsigned int i = 0; i < pcVPS->m_numHrdParameters; i++)
    {
      xReadUvlc(uiCode, "hrd_layer_set_idx[i]");
      pcVPS->m_hrdOpSetIdx[i] = uiCode;
      if (i > 0)
      {
        xReadFlag(uiCode, "cprms_present_flag[i]");
        pcVPS->m_cprmsPresentFlag[i] = uiCode == 1 ? true : false;
      }
      else
      {
        pcVPS->m_cprmsPresentFlag[i] = true;
      }

      parseHrdParameters(&pcVPS->m_hrdParameters[i], pcVPS->m_cprmsPresentFlag[i], pcVPS->m_uiMaxTLayers - 1);
    }
  }

  xReadFlag(uiCode, "vps_extension_flag");
  if (uiCode)
  {
    while (xMoreRbspData())
    {
      xReadFlag(uiCode, "vps_extension_data_flag");
    }
  }

  xReadRbspTrailingBits();
}

void parseNalH265::sps_parse(unsigned char *nal_bitstream, hevc::sps *pcSPS, int curLen, parsingLevel level)
{
  for (int i = 0; i < curLen; i++)
    m_bits->m_fifo.push_back(nal_bitstream[i]);
  setBitstream(m_bits);

  // bool isVclNalUnit = (m_pcBitstream->m_fifo[0] & 64) == 0;
  // convertPayloadToRBSP(m_pcBitstream->m_fifo, m_pcBitstream, isVclNalUnit);
  m_pcBitstream->m_fifo_idx = 2;
  m_pcBitstream->m_num_held_bits = 0;
  m_pcBitstream->m_held_bits = 1;
  m_pcBitstream->m_numBitsRead = 16;

  unsigned int uiCode;
  xReadCode(4, uiCode, "sps_video_parameter_set_id");
  pcSPS->m_VPSId = uiCode;
  xReadCode(3, uiCode, "sps_max_sub_layers_minus1");
  pcSPS->m_uiMaxTLayers = uiCode + 1;
  assert(uiCode <= 6);

  xReadFlag(uiCode, "sps_temporal_id_nesting_flag");
  pcSPS->m_SPSTemporalMVPEnabledFlag = uiCode > 0 ? true : false;

  parsePTL(&pcSPS->m_pcPTL, 1, pcSPS->m_uiMaxTLayers - 1);
  xReadUvlc(uiCode, "sps_seq_parameter_set_id");
  pcSPS->m_SPSId = uiCode;
  assert(uiCode <= 15);

  if (level == parsingLevel::PARSING_PARAM_ID)
  {
    return;
  }

  xReadUvlc(uiCode, "chroma_format_idc");
  pcSPS->m_chromaFormatIdc = hevc::ChromaFormat(uiCode);
  assert(uiCode <= 3);

  if (pcSPS->m_chromaFormatIdc == hevc::CHROMA_444)
  {
    xReadFlag(uiCode, "separate_colour_plane_flag");
    assert(uiCode == 0);
  }

  // pic_width_in_luma_samples and pic_height_in_luma_samples needs conformance checking - multiples of MinCbSizeY
  xReadUvlc(uiCode, "pic_width_in_luma_samples");
  pcSPS->m_picWidthInLumaSamples = uiCode;
  xReadUvlc(uiCode, "pic_height_in_luma_samples");
  pcSPS->m_picHeightInLumaSamples = uiCode;
  xReadFlag(uiCode, "conformance_window_flag");
  if (uiCode != 0)
  {
    hevc::Window &conf = pcSPS->m_conformanceWindow;
    const unsigned int subWidthC = pcSPS->m_winUnitX[pcSPS->m_chromaFormatIdc];
    const unsigned int subHeightC = pcSPS->m_winUnitY[pcSPS->m_chromaFormatIdc];

    xReadUvlc(uiCode, "conf_win_left_offset");
    conf.m_winLeftOffset = uiCode * subWidthC;
    xReadUvlc(uiCode, "conf_win_right_offset");
    conf.m_winRightOffset = uiCode * subWidthC;
    xReadUvlc(uiCode, "conf_win_top_offset");
    conf.m_winTopOffset = uiCode * subHeightC;
    xReadUvlc(uiCode, "conf_win_bottom_offset");
    conf.m_winBottomOffset = uiCode * subHeightC;
    // TDecConformanceCheck::checkRange<unsigned int>(conf.getWindowLeftOffset() + conf.getWindowRightOffset(), "conformance window width in pixels", 0, pcSPS->getPicWidthInLumaSamples() - 1); // TODO: Not implemented
  }

  xReadUvlc(uiCode, "bit_depth_luma_minus8");
  assert(uiCode <= 8);

  pcSPS->m_bitDepths.recon[hevc::CHANNEL_TYPE_LUMA] = 8 + uiCode;
  pcSPS->m_qpBDOffset[hevc::CHANNEL_TYPE_LUMA] = (int)(6 * uiCode);

  xReadUvlc(uiCode, "bit_depth_chroma_minus8");
  assert(uiCode <= 8);
  pcSPS->m_bitDepths.recon[hevc::CHANNEL_TYPE_CHROMA] = 8 + uiCode;
  pcSPS->m_qpBDOffset[hevc::CHANNEL_TYPE_CHROMA] = (int)(6 * uiCode);

  xReadUvlc(uiCode, "log2_max_pic_order_cnt_lsb_minus4");
  pcSPS->m_uiBitsForPOC = 4 + uiCode;

  unsigned int subLayerOrderingInfoPresentFlag;
  xReadFlag(subLayerOrderingInfoPresentFlag, "sps_sub_layer_ordering_info_present_flag");

  for (unsigned int i = 0; i <= pcSPS->m_uiMaxTLayers - 1; i++)
  {
    xReadUvlc(uiCode, "sps_max_dec_pic_buffering_minus1[i]");
    pcSPS->m_uiMaxDecPicBuffering[i] = uiCode + 1;
    xReadUvlc(uiCode, "sps_max_num_reorder_pics[i]");
    pcSPS->m_numReorderPics[i] = uiCode;
    xReadUvlc(uiCode, "sps_max_latency_increase_plus1[i]");
    pcSPS->m_uiMaxLatencyIncreasePlus1[i] = uiCode;

    if (!subLayerOrderingInfoPresentFlag)
    {
      for (i++; i <= pcSPS->m_uiMaxTLayers - 1; i++)
      {
        pcSPS->m_uiMaxDecPicBuffering[i] = pcSPS->m_uiMaxDecPicBuffering[0];
        pcSPS->m_numReorderPics[i] = pcSPS->m_numReorderPics[0];
        pcSPS->m_uiMaxLatencyIncreasePlus1[i] = pcSPS->m_uiMaxLatencyIncreasePlus1[0];
      }
      break;
    }
  }

  const unsigned int maxLog2CtbSize = getMaxLog2CtbSize(pcSPS->m_pcPTL, 0);
  const unsigned int minLog2CtbSize = getMinLog2CtbSize(pcSPS->m_pcPTL, 0);
  xReadUvlc(uiCode, "log2_min_luma_coding_block_size_minus3");
  assert(uiCode <= maxLog2CtbSize - 3);
  int minCbLog2SizeY = uiCode + 3;
  pcSPS->m_log2MinCodingBlockSize = minCbLog2SizeY;

  // Difference + log2MinCUSize must be <= maxLog2CtbSize
  // Difference + log2MinCUSize must be >= minLog2CtbSize
  __attribute__((unused)) const unsigned int minLog2DiffMaxMinLumaCodingBlockSize = minLog2CtbSize < (unsigned int)minCbLog2SizeY ? 0 : minLog2CtbSize - minCbLog2SizeY;
  __attribute__((unused)) const unsigned int maxLog2DiffMaxMinLumaCodingBlockSize = maxLog2CtbSize - minCbLog2SizeY;

  xReadUvlc(uiCode, "log2_diff_max_min_luma_coding_block_size");
  assert(uiCode >= minLog2DiffMaxMinLumaCodingBlockSize && uiCode <= maxLog2DiffMaxMinLumaCodingBlockSize);
  pcSPS->m_log2DiffMaxMinCodingBlockSize = uiCode;

  const int maxCUDepthDelta = uiCode;
  const int ctbLog2SizeY = minCbLog2SizeY + maxCUDepthDelta;
  pcSPS->m_uiMaxCUWidth = 1 << ctbLog2SizeY;
  pcSPS->m_uiMaxCUHeight = 1 << ctbLog2SizeY;
  xReadUvlc(uiCode, "log2_min_luma_transform_block_size_minus2");
  const unsigned int minTbLog2SizeY = uiCode + 2;
  pcSPS->m_uiQuadtreeTULog2MinSize = minTbLog2SizeY;

  //  log2_diff <= Min(CtbLog2SizeY, 5) - minTbLog2SizeY
  xReadUvlc(uiCode, "log2_diff_max_min_luma_transform_block_size");
  pcSPS->m_uiQuadtreeTULog2MaxSize = uiCode + pcSPS->m_uiQuadtreeTULog2MinSize;
  pcSPS->m_uiMaxTrSize = 1 << (uiCode + pcSPS->m_uiQuadtreeTULog2MinSize);

  xReadUvlc(uiCode, "max_transform_hierarchy_depth_inter");
  pcSPS->m_uiQuadtreeTUMaxDepthInter = uiCode;
  xReadUvlc(uiCode, "max_transform_hierarchy_depth_intra");
  pcSPS->m_uiQuadtreeTUMaxDepthIntra = uiCode;

  int addCuDepth = std::max(0, minCbLog2SizeY - (int)pcSPS->m_uiQuadtreeTULog2MinSize);
  pcSPS->m_uiMaxTotalCUDepth = maxCUDepthDelta + addCuDepth + getMaxCUDepthOffset(pcSPS->m_chromaFormatIdc, pcSPS->m_uiQuadtreeTULog2MinSize);
  xReadFlag(uiCode, "scaling_list_enabled_flag");
  pcSPS->m_scalingListEnabledFlag = uiCode;
  if (pcSPS->m_scalingListEnabledFlag)
  {
    xReadFlag(uiCode, "sps_scaling_list_data_present_flag");
    pcSPS->m_scalingListPresentFlag = uiCode;
    if (pcSPS->m_scalingListPresentFlag)
    {
      parseScalingList(&(pcSPS->m_scalingList));
    }
  }
  xReadFlag(uiCode, "amp_enabled_flag");
  pcSPS->m_useAMP = uiCode;
  xReadFlag(uiCode, "sample_adaptive_offset_enabled_flag");
  pcSPS->m_bUseSAO = uiCode ? true : false;

  xReadFlag(uiCode, "pcm_enabled_flag");
  pcSPS->m_usePCM = uiCode ? true : false;
  if (pcSPS->m_usePCM)
  {
    xReadCode(4, uiCode, "pcm_sample_bit_depth_luma_minus1");
    pcSPS->m_pcmBitDepths[hevc::CHANNEL_TYPE_LUMA] = 1 + uiCode;
    xReadCode(4, uiCode, "pcm_sample_bit_depth_chroma_minus1");
    pcSPS->m_pcmBitDepths[hevc::CHANNEL_TYPE_CHROMA] = 1 + uiCode;
    xReadUvlc(uiCode, "log2_min_pcm_luma_coding_block_size_minus3");
    const unsigned int log2MinIpcmCbSizeY = uiCode + 3;
    pcSPS->m_uiPCMLog2MinSize = log2MinIpcmCbSizeY;
    xReadUvlc(uiCode, "log2_diff_max_min_pcm_luma_coding_block_size");
    pcSPS->m_pcmLog2MaxSize = uiCode + pcSPS->m_uiPCMLog2MinSize;
    xReadFlag(uiCode, "pcm_loop_filter_disable_flag");
    pcSPS->m_bPCMFilterDisableFlag = uiCode ? true : false;
  }

  xReadUvlc(uiCode, "num_short_term_ref_pic_sets");
  assert(uiCode <= 64);
  pcSPS->m_RPSList.m_referencePictureSets.resize(uiCode);

  hevc::TComRPSList *rpsList = &(pcSPS->m_RPSList);
  hevc::TComReferencePictureSet *rps;

  for (int i = 0; i < (int)(rpsList->m_referencePictureSets.size()); i++)
  {
    rps = &(rpsList->m_referencePictureSets[i]);
    parseShortTermRefPicSet(pcSPS, rps, i);
  }
  xReadFlag(uiCode, "long_term_ref_pics_present_flag");
  pcSPS->m_bLongTermRefsPresent = uiCode;
  if (pcSPS->m_bLongTermRefsPresent)
  {
    xReadUvlc(uiCode, "num_long_term_ref_pics_sps");
    pcSPS->m_numLongTermRefPicSPS = uiCode;
    for (unsigned int k = 0; k < pcSPS->m_numLongTermRefPicSPS; k++)
    {
      xReadCode(pcSPS->m_uiBitsForPOC, uiCode, "lt_ref_pic_poc_lsb_sps");
      pcSPS->m_ltRefPicPocLsbSps[k] = uiCode;
      xReadFlag(uiCode, "used_by_curr_pic_lt_sps_flag[i]");
      pcSPS->m_usedByCurrPicLtSPSFlag[k] = uiCode ? 1 : 0;
    }
  }
  xReadFlag(uiCode, "sps_temporal_mvp_enabled_flag");
  pcSPS->m_SPSTemporalMVPEnabledFlag = uiCode;

  xReadFlag(uiCode, "strong_intra_smoothing_enable_flag");
  pcSPS->m_useStrongIntraSmoothing = uiCode;

  xReadFlag(uiCode, "vui_parameters_present_flag");
  pcSPS->m_vuiParametersPresentFlag = uiCode;

  if (pcSPS->m_vuiParametersPresentFlag)
  {
    parseVUI(&(pcSPS->m_vuiParameters), pcSPS);
  }

  xReadFlag(uiCode, "sps_extension_present_flag");
  if (uiCode)
  {
    static const char *syntaxStrings[] = {"sps_range_extension_flag",
                                          "sps_multilayer_extension_flag",
                                          "sps_extension_6bits[0]",
                                          "sps_extension_6bits[1]",
                                          "sps_extension_6bits[2]",
                                          "sps_extension_6bits[3]",
                                          "sps_extension_6bits[4]",
                                          "sps_extension_6bits[5]"};
    bool sps_extension_flags[hevc::NUM_SPS_EXTENSION_FLAGS];

    for (int i = 0; i < hevc::NUM_SPS_EXTENSION_FLAGS; i++)
    {
      xReadFlag(uiCode, syntaxStrings[i]);
      sps_extension_flags[i] = uiCode != 0;
    }

    bool bSkipTrailingExtensionBits = false;
    for (int i = 0; i < hevc::NUM_SPS_EXTENSION_FLAGS; i++) // loop used so that the order is determined by the enum.
    {
      if (sps_extension_flags[i])
      {
        switch (hevc::SPSExtensionFlagIndex(i))
        {
        case hevc::SPS_EXT__REXT:
          assert(!bSkipTrailingExtensionBits);
          {
            hevc::TComSPSRExt &spsRangeExtension = pcSPS->m_spsRangeExtension;
            xReadFlag(uiCode, "transform_skip_rotation_enabled_flag");
            spsRangeExtension.m_transformSkipRotationEnabledFlag = uiCode != 0;
            xReadFlag(uiCode, "transform_skip_context_enabled_flag");
            spsRangeExtension.m_transformSkipContextEnabledFlag = uiCode != 0;
            xReadFlag(uiCode, "implicit_rdpcm_enabled_flag");
            spsRangeExtension.m_rdpcmEnabledFlag[hevc::RDPCM_SIGNAL_IMPLICIT] = uiCode != 0;
            xReadFlag(uiCode, "explicit_rdpcm_enabled_flag");
            spsRangeExtension.m_rdpcmEnabledFlag[hevc::RDPCM_SIGNAL_EXPLICIT] = uiCode != 0;
            xReadFlag(uiCode, "extended_precision_processing_flag");
            spsRangeExtension.m_extendedPrecisionProcessingFlag = uiCode != 0;
            xReadFlag(uiCode, "intra_smoothing_disabled_flag");
            spsRangeExtension.m_intraSmoothingDisabledFlag = uiCode != 0;
            xReadFlag(uiCode, "high_precision_offsets_enabled_flag");
            spsRangeExtension.m_highPrecisionOffsetsEnabledFlag = uiCode != 0;
            xReadFlag(uiCode, "persistent_rice_adaptation_enabled_flag");
            spsRangeExtension.m_persistentRiceAdaptationEnabledFlag = uiCode != 0;
            xReadFlag(uiCode, "cabac_bypass_alignment_enabled_flag");
            spsRangeExtension.m_cabacBypassAlignmentEnabledFlag = uiCode != 0;
          }
          break;
        default:
          bSkipTrailingExtensionBits = true;
          break;
        }
      }
    }
    if (bSkipTrailingExtensionBits)
    {
      while (xMoreRbspData())
      {
        xReadFlag(uiCode, "sps_extension_data_flag");
      }
    }
  }

  xReadRbspTrailingBits();
}

void parseNalH265::pps_parse(unsigned char *nal_bitstream, hevc::pps *pcPPS, hevc::sps *pcSPS, int curLen, parsingLevel level)
{
  unsigned int uiCode;
  int iCode;

  for (int i = 0; i < curLen; i++)
    m_bits->m_fifo.push_back(nal_bitstream[i]);
  setBitstream(m_bits);

  // bool isVclNalUnit = (m_pcBitstream->m_fifo[0] & 64) == 0;
  // convertPayloadToRBSP(m_pcBitstream->m_fifo, m_pcBitstream, isVclNalUnit);

  m_pcBitstream->m_fifo_idx = 2;
  m_pcBitstream->m_num_held_bits = 0;
  m_pcBitstream->m_held_bits = 1;
  m_pcBitstream->m_numBitsRead = 16;

  xReadUvlc(uiCode, "pps_pic_parameter_set_id");
  assert(uiCode <= 63);
  pcPPS->m_PPSId = uiCode;

  xReadUvlc(uiCode, "pps_seq_parameter_set_id");
  assert(uiCode <= 15);
  pcPPS->m_SPSId = uiCode;

  if (level == parsingLevel::PARSING_PARAM_ID)
  {
    return;
  }

  xReadFlag(uiCode, "dependent_slice_segments_enabled_flag");
  pcPPS->m_dependentSliceSegmentsEnabledFlag = uiCode == 1;

  xReadFlag(uiCode, "output_flag_present_flag");
  pcPPS->m_OutputFlagPresentFlag = uiCode == 1;

  xReadCode(3, uiCode, "num_extra_slice_header_bits");
  pcPPS->m_numExtraSliceHeaderBits = uiCode;

  xReadFlag(uiCode, "sign_data_hiding_enabled_flag");
  pcPPS->m_signDataHidingEnabledFlag = uiCode;

  xReadFlag(uiCode, "cabac_init_present_flag");
  pcPPS->m_cabacInitPresentFlag = uiCode ? true : false;

  xReadUvlc(uiCode, "num_ref_idx_l0_default_active_minus1");
  assert(uiCode <= 14);
  pcPPS->m_numRefIdxL0DefaultActive = uiCode + 1;

  xReadUvlc(uiCode, "num_ref_idx_l1_default_active_minus1");
  assert(uiCode <= 14);
  pcPPS->m_numRefIdxL1DefaultActive = uiCode + 1;

  xReadSvlc(iCode, "init_qp_minus26");
  pcPPS->m_picInitQPMinus26 = iCode;
  xReadFlag(uiCode, "constrained_intra_pred_flag");
  pcPPS->m_bConstrainedIntraPred = uiCode ? true : false;
  xReadFlag(uiCode, "transform_skip_enabled_flag");
  pcPPS->m_useTransformSkip = uiCode ? true : false;

  xReadFlag(uiCode, "cu_qp_delta_enabled_flag");
  pcPPS->m_useDQP = uiCode ? true : false;
  if (pcPPS->m_useDQP)
  {
    xReadUvlc(uiCode, "diff_cu_qp_delta_depth");
    pcPPS->m_uiMaxCuDQPDepth = uiCode;
  }
  else
  {
    pcPPS->m_uiMaxCuDQPDepth = 0;
  }
  xReadSvlc(iCode, "pps_cb_qp_offset");
  pcPPS->m_chromaCbQpOffset = iCode;
  assert(pcPPS->m_chromaCbQpOffset >= -12);
  assert(pcPPS->m_chromaCbQpOffset <= 12);

  xReadSvlc(iCode, "pps_cr_qp_offset");
  pcPPS->m_chromaCrQpOffset = iCode;
  assert(pcPPS->m_chromaCrQpOffset >= -12);
  assert(pcPPS->m_chromaCrQpOffset <= 12);

  xReadFlag(uiCode, "pps_slice_chroma_qp_offsets_present_flag");
  pcPPS->m_bSliceChromaQpFlag = uiCode ? true : false;

  xReadFlag(uiCode, "weighted_pred_flag"); // Use of Weighting Prediction (P_SLICE)
  pcPPS->m_bUseWeightPred = uiCode == 1;
  xReadFlag(uiCode, "weighted_bipred_flag"); // Use of Bi-Directional Weighting Prediction (B_SLICE)
  pcPPS->m_useWeightedBiPred = uiCode == 1;

  xReadFlag(uiCode, "transquant_bypass_enabled_flag");
  pcPPS->m_TransquantBypassEnabledFlag = uiCode ? true : false;
  xReadFlag(uiCode, "tiles_enabled_flag");
  pcPPS->m_tilesEnabledFlag = uiCode == 1;
  xReadFlag(uiCode, "entropy_coding_sync_enabled_flag");
  pcPPS->m_entropyCodingSyncEnabledFlag = uiCode == 1;

  if (pcPPS->m_tilesEnabledFlag)
  {
    xReadUvlc(uiCode, "num_tile_columns_minus1");
    pcPPS->m_numTileColumnsMinus1 = uiCode;
    xReadUvlc(uiCode, "num_tile_rows_minus1");
    pcPPS->m_numTileRowsMinus1 = uiCode;
    xReadFlag(uiCode, "uniform_spacing_flag");
    pcPPS->m_uniformSpacingFlag = uiCode == 1;

    const unsigned int tileColumnsMinus1 = pcPPS->m_numTileColumnsMinus1;
    const unsigned int tileRowsMinus1 = pcPPS->m_numTileRowsMinus1;

    if (!pcPPS->m_uniformSpacingFlag)
    {
      if (tileColumnsMinus1 > 0)
      {
        std::vector<int> columnWidth(tileColumnsMinus1);
        for (unsigned int i = 0; i < tileColumnsMinus1; i++)
        {
          xReadUvlc(uiCode, "column_width_minus1");
          columnWidth[i] = uiCode + 1;
        }
        pcPPS->m_tileColumnWidth = columnWidth;
      }

      if (tileRowsMinus1 > 0)
      {
        std::vector<int> rowHeight(tileRowsMinus1);
        for (unsigned int i = 0; i < tileRowsMinus1; i++)
        {
          xReadUvlc(uiCode, "row_height_minus1");
          rowHeight[i] = uiCode + 1;
        }
        pcPPS->m_tileRowHeight = rowHeight;
      }
    }
    assert((tileColumnsMinus1 + tileRowsMinus1) != 0);
    xReadFlag(uiCode, "loop_filter_across_tiles_enabled_flag");
    pcPPS->m_loopFilterAcrossTilesEnabledFlag = uiCode ? true : false;
  }
  xReadFlag(uiCode, "pps_loop_filter_across_slices_enabled_flag");
  pcPPS->m_loopFilterAcrossSlicesEnabledFlag = uiCode ? true : false;
  xReadFlag(uiCode, "deblocking_filter_control_present_flag");
  pcPPS->m_deblockingFilterControlPresentFlag = uiCode ? true : false;
  if (pcPPS->m_deblockingFilterControlPresentFlag)
  {
    xReadFlag(uiCode, "deblocking_filter_override_enabled_flag");
    pcPPS->m_deblockingFilterOverrideEnabledFlag = uiCode ? true : false;
    xReadFlag(uiCode, "pps_deblocking_filter_disabled_flag");
    pcPPS->m_ppsDeblockingFilterDisabledFlag = uiCode ? true : false;
    if (!pcPPS->m_ppsDeblockingFilterDisabledFlag)
    {
      xReadSvlc(iCode, "pps_beta_offset_div2");
      pcPPS->m_deblockingFilterBetaOffsetDiv2 = iCode;
      xReadSvlc(iCode, "pps_tc_offset_div2");
      pcPPS->m_deblockingFilterTcOffsetDiv2 = iCode;
    }
  }
  xReadFlag(uiCode, "pps_scaling_list_data_present_flag");
  pcPPS->m_scalingListPresentFlag = uiCode ? true : false;
  if (pcPPS->m_scalingListPresentFlag)
  {
    parseScalingList(&pcPPS->m_scalingList);
  }

  xReadFlag(uiCode, "lists_modification_present_flag");
  pcPPS->m_listsModificationPresentFlag = uiCode;

  xReadUvlc(uiCode, "log2_parallel_merge_level_minus2");
  pcPPS->m_log2ParallelMergeLevelMinus2 = uiCode;

  xReadFlag(uiCode, "slice_segment_header_extension_present_flag");
  pcPPS->m_sliceHeaderExtensionPresentFlag = uiCode;

  xReadFlag(uiCode, "pps_extension_present_flag");
  if (uiCode)
  {
    bool pps_extension_flags[hevc::NUM_PPS_EXTENSION_FLAGS];
    for (int i = 0; i < hevc::NUM_PPS_EXTENSION_FLAGS; i++)
    {
      xReadFlag(uiCode, "");
      pps_extension_flags[i] = uiCode != 0;
    }

    bool bSkipTrailingExtensionBits = false;
    for (int i = 0; i < hevc::NUM_PPS_EXTENSION_FLAGS; i++) // loop used so that the order is determined by the enum.
    {
      if (pps_extension_flags[i])
      {
        switch (hevc::PPSExtensionFlagIndex(i))
        {
        case hevc::PPS_EXT__REXT:
        {
          hevc::TComPPSRExt &ppsRangeExtension = pcPPS->m_ppsRangeExtension;
          assert(!bSkipTrailingExtensionBits);

          if (pcPPS->m_useTransformSkip)
          {
            xReadUvlc(uiCode, "log2_max_transform_skip_block_size_minus2");
            ppsRangeExtension.m_log2MaxTransformSkipBlockSize = uiCode + 2;
          }

          xReadFlag(uiCode, "cross_component_prediction_enabled_flag");
          ppsRangeExtension.m_crossComponentPredictionEnabledFlag = uiCode != 0;

          xReadFlag(uiCode, "chroma_qp_offset_list_enabled_flag");
          if (uiCode == 0)
          {
            ppsRangeExtension.m_chromaQpOffsetListLen = 0;
            ppsRangeExtension.m_diffCuChromaQpOffsetDepth = 0;
          }
          else
          {
            xReadUvlc(uiCode, "diff_cu_chroma_qp_offset_depth");
            ppsRangeExtension.m_diffCuChromaQpOffsetDepth = uiCode;
            unsigned int tableSizeMinus1 = 0;
            xReadUvlc(tableSizeMinus1, "chroma_qp_offset_list_len_minus1");
            assert(tableSizeMinus1 < hevc::MAX_QP_OFFSET_LIST_SIZE);

            for (unsigned int cuChromaQpOffsetIdx = 0; cuChromaQpOffsetIdx <= (tableSizeMinus1); cuChromaQpOffsetIdx++)
            {
              int cbOffset;
              int crOffset;
              xReadSvlc(cbOffset, "cb_qp_offset_list[i]");
              assert(cbOffset >= -12 && cbOffset <= 12);
              xReadSvlc(crOffset, "cr_qp_offset_list[i]");
              assert(crOffset >= -12 && crOffset <= 12);
              // table uses +1 for index (see comment inside the function)
              assert(cuChromaQpOffsetIdx != 0 && cuChromaQpOffsetIdx <= hevc::MAX_QP_OFFSET_LIST_SIZE);
              ppsRangeExtension.m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdx].u.comp.CbOffset = cbOffset;
              ppsRangeExtension.m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdx].u.comp.CrOffset = crOffset;
              ppsRangeExtension.m_chromaQpOffsetListLen = std::max(ppsRangeExtension.m_chromaQpOffsetListLen, (int)cuChromaQpOffsetIdx);
            }
            assert(ppsRangeExtension.m_chromaQpOffsetListLen == (int)tableSizeMinus1 + 1);
          }

          xReadUvlc(uiCode, "log2_sao_offset_scale_luma");
          ppsRangeExtension.m_log2SaoOffsetScale[hevc::CHANNEL_TYPE_LUMA] = uiCode;
          xReadUvlc(uiCode, "log2_sao_offset_scale_chroma");
          ppsRangeExtension.m_log2SaoOffsetScale[hevc::CHANNEL_TYPE_CHROMA] = uiCode;
        }
        break;
        default:
          bSkipTrailingExtensionBits = true;
          break;
        }
      }
    }
    if (bSkipTrailingExtensionBits)
    {
      while (xMoreRbspData())
      {
        xReadFlag(uiCode, "pps_extension_data_flag");
      }
    }
  }
  xReadRbspTrailingBits();
}

void parseNalH265::sei_parse(unsigned char *nal_bitstream, nal_info &nal, int curLen)
{
  for (int i = 0; i < curLen - 2; i++)
    m_bits->m_fifo.push_back(nal_bitstream[i + 2]);
  setBitstream(m_bits);

  int payloadType = 0;
  unsigned int val = 0;
  do
  {
    xReadCode(8, val, "payload_type");
    payloadType += val;
  } while (val == 0xFF);

  unsigned int payloadSize = 0;
  do
  {
    xReadCode(8, val, "payload_size");
    payloadSize += val;
  } while (val == 0xFF);

  parseSeiH265 *sei_handler = new parseSeiH265;
  hevc::sps *sps = reinterpret_cast<hevc::sps *>(nal.mpegParamSet->sps);
  nal.sei_type = payloadType;
  nal.sei_length = payloadSize;

  sei_handler->xReadSEIPayloadData(payloadType, payloadSize, nal, (hevc::hevc_nal_type)nal.nal_unit_type, sps, m_bits);

  delete sei_handler;
}