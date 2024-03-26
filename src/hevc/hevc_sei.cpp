#include "nal_parse.h"
#include "hevc_nal.h"

void parseSeiH265::xParseSEIBufferingPeriod(SEIBufferingPeriod &sei, unsigned int payloadSize, const hevc::sps *sps)
{
  int nalOrVcl;
  unsigned int i, code;

  const hevc::TComVUI *vui = &(sps->m_vuiParameters);
  const hevc::TComHRD *hrd = &(vui->m_hrdParameters);

  xReadUvlc(code, "bp_seq_parameter_set_id");
  sei.bpSeqParameterSetId = code;
  if (!hrd->m_subPicCpbParamsPresentFlag)
  {
    xReadFlag(code, "irap_cpb_params_present_flag");
    sei.rapCpbParamsPresentFlag = code;
  }
  if (sei.rapCpbParamsPresentFlag)
  {
    xReadCode(hrd->m_cpbRemovalDelayLengthMinus1 + 1, code, "cpb_delay_offset");
    sei.cpbDelayOffset = code;
    xReadCode(hrd->m_dpbOutputDelayLengthMinus1 + 1, code, "dpb_delay_offset");
    sei.dpbDelayOffset = code;
  }

  // read splicing flag and cpb_removal_delay_delta
  xReadFlag(code, "concatenation_flag");
  sei.concatenationFlag = code;
  xReadCode((hrd->m_dpbOutputDelayLengthMinus1 + 1), code, "au_cpb_removal_delay_delta_minus1");
  sei.auCpbRemovalDelayDelta = code + 1;

  for (nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
  {
    if (((nalOrVcl == 0) && (hrd->m_nalHrdParametersPresentFlag)) ||
        ((nalOrVcl == 1) && (hrd->m_vclHrdParametersPresentFlag)))
    {
      for (i = 0; i < (hrd->m_HRD[0].cpbCntMinus1 + 1); i++)
      {
        xReadCode((hrd->m_initialCpbRemovalDelayLengthMinus1 + 1), code, nalOrVcl ? "vcl_initial_cpb_removal_delay" : "nal_initial_cpb_removal_delay");
        sei.initialCpbRemovalDelay[i][nalOrVcl] = code;
        xReadCode((hrd->m_initialCpbRemovalDelayLengthMinus1 + 1), code, nalOrVcl ? "vcl_initial_cpb_removal_offset" : "nal_initial_cpb_removal_offset");
        sei.initialCpbRemovalDelayOffset[i][nalOrVcl] = code;
        if (hrd->m_subPicCpbParamsPresentFlag || sei.rapCpbParamsPresentFlag)
        {
          xReadCode((hrd->m_initialCpbRemovalDelayLengthMinus1 + 1), code, nalOrVcl ? "vcl_initial_alt_cpb_removal_delay" : "nal_initial_alt_cpb_removal_delay");
          sei.initialAltCpbRemovalDelay[i][nalOrVcl] = code;
          xReadCode((hrd->m_initialCpbRemovalDelayLengthMinus1 + 1), code, nalOrVcl ? "vcl_initial_alt_cpb_removal_offset" : "nal_initial_alt_cpb_removal_offset");
          sei.initialAltCpbRemovalDelayOffset[i][nalOrVcl] = code;
        }
      }
    }
  }
}

void parseSeiH265::xParseSEIPictureTiming(SEIPictureTimingH265 &sei, unsigned int payloadSize, const hevc::sps *sps)
{
  unsigned int i;
  unsigned int code;

  const hevc::TComVUI *vui = &(sps->m_vuiParameters);
  const hevc::TComHRD *hrd = &(vui->m_hrdParameters);

  if (vui->m_frameFieldInfoPresentFlag)
  {
    xReadCode(4, code, "pic_struct");
    sei.picStruct = code;
    xReadCode(2, code, "source_scan_type");
    sei.sourceScanType = code;
    xReadFlag(code, "duplicate_flag");
    sei.duplicateFlag = (code == 1);
  }

  if (hrd->m_nalHrdParametersPresentFlag || hrd->m_vclHrdParametersPresentFlag)
  {
    xReadCode((hrd->m_cpbRemovalDelayLengthMinus1 + 1), code, "au_cpb_removal_delay_minus1");
    sei.auCpbRemovalDelay = code + 1;
    xReadCode((hrd->m_dpbOutputDelayLengthMinus1 + 1), code, "pic_dpb_output_delay");
    sei.picDpbOutputDelay = code;

    if (hrd->m_subPicCpbParamsPresentFlag)
    {
      xReadCode(hrd->m_dpbOutputDelayDuLengthMinus1 + 1, code, "pic_dpb_output_du_delay");
      sei.picDpbOutputDuDelay = code;
    }

    if (hrd->m_subPicCpbParamsPresentFlag && hrd->m_subPicCpbParamsInPicTimingSEIFlag)
    {
      xReadUvlc(code, "num_decoding_units_minus1");
      sei.numDecodingUnitsMinus1 = code;
      xReadFlag(code, "du_common_cpb_removal_delay_flag");
      sei.duCommonCpbRemovalDelayFlag = code;
      if (sei.duCommonCpbRemovalDelayFlag)
      {
        xReadCode((hrd->m_duCpbRemovalDelayLengthMinus1 + 1), code, "du_common_cpb_removal_delay_increment_minus1");
        sei.duCommonCpbRemovalDelayMinus1 = code;
      }
      sei.numNalusInDuMinus1.resize(sei.numDecodingUnitsMinus1 + 1);
      sei.duCpbRemovalDelayMinus1.resize(sei.numDecodingUnitsMinus1 + 1);

      for (i = 0; i <= sei.numDecodingUnitsMinus1; i++)
      {
        xReadUvlc(code, "num_nalus_in_du_minus1[i]");
        sei.numNalusInDuMinus1[i] = code;
        if ((!sei.duCommonCpbRemovalDelayFlag) && (i < sei.numDecodingUnitsMinus1))
        {
          xReadCode((hrd->m_duCpbRemovalDelayLengthMinus1 + 1), code, "du_cpb_removal_delay_minus1[i]");
          sei.duCpbRemovalDelayMinus1[i] = code;
        }
      }
    }
  }
}

void parseSeiH265::xParseSEIUserDataRegistered(SEIUserDataRegistered &sei, unsigned int payloadSize)
{
  unsigned int code;
  assert(payloadSize > 0);
  xReadCode(8, code, "itu_t_t35_country_code");
  payloadSize--;
  if (code == 255)
  {
    assert(payloadSize > 0);
    xReadCode(8, code, "itu_t_t35_country_code_extension_byte");
    payloadSize--;
    code += 255;
  }
  sei.ituCountryCode = code;
  sei.userData.resize(payloadSize);
  for (unsigned int i = 0; i < sei.userData.size(); i++)
  {
    xReadCode(8, code, "itu_t_t35_payload_byte");
    sei.userData[i] = code;
  }
}

void parseSeiH265::xParseSEIUserDataUnregistered(SEIUserDataUnregistered &sei, unsigned int payloadSize)
{
  assert(payloadSize >= ISO_IEC_11578_LEN);
  unsigned int val;

  for (unsigned int i = 0; i < ISO_IEC_11578_LEN; i++)
  {
    xReadCode(8, val, "uuid_iso_iec_11578");
    sei.uuid_iso_iec_11578[i] = val;
  }

  sei.userData.resize(payloadSize - ISO_IEC_11578_LEN);
  for (unsigned int i = 0; i < sei.userData.size(); i++)
  {
    xReadCode(8, val, "user_data_payload_byte");
    sei.userData[i] = val;
  }
}

void parseSeiH265::xParseSEITimeCode(SEITimeCode &sei, unsigned int payloadSize)
{
  unsigned int code;
  xReadCode(2, code, "num_clock_ts");
  sei.numClockTs = code;
  for (unsigned int i = 0; i < sei.numClockTs; i++)
  {
    SEITimeSet currentTimeSet;
    xReadFlag(code, "clock_time_stamp_flag[i]");
    currentTimeSet.clockTimeStampFlag = code;
    if (currentTimeSet.clockTimeStampFlag)
    {
      xReadFlag(code, "nuit_field_based_flag");
      currentTimeSet.numUnitFieldBasedFlag = code;
      xReadCode(5, code, "counting_type");
      currentTimeSet.countingType = code;
      xReadFlag(code, "full_timestamp_flag");
      currentTimeSet.fullTimeStampFlag = code;
      xReadFlag(code, "discontinuity_flag");
      currentTimeSet.discontinuityFlag = code;
      xReadFlag(code, "cnt_dropped_flag");
      currentTimeSet.cntDroppedFlag = code;
      xReadCode(9, code, "n_frames");
      currentTimeSet.numberOfFrames = code;
      if (currentTimeSet.fullTimeStampFlag)
      {
        xReadCode(6, code, "seconds_value");
        currentTimeSet.secondsValue = code;
        xReadCode(6, code, "minutes_value");
        currentTimeSet.minutesValue = code;
        xReadCode(5, code, "hours_value");
        currentTimeSet.hoursValue = code;
      }
      else
      {
        xReadFlag(code, "seconds_flag");
        currentTimeSet.secondsFlag = code;
        if (currentTimeSet.secondsFlag)
        {
          xReadCode(6, code, "seconds_value");
          currentTimeSet.secondsValue = code;
          xReadFlag(code, "minutes_flag");
          currentTimeSet.minutesFlag = code;
          if (currentTimeSet.minutesFlag)
          {
            xReadCode(6, code, "minutes_value");
            currentTimeSet.minutesValue = code;
            xReadFlag(code, "hours_flag");
            currentTimeSet.hoursFlag = code;
            if (currentTimeSet.hoursFlag)
            {
              xReadCode(5, code, "hours_value");
              currentTimeSet.hoursValue = code;
            }
          }
        }
      }
      xReadCode(5, code, "time_offset_length");
      currentTimeSet.timeOffsetLength = code;
      if (currentTimeSet.timeOffsetLength > 0)
      {
        xReadCode(currentTimeSet.timeOffsetLength, code, "time_offset_value");
        if ((code & (1 << (currentTimeSet.timeOffsetLength - 1))) == 0)
        {
          currentTimeSet.timeOffsetValue = code;
        }
        else
        {
          code &= (1 << (currentTimeSet.timeOffsetLength - 1)) - 1;
          currentTimeSet.timeOffsetValue = ~code + 1;
        }
      }
    }
    sei.timeSetArray[i] = currentTimeSet;
  }
}

void parseSeiH265::xReadSEIPayloadData(int payloadType, int payloadSize, nal_info &nal, hevc::hevc_nal_type nalUnitType, hevc::sps *sps, TComInputBitstream *bits)
{
  setBitstream(bits);
  switch (static_cast<hevc::hevc_sei_type>(payloadType))
  {
  case hevc::hevc_sei_type::BUFFERING_PERIOD:
    xParseSEIBufferingPeriod(nal.hevcSEI->hevc_sei_bp, payloadSize, sps);
    break;
  case hevc::hevc_sei_type::PICTURE_TIMING:
    xParseSEIPictureTiming(nal.hevcSEI->hevc_sei_pt, payloadSize, sps);
    break;
  case hevc::hevc_sei_type::PAN_SCAN_RECT:
    // xParseSEIPanScanRect((SEIPanScanRect &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::FILLER_PAYLOAD:
    // xParseSEIFillerPayload((SEIFillerPayload &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::USER_DATA_REGISTERED_ITU_T_T35:
    xParseSEIUserDataRegistered(nal.mpegCommonSEI->common_sei_dr, payloadSize);
    break;
  case hevc::hevc_sei_type::USER_DATA_UNREGISTERED:
    xParseSEIUserDataUnregistered(nal.mpegCommonSEI->common_sei_du, payloadSize);
    break;
  case hevc::hevc_sei_type::RECOVERY_POINT:
    // xParseSEIRecoveryPoint((SEIRecoveryPoint &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::SCENE_INFO:
    // xParseSEISceneInfo((SEISceneInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::PICTURE_SNAPSHOT:
    // xParseSEIPictureSnapshot((SEIPictureSnapshot &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::PROGRESSIVE_REFINEMENT_SEGMENT_START:
    // xParseSEIProgressiveRefinementSegmentStart((SEIProgressiveRefinementSegmentStart &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::PROGRESSIVE_REFINEMENT_SEGMENT_END:
    // xParseSEIProgressiveRefinementSegmentEnd((SEIProgressiveRefinementSegmentEnd &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::FILM_GRAIN_CHARACTERISTICS:
    // xParseSEIFilmGrainCharacteristics((SEIFilmGrainCharacteristics &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::POST_FILTER_HINT:
    // xParseSEIPostFilterHint((SEIPostFilterHint &)sei, payloadSize, sps);
    break;
  case hevc::hevc_sei_type::TONE_MAPPING_INFO:
    // xParseSEIToneMappingInfo((SEIToneMappingInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::FRAME_PACKING:
    // xParseSEIFramePacking((SEIFramePacking &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::DISPLAY_ORIENTATION:
    // xParseSEIDisplayOrientation((SEIDisplayOrientation &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::GREEN_METADATA:
    // xParseSEIGreenMetadataInfo((SEIGreenMetadataInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::SOP_DESCRIPTION:
    // xParseSEISOPDescription((SEISOPDescription &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::DECODED_PICTURE_HASH:
    // xParseSEIDecodedPictureHash((SEIDecodedPictureHash &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::ACTIVE_PARAMETER_SETS:
    // xParseSEIActiveParameterSets((SEIActiveParameterSets &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::DECODING_UNIT_INFO:
    // xParseSEIDecodingUnitInfo((SEIDecodingUnitInfo &)sei, payloadSize, sps);
    break;
  case hevc::hevc_sei_type::TEMPORAL_LEVEL0_INDEX:
    // xParseSEITemporalLevel0Index((SEITemporalLevel0Index &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::SCALABLE_NESTING:
    // xParseSEIScalableNesting((SEIScalableNesting &)sei, nalUnitType, payloadSize, sps);
    break;
  case hevc::hevc_sei_type::REGION_REFRESH_INFO:
    // xParseSEIRegionRefreshInfo((SEIRegionRefreshInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::NO_DISPLAY:
    // xParseSEINoDisplay((SEINoDisplay &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::TIME_CODE:
    xParseSEITimeCode(nal.hevcSEI->hevc_sei_tc, payloadSize);
    break;
  case hevc::hevc_sei_type::MASTERING_DISPLAY_COLOUR_VOLUME:
    // xParseSEIMasteringDisplayColourVolume((SEIMasteringDisplayColourVolume &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::SEGM_RECT_FRAME_PACKING:
    // xParseSEISegmentedRectFramePacking((SEISegmentedRectFramePacking &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::TEMP_MOTION_CONSTRAINED_TILE_SETS:
    // xParseSEITempMotionConstraintsTileSets((SEITempMotionConstrainedTileSets &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::MCTS_EXTRACTION_INFO_SET:
    // xParseSEIMCTSExtractionInfoSet((SEIMCTSExtractionInfoSet &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::CHROMA_RESAMPLING_FILTER_HINT:
    // xParseSEIChromaResamplingFilterHint((SEIChromaResamplingFilterHint &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::KNEE_FUNCTION_INFO:
    // xParseSEIKneeFunctionInfo((SEIKneeFunctionInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::COLOUR_REMAPPING_INFO:
    // xParseSEIColourRemappingInfo((SEIColourRemappingInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::DEINTERLACE_FIELD_IDENTIFICATION:
    // xParseSEIDeinterlaceFieldIdentification((SEIDeinterlaceFieldIdentification &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::CONTENT_LIGHT_LEVEL_INFO:
    // xParseSEIContentLightLevelInfo((SEIContentLightLevelInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::DEPENDENT_RAP_INDICATION:
    // xParseSEIDependentRAPIndication((SEIDependentRAPIndication &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::CODED_REGION_COMPLETION:
    // xParseSEICodedRegionCompletion((SEICodedRegionCompletion &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::ALTERNATIVE_TRANSFER_CHARACTERISTICS:
    // xParseSEIAlternativeTransferCharacteristics((SEIAlternativeTransferCharacteristics &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::AMBIENT_VIEWING_ENVIRONMENT:
    // xParseSEIAmbientViewingEnvironment((SEIAmbientViewingEnvironment &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::CONTENT_COLOUR_VOLUME:
    // xParseSEIContentColourVolume((SEIContentColourVolume &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::EQUIRECTANGULAR_PROJECTION:
    // xParseSEIEquirectangularProjection((SEIEquirectangularProjection &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::SPHERE_ROTATION:
    // xParseSEISphereRotation((SEISphereRotation &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::OMNI_VIEWPORT:
    // xParseSEIOmniViewport((SEIOmniViewport &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::CUBEMAP_PROJECTION:
    // xParseSEICubemapProjection((SEICubemapProjection &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::REGION_WISE_PACKING:
    // xParseSEIRegionWisePacking((SEIRegionWisePacking &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::ANNOTATED_REGIONS:
    // xParseSEIAnnotatedRegions((SEIAnnotatedRegions &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::FISHEYE_VIDEO_INFO:
    // xParseSEIFisheyeVideoInfo((SEIFisheyeVideoInfo &)sei, payloadSize);
    break;
  case hevc::hevc_sei_type::REGIONAL_NESTING:
    // xParseSEIRegionalNesting((SEIRegionalNesting &)sei, payloadSize, sps);
    break;
  case hevc::hevc_sei_type::SHUTTER_INTERVAL_INFO:
    // xParseSEIShutterInterval((SEIShutterIntervalInfo &)sei, payloadSize);
    break;
  }
}