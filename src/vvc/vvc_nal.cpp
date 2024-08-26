#include "nal_parse.h"
#include "vvc_nal.h"
#include "vvc_vlc.h"

static inline int floorLog2(uint32_t x)
{
  if (x == 0)
  {
    // note: ceilLog2() expects -1 as return value
    return -1;
  }
#ifdef __GNUC__
  return 31 - __builtin_clz(x);
#else
#ifdef _MSC_VER
  unsigned long r = 0;
  _BitScanReverse(&r, x);
  return r;
#else
  int result = 0;
  if (x & 0xffff0000)
  {
    x >>= 16;
    result += 16;
  }
  if (x & 0xff00)
  {
    x >>= 8;
    result += 8;
  }
  if (x & 0xf0)
  {
    x >>= 4;
    result += 4;
  }
  if (x & 0xc)
  {
    x >>= 2;
    result += 2;
  }
  if (x & 0x2)
  {
    x >>= 1;
    result += 1;
  }
  return result;
#endif
#endif
}

static inline int ceilLog2(uint32_t x)
{
  return (x == 0) ? -1 : floorLog2(x - 1) + 1;
}

void parseNalH266::copyRefPicList(vvc::SPS *sps, vvc::ReferencePictureList *source_rpl, vvc::ReferencePictureList *dest_rp)
{
  dest_rp->m_numberOfShorttermPictures = source_rpl->m_numberOfShorttermPictures;
  dest_rp->m_numberOfInterLayerPictures = sps->m_interLayerPresentFlag ? source_rpl->m_numberOfInterLayerPictures : 0;

  if (sps->m_interLayerPresentFlag)
  {
    dest_rp->m_ltrp_in_slice_header_flag = source_rpl->m_ltrp_in_slice_header_flag;
    dest_rp->m_numberOfLongtermPictures = source_rpl->m_numberOfLongtermPictures;
  }
  else
  {
    dest_rp->m_numberOfLongtermPictures = 0;
  }

  int numRefPic = dest_rp->m_numberOfShorttermPictures + dest_rp->m_numberOfLongtermPictures + dest_rp->m_numberOfInterLayerPictures;
  for (int ii = 0; ii < numRefPic; ii++)
  {
    dest_rp->setRefPicIdentifier(ii, source_rpl->m_refPicIdentifier[ii], source_rpl->m_isLongtermRefPic[ii], source_rpl->m_isInterLayerRefPic[ii], source_rpl->m_interLayerRefPicIdx[ii]);
  }
}

void parseNalH266::parseRefPicList(vvc::SPS *sps, vvc::ReferencePictureList *rpl, int rplIdx)
{
  uint32_t code;
  READ_UVLC(code, "num_ref_entries[ listIdx ][ rplsIdx ]");
  uint32_t numRefPic = code;
  uint32_t numStrp = 0;
  uint32_t numLtrp = 0;
  uint32_t numIlrp = 0;

  if (sps->m_bLongTermRefsPresent && numRefPic > 0 && rplIdx != -1)
  {
    READ_FLAG(code, "ltrp_in_slice_header_flag[ listIdx ][ rplsIdx ]");
    rpl->m_ltrp_in_slice_header_flag = code;
  }
  else if (sps->m_bLongTermRefsPresent)
  {
    rpl->m_ltrp_in_slice_header_flag = 1;
  }

  bool isLongTerm;
  int prevDelta = 2147483647;
  int deltaValue = 0;
  bool firstSTRP = true;

  rpl->m_interLayerPresentFlag = sps->m_interLayerPresentFlag;

  for (int ii = 0; ii < (int)numRefPic; ii++)
  {
    uint32_t isInterLayerRefPic = 0;

    if (rpl->m_interLayerPresentFlag)
    {
      READ_FLAG(isInterLayerRefPic, "inter_layer_ref_pic_flag[ listIdx ][ rplsIdx ][ i ]");

      if (isInterLayerRefPic)
      {
        READ_UVLC(code, "ilrp_idx[ listIdx ][ rplsIdx ][ i ]");
        rpl->setRefPicIdentifier(ii, 0, true, true, code);
        numIlrp++;
      }
    }

    if (!isInterLayerRefPic)
    {
      isLongTerm = false;
      if (sps->m_bLongTermRefsPresent)
      {
        READ_FLAG(code, "st_ref_pic_flag[ listIdx ][ rplsIdx ][ i ]");
        isLongTerm = (code == 1) ? false : true;
      }
      else
      {
        isLongTerm = false;
      }

      if (!isLongTerm)
      {
        READ_UVLC(code, "abs_delta_poc_st[ listIdx ][ rplsIdx ][ i ]");
        if ((!sps->m_useWeightPred && !sps->m_useWeightedBiPred) || (ii == 0))
        {
          code++;
        }
        int readValue = code;
        if (readValue > 0)
        {
          READ_FLAG(code, "strp_entry_sign_flag[ listIdx ][ rplsIdx ][ i ]");
          if (code)
          {
            readValue = -readValue;
          }
        }
        if (firstSTRP)
        {
          firstSTRP = false;
          prevDelta = deltaValue = readValue;
        }
        else
        {
          deltaValue = prevDelta + readValue;
          prevDelta = deltaValue;
        }

        rpl->setRefPicIdentifier(ii, deltaValue, isLongTerm, false, 0);
        numStrp++;
      }
      else
      {
        if (!rpl->m_ltrp_in_slice_header_flag)
        {
          READ_CODE(sps->m_uiBitsForPOC, code, "poc_lsb_lt[listIdx][rplsIdx][j]");
        }

        rpl->setRefPicIdentifier(ii, code, isLongTerm, false, 0);
        numLtrp++;
      }
    }
  }

  rpl->m_numberOfShorttermPictures = numStrp;
  rpl->m_numberOfLongtermPictures = numLtrp;
  rpl->m_numberOfInterLayerPictures = numIlrp;
}

void parseNalH266::pps_parse(unsigned char *nal_bitstream, vvc::PPS *pcPPS,  vvc::SPS *pcSPS, int curLen, parsingLevel level)
{
  uint32_t uiCode;

  for (int i = 0; i < curLen; i++)
    m_bits->m_fifo.push_back(nal_bitstream[i]);
  setBitstream(m_bits);

  int iCode;
  READ_CODE(6, uiCode, "pps_pic_parameter_set_id");
  CHECK(uiCode > 63, "PPS id exceeds boundary (63)");
  pcPPS->m_PPSId = uiCode;

  READ_CODE(4, uiCode, "pps_seq_parameter_set_id");
  pcPPS->m_SPSId = uiCode;

  if (level == parsingLevel::PARSING_PARAM_ID)
  {
    return;
  }

  READ_FLAG(uiCode, "pps_mixed_nalu_types_in_pic_flag");
  pcPPS->m_mixedNaluTypesInPicFlag = uiCode == 1;

  READ_UVLC(uiCode, "pps_pic_width_in_luma_samples");
  pcPPS->m_picWidthInLumaSamples = uiCode;
  READ_UVLC(uiCode, "pps_pic_height_in_luma_samples");
  pcPPS->m_picHeightInLumaSamples = uiCode;
  READ_FLAG(uiCode, "pps_conformance_window_flag");
  pcPPS->m_conformanceWindowFlag = uiCode;
  if (uiCode != 0)
  {
    vvc::Window &conf = pcPPS->m_conformanceWindow;
    READ_UVLC(uiCode, "pps_conf_win_left_offset");
    conf.m_winLeftOffset = uiCode;
    READ_UVLC(uiCode, "pps_conf_win_right_offset");
    conf.m_winRightOffset = uiCode;
    READ_UVLC(uiCode, "pps_conf_win_top_offset");
    conf.m_winTopOffset = uiCode;
    READ_UVLC(uiCode, "pps_conf_win_bottom_offset");
    conf.m_winBottomOffset = uiCode;
  }
  READ_FLAG(uiCode, "pps_scaling_window_explicit_signalling_flag");
  pcPPS->m_explicitScalingWindowFlag = uiCode;
  if (uiCode != 0)
  {
    vvc::Window &scalingWindow = pcPPS->m_conformanceWindow;
    READ_SVLC(iCode, "pps_scaling_win_left_offset");
    scalingWindow.m_winLeftOffset = iCode;
    READ_SVLC(iCode, "pps_scaling_win_right_offset");
    scalingWindow.m_winRightOffset = iCode;
    READ_SVLC(iCode, "pps_scaling_win_top_offset");
    scalingWindow.m_winTopOffset = iCode;
    READ_SVLC(iCode, "pps_scaling_win_bottom_offset");
    scalingWindow.m_winBottomOffset = iCode;
  }
  else
  {
    vvc::Window &scalingWindow = pcPPS->m_scalingWindow;
    vvc::Window &conf = pcPPS->m_conformanceWindow;
    scalingWindow.m_winLeftOffset = conf.m_winLeftOffset;
    scalingWindow.m_winRightOffset = conf.m_winRightOffset;
    scalingWindow.m_winTopOffset = conf.m_winTopOffset;
    scalingWindow.m_winBottomOffset = conf.m_winBottomOffset;
  }

  READ_FLAG(uiCode, "pps_output_flag_present_flag");
  pcPPS->m_OutputFlagPresentFlag = uiCode == 1;

  READ_FLAG(uiCode, "pps_no_pic_partition_flag");
  pcPPS->m_noPicPartitionFlag = uiCode == 1;
  READ_FLAG(uiCode, "pps_subpic_id_mapping_present_flag");
  pcPPS->m_subPicIdMappingInPpsFlag = uiCode != 0;
  if (pcPPS->m_subPicIdMappingInPpsFlag)
  {
    if (!pcPPS->m_noPicPartitionFlag)
    {
      READ_UVLC(uiCode, "pps_num_subpics_minus1");
      pcPPS->m_numSubPics = uiCode + 1;
    }
    else
    {
      pcPPS->m_numSubPics = 1;
    }
    CHECK(uiCode > vvc::MAX_NUM_SUB_PICS - 1, "Number of sub-pictures exceeds limit");

    READ_UVLC(uiCode, "pps_subpic_id_len_minus1");
    pcPPS->m_subPicIdLen = uiCode + 1;
    CHECK(uiCode > 15, "Invalid pps_subpic_id_len_minus1 signalled");

    CHECK((uint32_t)(1 << pcPPS->m_subPicIdLen) < pcPPS->m_numSubPics, "pps_subpic_id_len exceeds valid range");
    for (int picIdx = 0; picIdx < (int)pcPPS->m_numSubPics; picIdx++)
    {
      READ_CODE(pcPPS->m_subPicIdLen, uiCode, "pps_subpic_id[i]");
      pcPPS->m_subPicId[picIdx] = uiCode;
    }
  }
  if (!pcPPS->m_noPicPartitionFlag)
  {
    int colIdx, rowIdx;
    pcPPS->resetTileSliceInfo();

    // CTU size - required to match size in SPS
    READ_CODE(2, uiCode, "pps_log2_ctu_size_minus5");
    pcPPS->m_log2CtuSize = uiCode + 5;
    pcPPS->m_picWidthInCtu = std::ceil((float)(pcPPS->m_picWidthInLumaSamples) / (1 << pcPPS->m_log2CtuSize));
    pcPPS->m_picHeightInCtu = std::ceil((float)(pcPPS->m_picHeightInLumaSamples) / (1 << pcPPS->m_log2CtuSize));
    CHECK(uiCode > 2, "pps_log2_ctu_size_minus5 must be less than or equal to 2");

    // number of explicit tile columns/rows
    READ_UVLC(uiCode, "pps_num_exp_tile_columns_minus1");
    pcPPS->m_numExpTileCols = uiCode + 1;
    READ_UVLC(uiCode, "pps_num_exp_tile_rows_minus1");
    pcPPS->m_numExpTileRows = uiCode + 1;
    CHECK(pcPPS->m_numExpTileCols > vvc::MAX_TILE_COLS, "Number of explicit tile columns exceeds valid range");

    // tile sizes
    for (colIdx = 0; colIdx < (int)pcPPS->m_numExpTileCols; colIdx++)
    {
      READ_UVLC(uiCode, "pps_tile_column_width_minus1[i]");
      // pcPPS->addTileColumnWidth(uiCode + 1);
      pcPPS->m_tileColWidth.push_back(uiCode + 1);
      CHECK(uiCode > (pcPPS->m_picWidthInCtu - 1), "The value of pps_tile_column_width_minus1[i] shall be in the range of 0 to PicWidthInCtbY-1, inclusive");
    }
    for (rowIdx = 0; rowIdx < (int)pcPPS->m_numExpTileRows; rowIdx++)
    {
      READ_UVLC(uiCode, "pps_tile_row_height_minus1[i]");
      pcPPS->m_tileRowHeight.push_back(uiCode + 1);
      CHECK(uiCode > (pcPPS->m_picHeightInCtu - 1), "The value of pps_tile_row_height_minus shall be in the range of 0 to PicHeightInCtbY-1, inclusive");
    }
    pcPPS->initTiles();
    // rectangular slice signalling
    if (pcPPS->m_numTileCols * pcPPS->m_numTileRows > 1)
    {
      READ_CODE(1, uiCode, "pps_loop_filter_across_tiles_enabled_flag");
      pcPPS->m_loopFilterAcrossTilesEnabledFlag = uiCode == 1;
      READ_CODE(1, uiCode, "pps_rect_slice_flag");
    }
    else
    {
      pcPPS->m_loopFilterAcrossTilesEnabledFlag = false;
      uiCode = 1;
    }
    pcPPS->m_rectSliceFlag = uiCode == 1;
    if (pcPPS->m_rectSliceFlag)
    {
      READ_FLAG(uiCode, "pps_single_slice_per_subpic_flag");
      pcPPS->m_singleSlicePerSubPicFlag = uiCode == 1;
    }
    else
    {
      pcPPS->m_singleSlicePerSubPicFlag = 0;
    }
    if (pcPPS->m_rectSliceFlag & !(pcPPS->m_singleSlicePerSubPicFlag))
    {
      int32_t tileIdx = 0;

      READ_UVLC(uiCode, "pps_num_slices_in_pic_minus1");
      pcPPS->m_numSlicesInPic = uiCode + 1;
      CHECK(pcPPS->m_numSlicesInPic > vvc::MAX_SLICES, "Number of slices in picture exceeds valid range");

      if ((pcPPS->m_numSlicesInPic - 1) > 1)
      {
        READ_CODE(1, uiCode, "pps_tile_idx_delta_present_flag");
        pcPPS->m_tileIdxDeltaPresentFlag = uiCode == 1;
      }
      else
      {
        pcPPS->m_tileIdxDeltaPresentFlag = 0;
      }
      pcPPS->initRectSlices();

      // read rectangular slice parameters
      for (int i = 0; i < (int)pcPPS->m_numSlicesInPic - 1; i++)
      {
        pcPPS->m_rectSlices[i].m_tileIdx = tileIdx;

        // complete tiles within a single slice
        if ((tileIdx % pcPPS->m_numTileCols) != pcPPS->m_numTileCols - 1)
        {
          READ_UVLC(uiCode, "pps_slice_width_in_tiles_minus1[i]");
          pcPPS->m_rectSlices[i].m_sliceWidthInTiles = uiCode + 1;
        }
        else
        {
          pcPPS->m_rectSlices[i].m_sliceWidthInTiles = 1;
        }

        if (tileIdx / pcPPS->m_numTileCols != pcPPS->m_numTileRows - 1 &&
            (pcPPS->m_tileIdxDeltaPresentFlag || tileIdx % pcPPS->m_numTileCols == 0))
        {
          READ_UVLC(uiCode, "pps_slice_height_in_tiles_minus1[i]");
          pcPPS->m_rectSlices[i].m_sliceHeightInTiles = uiCode + 1;
        }
        else
        {
          if ((tileIdx / pcPPS->m_numTileCols) == pcPPS->m_numTileRows - 1)
          {
            pcPPS->m_rectSlices[i].m_sliceHeightInTiles = 1;
          }
          else
          {
            pcPPS->m_rectSlices[i].m_sliceHeightInTiles = pcPPS->m_rectSlices[i - 1].m_sliceHeightInTiles;
          }
        }

        // multiple slices within a single tile special case
        if (pcPPS->m_rectSlices[i].m_sliceWidthInTiles == 1 && pcPPS->m_rectSlices[i].m_sliceHeightInTiles == 1)
        {
          if (pcPPS->m_tileRowHeight[tileIdx / pcPPS->m_numTileCols] > 1)
          {
            READ_UVLC(uiCode, "pps_num_exp_slices_in_tile[i]");
            if (uiCode == 0)
            {
              pcPPS->m_rectSlices[i].m_numSlicesInTile = 1;
              pcPPS->m_rectSlices[i].m_sliceHeightInCtu = pcPPS->m_tileRowHeight[tileIdx / pcPPS->m_numTileCols];
            }
            else
            {
              uint32_t numExpSliceInTile = uiCode;
              uint32_t remTileRowHeight = pcPPS->m_tileRowHeight[tileIdx / pcPPS->m_numTileCols];
              int j = 0;

              for (; j < (int)numExpSliceInTile; j++)
              {
                READ_UVLC(uiCode, "pps_exp_slice_height_in_ctus_minus1[i]");
                pcPPS->m_rectSlices[i + j].m_sliceHeightInCtu = uiCode + 1;
                remTileRowHeight -= (uiCode + 1);
              }
              uint32_t uniformSliceHeight = uiCode + 1;

              while (remTileRowHeight >= uniformSliceHeight)
              {
                pcPPS->m_rectSlices[i + j].m_sliceHeightInCtu = uniformSliceHeight;
                remTileRowHeight -= uniformSliceHeight;
                j++;
              }
              if (remTileRowHeight > 0)
              {
                pcPPS->m_rectSlices[i + j].m_sliceHeightInCtu = remTileRowHeight;
                j++;
              }
              for (int k = 0; k < j; k++)
              {
                pcPPS->m_rectSlices[i + k].m_numSlicesInTile = j;
                pcPPS->m_rectSlices[i + k].m_sliceWidthInTiles = 1;
                pcPPS->m_rectSlices[i + k].m_sliceHeightInTiles = 1;
                pcPPS->m_rectSlices[i + k].m_tileIdx = tileIdx;
              }
              i += (j - 1);
            }
          }
          else
          {
            pcPPS->m_rectSlices[i].m_numSlicesInTile = 1;
            pcPPS->m_rectSlices[i].m_sliceHeightInCtu = pcPPS->m_tileRowHeight[tileIdx / pcPPS->m_numTileCols];
          }
        }

        // tile index offset to start of next slice
        if (i < (int)pcPPS->m_numSlicesInPic - 1)
        {
          if (pcPPS->m_tileIdxDeltaPresentFlag)
          {
            int32_t tileIdxDelta;
            READ_SVLC(tileIdxDelta, "pps_tile_idx_delta[i]");
            tileIdx += tileIdxDelta;
            CHECK(tileIdx < 0 || tileIdx >= (int)(pcPPS->m_numTileCols * pcPPS->m_numTileRows), "Invalid pps_tile_idx_delta.");
          }
          else
          {
            tileIdx += pcPPS->m_rectSlices[i].m_sliceWidthInTiles;
            if (tileIdx % pcPPS->m_numTileCols == 0)
            {
              tileIdx += (pcPPS->m_rectSlices[i].m_sliceHeightInTiles - 1) * pcPPS->m_numTileCols;
            }
          }
        }
      }
      pcPPS->m_rectSlices[pcPPS->m_numSlicesInPic - 1].m_tileIdx = tileIdx;
    }

    if (pcPPS->m_rectSliceFlag == 0 || pcPPS->m_singleSlicePerSubPicFlag || pcPPS->m_numSlicesInPic > 1)
    {
      READ_CODE(1, uiCode, "pps_loop_filter_across_slices_enabled_flag");
      pcPPS->m_loopFilterAcrossSlicesEnabledFlag = uiCode == 1;
    }
    else
    {
      pcPPS->m_loopFilterAcrossSlicesEnabledFlag = false;
    }
  }
  else
  {
    pcPPS->m_singleSlicePerSubPicFlag = 1;
  }

  READ_FLAG(uiCode, "pps_cabac_init_present_flag");
  pcPPS->m_cabacInitPresentFlag = uiCode ? true : false;

  READ_UVLC(uiCode, "pps_num_ref_idx_default_active_minus1[0]");
  CHECK(uiCode > 14, "Invalid code read");
  pcPPS->m_numRefIdxL0DefaultActive = uiCode + 1;

  READ_UVLC(uiCode, "pps_num_ref_idx_default_active_minus1[1]");
  CHECK(uiCode > 14, "Invalid code read");
  pcPPS->m_numRefIdxL1DefaultActive = uiCode + 1;

  READ_FLAG(uiCode, "pps_rpl1_idx_present_flag");
  pcPPS->m_rpl1IdxPresentFlag = uiCode;
  READ_FLAG(uiCode, "pps_weighted_pred_flag"); // Use of Weighting Prediction (P_SLICE)
  pcPPS->m_bUseWeightPred = uiCode == 1;
  READ_FLAG(uiCode, "pps_weighted_bipred_flag"); // Use of Bi-Directional Weighting Prediction (B_SLICE)
  pcPPS->m_useWeightedBiPred = uiCode == 1;
  READ_FLAG(uiCode, "pps_ref_wraparound_enabled_flag");
  pcPPS->m_wrapAroundEnabledFlag = uiCode ? true : false;
  if (pcPPS->m_wrapAroundEnabledFlag)
  {
    READ_UVLC(uiCode, "pps_ref_wraparound_offset");
    pcPPS->m_picWidthMinusWrapAroundOffset = uiCode;
  }
  else
  {
    pcPPS->m_picWidthMinusWrapAroundOffset = 0;
  }

  READ_SVLC(iCode, "pps_init_qp_minus26");
  pcPPS->m_picInitQPMinus26 = iCode;
  READ_FLAG(uiCode, "pps_cu_qp_delta_enabled_flag");
  pcPPS->m_useDQP = uiCode ? true : false;
  READ_FLAG(uiCode, "pps_chroma_tool_offsets_present_flag");
  pcPPS->m_usePPSChromaTool = uiCode ? true : false;
  if (pcPPS->m_usePPSChromaTool)
  {
    READ_SVLC(iCode, "pps_cb_qp_offset");
    pcPPS->m_chromaCbQpOffset = iCode;
    CHECK(pcPPS->m_chromaCbQpOffset < -12, "Invalid Cb QP offset");
    CHECK(pcPPS->m_chromaCbQpOffset > 12, "Invalid Cb QP offset");

    READ_SVLC(iCode, "pps_cr_qp_offset");
    pcPPS->m_chromaCrQpOffset = iCode;
    CHECK(pcPPS->m_chromaCrQpOffset < -12, "Invalid Cr QP offset");
    CHECK(pcPPS->m_chromaCrQpOffset > 12, "Invalid Cr QP offset");

    READ_FLAG(uiCode, "pps_joint_cbcr_qp_offset_present_flag");
    pcPPS->m_chromaJointCbCrQpOffsetPresentFlag = uiCode ? true : false;

    if (pcPPS->m_chromaJointCbCrQpOffsetPresentFlag)
    {
      READ_SVLC(iCode, "pps_joint_cbcr_qp_offset_value");
    }
    else
    {
      iCode = 0;
    }
    pcPPS->m_chromaCbCrQpOffset = iCode;

    CHECK(pcPPS->m_chromaCbCrQpOffset < -12, "Invalid CbCr QP offset");
    CHECK(pcPPS->m_chromaCbCrQpOffset > 12, "Invalid CbCr QP offset");

    CHECK(vvc::MAX_NUM_COMPONENT > 3, "Invalid maximal number of components");

    READ_FLAG(uiCode, "pps_slice_chroma_qp_offsets_present_flag");
    pcPPS->m_bSliceChromaQpFlag = uiCode ? true : false;

    READ_FLAG(uiCode, "pps_cu_chroma_qp_offset_list_enabled_flag");
    if (uiCode == 0)
    {
      pcPPS->m_chromaQpOffsetListLen = 0;
    }
    else
    {
      uint32_t tableSizeMinus1 = 0;
      READ_UVLC(tableSizeMinus1, "pps_chroma_qp_offset_list_len_minus1");
      CHECK(tableSizeMinus1 >= vvc::MAX_QP_OFFSET_LIST_SIZE, "Table size exceeds maximum");

      for (int cuChromaQpOffsetIdx = 0; cuChromaQpOffsetIdx <= (int)tableSizeMinus1; cuChromaQpOffsetIdx++)
      {
        int cbOffset;
        int crOffset;
        int jointCbCrOffset;
        READ_SVLC(cbOffset, "pps_cb_qp_offset_list[i]");
        CHECK(cbOffset < -12 || cbOffset > 12, "Invalid chroma QP offset");
        READ_SVLC(crOffset, "pps_cr_qp_offset_list[i]");
        CHECK(crOffset < -12 || crOffset > 12, "Invalid chroma QP offset");
        if (pcPPS->m_chromaJointCbCrQpOffsetPresentFlag)
        {
          READ_SVLC(jointCbCrOffset, "pps_joint_cbcr_qp_offset_list[i]");
        }
        else
        {
          jointCbCrOffset = 0;
        }
        CHECK(jointCbCrOffset < -12 || jointCbCrOffset > 12, "Invalid chroma QP offset");
        // table uses +1 for index (see comment inside the function)
        pcPPS->setChromaQpOffsetListEntry(cuChromaQpOffsetIdx + 1, cbOffset, crOffset, jointCbCrOffset);
      }
      CHECK(pcPPS->m_chromaQpOffsetListLen != (int)tableSizeMinus1 + 1, "Invalid chroma QP offset list length");
    }
  }
  else
  {
    pcPPS->m_chromaCbQpOffset = 0;
    pcPPS->m_chromaCrQpOffset = 0;
    pcPPS->m_chromaJointCbCrQpOffsetPresentFlag = 0;
    pcPPS->m_bSliceChromaQpFlag = 0;
    pcPPS->m_chromaQpOffsetListLen = 0;
  }
  READ_FLAG(uiCode, "pps_deblocking_filter_control_present_flag");
  pcPPS->m_deblockingFilterControlPresentFlag = uiCode ? true : false;
  if (pcPPS->m_deblockingFilterControlPresentFlag)
  {
    READ_FLAG(uiCode, "pps_deblocking_filter_override_enabled_flag");
    pcPPS->m_deblockingFilterOverrideEnabledFlag = uiCode ? true : false;
    READ_FLAG(uiCode, "pps_deblocking_filter_disabled_flag");
    pcPPS->m_ppsDeblockingFilterDisabledFlag = uiCode ? true : false;
    if (!pcPPS->m_noPicPartitionFlag && pcPPS->m_deblockingFilterOverrideEnabledFlag)
    {
      READ_FLAG(uiCode, "pps_dbf_info_in_ph_flag");
      pcPPS->m_dbfInfoInPhFlag = uiCode ? true : false;
    }
    else
    {
      pcPPS->m_dbfInfoInPhFlag = false;
    }
    if (!pcPPS->m_ppsDeblockingFilterDisabledFlag)
    {
      READ_SVLC(iCode, "pps_beta_offset_div2");
      pcPPS->m_deblockingFilterBetaOffsetDiv2 = iCode;
      CHECK(pcPPS->m_deblockingFilterBetaOffsetDiv2 < -12 ||
                pcPPS->m_deblockingFilterBetaOffsetDiv2 > 12,
            "Invalid deblocking filter configuration");

      READ_SVLC(iCode, "pps_tc_offset_div2");
      pcPPS->m_deblockingFilterTcOffsetDiv2 = iCode;
      CHECK(pcPPS->m_deblockingFilterTcOffsetDiv2 < -12 ||
                pcPPS->m_deblockingFilterTcOffsetDiv2 > 12,
            "Invalid deblocking filter configuration");

      if (pcPPS->m_usePPSChromaTool)
      {
        READ_SVLC(iCode, "pps_cb_beta_offset_div2");
        pcPPS->m_deblockingFilterCbBetaOffsetDiv2 = iCode;
        CHECK(pcPPS->m_deblockingFilterCbBetaOffsetDiv2 < -12 ||
                  pcPPS->m_deblockingFilterCbBetaOffsetDiv2 > 12,
              "Invalid deblocking filter configuration");

        READ_SVLC(iCode, "pps_cb_tc_offset_div2");
        pcPPS->m_deblockingFilterCbTcOffsetDiv2 = iCode;
        CHECK(pcPPS->m_deblockingFilterCbTcOffsetDiv2 < -12 ||
                  pcPPS->m_deblockingFilterCbTcOffsetDiv2 > 12,
              "Invalid deblocking filter configuration");

        READ_SVLC(iCode, "pps_cr_beta_offset_div2");
        pcPPS->m_deblockingFilterCrBetaOffsetDiv2 = iCode;
        CHECK(pcPPS->m_deblockingFilterCrBetaOffsetDiv2 < -12 ||
                  pcPPS->m_deblockingFilterCrBetaOffsetDiv2 > 12,
              "Invalid deblocking filter configuration");

        READ_SVLC(iCode, "pps_cr_tc_offset_div2");
        pcPPS->m_deblockingFilterCrTcOffsetDiv2 = iCode;
        CHECK(pcPPS->m_deblockingFilterCrTcOffsetDiv2 < -12 ||
                  pcPPS->m_deblockingFilterCrTcOffsetDiv2 > 12,
              "Invalid deblocking filter configuration");
      }
      else
      {
        pcPPS->m_deblockingFilterCbBetaOffsetDiv2 = pcPPS->m_deblockingFilterBetaOffsetDiv2;
        pcPPS->m_deblockingFilterCbTcOffsetDiv2 = pcPPS->m_deblockingFilterTcOffsetDiv2;
        pcPPS->m_deblockingFilterCrBetaOffsetDiv2 = pcPPS->m_deblockingFilterBetaOffsetDiv2;
        pcPPS->m_deblockingFilterCrTcOffsetDiv2 = pcPPS->m_deblockingFilterTcOffsetDiv2;
      }
    }
  }
  else
  {
    pcPPS->m_deblockingFilterOverrideEnabledFlag = false;
    pcPPS->m_dbfInfoInPhFlag = false;
  }

  if (!pcPPS->m_noPicPartitionFlag)
  {
    READ_FLAG(uiCode, "pps_rpl_info_in_ph_flag");
    pcPPS->m_rplInfoInPhFlag = uiCode ? true : false;
    READ_FLAG(uiCode, "pps_sao_info_in_ph_flag");
    pcPPS->m_saoInfoInPhFlag = uiCode ? true : false;
    READ_FLAG(uiCode, "pps_alf_info_in_ph_flag");
    pcPPS->m_alfInfoInPhFlag = uiCode ? true : false;
    if ((pcPPS->m_bUseWeightPred || pcPPS->m_useWeightedBiPred) && pcPPS->m_rplInfoInPhFlag)
    {
      READ_FLAG(uiCode, "pps_wp_info_in_ph_flag");
      pcPPS->m_wpInfoInPhFlag = uiCode ? true : false;
    }
    else
    {
      pcPPS->m_wpInfoInPhFlag = false;
    }
    READ_FLAG(uiCode, "pps_qp_delta_info_in_ph_flag");
    pcPPS->m_qpDeltaInfoInPhFlag = uiCode ? true : false;
  }
  else
  {
    pcPPS->m_rplInfoInPhFlag = false;
    pcPPS->m_saoInfoInPhFlag = false;
    pcPPS->m_alfInfoInPhFlag = false;
    pcPPS->m_wpInfoInPhFlag = false;
    pcPPS->m_qpDeltaInfoInPhFlag = false;
  }

  READ_FLAG(uiCode, "pps_picture_header_extension_present_flag");
  pcPPS->m_pictureHeaderExtensionPresentFlag = uiCode;
  READ_FLAG(uiCode, "pps_slice_header_extension_present_flag");
  pcPPS->m_sliceHeaderExtensionPresentFlag = uiCode;

  READ_FLAG(uiCode, "pps_extension_flag");

  if (uiCode)
  {
    while (xMoreRbspData())
    {
      READ_FLAG(uiCode, "pps_extension_data_flag");
    }
  }
  xReadRbspTrailingBits();
}

void parseNalH266::aps_parse(unsigned char *nal_bitstream, vvc::APS *aps, int curLen, parsingLevel level)
{
  uint32_t code;
  for (int i = 0; i < curLen; i++)
    m_bits->m_fifo.push_back(nal_bitstream[i]);
  setBitstream(m_bits);

  READ_CODE(3, code, "aps_params_type");
  aps->m_APSType = (vvc::ApsType)code;

  READ_CODE(5, code, "adaptation_parameter_set_id");
  aps->m_APSId = code;
  uint32_t codeApsChromaPresentFlag;
  READ_FLAG(codeApsChromaPresentFlag, "aps_chroma_present_flag");
  aps->chromaPresentFlag = codeApsChromaPresentFlag;

  const vvc::ApsType apsType = aps->m_APSType;
  if (apsType == vvc::ALF_APS)
  {
    alf_aps_parse(aps);
  }
  else if (apsType == vvc::LMCS_APS)
  {
    lmcs_aps_parse(aps);
  }
  else if (apsType == vvc::SCALING_LIST_APS)
  {
    scalinglist_aps_parse(aps);
  }
  READ_FLAG(code, "aps_extension_flag");
  if (code)
  {
    while (xMoreRbspData())
    {
      READ_FLAG(code, "aps_extension_data_flag");
    }
  }
  xReadRbspTrailingBits();
}

void parseNalH266::alf_aps_parse(vvc::APS *aps)
{
  uint32_t code;

  vvc::AlfParam param = aps->m_alfAPSParam;
  param.reset();
  param.enabledFlag[vvc::COMPONENT_Y] = param.enabledFlag[vvc::COMPONENT_Cb] = param.enabledFlag[vvc::COMPONENT_Cr] = true;
  READ_FLAG(code, "alf_luma_new_filter");
  param.newFilterFlag[vvc::CHANNEL_TYPE_LUMA] = code;

  if (aps->chromaPresentFlag)
  {
    READ_FLAG(code, "alf_chroma_new_filter");
    param.newFilterFlag[vvc::CHANNEL_TYPE_CHROMA] = code;
  }
  else
  {
    param.newFilterFlag[vvc::CHANNEL_TYPE_CHROMA] = 0;
  }

  vvc::CcAlfFilterParam ccAlfParam = aps->m_ccAlfAPSParam;
  if (aps->chromaPresentFlag)
  {
    READ_FLAG(code, "alf_cc_cb_filter_signal_flag");
    ccAlfParam.newCcAlfFilter[vvc::COMPONENT_Cb - 1] = code;
  }
  else
  {
    ccAlfParam.newCcAlfFilter[vvc::COMPONENT_Cb - 1] = 0;
  }
  if (aps->chromaPresentFlag)
  {
    READ_FLAG(code, "alf_cc_cr_filter_signal_flag");
    ccAlfParam.newCcAlfFilter[vvc::COMPONENT_Cr - 1] = code;
  }
  else
  {
    ccAlfParam.newCcAlfFilter[vvc::COMPONENT_Cr - 1] = 0;
  }
  CHECK(param.newFilterFlag[vvc::CHANNEL_TYPE_LUMA] == 0 && param.newFilterFlag[vvc::CHANNEL_TYPE_CHROMA] == 0 && ccAlfParam.newCcAlfFilter[vvc::COMPONENT_Cb - 1] == 0 && ccAlfParam.newCcAlfFilter[vvc::COMPONENT_Cr - 1] == 0,
        "bitstream conformance error: one of alf_luma_filter_signal_flag, alf_chroma_filter_signal_flag, "
        "alf_cross_component_cb_filter_signal_flag, and alf_cross_component_cr_filter_signal_flag shall be nonzero");

  if (param.newFilterFlag[vvc::CHANNEL_TYPE_LUMA])
  {
    READ_FLAG(code, "alf_luma_clip");
    param.nonLinearFlag[vvc::CHANNEL_TYPE_LUMA] = code ? true : false;
    READ_UVLC(code, "alf_luma_num_filters_signalled_minus1");
    param.numLumaFilters = code + 1;
    if (param.numLumaFilters > 1)
    {
      const int length = ceilLog2(param.numLumaFilters);
      for (int i = 0; i < vvc::MAX_NUM_ALF_CLASSES; i++)
      {
        READ_CODE(length, code, "alf_luma_coeff_delta_idx");
        param.filterCoeffDeltaIdx[i] = code;
      }
    }
    else
    {
      memset(param.filterCoeffDeltaIdx, 0, sizeof(param.filterCoeffDeltaIdx));
    }
    alfFilter(param, false, 0);
  }
  if (param.newFilterFlag[vvc::CHANNEL_TYPE_CHROMA])
  {
    READ_FLAG(code, "alf_nonlinear_enable_flag_chroma");
    param.nonLinearFlag[vvc::CHANNEL_TYPE_CHROMA] = code ? true : false;

    if (vvc::MAX_NUM_ALF_ALTERNATIVES_CHROMA > 1)
    {
      READ_UVLC(code, "alf_chroma_num_alts_minus1");
    }
    else
    {
      code = 0;
    }

    param.numAlternativesChroma = code + 1;

    for (int altIdx = 0; altIdx < param.numAlternativesChroma; ++altIdx)
    {
      alfFilter(param, true, altIdx);
    }
  }

  for (int ccIdx = 0; ccIdx < 2; ccIdx++)
  {
    if (ccAlfParam.newCcAlfFilter[ccIdx])
    {
      if (vvc::MAX_NUM_CC_ALF_FILTERS > 1)
      {
        READ_UVLC(code, ccIdx == 0 ? "alf_cc_cb_filters_signalled_minus1" : "alf_cc_cr_filters_signalled_minus1");
      }
      else
      {
        code = 0;
      }
      ccAlfParam.ccAlfFilterCount[ccIdx] = code + 1;

      for (int filterIdx = 0; filterIdx < ccAlfParam.ccAlfFilterCount[ccIdx]; filterIdx++)
      {
        ccAlfParam.ccAlfFilterIdxEnabled[ccIdx][filterIdx] = true;
        vvc::AlfFilterShape alfShape(vvc::size_CC_ALF);

        short *coeff = ccAlfParam.ccAlfCoeff[ccIdx][filterIdx];
        // Filter coefficients
        for (int i = 0; i < alfShape.numCoeff - 1; i++)
        {
          READ_CODE(vvc::CCALF_BITS_PER_COEFF_LEVEL, code,
                    ccIdx == 0 ? "alf_cc_cb_mapped_coeff_abs" : "alf_cc_cr_mapped_coeff_abs");
          if (code == 0)
          {
            coeff[i] = 0;
          }
          else
          {
            coeff[i] = 1 << (code - 1);
            READ_FLAG(code, ccIdx == 0 ? "alf_cc_cb_coeff_sign" : "alf_cc_cr_coeff_sign");
            coeff[i] *= 1 - 2 * code;
          }
        }
      }

      for (int filterIdx = ccAlfParam.ccAlfFilterCount[ccIdx]; filterIdx < vvc::MAX_NUM_CC_ALF_FILTERS; filterIdx++)
      {
        ccAlfParam.ccAlfFilterIdxEnabled[ccIdx][filterIdx] = false;
      }
    }
  }

  aps->m_ccAlfAPSParam = ccAlfParam;
  aps->m_alfAPSParam = param;
}

void parseNalH266::lmcs_aps_parse(vvc::APS *aps)
{
  uint32_t code;

  vvc::SliceReshapeInfo &info = aps->m_reshapeAPSInfo;
  memset(info.reshaperModelBinCWDelta, 0, vvc::PIC_CODE_CW_BINS * sizeof(int));
  READ_UVLC(code, "lmcs_min_bin_idx");
  info.reshaperModelMinBinIdx = code;
  READ_UVLC(code, "lmcs_delta_max_bin_idx");
  info.reshaperModelMaxBinIdx = vvc::PIC_CODE_CW_BINS - 1 - code;
  READ_UVLC(code, "lmcs_delta_cw_prec_minus1");
  info.maxNbitsNeededDeltaCW = code + 1;
  assert(info.maxNbitsNeededDeltaCW > 0);
  for (uint32_t i = info.reshaperModelMinBinIdx; i <= info.reshaperModelMaxBinIdx; i++)
  {
    READ_CODE(info.maxNbitsNeededDeltaCW, code, "lmcs_delta_abs_cw[ i ]");
    int absCW = code;
    if (absCW > 0)
    {
      READ_CODE(1, code, "lmcs_delta_sign_cw_flag[ i ]");
    }
    int signCW = code;
    info.reshaperModelBinCWDelta[i] = (1 - 2 * signCW) * absCW;
  }
  if (aps->chromaPresentFlag)
  {
    READ_CODE(3, code, "lmcs_delta_abs_crs");
  }
  int absCW = aps->chromaPresentFlag ? code : 0;
  if (absCW > 0)
  {
    READ_CODE(1, code, "lmcs_delta_sign_crs_flag");
  }
  int signCW = code;
  info.chrResScalingOffset = (1 - 2 * signCW) * absCW;

  aps->m_reshapeAPSInfo = info;
}

void parseNalH266::scalinglist_aps_parse(vvc::APS *aps)
{
  vvc::ScalingList &info = aps->m_scalingListApsInfo;
  parseScalingList(&info, aps->chromaPresentFlag);
}

void parseNalH266::parseVUI(vvc::VUI *pcVUI, vvc::SPS *pcSPS)
{
  unsigned vuiPayloadSize = pcSPS->m_vuiPayloadSize;
  InputBitstream *bs = getBitstream();
  setBitstream(bs->extractSubstream(vuiPayloadSize * 8));

  uint32_t symbol;

  READ_FLAG(symbol, "vui_progressive_source_flag");
  pcVUI->m_progressiveSourceFlag = symbol ? true : false;
  READ_FLAG(symbol, "vui_interlaced_source_flag");
  pcVUI->m_interlacedSourceFlag = symbol ? true : false;
  READ_FLAG(symbol, "vui_non_packed_constraint_flag");
  pcVUI->m_nonPackedFlag = symbol ? true : false;
  READ_FLAG(symbol, "vui_non_projected_constraint_flag");
  pcVUI->m_nonProjectedFlag = symbol ? true : false;
  READ_FLAG(symbol, "vui_aspect_ratio_info_present_flag");
  pcVUI->m_aspectRatioInfoPresentFlag = symbol;
  if (pcVUI->m_aspectRatioInfoPresentFlag)
  {
    READ_FLAG(symbol, "vui_aspect_ratio_constant_flag");
    pcVUI->m_aspectRatioConstantFlag = symbol;
    READ_CODE(8, symbol, "vui_aspect_ratio_idc");
    pcVUI->m_aspectRatioIdc = symbol;
    if (pcVUI->m_aspectRatioIdc == 255)
    {
      READ_CODE(16, symbol, "vui_sar_width");
      pcVUI->m_sarWidth = symbol;
      READ_CODE(16, symbol, "vui_sar_height");
      pcVUI->m_sarHeight = symbol;
    }
  }

  READ_FLAG(symbol, "vui_overscan_info_present_flag");
  pcVUI->m_overscanInfoPresentFlag = symbol;
  if (pcVUI->m_overscanInfoPresentFlag)
  {
    READ_FLAG(symbol, "vui_overscan_appropriate_flag");
    pcVUI->m_overscanAppropriateFlag = symbol;
  }

  READ_FLAG(symbol, "vui_colour_description_present_flag");
  pcVUI->m_colourDescriptionPresentFlag = symbol;
  if (pcVUI->m_colourDescriptionPresentFlag)
  {
    READ_CODE(8, symbol, "vui_colour_primaries");
    pcVUI->m_colourPrimaries = symbol;
    READ_CODE(8, symbol, "vui_transfer_characteristics");
    pcVUI->m_transferCharacteristics = symbol;
    READ_CODE(8, symbol, "vui_matrix_coeffs");
    pcVUI->m_matrixCoefficients = symbol;
    READ_FLAG(symbol, "vui_full_range_flag");
    pcVUI->m_videoFullRangeFlag = symbol;
  }

  READ_FLAG(symbol, "vui_chroma_loc_info_present_flag");
  pcVUI->m_chromaLocInfoPresentFlag = symbol;
  if (pcVUI->m_chromaLocInfoPresentFlag)
  {
    if (pcVUI->m_progressiveSourceFlag && !pcVUI->m_interlacedSourceFlag)
    {
      READ_UVLC(symbol, "vui_chroma_sample_loc_type");
      pcVUI->m_chromaSampleLocType = symbol;
    }
    else
    {
      READ_UVLC(symbol, "vui_chroma_sample_loc_type_top_field");
      pcVUI->m_chromaSampleLocTypeTopField = symbol;
      READ_UVLC(symbol, "vui_chroma_sample_loc_type_bottom_field");
      pcVUI->m_chromaSampleLocTypeBottomField = symbol;
    }
  }

  int payloadBitsRem = getBitstream()->getNumBitsLeft();
  if (payloadBitsRem) // Corresponds to more_data_in_payload()
  {
    while (payloadBitsRem > 9) // payload_extension_present()
    {
      READ_CODE(1, symbol, "vui_reserved_payload_extension_data");
      payloadBitsRem--;
    }
    int finalBits = getBitstream()->peekBits(payloadBitsRem);
    int numFinalZeroBits = 0;
    int mask = 0xff;
    while (finalBits & (mask >> numFinalZeroBits))
    {
      numFinalZeroBits++;
    }
    while (payloadBitsRem > 9 - numFinalZeroBits) // payload_extension_present()
    {
      READ_CODE(1, symbol, "vui_reserved_payload_extension_data");
      payloadBitsRem--;
    }
    READ_FLAG(symbol, "vui_payload_bit_equal_to_one");
    CHECK(symbol != 1, "vui_payload_bit_equal_to_one not equal to 1");
    payloadBitsRem--;
    while (payloadBitsRem)
    {
      READ_FLAG(symbol, "vui_payload_bit_equal_to_zero");
      CHECK(symbol != 0, "vui_payload_bit_equal_to_zero not equal to 0");
      payloadBitsRem--;
    }
  }
  delete getBitstream();
  setBitstream(bs);
}

void parseNalH266::parseGeneralHrdParameters(vvc::GeneralHrdParams *hrd)
{
  uint32_t symbol;
  READ_CODE(32, symbol, "num_units_in_tick");
  hrd->m_numUnitsInTick = symbol;
  READ_CODE(32, symbol, "time_scale");
  hrd->m_timeScale = symbol;
  READ_FLAG(symbol, "general_nal_hrd_parameters_present_flag");
  hrd->m_generalNalHrdParamsPresentFlag = symbol == 1 ? true : false;
  READ_FLAG(symbol, "general_vcl_hrd_parameters_present_flag");
  hrd->m_generalVclHrdParamsPresentFlag = symbol == 1 ? true : false;
  if (hrd->m_generalNalHrdParamsPresentFlag || hrd->m_generalVclHrdParamsPresentFlag)
  {
    READ_FLAG(symbol, "general_same_pic_timing_in_all_ols_flag");
    hrd->m_generalSamePicTimingInAllOlsFlag = symbol == 1 ? true : false;
    READ_FLAG(symbol, "general_decoding_unit_hrd_params_present_flag");
    hrd->m_generalDecodingUnitHrdParamsPresentFlag = symbol == 1 ? true : false;
    if (hrd->m_generalDecodingUnitHrdParamsPresentFlag)
    {
      READ_CODE(8, symbol, "tick_divisor_minus2");
      hrd->m_tickDivisorMinus2 = symbol;
    }
    READ_CODE(4, symbol, "bit_rate_scale");
    hrd->m_bitRateScale = symbol;
    READ_CODE(4, symbol, "cpb_size_scale");
    hrd->m_cpbSizeScale = symbol;
    if (hrd->m_generalDecodingUnitHrdParamsPresentFlag)
    {
      READ_CODE(4, symbol, "cpb_size_du_scale");
      hrd->m_cpbSizeDuScale = symbol;
    }
    READ_UVLC(symbol, "hrd_cpb_cnt_minus1");
    hrd->m_hrdCpbCntMinus1 = symbol;
    CHECK(symbol > 31, "The value of hrd_cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive");
  }
}

void parseNalH266::parseOlsHrdParameters(vvc::GeneralHrdParams *generalHrd, vvc::OlsHrdParams *olsHrd, uint32_t firstSubLayer, uint32_t maxNumSubLayersMinus1)
{
  uint32_t symbol;

  for (int i = firstSubLayer; i <= (int)maxNumSubLayersMinus1; i++)
  {
    vvc::OlsHrdParams *hrd = &(olsHrd[i]);
    READ_FLAG(symbol, "fixed_pic_rate_general_flag");
    hrd->m_fixedPicRateGeneralFlag = symbol == 1 ? true : false;
    if (!hrd->m_fixedPicRateGeneralFlag)
    {
      READ_FLAG(symbol, "fixed_pic_rate_within_cvs_flag");
      hrd->m_fixedPicRateWithinCvsFlag = symbol == 1 ? true : false;
    }
    else
    {
      hrd->m_fixedPicRateWithinCvsFlag = true;
    }

    hrd->m_lowDelayHrdFlag = false;

    if (hrd->m_fixedPicRateWithinCvsFlag)
    {
      READ_UVLC(symbol, "elemental_duration_in_tc_minus1");
      hrd->m_elementDurationInTcMinus1 = symbol;
    }
    else if ((generalHrd->m_generalNalHrdParamsPresentFlag || generalHrd->m_generalVclHrdParamsPresentFlag) && generalHrd->m_hrdCpbCntMinus1 == 0)
    {
      READ_FLAG(symbol, "low_delay_hrd_flag");
      hrd->m_lowDelayHrdFlag = symbol == 1 ? true : false;
    }

    for (int nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
    {
      if (((nalOrVcl == 0) && (generalHrd->m_generalNalHrdParamsPresentFlag)) || ((nalOrVcl == 1) && (generalHrd->m_generalVclHrdParamsPresentFlag)))
      {
        for (int j = 0; j <= (int)generalHrd->m_hrdCpbCntMinus1; j++)
        {
          READ_UVLC(symbol, "bit_rate_value_minus1");
          hrd->m_duBitRateValueMinus1[j][nalOrVcl] = symbol;
          READ_UVLC(symbol, "cpb_size_value_minus1");
          hrd->m_cpbSizeValueMinus1[j][nalOrVcl] = symbol;
          if (generalHrd->m_generalDecodingUnitHrdParamsPresentFlag)
          {
            READ_UVLC(symbol, "cpb_size_du_value_minus1");
            hrd->m_ducpbSizeValueMinus1[j][nalOrVcl] = symbol;
            READ_UVLC(symbol, "bit_rate_du_value_minus1");
            hrd->m_duBitRateValueMinus1[j][nalOrVcl] = symbol;
          }
          READ_FLAG(symbol, "cbr_flag");
          hrd->m_cbrFlag[j][nalOrVcl] = symbol == 1 ? true : false;
        }
      }
    }
  }
  for (int i = 0; i < (int)firstSubLayer; i++)
  {
    vvc::OlsHrdParams *hrdHighestTLayer = &(olsHrd[maxNumSubLayersMinus1]);
    vvc::OlsHrdParams *hrdTemp = &(olsHrd[i]);
    bool tempFlag = hrdHighestTLayer->m_fixedPicRateGeneralFlag;
    hrdTemp->m_fixedPicRateGeneralFlag = tempFlag;
    tempFlag = hrdHighestTLayer->m_fixedPicRateWithinCvsFlag;
    hrdTemp->m_fixedPicRateWithinCvsFlag = tempFlag;
    uint32_t tempElementDurationInTcMinus1 = hrdHighestTLayer->m_elementDurationInTcMinus1;
    hrdTemp->m_elementDurationInTcMinus1 = tempElementDurationInTcMinus1;
    for (int nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
    {
      if (((nalOrVcl == 0) && (generalHrd->m_generalNalHrdParamsPresentFlag)) || ((nalOrVcl == 1) && (generalHrd->m_generalVclHrdParamsPresentFlag)))
      {
        for (int j = 0; j <= (int)generalHrd->m_hrdCpbCntMinus1; j++)
        {
          uint32_t bitRate = hrdHighestTLayer->m_bitRateValueMinus1[j][nalOrVcl];
          hrdTemp->m_bitRateValueMinus1[j][nalOrVcl] = bitRate;
          uint32_t cpbSize = hrdHighestTLayer->m_cpbSizeValueMinus1[j][nalOrVcl];
          hrdTemp->m_cpbSizeValueMinus1[j][nalOrVcl] = cpbSize;
          if (generalHrd->m_generalDecodingUnitHrdParamsPresentFlag)
          {
            uint32_t bitRateDu = hrdHighestTLayer->m_duBitRateValueMinus1[j][nalOrVcl];
            hrdTemp->m_duBitRateValueMinus1[j][nalOrVcl] = bitRateDu;
            uint32_t cpbSizeDu = hrdHighestTLayer->m_ducpbSizeValueMinus1[j][nalOrVcl];
            hrdTemp->m_ducpbSizeValueMinus1[j][nalOrVcl] = cpbSizeDu;
          }
          bool flag = hrdHighestTLayer->m_cbrFlag[j][nalOrVcl];
          hrdTemp->m_cbrFlag[j][nalOrVcl] = flag;
        }
      }
    }
  }
}

void parseNalH266::dpb_parameters(int maxSubLayersMinus1, bool subLayerInfoFlag, vvc::SPS *pcSPS)
{
  uint32_t code;
  for (int i = (subLayerInfoFlag ? 0 : maxSubLayersMinus1); i <= maxSubLayersMinus1; i++)
  {
    READ_UVLC(code, "dpb_max_dec_pic_buffering_minus1[i]");
    pcSPS->m_maxDecPicBuffering[i] = code + 1;
    READ_UVLC(code, "dpb_max_num_reorder_pics[i]");
    pcSPS->m_maxNumReorderPics[i] = code;
    CHECK(pcSPS->m_maxNumReorderPics[i] >= (int)pcSPS->m_maxDecPicBuffering[i], "The value of dpb_max_num_reorder_pics[ i ] shall be in the range of 0 to dpb_max_dec_pic_buffering_minus1[ i ], inclusive");
    READ_UVLC(code, "dpb_max_latency_increase_plus1[i]");
    pcSPS->m_maxLatencyIncreasePlus1[i] = code;
  }

  if (!subLayerInfoFlag)
  {
    for (int i = 0; i < maxSubLayersMinus1; ++i)
    {
      pcSPS->m_maxDecPicBuffering[i] = pcSPS->m_maxDecPicBuffering[maxSubLayersMinus1];
      pcSPS->m_maxNumReorderPics[i] = pcSPS->m_maxNumReorderPics[maxSubLayersMinus1];
      pcSPS->m_maxLatencyIncreasePlus1[i] = pcSPS->m_maxLatencyIncreasePlus1[maxSubLayersMinus1];
    }
  }
}

void parseNalH266::vps_parse(unsigned char *nal_bitstream, vvc::VPS *pcVPS, int curLen, parsingLevel level)
{

}

void parseNalH266::sps_parse(unsigned char *nal_bitstream, vvc::SPS *pcSPS, int curLen, parsingLevel level)
{
  uint32_t uiCode;

  for (int i = 0; i < curLen; i++)
    m_bits->m_fifo.push_back(nal_bitstream[i]);
  setBitstream(m_bits);

  READ_CODE(4, uiCode, "sps_seq_parameter_set_id");
  pcSPS->m_SPSId = uiCode;
  READ_CODE(4, uiCode, "sps_video_parameter_set_id");
  pcSPS->m_VPSId = uiCode;

  if (level == parsingLevel::PARSING_PARAM_ID)
  {
    return;
  }

  READ_CODE(3, uiCode, "sps_max_sub_layers_minus1");
  pcSPS->m_uiMaxTLayers = uiCode + 1;
  CHECK(uiCode > 6, "Invalid maximum number of T-layer signalled");
  READ_CODE(2, uiCode, "sps_chroma_format_idc");
  pcSPS->m_chromaFormatIdc = vvc::ChromaFormat(uiCode);

  READ_CODE(2, uiCode, "sps_log2_ctu_size_minus5");
  pcSPS->m_CTUSize = 1 << (uiCode + 5);
  CHECK(uiCode > 2, "sps_log2_ctu_size_minus5 must be less than or equal to 2");
  unsigned ctbLog2SizeY = uiCode + 5;
  pcSPS->m_uiMaxCUWidth = pcSPS->m_CTUSize;
  pcSPS->m_uiMaxCUHeight = pcSPS->m_CTUSize;
  READ_FLAG(uiCode, "sps_ptl_dpb_hrd_params_present_flag");
  pcSPS->m_ptlDpbHrdParamsPresentFlag = uiCode;

  if (!pcSPS->m_VPSId)
  {
    CHECK(!pcSPS->m_ptlDpbHrdParamsPresentFlag, "When sps_video_parameter_set_id is equal to 0, the value of sps_ptl_dpb_hrd_params_present_flag shall be equal to 1");
  }

  if (pcSPS->m_ptlDpbHrdParamsPresentFlag)
  {
    parseProfileTierLevel(&pcSPS->m_profileTierLevel, true, (int)pcSPS->m_uiMaxTLayers - 1);
  }

  READ_FLAG(uiCode, "sps_gdr_enabled_flag");
  pcSPS->m_GDREnabledFlag = uiCode;

  if (pcSPS->m_profileTierLevel.m_constraintInfo.m_noGdrConstraintFlag)
  {
    CHECK(uiCode != 0, "When gci_no_gdr_constraint_flag equal to 1 , the value of sps_gdr_enabled_flag shall be equal to 0");
  }

  READ_FLAG(uiCode, "sps_ref_pic_resampling_enabled_flag");
  pcSPS->m_rprEnabledFlag = uiCode;
  if (pcSPS->m_profileTierLevel.m_constraintInfo.m_noRprConstraintFlag)
  {
    CHECK(uiCode != 0, "When gci_no_ref_pic_resampling_constraint_flag is equal to 1, sps_ref_pic_resampling_enabled_flag shall be equal to 0");
  }
  if (uiCode)
  {
    READ_FLAG(uiCode, "sps_res_change_in_clvs_allowed_flag");
    pcSPS->m_resChangeInClvsEnabledFlag = uiCode;
  }
  else
  {
    pcSPS->m_resChangeInClvsEnabledFlag = 0;
  }

  if (pcSPS->m_profileTierLevel.m_constraintInfo.m_noResChangeInClvsConstraintFlag)
  {
    CHECK(uiCode != 0, "When no_res_change_in_clvs_constraint_flag is equal to 1, sps_res_change_in_clvs_allowed_flag shall be equal to 0");
  }

  READ_UVLC(uiCode, "sps_pic_width_max_in_luma_samples");
  pcSPS->m_maxWidthInLumaSamples = uiCode;
  READ_UVLC(uiCode, "sps_pic_height_max_in_luma_samples");
  pcSPS->m_maxHeightInLumaSamples = uiCode;
  READ_FLAG(uiCode, "sps_conformance_window_flag");
  if (uiCode != 0)
  {
    vvc::Window &conf = pcSPS->m_conformanceWindow;
    READ_UVLC(uiCode, "sps_conf_win_left_offset");
    conf.m_winLeftOffset = uiCode;
    READ_UVLC(uiCode, "sps_conf_win_right_offset");
    conf.m_winRightOffset = uiCode;
    READ_UVLC(uiCode, "sps_conf_win_top_offset");
    conf.m_winTopOffset = uiCode;
    READ_UVLC(uiCode, "sps_conf_win_bottom_offset");
    conf.m_winBottomOffset = uiCode;
  }

  READ_FLAG(uiCode, "sps_subpic_info_present_flag");
  pcSPS->m_subPicInfoPresentFlag = uiCode;
  if (pcSPS->m_profileTierLevel.m_constraintInfo.m_noSubpicInfoConstraintFlag)
  {
    CHECK(uiCode != 0, "When gci_no_subpic_info_constraint_flag is equal to 1, the value of sps_subpic_info_present_flag shall be equal to 0");
  }

  if (pcSPS->m_subPicInfoPresentFlag)
  {
    const int maxPicWidthInCtus = ((pcSPS->m_maxWidthInLumaSamples - 1) / pcSPS->m_CTUSize) + 1;
    const int maxPicHeightInCtus = ((pcSPS->m_maxHeightInLumaSamples - 1) / pcSPS->m_CTUSize) + 1;

    READ_UVLC(uiCode, "sps_num_subpics_minus1");
    pcSPS->m_numSubPics = uiCode + 1;
    CHECK((int)uiCode > maxPicWidthInCtus * maxPicHeightInCtus - 1, "Invalid sps_num_subpics_minus1 value");
    if (pcSPS->m_numSubPics == 1)
    {
      pcSPS->m_subPicCtuTopLeftX.insert(pcSPS->m_subPicCtuTopLeftX.begin(), 0);
      pcSPS->m_subPicCtuTopLeftY.insert(pcSPS->m_subPicCtuTopLeftY.begin(), 0);
      pcSPS->m_subPicWidth.insert(pcSPS->m_subPicWidth.begin(), maxPicWidthInCtus);
      pcSPS->m_subPicHeight.insert(pcSPS->m_subPicHeight.begin(), maxPicHeightInCtus);

      pcSPS->m_independentSubPicsFlag = 1;
      pcSPS->m_subPicSameSizeFlag = 0;

      pcSPS->m_subPicTreatedAsPicFlag.insert(pcSPS->m_subPicTreatedAsPicFlag.begin(), 1);
      pcSPS->m_loopFilterAcrossSubpicEnabledFlag.insert(pcSPS->m_loopFilterAcrossSubpicEnabledFlag.begin(), 0);
    }
    else
    {
      READ_FLAG(uiCode, "sps_independent_subpics_flag");
      pcSPS->m_independentSubPicsFlag = uiCode != 0;
      READ_FLAG(uiCode, "sps_subpic_same_size_flag");
      pcSPS->m_subPicSameSizeFlag = uiCode;
      uint32_t tmpWidthVal = maxPicWidthInCtus;
      uint32_t tmpHeightVal = maxPicHeightInCtus;
      uint32_t numSubpicCols = 1;
      for (int picIdx = 0; picIdx < (int)pcSPS->m_numSubPics; picIdx++)
      {
        if (!pcSPS->m_subPicSameSizeFlag || picIdx == 0)
        {
          if ((picIdx > 0) && (pcSPS->m_maxWidthInLumaSamples > pcSPS->m_CTUSize))
          {
            READ_CODE(ceilLog2(tmpWidthVal), uiCode, "sps_subpic_ctu_top_left_x[ i ]");
            pcSPS->m_subPicCtuTopLeftX.insert(pcSPS->m_subPicCtuTopLeftX.begin() + picIdx, uiCode);
          }
          else
          {
            pcSPS->m_subPicCtuTopLeftY.insert(pcSPS->m_subPicCtuTopLeftY.begin() + picIdx, 0);
          }
          if ((picIdx > 0) && (pcSPS->m_maxHeightInLumaSamples > pcSPS->m_CTUSize))
          {

            READ_CODE(ceilLog2(tmpHeightVal), uiCode, "sps_subpic_ctu_top_left_y[ i ]");
            pcSPS->m_subPicCtuTopLeftX.insert(pcSPS->m_subPicCtuTopLeftX.begin() + picIdx, uiCode);
          }
          else
          {
            pcSPS->m_subPicCtuTopLeftY.insert(pcSPS->m_subPicCtuTopLeftY.begin() + picIdx, 0);
          }
          if (picIdx < (int)pcSPS->m_numSubPics - 1 && pcSPS->m_maxWidthInLumaSamples > pcSPS->m_CTUSize)
          {
            READ_CODE(ceilLog2(tmpWidthVal), uiCode, "sps_subpic_width_minus1[ i ]");
            pcSPS->m_subPicWidth.insert(pcSPS->m_subPicWidth.begin() + picIdx, uiCode + 1);
          }
          else
          {
            pcSPS->m_subPicWidth.insert(pcSPS->m_subPicWidth.begin() + picIdx, tmpWidthVal - pcSPS->m_subPicCtuTopLeftX[picIdx]);
          }
          if (picIdx < (int)pcSPS->m_numSubPics - 1 && pcSPS->m_maxHeightInLumaSamples > pcSPS->m_CTUSize)
          {

            READ_CODE(ceilLog2(tmpHeightVal), uiCode, "sps_subpic_height_minus1[ i ]");
            pcSPS->m_subPicHeight.insert(pcSPS->m_subPicHeight.begin() + picIdx, uiCode + 1);
          }
          else
          {
            pcSPS->m_subPicHeight.insert(pcSPS->m_subPicHeight.begin() + picIdx, tmpHeightVal - pcSPS->m_subPicCtuTopLeftY[picIdx]);
          }
          if (pcSPS->m_subPicSameSizeFlag)
          {
            numSubpicCols = tmpWidthVal / pcSPS->m_subPicWidth[0];
            CHECK(!(tmpWidthVal % pcSPS->m_subPicWidth[0] == 0), "sps_subpic_width_minus1[0] is invalid.");
            CHECK(!(tmpHeightVal % pcSPS->m_subPicHeight[0] == 0), "sps_subpic_height_minus1[0] is invalid.");
            CHECK(!(numSubpicCols * (tmpHeightVal / pcSPS->m_subPicHeight[0]) == pcSPS->m_numSubPics), "when sps_subpic_same_size_flag is equal to, sps_num_subpics_minus1 is invalid");
          }
        }
        else
        {
          pcSPS->m_subPicCtuTopLeftX.insert(pcSPS->m_subPicCtuTopLeftX.begin() + picIdx, (picIdx % numSubpicCols) * pcSPS->m_subPicWidth[0]);
          pcSPS->m_subPicCtuTopLeftY.insert(pcSPS->m_subPicCtuTopLeftY.begin() + picIdx, (picIdx / numSubpicCols) * pcSPS->m_subPicHeight[0]);
          pcSPS->m_subPicWidth.insert(pcSPS->m_subPicWidth.begin() + picIdx, pcSPS->m_subPicWidth[0]);
          pcSPS->m_subPicHeight.insert(pcSPS->m_subPicHeight.begin() + picIdx, pcSPS->m_subPicHeight[0]);
        }
        if (!pcSPS->m_independentSubPicsFlag)
        {
          READ_FLAG(uiCode, "sps_subpic_treated_as_pic_flag[ i ]");
          pcSPS->m_subPicTreatedAsPicFlag.insert(pcSPS->m_subPicTreatedAsPicFlag.begin() + picIdx, uiCode);
          READ_FLAG(uiCode, "sps_loop_filter_across_subpic_enabled_flag[ i ]");
          pcSPS->m_loopFilterAcrossSubpicEnabledFlag.insert(pcSPS->m_loopFilterAcrossSubpicEnabledFlag.begin() + picIdx, uiCode);
        }
        else
        {
          pcSPS->m_subPicTreatedAsPicFlag.insert(pcSPS->m_subPicTreatedAsPicFlag.begin() + picIdx, 1);
          pcSPS->m_loopFilterAcrossSubpicEnabledFlag.insert(pcSPS->m_loopFilterAcrossSubpicEnabledFlag.begin() + picIdx, 0);
        }
      }
    }

    READ_UVLC(uiCode, "sps_subpic_id_len_minus1");
    pcSPS->m_subPicIdLen = uiCode + 1;
    CHECK(uiCode > 15, "Invalid sps_subpic_id_len_minus1 value");
    CHECK((1 << (uiCode + 1)) < (int)pcSPS->m_numSubPics, "Invalid sps_subpic_id_len_minus1 value");
    READ_FLAG(uiCode, "sps_subpic_id_mapping_explicitly_signalled_flag");
    pcSPS->m_subPicIdMappingExplicitlySignalledFlag = uiCode != 0;
    if (pcSPS->m_subPicIdMappingExplicitlySignalledFlag)
    {
      READ_FLAG(uiCode, "sps_subpic_id_mapping_present_flag");
      pcSPS->m_subPicIdMappingPresentFlag = uiCode != 0;
      if (pcSPS->m_subPicIdMappingPresentFlag)
      {
        for (int picIdx = 0; picIdx < (int)pcSPS->m_numSubPics; picIdx++)
        {
          READ_CODE(pcSPS->m_subPicIdLen, uiCode, "sps_subpic_id[i]");
          pcSPS->m_subPicId.insert(pcSPS->m_subPicId.begin() + picIdx, uiCode);
        }
      }
    }
  }
  else
  {
    pcSPS->m_subPicIdMappingExplicitlySignalledFlag = 0;
    pcSPS->m_numSubPics = 1;
    pcSPS->m_subPicCtuTopLeftX.insert(pcSPS->m_subPicCtuTopLeftX.begin(), 0);
    pcSPS->m_subPicCtuTopLeftY.insert(pcSPS->m_subPicCtuTopLeftY.begin(), 0);
    pcSPS->m_subPicWidth.insert(pcSPS->m_subPicWidth.begin(), (pcSPS->m_maxWidthInLumaSamples + pcSPS->m_CTUSize - 1) >> floorLog2(pcSPS->m_CTUSize));
    pcSPS->m_subPicHeight.insert(pcSPS->m_subPicHeight.begin(), (pcSPS->m_maxHeightInLumaSamples + pcSPS->m_CTUSize - 1) >> floorLog2(pcSPS->m_CTUSize));
  }

  if (!pcSPS->m_subPicIdMappingExplicitlySignalledFlag || !pcSPS->m_subPicIdMappingPresentFlag)
  {
    for (int picIdx = 0; picIdx < (int)pcSPS->m_numSubPics; picIdx++)
    {
      pcSPS->m_subPicId.insert(pcSPS->m_subPicId.begin() + picIdx, picIdx);
    }
  }

  READ_UVLC(uiCode, "sps_bitdepth_minus8");
  CHECK(uiCode > 8, "Invalid bit depth signalled");
  const vvc::Profile::Name profile = pcSPS->m_profileTierLevel.m_profileIdc;
  if (profile != vvc::Profile::NONE)
  {
    CHECK(uiCode + 8 > vvc::ProfileFeatures::getProfileFeatures(profile)->maxBitDepth, "sps_bitdepth_minus8 exceeds range supported by signalled profile");
  }
  pcSPS->m_bitDepths.recon[vvc::CHANNEL_TYPE_LUMA] = 8 + uiCode;
  pcSPS->m_bitDepths.recon[vvc::CHANNEL_TYPE_CHROMA] = 8 + uiCode;
  pcSPS->m_qpBDOffset[vvc::CHANNEL_TYPE_LUMA] = (int)(6 * uiCode);
  pcSPS->m_qpBDOffset[vvc::CHANNEL_TYPE_CHROMA] = (int)(6 * uiCode);

  READ_FLAG(uiCode, "sps_entropy_coding_sync_enabled_flag");
  pcSPS->m_entropyCodingSyncEnabledFlag = uiCode == 1;
  READ_FLAG(uiCode, "sps_entry_point_offsets_present_flag");
  pcSPS->m_entryPointPresentFlag = uiCode == 1;
  READ_CODE(4, uiCode, "sps_log2_max_pic_order_cnt_lsb_minus4");
  pcSPS->m_uiBitsForPOC = 4 + uiCode;
  CHECK(uiCode > 12, "sps_log2_max_pic_order_cnt_lsb_minus4 shall be in the range of 0 to 12");

  READ_FLAG(uiCode, "sps_poc_msb_cycle_flag");
  pcSPS->m_pocMsbCycleFlag = uiCode ? true : false;
  if (pcSPS->m_pocMsbCycleFlag)
  {
    READ_UVLC(uiCode, "sps_poc_msb_cycle_len_minus1");
    pcSPS->m_pocMsbCycleLen = 1 + uiCode;
    CHECK(uiCode > (32 - (pcSPS->m_uiBitsForPOC - 4) - 5), "The value of sps_poc_msb_cycle_len_minus1 shall be in the range of 0 to 32 - sps_log2_max_pic_order_cnt_lsb_minus4 - 5, inclusive");
  }

  // extra bits are for future extensions, we will read, but ignore them,
  // unless a meaning is specified in the spec
  READ_CODE(2, uiCode, "sps_num_extra_ph_bytes");
  pcSPS->m_numExtraPHBytes = uiCode;
  int numExtraPhBytes = uiCode;
  std::vector<bool> extraPhBitPresentFlags;
  extraPhBitPresentFlags.resize(8 * numExtraPhBytes);
  for (int i = 0; i < 8 * numExtraPhBytes; i++)
  {
    READ_FLAG(uiCode, "sps_extra_ph_bit_present_flag[ i ]");
    extraPhBitPresentFlags[i] = uiCode;
  }
  pcSPS->m_extraPHBitPresentFlag = extraPhBitPresentFlags;
  READ_CODE(2, uiCode, "sps_num_extra_sh_bytes");
  pcSPS->m_numExtraSHBytes = uiCode;
  int numExtraShBytes = uiCode;
  std::vector<bool> extraShBitPresentFlags;
  extraShBitPresentFlags.resize(8 * numExtraShBytes);
  for (int i = 0; i < 8 * numExtraShBytes; i++)
  {
    READ_FLAG(uiCode, "sps_extra_sh_bit_present_flag[ i ]");
    extraShBitPresentFlags[i] = uiCode;
  }
  pcSPS->m_extraSHBitPresentFlag = extraShBitPresentFlags;

  if (pcSPS->m_ptlDpbHrdParamsPresentFlag)
  {
    if (pcSPS->m_uiMaxTLayers - 1 > 0)
    {
      READ_FLAG(uiCode, "sps_sublayer_dpb_params_flag");
      pcSPS->m_SubLayerDpbParamsFlag = uiCode ? true : false;
    }
    dpb_parameters(pcSPS->m_uiMaxTLayers - 1, pcSPS->m_SubLayerDpbParamsFlag, pcSPS);
  }
  unsigned minQT[3] = {0, 0, 0};
  unsigned maxBTD[3] = {0, 0, 0};

  unsigned maxBTSize[3] = {0, 0, 0};
  unsigned maxTTSize[3] = {0, 0, 0};
  READ_UVLC(uiCode, "sps_log2_min_luma_coding_block_size_minus2");
  int log2MinCUSize = uiCode + 2;
  pcSPS->m_log2MinCodingBlockSize = log2MinCUSize;
  CHECK(uiCode > ctbLog2SizeY - 2, "Invalid sps_log2_min_luma_coding_block_size_minus2 signalled");

  CHECK(log2MinCUSize > std::min(6, (int)(ctbLog2SizeY)), "sps_log2_min_luma_coding_block_size_minus2 shall be in the range of 0 to min (4, log2_ctu_size - 2)");
  const int minCuSize = 1 << pcSPS->m_log2MinCodingBlockSize;
  CHECK((pcSPS->m_maxWidthInLumaSamples % (std::max(8, minCuSize))) != 0, "Coded frame width must be a multiple of Max(8, the minimum unit size)");
  CHECK((pcSPS->m_maxHeightInLumaSamples % (std::max(8, minCuSize))) != 0, "Coded frame height must be a multiple of Max(8, the minimum unit size)");

  READ_FLAG(uiCode, "sps_partition_constraints_override_enabled_flag");
  pcSPS->m_partitionOverrideEnalbed = uiCode;
  READ_UVLC(uiCode, "sps_log2_diff_min_qt_min_cb_intra_slice_luma");
  unsigned minQtLog2SizeIntraY = uiCode + pcSPS->m_log2MinCodingBlockSize;
  minQT[0] = 1 << minQtLog2SizeIntraY;
  CHECK(minQT[0] > 64, "The value of sps_log2_diff_min_qt_min_cb_intra_slice_luma shall be in the range of 0 to min(6,CtbLog2SizeY) - MinCbLog2Size");
  CHECK(minQT[0] > (unsigned int)(1 << ctbLog2SizeY), "The value of sps_log2_diff_min_qt_min_cb_intra_slice_luma shall be in the range of 0 to min(6,CtbLog2SizeY) - MinCbLog2Size");
  READ_UVLC(uiCode, "sps_max_mtt_hierarchy_depth_intra_slice_luma");
  maxBTD[0] = uiCode;
  CHECK(uiCode > 2 * (ctbLog2SizeY - log2MinCUSize), "sps_max_mtt_hierarchy_depth_intra_slice_luma shall be in the range 0 to 2*(ctbLog2SizeY - log2MinCUSize)");

  maxTTSize[0] = maxBTSize[0] = minQT[0];
  if (maxBTD[0] != 0)
  {
    READ_UVLC(uiCode, "sps_log2_diff_max_bt_min_qt_intra_slice_luma");
    maxBTSize[0] <<= uiCode;
    CHECK(uiCode > ctbLog2SizeY - minQtLog2SizeIntraY, "The value of sps_log2_diff_max_bt_min_qt_intra_slice_luma shall be in the range of 0 to CtbLog2SizeY - MinQtLog2SizeIntraY");
    READ_UVLC(uiCode, "sps_log2_diff_max_tt_min_qt_intra_slice_luma");
    maxTTSize[0] <<= uiCode;
    CHECK(uiCode > ctbLog2SizeY - minQtLog2SizeIntraY, "The value of sps_log2_diff_max_tt_min_qt_intra_slice_luma shall be in the range of 0 to CtbLog2SizeY - MinQtLog2SizeIntraY");
    CHECK(maxTTSize[0] > 64, "The value of sps_log2_diff_max_tt_min_qt_intra_slice_luma shall be in the range of 0 to min(6,CtbLog2SizeY) - MinQtLog2SizeIntraY");
  }
  if (pcSPS->m_chromaFormatIdc != vvc::CHROMA_400)
  {
    READ_FLAG(uiCode, "sps_qtbtt_dual_tree_intra_flag");
    pcSPS->m_dualITree = uiCode;
  }
  else
  {
    pcSPS->m_dualITree = 0;
  }
  if (pcSPS->m_dualITree)
  {
    READ_UVLC(uiCode, "sps_log2_diff_min_qt_min_cb_intra_slice_chroma");
    minQT[2] = 1 << (uiCode + pcSPS->m_log2MinCodingBlockSize);
    READ_UVLC(uiCode, "sps_max_mtt_hierarchy_depth_intra_slice_chroma");
    maxBTD[2] = uiCode;
    CHECK(uiCode > 2 * (ctbLog2SizeY - log2MinCUSize), "sps_max_mtt_hierarchy_depth_intra_slice_chroma shall be in the range 0 to 2*(ctbLog2SizeY - log2MinCUSize)");
    maxTTSize[2] = maxBTSize[2] = minQT[2];
    if (maxBTD[2] != 0)
    {
      READ_UVLC(uiCode, "sps_log2_diff_max_bt_min_qt_intra_slice_chroma");
      maxBTSize[2] <<= uiCode;
      READ_UVLC(uiCode, "sps_log2_diff_max_tt_min_qt_intra_slice_chroma");
      maxTTSize[2] <<= uiCode;
      CHECK(maxTTSize[2] > 64, "The value of sps_log2_diff_max_tt_min_qt_intra_slice_chroma shall be in the range of 0 to min(6,CtbLog2SizeY) - MinQtLog2SizeIntraChroma");
      CHECK(maxBTSize[2] > 64, "The value of sps_log2_diff_max_bt_min_qt_intra_slice_chroma shall be in the range of 0 to min(6,CtbLog2SizeY) - MinQtLog2SizeIntraChroma");
    }
  }
  READ_UVLC(uiCode, "sps_log2_diff_min_qt_min_cb_inter_slice");
  unsigned minQtLog2SizeInterY = uiCode + pcSPS->m_log2MinCodingBlockSize;
  minQT[1] = 1 << minQtLog2SizeInterY;
  READ_UVLC(uiCode, "sps_max_mtt_hierarchy_depth_inter_slice");
  maxBTD[1] = uiCode;
  CHECK(uiCode > 2 * (ctbLog2SizeY - log2MinCUSize), "sps_max_mtt_hierarchy_depth_inter_slice shall be in the range 0 to 2*(ctbLog2SizeY - log2MinCUSize)");
  maxTTSize[1] = maxBTSize[1] = minQT[1];
  if (maxBTD[1] != 0)
  {
    READ_UVLC(uiCode, "sps_log2_diff_max_bt_min_qt_inter_slice");
    maxBTSize[1] <<= uiCode;
    CHECK(uiCode > ctbLog2SizeY - minQtLog2SizeInterY, "The value of sps_log2_diff_max_bt_min_qt_inter_slice shall be in the range of 0 to CtbLog2SizeY - MinQtLog2SizeInterY");
    READ_UVLC(uiCode, "sps_log2_diff_max_tt_min_qt_inter_slice");
    maxTTSize[1] <<= uiCode;
    CHECK(uiCode > ctbLog2SizeY - minQtLog2SizeInterY, "The value of sps_log2_diff_max_tt_min_qt_inter_slice shall be in the range of 0 to CtbLog2SizeY - MinQtLog2SizeInterY");
    CHECK(maxTTSize[1] > 64, "The value of sps_log2_diff_max_tt_min_qt_inter_slice shall be in the range of 0 to min(6,CtbLog2SizeY) - MinQtLog2SizeInterY");
  }

  pcSPS->m_minQT[0] = minQT[0];
  pcSPS->m_minQT[1] = minQT[1];
  pcSPS->m_minQT[2] = minQT[2];
  pcSPS->m_maxMTTHierarchyDepth[1] = maxBTD[1];
  pcSPS->m_maxMTTHierarchyDepth[0] = maxBTD[0];
  pcSPS->m_maxMTTHierarchyDepth[2] = maxBTD[2];
  pcSPS->m_maxBTSize[1] = maxBTSize[1];
  pcSPS->m_maxBTSize[0] = maxBTSize[0];
  pcSPS->m_maxBTSize[2] = maxBTSize[2];
  pcSPS->m_maxTTSize[1] = maxTTSize[1];
  pcSPS->m_maxTTSize[0] = maxTTSize[0];
  pcSPS->m_maxTTSize[2] = maxTTSize[2];

  if (pcSPS->m_CTUSize > 32)
  {
    READ_FLAG(uiCode, "sps_max_luma_transform_size_64_flag");
    pcSPS->m_log2MaxTbSize = (uiCode ? 1 : 0) + 5;
  }
  else
  {
    pcSPS->m_log2MaxTbSize = 5;
  }

  READ_FLAG(uiCode, "sps_transform_skip_enabled_flag");
  pcSPS->m_transformSkipEnabledFlag = uiCode ? true : false;
  if (pcSPS->m_transformSkipEnabledFlag)
  {
    READ_UVLC(uiCode, "sps_log2_transform_skip_max_size_minus2");
    pcSPS->m_log2MaxTransformSkipBlockSize = uiCode + 2;
    READ_FLAG(uiCode, "sps_bdpcm_enabled_flag");
    pcSPS->m_BDPCMEnabledFlag = uiCode ? true : false;
  }
  READ_FLAG(uiCode, "sps_mts_enabled_flag");
  pcSPS->m_mtsEnabled = uiCode != 0;
  if (pcSPS->m_mtsEnabled)
  {
    READ_FLAG(uiCode, "sps_explicit_mts_intra_enabled_flag");
    pcSPS->m_explicitMtsIntra = uiCode != 0;
    READ_FLAG(uiCode, "sps_explicit_mts_inter_enabled_flag");
    pcSPS->m_explicitMtsIntra = uiCode != 0;
  }
  READ_FLAG(uiCode, "sps_lfnst_enabled_flag");
  pcSPS->m_LFNST = uiCode != 0;

  if (pcSPS->m_chromaFormatIdc != vvc::CHROMA_400)
  {
    READ_FLAG(uiCode, "sps_joint_cbcr_enabled_flag");
    pcSPS->m_JointCbCrEnabledFlag = uiCode ? true : false;
    vvc::ChromaQpMappingTableParams chromaQpMappingTableParams;
    READ_FLAG(uiCode, "sps_same_qp_table_for_chroma_flag");
    chromaQpMappingTableParams.setSameCQPTableForAllChromaFlag(uiCode);
    int numQpTables = chromaQpMappingTableParams.getSameCQPTableForAllChromaFlag() ? 1 : (pcSPS->m_JointCbCrEnabledFlag ? 3 : 2);
    chromaQpMappingTableParams.setNumQpTables(numQpTables);
    for (int i = 0; i < numQpTables; i++)
    {
      int32_t qpTableStart = 0;
      READ_SVLC(qpTableStart, "sps_qp_table_starts_minus26");
      chromaQpMappingTableParams.setQpTableStartMinus26(i, qpTableStart);
      CHECK(qpTableStart < -26 - pcSPS->m_qpBDOffset[vvc::CHANNEL_TYPE_LUMA] || qpTableStart > 36,
            "The value of sps_qp_table_start_minus26[ i ] shall be in the range of -26 - QpBdOffset to 36 inclusive");
      READ_UVLC(uiCode, "sps_num_points_in_qp_table_minus1");
      chromaQpMappingTableParams.setNumPtsInCQPTableMinus1(i, uiCode);
      CHECK((int)uiCode > 36 - qpTableStart, "The value of sps_num_points_in_qp_table_minus1[ i ] shall be in the range of "
                                             "0 to 36 - sps_qp_table_start_minus26[ i ], inclusive");
      std::vector<int> deltaQpInValMinus1(chromaQpMappingTableParams.getNumPtsInCQPTableMinus1(i) + 1);
      std::vector<int> deltaQpOutVal(chromaQpMappingTableParams.getNumPtsInCQPTableMinus1(i) + 1);
      for (int j = 0; j <= chromaQpMappingTableParams.getNumPtsInCQPTableMinus1(i); j++)
      {
        READ_UVLC(uiCode, "sps_delta_qp_in_val_minus1");
        deltaQpInValMinus1[j] = uiCode;
        READ_UVLC(uiCode, "sps_delta_qp_diff_val");
        deltaQpOutVal[j] = uiCode ^ deltaQpInValMinus1[j];
      }
      chromaQpMappingTableParams.setDeltaQpInValMinus1(i, deltaQpInValMinus1);
      chromaQpMappingTableParams.setDeltaQpOutVal(i, deltaQpOutVal);
    }
    pcSPS->setChromaQpMappingTableFromParams(chromaQpMappingTableParams, pcSPS->m_qpBDOffset[vvc::CHANNEL_TYPE_CHROMA]);
    pcSPS->derivedChromaQPMappingTables();
  }

  READ_FLAG(uiCode, "sps_sao_enabled_flag");
  pcSPS->m_saoEnabledFlag = uiCode ? true : false;
  READ_FLAG(uiCode, "sps_alf_enabled_flag");
  pcSPS->m_alfEnabledFlag = uiCode ? true : false;
  if (pcSPS->m_alfEnabledFlag && pcSPS->m_chromaFormatIdc != vvc::CHROMA_400)
  {
    READ_FLAG(uiCode, "sps_ccalf_enabled_flag");
    pcSPS->m_ccalfEnabledFlag = uiCode ? true : false;
  }
  else
  {
    pcSPS->m_ccalfEnabledFlag = false;
  }

  READ_FLAG(uiCode, "sps_lmcs_enable_flag");
  pcSPS->m_lmcsEnabled = uiCode == 1;

  READ_FLAG(uiCode, "sps_weighted_pred_flag");
  pcSPS->m_useWeightPred = uiCode ? true : false;
  READ_FLAG(uiCode, "sps_weighted_bipred_flag");
  pcSPS->m_useWeightedBiPred = uiCode ? true : false;

  READ_FLAG(uiCode, "sps_long_term_ref_pics_flag");
  pcSPS->m_bLongTermRefsPresent = uiCode;
  if (pcSPS->m_VPSId > 0)
  {
    READ_FLAG(uiCode, "sps_inter_layer_prediction_enabled_flag");
    pcSPS->m_interLayerPresentFlag = uiCode;
  }
  else
  {
    pcSPS->m_interLayerPresentFlag = 0;
  }
  READ_FLAG(uiCode, "sps_idr_rpl_present_flag");
  pcSPS->m_idrRefParamList = (bool)uiCode;
  if (pcSPS->m_profileTierLevel.m_constraintInfo.m_noIdrRplConstraintFlag)
  {
    CHECK(uiCode != 0, "When gci_no_idr_rpl_constraint_flag equal to 1 , the value of sps_idr_rpl_present_flag shall be equal to 0");
  }

  READ_FLAG(uiCode, "sps_rpl1_same_as_rpl0_flag");
  pcSPS->m_rpl1CopyFromRpl0Flag = uiCode;

  // Read candidate for List0
  READ_UVLC(uiCode, "sps_num_ref_pic_lists[0]");
  uint32_t numberOfRPL = uiCode;
  pcSPS->createRPLList0(numberOfRPL);
  vvc::RPLList *rplList = &pcSPS->m_RPLList0;
  vvc::ReferencePictureList *rpl;
  for (uint32_t ii = 0; ii < numberOfRPL; ii++)
  {
    rpl = rplList->getReferencePictureList(ii);
    parseRefPicList(pcSPS, rpl, ii);
  }

  // Read candidate for List1
  if (!pcSPS->m_rpl1CopyFromRpl0Flag)
  {
    READ_UVLC(uiCode, "sps_num_ref_pic_lists[1]");
    numberOfRPL = uiCode;
    pcSPS->createRPLList1(numberOfRPL);
    rplList = &pcSPS->m_RPLList1;
    for (uint32_t ii = 0; ii < numberOfRPL; ii++)
    {
      rpl = rplList->getReferencePictureList(ii);
      parseRefPicList(pcSPS, rpl, ii);
    }
  }
  else
  {
    numberOfRPL = pcSPS->m_numRPL0;
    pcSPS->createRPLList1(numberOfRPL);
    vvc::RPLList *rplListSource = &pcSPS->m_RPLList0;
    vvc::RPLList *rplListDest = &pcSPS->m_RPLList1;
    for (uint32_t ii = 0; ii < numberOfRPL; ii++)
    {
      copyRefPicList(pcSPS, rplListSource->getReferencePictureList(ii), rplListDest->getReferencePictureList(ii));
    }
  }

  READ_FLAG(uiCode, "sps_ref_wraparound_enabled_flag");
  pcSPS->m_wrapAroundEnabledFlag = uiCode ? true : false;

  if (pcSPS->m_wrapAroundEnabledFlag)
  {
    for (int i = 0; i < (int)pcSPS->m_numSubPics; i++)
    {
      CHECK(pcSPS->m_subPicTreatedAsPicFlag[i] && (pcSPS->m_subPicWidth[i] != (pcSPS->m_maxWidthInLumaSamples + pcSPS->m_CTUSize - 1) / pcSPS->m_CTUSize), "sps_ref_wraparound_enabled_flag cannot be equal to 1 when there is at least one subpicture with SubPicTreatedAsPicFlag equal to 1 and the subpicture's width is not equal to picture's width");
    }
  }

  READ_FLAG(uiCode, "sps_temporal_mvp_enabled_flag");
  pcSPS->m_SPSTemporalMVPEnabledFlag = uiCode;

  if (pcSPS->m_SPSTemporalMVPEnabledFlag)
  {
    READ_FLAG(uiCode, "sps_sbtmvp_enabled_flag");
    pcSPS->m_sbtmvpEnabledFlag = uiCode != 0;
  }
  else
  {
    pcSPS->m_sbtmvpEnabledFlag = false;
  }

  READ_FLAG(uiCode, "sps_amvr_enabled_flag");
  pcSPS->m_affineAmvrEnabledFlag = uiCode != 0;

  READ_FLAG(uiCode, "sps_bdof_enabled_flag");
  pcSPS->m_bdofEnabledFlag = uiCode != 0;
  if (pcSPS->m_bdofEnabledFlag)
  {
    READ_FLAG(uiCode, "sps_bdof_control_present_in_ph_flag");
    pcSPS->m_BdofControlPresentInPhFlag = uiCode != 0;
  }
  else
  {
    pcSPS->m_BdofControlPresentInPhFlag = false;
  }
  READ_FLAG(uiCode, "sps_smvd_enabled_flag");
  pcSPS->m_SMVD = uiCode != 0;
  READ_FLAG(uiCode, "sps_dmvr_enabled_flag");
  pcSPS->m_DMVR = uiCode != 0;
  if (pcSPS->m_DMVR)
  {
    READ_FLAG(uiCode, "sps_dmvr_control_present_in_ph_flag");
    pcSPS->m_DmvrControlPresentInPhFlag = uiCode != 0;
  }
  else
  {
    pcSPS->m_DmvrControlPresentInPhFlag = false;
  }
  READ_FLAG(uiCode, "sps_mmvd_enabled_flag");
  pcSPS->m_MMVD = uiCode != 0;
  if (pcSPS->m_MMVD)
  {
    READ_FLAG(uiCode, "sps_mmvd_fullpel_only_flag");
    pcSPS->m_fpelMmvdEnabledFlag = uiCode != 0;
  }
  else
  {
    pcSPS->m_fpelMmvdEnabledFlag = false;
  }

  READ_UVLC(uiCode, "sps_six_minus_max_num_merge_cand");
  CHECK(vvc::MRG_MAX_NUM_CANDS <= uiCode, "Incorrrect max number of merge candidates!");
  pcSPS->m_maxNumMergeCand = vvc::MRG_MAX_NUM_CANDS - uiCode;
  READ_FLAG(uiCode, "sps_sbt_enabled_flag");
  pcSPS->m_SBT = uiCode != 0;
  READ_FLAG(uiCode, "sps_affine_enabled_flag");
  pcSPS->m_Affine = uiCode != 0;
  if (pcSPS->m_Affine)
  {
    READ_UVLC(uiCode, "sps_five_minus_max_num_subblock_merge_cand");
    CHECK(
        uiCode > 5 - (pcSPS->m_sbtmvpEnabledFlag ? 1 : 0),
        "The value of sps_five_minus_max_num_subblock_merge_cand shall be in the range of 0 to 5 - sps_sbtmvp_enabled_flag");
    CHECK(vvc::AFFINE_MRG_MAX_NUM_CANDS < uiCode, "The value of sps_five_minus_max_num_subblock_merge_cand shall be in the range of 0 to 5 - sps_sbtmvp_enabled_flag");
    pcSPS->m_maxNumAffineMergeCand = vvc::AFFINE_MRG_MAX_NUM_CANDS - uiCode;
    READ_FLAG(uiCode, "sps_affine_type_flag");
    pcSPS->m_AffineType = uiCode != 0;
    if (pcSPS->m_AMVREnabledFlag)
    {
      READ_FLAG(uiCode, "sps_affine_amvr_enabled_flag");
      pcSPS->m_affineAmvrEnabledFlag = uiCode != 0;
    }
    READ_FLAG(uiCode, "sps_affine_prof_enabled_flag");
    pcSPS->m_PROF = uiCode != 0;
    if (pcSPS->m_PROF)
    {
      READ_FLAG(uiCode, "sps_prof_control_present_in_ph_flag");
      pcSPS->m_ProfControlPresentInPhFlag = uiCode != 0;
    }
    else
    {
      pcSPS->m_ProfControlPresentInPhFlag = false;
    }
  }

  READ_FLAG(uiCode, "sps_bcw_enabled_flag");
  pcSPS->m_bcw = uiCode != 0;

  READ_FLAG(uiCode, "sps_ciip_enabled_flag");
  pcSPS->m_ciip = uiCode != 0;

  if (pcSPS->m_maxNumMergeCand >= 2)
  {
    READ_FLAG(uiCode, "sps_gpm_enabled_flag");
    pcSPS->m_Geo = uiCode != 0;
    if (pcSPS->m_Geo)
    {
      if (pcSPS->m_maxNumMergeCand >= 3)
      {
        READ_UVLC(uiCode, "sps_max_num_merge_cand_minus_max_num_gpm_cand");
        CHECK(pcSPS->m_maxNumMergeCand - 2 < uiCode,
              "sps_max_num_merge_cand_minus_max_num_gpm_cand must not be greater than the number of merge candidates minus 2");
        pcSPS->m_maxNumGeoCand = (uint32_t)(pcSPS->m_maxNumMergeCand - uiCode);
      }
      else
      {
        pcSPS->m_maxNumGeoCand = 2;
      }
    }
  }
  else
  {
    pcSPS->m_Geo = 0;
    pcSPS->m_maxNumGeoCand = 0;
  }

  READ_UVLC(uiCode, "sps_log2_parallel_merge_level_minus2");
  CHECK(uiCode + 2 > ctbLog2SizeY, "The value of sps_log2_parallel_merge_level_minus2 shall be in the range of 0 to ctbLog2SizeY - 2");
  pcSPS->m_log2ParallelMergeLevelMinus2 = uiCode;

  READ_FLAG(uiCode, "sps_isp_enabled_flag");
  pcSPS->m_ISP = uiCode != 0;
  READ_FLAG(uiCode, "sps_mrl_enabled_flag");
  pcSPS->m_MRL = uiCode != 0;
  READ_FLAG(uiCode, "sps_mip_enabled_flag");
  pcSPS->m_MIP = uiCode != 0;
  if (pcSPS->m_chromaFormatIdc != vvc::CHROMA_400)
  {
    READ_FLAG(uiCode, "sps_cclm_enabled_flag");
    pcSPS->m_LMChroma = uiCode != 0;
  }
  else
  {
    pcSPS->m_LMChroma = 0;
  }
  if (pcSPS->m_chromaFormatIdc == vvc::CHROMA_420)
  {
    READ_FLAG(uiCode, "sps_chroma_horizontal_collocated_flag");
    pcSPS->m_horCollocatedChromaFlag = uiCode != 0;
    READ_FLAG(uiCode, "sps_chroma_vertical_collocated_flag");
    pcSPS->m_verCollocatedChromaFlag = uiCode != 0;
  }
  else
  {
    pcSPS->m_horCollocatedChromaFlag = true;
    pcSPS->m_verCollocatedChromaFlag = true;
  }
  READ_FLAG(uiCode, "sps_palette_enabled_flag");
  pcSPS->m_PLTMode = uiCode != 0;
  CHECK((profile == vvc::Profile::MAIN_12 || profile == vvc::Profile::MAIN_12_INTRA || profile == vvc::Profile::MAIN_12_STILL_PICTURE) && uiCode != 0, "sps_palette_enabled_flag shall be equal to 0 for Main 12 (420) profiles");
  if (pcSPS->m_chromaFormatIdc == vvc::CHROMA_444 && pcSPS->m_log2MaxTbSize != 6)
  {
    READ_FLAG(uiCode, "sps_act_enabled_flag");
    pcSPS->m_useColorTrans = uiCode != 0;
  }
  else
  {
    pcSPS->m_useColorTrans = false;
  }
  if (pcSPS->m_transformSkipEnabledFlag || pcSPS->m_PLTMode)
  {
    READ_UVLC(uiCode, "sps_internal_bit_depth_minus_input_bit_depth");
    pcSPS->m_internalMinusInputBitDepth[vvc::CHANNEL_TYPE_LUMA] = uiCode;
    CHECK(uiCode > 8, "Invalid sps_internal_bit_depth_minus_input_bit_depth signalled");
    pcSPS->m_internalMinusInputBitDepth[vvc::CHANNEL_TYPE_CHROMA] = uiCode;
  }
  READ_FLAG(uiCode, "sps_ibc_enabled_flag");
  pcSPS->m_IBCFlag = uiCode;
  if (pcSPS->m_IBCFlag)
  {
    READ_UVLC(uiCode, "sps_six_minus_max_num_ibc_merge_cand");
    CHECK(vvc::IBC_MRG_MAX_NUM_CANDS <= uiCode, "Incorrect max number of IBC merge candidates!");
    pcSPS->m_maxNumIBCMergeCand = vvc::IBC_MRG_MAX_NUM_CANDS - uiCode;
  }
  else
  {
    pcSPS->m_maxNumIBCMergeCand = 0;
  }

  READ_FLAG(uiCode, "sps_explicit_scaling_list_enabled_flag");
  pcSPS->m_scalingListEnabledFlag = uiCode;
  if (pcSPS->m_profileTierLevel.m_constraintInfo.m_noExplicitScaleListConstraintFlag)
  {
    CHECK(uiCode != 0, "When gci_no_explicit_scaling_list_constraint_flag is equal to 1, sps_explicit_scaling_list_enabled_flag shall be equal to 0");
  }

  if (pcSPS->m_LFNST && pcSPS->m_scalingListEnabledFlag)
  {
    READ_FLAG(uiCode, "sps_scaling_matrix_for_lfnst_disabled_flag");
    pcSPS->m_disableScalingMatrixForLfnstBlks = uiCode ? true : false;
  }

  if (pcSPS->m_useColorTrans && pcSPS->m_scalingListEnabledFlag)
  {
    READ_FLAG(uiCode, "sps_scaling_matrix_for_alternative_colour_space_disabled_flag");
    pcSPS->m_scalingMatrixAlternativeColourSpaceDisabledFlag = uiCode;
  }
  if (pcSPS->m_scalingMatrixAlternativeColourSpaceDisabledFlag)
  {
    READ_FLAG(uiCode, "sps_scaling_matrix_designated_colour_space_flag");
    pcSPS->m_scalingMatrixDesignatedColourSpaceFlag = uiCode;
  }
  READ_FLAG(uiCode, "sps_dep_quant_enabled_flag");
  pcSPS->m_depQuantEnabledFlag = uiCode;
  READ_FLAG(uiCode, "sps_sign_data_hiding_enabled_flag");
  pcSPS->m_signDataHidingEnabledFlag = uiCode;

  READ_FLAG(uiCode, "sps_virtual_boundaries_enabled_flag");
  pcSPS->m_virtualBoundariesEnabledFlag = uiCode != 0;
  if (pcSPS->m_profileTierLevel.m_constraintInfo.m_noVirtualBoundaryConstraintFlag)
  {
    CHECK(uiCode != 0, "When gci_no_virtual_boundaries_constraint_flag is equal to 1, sps_virtual_boundaries_enabled_flag shall be equal to 0");
  }

  if (pcSPS->m_virtualBoundariesEnabledFlag)
  {
    READ_FLAG(uiCode, "sps_loop_filter_across_virtual_boundaries_present_flag");
    pcSPS->m_virtualBoundariesPresentFlag = uiCode != 0;
    if (pcSPS->m_virtualBoundariesPresentFlag)
    {
      READ_UVLC(uiCode, "sps_num_ver_virtual_boundaries");
      pcSPS->m_numVerVirtualBoundaries = uiCode;
      if (pcSPS->m_maxWidthInLumaSamples <= 8)
      {
        CHECK(pcSPS->m_numVerVirtualBoundaries != 0, "SPS: When picture width is less than or equal to 8, the "
                                                     "number of vertical virtual boundaries shall be equal to 0");
      }
      else
      {
        CHECK(pcSPS->m_numVerVirtualBoundaries > 3,
              "SPS: The number of vertical virtual boundaries shall be in the range of 0 to 3");
      }
      for (unsigned i = 0; i < pcSPS->m_numVerVirtualBoundaries; i++)
      {
        READ_UVLC(uiCode, "sps_virtual_boundary_pos_x_minus1[i]");
        pcSPS->m_virtualBoundariesPosX[i] = (uiCode + 1) << 3;
        CHECK(uiCode > (((pcSPS->m_maxWidthInLumaSamples + 7) >> 3) - 2),
              "The value of sps_virtual_boundary_pos_x_minus1[ i ] shall be in the range of 0 to Ceil( "
              "sps_pic_width_max_in_luma_samples / 8 ) - 2, inclusive.");
      }
      READ_UVLC(uiCode, "sps_num_hor_virtual_boundaries");
      pcSPS->m_numHorVirtualBoundaries = uiCode;
      if (pcSPS->m_maxHeightInLumaSamples <= 8)
      {
        CHECK(pcSPS->m_numHorVirtualBoundaries != 0, "SPS: When picture height is less than or equal to 8, the "
                                                     "number of horizontal virtual boundaries shall be equal to 0");
      }
      else
      {
        CHECK(pcSPS->m_numHorVirtualBoundaries > 3,
              "SPS: The number of horizontal virtual boundaries shall be in the range of 0 to 3");
      }
      for (unsigned i = 0; i < pcSPS->m_numHorVirtualBoundaries; i++)
      {
        READ_UVLC(uiCode, "sps_virtual_boundary_pos_y_minus1[i]");
        pcSPS->m_virtualBoundariesPosY[i] = (uiCode + 1) << 3;
        CHECK(uiCode > (((pcSPS->m_maxHeightInLumaSamples + 7) >> 3) - 2),
              "The value of sps_virtual_boundary_pos_y_minus1[ i ] shall be in the range of 0 to Ceil( "
              "sps_pic_height_max_in_luma_samples / 8 ) - 2, inclusive.");
      }
    }
    else
    {
      pcSPS->m_numVerVirtualBoundaries = 0;
      pcSPS->m_numHorVirtualBoundaries = 0;
    }
  }
  else
  {
    pcSPS->m_virtualBoundariesPresentFlag = false;
  }

  if (pcSPS->m_ptlDpbHrdParamsPresentFlag)
  {
    READ_FLAG(uiCode, "sps_timing_hrd_params_present_flag");
    pcSPS->m_generalHrdParametersPresentFlag = uiCode;
    if (pcSPS->m_generalHrdParametersPresentFlag)
    {
      parseGeneralHrdParameters(&pcSPS->m_generalHrdParams);
      if ((pcSPS->m_uiMaxTLayers - 1) > 0)
      {
        READ_FLAG(uiCode, "sps_sublayer_cpb_params_present_flag");
        pcSPS->m_SubLayerCbpParametersPresentFlag = uiCode;
      }
      else if ((pcSPS->m_uiMaxTLayers - 1) == 0)
      {
        pcSPS->m_SubLayerCbpParametersPresentFlag = 0;
      }

      uint32_t firstSubLayer = pcSPS->m_SubLayerCbpParametersPresentFlag ? 0 : (pcSPS->m_uiMaxTLayers - 1);
      parseOlsHrdParameters(&pcSPS->m_generalHrdParams, &pcSPS->m_olsHrdParams[0], firstSubLayer, pcSPS->m_uiMaxTLayers - 1);
    }
  }

  READ_FLAG(uiCode, "sps_field_seq_flag");
  pcSPS->m_fieldSeqFlag = uiCode;
  CHECK(pcSPS->m_profileTierLevel.m_frameOnlyConstraintFlag && uiCode, "When ptl_frame_only_constraint_flag equal to 1 , the value of sps_field_seq_flag shall be equal to 0");

  READ_FLAG(uiCode, "sps_vui_parameters_present_flag");
  pcSPS->m_vuiParametersPresentFlag = uiCode;

  if (pcSPS->m_vuiParametersPresentFlag)
  {
    READ_UVLC(uiCode, "sps_vui_payload_size_minus1");
    pcSPS->m_vuiPayloadSize = uiCode + 1;
    while (!isByteAligned())
    {
      READ_FLAG(uiCode, "sps_vui_alignment_zero_bit");
      CHECK(uiCode != 0, "sps_vui_alignment_zero_bit not equal to 0");
    }
    parseVUI(&pcSPS->m_vuiParameters, pcSPS);
  }

  READ_FLAG(uiCode, "sps_extension_present_flag");

  if (uiCode)
  {
    bool sps_extension_flags[vvc::NUM_SPS_EXTENSION_FLAGS];

    for (int i = 0; i < vvc::NUM_SPS_EXTENSION_FLAGS; i++)
    {
      READ_FLAG(uiCode, syntaxStrings[i]);
      sps_extension_flags[i] = uiCode != 0;
    }

    if (pcSPS->m_bitDepths[vvc::CHANNEL_TYPE_LUMA] <= 10)
      CHECK(sps_extension_flags[vvc::SPS_EXT__REXT] == 1,
            "The value of sps_range_extension_flag shall be 0 when BitDepth is less than or equal to 10.");

    bool bSkipTrailingExtensionBits = false;
    for (int i = 0; i < vvc::NUM_SPS_EXTENSION_FLAGS; i++) // loop used so that the order is determined by the enum.
    {
      if (sps_extension_flags[i])
      {
        switch (vvc::SPSExtensionFlagIndex(i))
        {
        case vvc::SPS_EXT__REXT:
          CHECK(bSkipTrailingExtensionBits, "Skipping trailing extension bits not supported");
          {
            vvc::SPSRExt &spsRangeExtension = pcSPS->m_spsRangeExtension;
            READ_FLAG(uiCode, "extended_precision_processing_flag");
            spsRangeExtension.m_extendedPrecisionProcessingFlag = uiCode != 0;
            if (pcSPS->m_transformSkipEnabledFlag)
            {
              READ_FLAG(uiCode, "sps_ts_residual_coding_rice_present_in_sh_flag");
              spsRangeExtension.m_tsrcRicePresentFlag = uiCode != 0;
            }
            READ_FLAG(uiCode, "rrc_rice_extension_flag");
            spsRangeExtension.m_rrcRiceExtensionEnableFlag = uiCode != 0;
            READ_FLAG(uiCode, "persistent_rice_adaptation_enabled_flag");
            spsRangeExtension.m_persistentRiceAdaptationEnabledFlag = uiCode != 0;
            READ_FLAG(uiCode, "reverse_last_position_enabled_flag");
            spsRangeExtension.m_reverseLastSigCoeffEnabledFlag = uiCode != 0;
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
        READ_FLAG(uiCode, "sps_extension_data_flag");
      }
    }
  }
  xReadRbspTrailingBits();
}

void parseNalH266::parseOPI(vvc::OPI *opi)
{
  uint32_t symbol;

  READ_FLAG(symbol, "opi_ols_info_present_flag");
  opi->m_olsinfopresentflag = symbol;
  READ_FLAG(symbol, "opi_htid_info_present_flag");
  opi->m_htidinfopresentflag = symbol;

  if (opi->m_olsinfopresentflag)
  {
    READ_UVLC(symbol, "opi_ols_idx");
    opi->m_opiolsidx = symbol;
  }

  if (opi->m_htidinfopresentflag)
  {
    READ_CODE(3, symbol, "opi_htid_plus1");
    opi->m_opihtidplus1 = symbol;
  }

  READ_FLAG(symbol, "opi_extension_flag");
  if (symbol)
  {
    while (xMoreRbspData())
    {
      READ_FLAG(symbol, "opi_extension_data_flag");
    }
  }
  xReadRbspTrailingBits();
}

void parseNalH266::parseDCI(vvc::DCI *dci)
{
  uint32_t symbol;

  READ_CODE(4, symbol, "dci_reserved_zero_4bits");

  uint32_t numPTLs;
  READ_CODE(4, numPTLs, "dci_num_ptls_minus1");
  numPTLs += 1;

  std::vector<vvc::ProfileTierLevel> ptls;
  ptls.resize(numPTLs);
  for (int i = 0; i < (int)numPTLs; i++)
  {
    parseProfileTierLevel(&ptls[i], true, 0);
  }
  dci->m_profileTierLevel = ptls;

  READ_FLAG(symbol, "dci_extension_flag");
  if (symbol)
  {
    while (xMoreRbspData())
    {
      READ_FLAG(symbol, "dci_extension_data_flag");
    }
  }
  xReadRbspTrailingBits();
}

void parseNalH266::parseVPS(vvc::VPS *pcVPS)
{
  uint32_t uiCode;

  READ_CODE(4, uiCode, "vps_video_parameter_set_id");
  CHECK(uiCode == 0, "vps_video_parameter_set_id equal to zero is reserved and shall not be used in a bitstream");
  pcVPS->m_VPSId = uiCode;

  READ_CODE(6, uiCode, "vps_max_layers_minus1");
  pcVPS->m_maxLayers = uiCode + 1;
  CHECK(uiCode + 1 > vvc::MAX_VPS_LAYERS, "Signalled number of layers larger than MAX_VPS_LAYERS.");
  if (pcVPS->m_maxLayers - 1 == 0)
  {
    pcVPS->m_vpsEachLayerIsAnOlsFlag = 1;
  }
  READ_CODE(3, uiCode, "vps_max_sublayers_minus1");
  pcVPS->m_vpsMaxSubLayers = uiCode + 1;
  CHECK(uiCode + 1 > vvc::MAX_VPS_SUBLAYERS, "Signalled number of sublayers larger than MAX_VPS_SUBLAYERS.");
  if (pcVPS->m_maxLayers > 1 && pcVPS->m_vpsMaxSubLayers > 1)
  {
    READ_FLAG(uiCode, "vps_default_ptl_dpb_hrd_max_tid_flag");
    pcVPS->m_vpsDefaultPtlDpbHrdMaxTidFlag = uiCode;
  }
  else
  {
    pcVPS->m_vpsDefaultPtlDpbHrdMaxTidFlag = 1;
  }
  if (pcVPS->m_maxLayers > 1)
  {
    READ_FLAG(uiCode, "vps_all_independent_layers_flag");
    pcVPS->m_vpsAllIndependentLayersFlag = uiCode;
    if (pcVPS->m_vpsAllIndependentLayersFlag == 0)
    {
      pcVPS->m_vpsEachLayerIsAnOlsFlag = 0;
    }
  }
  std::vector<std::vector<uint32_t>> maxTidilRefPicsPlus1;
  maxTidilRefPicsPlus1.resize(pcVPS->m_maxLayers, std::vector<uint32_t>(pcVPS->m_maxLayers, vvc::NOT_VALID));

  pcVPS->m_vpsMaxTidIlRefPicsPlus1 = maxTidilRefPicsPlus1;
  for (uint32_t i = 0; i < pcVPS->m_maxLayers; i++)
  {
    READ_CODE(6, uiCode, "vps_layer_id");
    pcVPS->m_vpsLayerId[i] = uiCode;
    pcVPS->m_generalLayerIdx[i] = uiCode;

    if (i > 0 && !pcVPS->m_vpsAllIndependentLayersFlag)
    {
      READ_FLAG(uiCode, "vps_independent_layer_flag");
      pcVPS->m_vpsIndependentLayerFlag[i] = uiCode;
      if (!pcVPS->m_vpsIndependentLayerFlag[i])
      {
        READ_FLAG(uiCode, "max_tid_ref_present_flag[ i ]");
        bool presentFlag = uiCode;
        uint16_t sumUiCode = 0;
        for (int j = 0, k = 0; j < (int)i; j++)
        {
          READ_FLAG(uiCode, "vps_direct_ref_layer_flag");
          pcVPS->m_directRefLayerIdx[i][j] = uiCode;
          if (uiCode)
          {
            pcVPS->m_interLayerRefIdx[i][j] = k;
            pcVPS->m_directRefLayerIdx[i][k++] = j;
            sumUiCode++;
          }
          if (presentFlag && pcVPS->m_directRefLayerIdx[i][j])
          {
            READ_CODE(3, uiCode, "max_tid_il_ref_pics_plus1[ i ][ j ]");
            pcVPS->m_vpsMaxTidIlRefPicsPlus1[i][j] = uiCode;
          }
          else
          {
            pcVPS->m_vpsMaxTidIlRefPicsPlus1[i][j] = 7;
          }
        }
        CHECK(sumUiCode == 0, "There has to be at least one value of j such that the value of vps_direct_dependency_flag[ i ][ j ] is equal to 1,when vps_independent_layer_flag[ i ] is equal to 0 ");
      }
    }
  }

  if (pcVPS->m_maxLayers > 1)
  {
    if (pcVPS->m_vpsAllIndependentLayersFlag)
    {
      READ_FLAG(uiCode, "vps_each_layer_is_an_ols_flag");
      pcVPS->m_vpsEachLayerIsAnOlsFlag = uiCode;
      if (pcVPS->m_vpsEachLayerIsAnOlsFlag == 0)
      {
        pcVPS->m_vpsOlsModeIdc = 2;
      }
    }
    if (!pcVPS->m_vpsEachLayerIsAnOlsFlag)
    {
      if (!pcVPS->m_vpsAllIndependentLayersFlag)
      {
        READ_CODE(2, uiCode, "vps_ols_mode_idc");
        pcVPS->m_vpsOlsModeIdc = uiCode;
        CHECK(uiCode > vvc::MAX_VPS_OLS_MODE_IDC, "vps_ols_mode_idc shall be in the range of 0 to 2");
      }
      if (pcVPS->m_vpsOlsModeIdc == 2)
      {
        READ_CODE(8, uiCode, "vps_num_output_layer_sets_minus2");
        pcVPS->m_vpsNumOutputLayerSets = uiCode + 2;
        pcVPS->m_vpsOlsOutputLayerFlag[0][0] = true;
        for (uint32_t i = 1; i <= pcVPS->m_vpsNumOutputLayerSets - 1; i++)
        {
          for (uint32_t j = 0; j < pcVPS->m_maxLayers; j++)
          {
            READ_FLAG(uiCode, "vps_ols_output_layer_flag");
            pcVPS->m_vpsOlsOutputLayerFlag[i][j] = uiCode;
          }
        }
      }
    }
    READ_CODE(8, uiCode, "vps_num_ptls_minus1");
    pcVPS->m_vpsNumPtls = uiCode + 1;
  }
  else
  {
    pcVPS->m_vpsNumPtls = 1;
  }
  pcVPS->deriveOutputLayerSets();
  CHECK((int)pcVPS->m_vpsNumPtls > pcVPS->m_totalNumOLSs, "The value of vps_num_ptls_minus1 shall be less than TotalNumOlss");
  std::vector<bool> isPTLReferred(pcVPS->m_vpsNumPtls, false);

  for (int i = 0; i < (int)pcVPS->m_vpsNumPtls; i++)
  {
    if (i > 0)
    {
      READ_FLAG(uiCode, "vps_pt_present_flag");
      pcVPS->m_ptPresentFlag[i] = uiCode;
    }
    else
    {
      pcVPS->m_ptPresentFlag[0] = 1;
    }
    if (!pcVPS->m_vpsDefaultPtlDpbHrdMaxTidFlag)
    {
      READ_CODE(3, uiCode, "vps_ptl_max_tid");
      pcVPS->m_ptlMaxTemporalId[i] = uiCode;
    }
    else
    {
      pcVPS->m_ptlMaxTemporalId[i] = pcVPS->m_vpsMaxSubLayers - 1;
    }
  }
  int cnt = 0;
  while (m_pcBitstream->getNumBitsUntilByteAligned())
  {
    READ_FLAG(uiCode, "vps_ptl_reserved_zero_bit");
    CHECK(uiCode != 0, "Alignment bit is not '0'");
    cnt++;
  }
  CHECK(cnt >= 8, "Read more than '8' alignment bits");
  std::vector<vvc::ProfileTierLevel> ptls;
  ptls.resize(pcVPS->m_vpsNumPtls);
  for (int i = 0; i < (int)pcVPS->m_vpsNumPtls; i++)
  {
    parseProfileTierLevel(&ptls[i], pcVPS->m_ptPresentFlag[i], pcVPS->m_ptlMaxTemporalId[i]);
    if (!pcVPS->m_ptPresentFlag[i])
    {
      ptls[i].m_profileIdc = ptls[i - 1].m_profileIdc;
      ptls[i].m_tierFlag = ptls[i - 1].m_tierFlag;
      ptls[i].m_constraintInfo = ptls[i - 1].m_constraintInfo;
    }
  }
  pcVPS->m_vpsProfileTierLevel = ptls;
  for (int i = 0; i < pcVPS->m_totalNumOLSs; i++)
  {
    if (pcVPS->m_vpsNumPtls > 1 && (int)pcVPS->m_vpsNumPtls != pcVPS->m_totalNumOLSs)
    {
      READ_CODE(8, uiCode, "vps_ols_ptl_idx");
      pcVPS->m_olsPtlIdx[i] = uiCode;
    }
    else if ((int)pcVPS->m_vpsNumPtls == pcVPS->m_totalNumOLSs)
    {
      pcVPS->m_olsPtlIdx[i] = i;
    }
    else
    {
      pcVPS->m_olsPtlIdx[i] = 0;
    }
    isPTLReferred[pcVPS->m_olsPtlIdx[i]] = true;
  }
  for (int i = 0; i < (int)pcVPS->m_vpsNumPtls; i++)
  {
    CHECK(!isPTLReferred[i], "Each profile_tier_level( ) syntax structure in the VPS shall be referred to by at least one value of vps_ols_ptl_idx[ i ] for i in the range of 0 to TotalNumOlss ? 1, inclusive");
  }

  if (!pcVPS->m_vpsEachLayerIsAnOlsFlag)
  {
    READ_UVLC(uiCode, "vps_num_dpb_params_minus1");
    pcVPS->m_numDpbParams = uiCode + 1;

    CHECK(pcVPS->m_numDpbParams > pcVPS->m_numMultiLayeredOlss, "The value of vps_num_dpb_params_minus1 shall be in the range of 0 to NumMultiLayerOlss - 1, inclusive");
    std::vector<bool> isDPBParamReferred(pcVPS->m_numDpbParams, false);

    if (pcVPS->m_numDpbParams > 0 && pcVPS->m_vpsMaxSubLayers > 1)
    {
      READ_FLAG(uiCode, "vps_sublayer_dpb_params_present_flag");
      pcVPS->m_sublayerDpbParamsPresentFlag = uiCode;
    }

    pcVPS->m_dpbParameters.resize(pcVPS->m_numDpbParams);

    for (int i = 0; i < pcVPS->m_numDpbParams; i++)
    {
      if (!pcVPS->m_vpsDefaultPtlDpbHrdMaxTidFlag)
      {
        READ_CODE(3, uiCode, "vps_dpb_max_tid[i]");
        pcVPS->m_dpbMaxTemporalId.push_back(uiCode);
        CHECK(uiCode > (pcVPS->m_vpsMaxSubLayers - 1), "The value of vps_dpb_max_tid[i] shall be in the range of 0 to vps_max_sublayers_minus1, inclusive.")
      }
      else
      {
        pcVPS->m_dpbMaxTemporalId.push_back(pcVPS->m_vpsMaxSubLayers - 1);
      }

      for (int j = (pcVPS->m_sublayerDpbParamsPresentFlag ? 0 : pcVPS->m_dpbMaxTemporalId[i]); j <= pcVPS->m_dpbMaxTemporalId[i]; j++)
      {
        READ_UVLC(uiCode, "dpb_max_dec_pic_buffering_minus1[i]");
        pcVPS->m_dpbParameters[i].m_maxDecPicBuffering[j] = uiCode + 1;
        READ_UVLC(uiCode, "dpb_max_num_reorder_pics[i]");
        pcVPS->m_dpbParameters[i].m_maxNumReorderPics[j] = uiCode;
        READ_UVLC(uiCode, "dpb_max_latency_increase_plus1[i]");
        pcVPS->m_dpbParameters[i].m_maxLatencyIncreasePlus1[j] = uiCode;
      }

      for (int j = (pcVPS->m_sublayerDpbParamsPresentFlag ? pcVPS->m_dpbMaxTemporalId[i] : 0); j < pcVPS->m_dpbMaxTemporalId[i]; j++)
      {
        // When dpb_max_dec_pic_buffering_minus1[ i ] is not present for i in the range of 0 to maxSubLayersMinus1 - 1, inclusive, due to subLayerInfoFlag being equal to 0, it is inferred to be equal to dpb_max_dec_pic_buffering_minus1[ maxSubLayersMinus1 ].
        pcVPS->m_dpbParameters[i].m_maxDecPicBuffering[j] = pcVPS->m_dpbParameters[i].m_maxDecPicBuffering[pcVPS->m_dpbMaxTemporalId[i]];

        // When dpb_max_num_reorder_pics[ i ] is not present for i in the range of 0 to maxSubLayersMinus1 - 1, inclusive, due to subLayerInfoFlag being equal to 0, it is inferred to be equal to dpb_max_num_reorder_pics[ maxSubLayersMinus1 ].
        pcVPS->m_dpbParameters[i].m_maxNumReorderPics[j] = pcVPS->m_dpbParameters[i].m_maxNumReorderPics[pcVPS->m_dpbMaxTemporalId[i]];

        // When dpb_max_latency_increase_plus1[ i ] is not present for i in the range of 0 to maxSubLayersMinus1 - 1, inclusive, due to subLayerInfoFlag being equal to 0, it is inferred to be equal to dpb_max_latency_increase_plus1[ maxSubLayersMinus1 ].
        pcVPS->m_dpbParameters[i].m_maxLatencyIncreasePlus1[j] = pcVPS->m_dpbParameters[i].m_maxLatencyIncreasePlus1[pcVPS->m_dpbMaxTemporalId[i]];
      }
    }

    for (int i = 0, j = 0; i < pcVPS->m_totalNumOLSs; i++)
    {
      if (pcVPS->m_numLayersInOls[i] > 1)
      {
        READ_UVLC(uiCode, "vps_ols_dpb_pic_width[i]");
        pcVPS->m_olsDpbPicSize[i].width = uiCode;
        READ_UVLC(uiCode, "vps_ols_dpb_pic_height[i]");
        pcVPS->m_olsDpbPicSize[i].height = uiCode;
        READ_CODE(2, uiCode, "vps_ols_dpb_chroma_format[i]");
        pcVPS->m_olsDpbChromaFormatIdc[i] = uiCode;
        READ_UVLC(uiCode, "vps_ols_dpb_bitdepth_minus8[i]");
        pcVPS->m_olsDpbBitDepthMinus8[i] = uiCode;
        const vvc::Profile::Name profile = pcVPS->m_vpsProfileTierLevel[pcVPS->m_olsPtlIdx[i]].m_profileIdc;
        if (profile != vvc::Profile::NONE)
        {
          CHECK(uiCode + 8 > vvc::ProfileFeatures::getProfileFeatures(profile)->maxBitDepth, "vps_ols_dpb_bitdepth_minus8[ i ] exceeds range supported by signalled profile");
        }
        if ((pcVPS->m_numDpbParams > 1) && (pcVPS->m_numDpbParams != pcVPS->m_numMultiLayeredOlss))
        {
          READ_UVLC(uiCode, "vps_ols_dpb_params_idx[i]");
          pcVPS->m_olsDpbParamsIdx[i] = uiCode;
        }
        else if (pcVPS->m_numDpbParams == 1)
        {
          pcVPS->m_olsDpbParamsIdx[i] = 0;
        }
        else
        {
          pcVPS->m_olsDpbParamsIdx[i] = j;
        }
        j += 1;
        isDPBParamReferred[pcVPS->m_olsDpbParamsIdx[i]] = true;
      }
    }
    for (int i = 0; i < pcVPS->m_numDpbParams; i++)
    {
      CHECK(!isDPBParamReferred[i], "Each dpb_parameters( ) syntax structure in the VPS shall be referred to by at least one value of vps_ols_dpb_params_idx[i] for i in the range of 0 to NumMultiLayerOlss - 1, inclusive");
    }
  }

  if (!pcVPS->m_vpsEachLayerIsAnOlsFlag)
  {
    READ_FLAG(uiCode, "vps_general_hrd_params_present_flag");
    pcVPS->m_vpsGeneralHrdParamsPresentFlag = uiCode;
  }
  if (pcVPS->m_vpsGeneralHrdParamsPresentFlag)
  {
    parseGeneralHrdParameters(&pcVPS->m_generalHrdParams);
    if ((pcVPS->m_vpsMaxSubLayers - 1) > 0)
    {
      READ_FLAG(uiCode, "vps_sublayer_cpb_params_present_flag");
      pcVPS->m_vpsSublayerCpbParamsPresentFlag = uiCode;
    }
    else
    {
      pcVPS->m_vpsSublayerCpbParamsPresentFlag = 0;
    }
    READ_UVLC(uiCode, "vps_num_ols_timing_hrd_params_minus1");
    pcVPS->m_numOlsTimingHrdParamsMinus1 = uiCode;
    CHECK((int)uiCode >= pcVPS->m_numMultiLayeredOlss, "The value of vps_num_ols_timing_hrd_params_minus1 shall be in the range of 0 to NumMultiLayerOlss - 1, inclusive");
    std::vector<bool> isHRDParamReferred(uiCode + 1, false);
    pcVPS->m_olsHrdParams.clear();
    pcVPS->m_olsHrdParams.resize(pcVPS->m_numOlsTimingHrdParamsMinus1 + 1, std::vector<vvc::OlsHrdParams>(pcVPS->m_vpsMaxSubLayers));

    for (int i = 0; i <= (int)pcVPS->m_numOlsTimingHrdParamsMinus1; i++)
    {
      if (!pcVPS->m_vpsDefaultPtlDpbHrdMaxTidFlag)
      {
        READ_CODE(3, uiCode, "vps_hrd_max_tid[i]");
        pcVPS->m_hrdMaxTid[i] = uiCode;
        CHECK(uiCode > (pcVPS->m_vpsMaxSubLayers - 1), "The value of vps_hrd_max_tid[i] shall be in the range of 0 to vps_max_sublayers_minus1, inclusive.")
      }
      else
      {
        pcVPS->m_hrdMaxTid[i] = pcVPS->m_vpsMaxSubLayers - 1;
      }
      uint32_t firstSublayer = pcVPS->m_vpsSublayerCpbParamsPresentFlag ? 0 : pcVPS->m_hrdMaxTid[i];

      parseOlsHrdParameters(&pcVPS->m_generalHrdParams, &pcVPS->m_olsHrdParams[i][0], firstSublayer, pcVPS->m_hrdMaxTid[i]);
    }
    for (int i = pcVPS->m_numOlsTimingHrdParamsMinus1 + 1; i < pcVPS->m_totalNumOLSs; i++)
    {
      pcVPS->m_hrdMaxTid[i] = pcVPS->m_vpsMaxSubLayers - 1;
    }
    for (int i = 0; i < pcVPS->m_numMultiLayeredOlss; i++)
    {
      if ((((int)pcVPS->m_numOlsTimingHrdParamsMinus1 + 1) != pcVPS->m_numMultiLayeredOlss) && (pcVPS->m_numOlsTimingHrdParamsMinus1 > 0))
      {
        READ_UVLC(uiCode, "vps_ols_timing_hrd_idx[i]");
        pcVPS->m_olsTimingHrdIdx[i] = uiCode;
        CHECK(uiCode > pcVPS->m_numOlsTimingHrdParamsMinus1, "The value of vps_ols_timing_hrd_idx[[ i ] shall be in the range of 0 to vps_num_ols_timing_hrd_params_minus1, inclusive.");
      }
      else if (pcVPS->m_numOlsTimingHrdParamsMinus1 == 0)
      {
        pcVPS->m_olsTimingHrdIdx[i] = 0;
      }
      else
      {
        pcVPS->m_olsTimingHrdIdx[i] = i;
      }
      isHRDParamReferred[pcVPS->m_olsTimingHrdIdx[i]] = true;
    }
    for (int i = 0; i <= (int)pcVPS->m_numOlsTimingHrdParamsMinus1; i++)
    {
      CHECK(!isHRDParamReferred[i], "Each vps_ols_timing_hrd_parameters( ) syntax structure in the VPS shall be referred to by at least one value of vps_ols_timing_hrd_idx[ i ] for i in the range of 1 to NumMultiLayerOlss - 1, inclusive");
    }
  }
  else
  {
    for (int i = 0; i < pcVPS->m_totalNumOLSs; i++)
    {
      pcVPS->m_hrdMaxTid[i] = pcVPS->m_vpsMaxSubLayers - 1;
    }
  }

  READ_FLAG(uiCode, "vps_extension_flag");
  if (uiCode)
  {
    while (xMoreRbspData())
    {
      READ_FLAG(uiCode, "vps_extension_data_flag");
    }
  }
  pcVPS->checkVPS();
  xReadRbspTrailingBits();
}

void parseNalH266::parseConstraintInfo(vvc::ConstraintInfo *cinfo, const vvc::ProfileTierLevel *ptl)
{
  uint32_t symbol;
  READ_FLAG(symbol, "gci_present_flag");
  cinfo->m_gciPresentFlag = symbol ? true : false;
  if (cinfo->m_gciPresentFlag)
  {
    /* general */
    READ_FLAG(symbol, "gci_intra_only_constraint_flag");
    cinfo->m_intraOnlyConstraintFlag = symbol ? true : false;
    READ_FLAG(symbol, "gci_all_layers_independent_constraint_flag");
    cinfo->m_allLayersIndependentConstraintFlag = symbol ? true : false;
    READ_FLAG(symbol, "gci_one_au_only_constraint_flag");
    cinfo->m_onePictureOnlyConstraintFlag = symbol ? true : false;

    /* picture format */
    READ_CODE(4, symbol, "gci_sixteen_minus_max_bitdepth_constraint_idc");
    cinfo->m_maxBitDepthConstraintIdc = symbol > 8 ? 16 : (16 - symbol);
    CHECK(symbol > 8, "gci_sixteen_minus_max_bitdepth_constraint_idc shall be in the range 0 to 8, inclusive");
    READ_CODE(2, symbol, "gci_three_minus_max_chroma_format_constraint_idc");
    cinfo->m_maxChromaFormatConstraintIdc = (vvc::ChromaFormat)(3 - symbol);

    /* NAL unit type related */
    READ_FLAG(symbol, "gci_no_mixed_nalu_types_in_pic_constraint_flag");
    cinfo->m_noMixedNaluTypesInPicConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_trail_constraint_flag");
    cinfo->m_noTrailConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_stsa_constraint_flag");
    cinfo->m_noStsaConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_rasl_constraint_flag");
    cinfo->m_noRaslConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_radl_constraint_flag");
    cinfo->m_noRadlConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_idr_constraint_flag");
    cinfo->m_noIdrConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_cra_constraint_flag");
    cinfo->m_noCraConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_gdr_constraint_flag");
    cinfo->m_noGdrConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_aps_constraint_flag");
    cinfo->m_noApsConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_idr_rpl_constraint_flag");
    cinfo->m_noIdrRplConstraintFlag = symbol > 0 ? true : false;

    /* tile, slice, subpicture partitioning */
    READ_FLAG(symbol, "gci_one_tile_per_pic_constraint_flag");
    cinfo->m_oneTilePerPicConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_pic_header_in_slice_header_constraint_flag");
    cinfo->m_picHeaderInSliceHeaderConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_one_slice_per_pic_constraint_flag");
    cinfo->m_oneSlicePerPicConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_rectangular_slice_constraint_flag");
    cinfo->m_noRectSliceConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_one_slice_per_subpic_constraint_flag");
    cinfo->m_oneSlicePerSubpicConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_subpic_info_constraint_flag");
    cinfo->m_noSubpicInfoConstraintFlag = symbol > 0 ? true : false;

    /* CTU and block partitioning */
    READ_CODE(2, symbol, "gci_three_minus_max_log2_ctu_size_constraint_idc");
    cinfo->m_maxLog2CtuSizeConstraintIdc = ((3 - symbol) + 5);
    READ_FLAG(symbol, "gci_no_partition_constraints_override_constraint_flag");
    cinfo->m_noPartitionConstraintsOverrideConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_mtt_constraint_flag");
    cinfo->m_noMttConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_qtbtt_dual_tree_intra_constraint_flag");
    cinfo->m_noQtbttDualTreeIntraConstraintFlag = symbol > 0 ? true : false;

    /* intra */
    READ_FLAG(symbol, "gci_no_palette_constraint_flag");
    cinfo->m_noPaletteConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_ibc_constraint_flag");
    cinfo->m_noIbcConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_isp_constraint_flag");
    cinfo->m_noIspConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_mrl_constraint_flag");
    cinfo->m_noMrlConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_mip_constraint_flag");
    cinfo->m_noMipConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_cclm_constraint_flag");
    cinfo->m_noCclmConstraintFlag = symbol > 0 ? true : false;

    /* inter */
    READ_FLAG(symbol, "gci_no_ref_pic_resampling_constraint_flag");
    cinfo->m_noRprConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_res_change_in_clvs_constraint_flag");
    cinfo->m_noResChangeInClvsConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_weighted_prediction_constraint_flag");
    cinfo->m_noWeightedPredictionConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_ref_wraparound_constraint_flag");
    cinfo->m_noRefWraparoundConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_temporal_mvp_constraint_flag");
    cinfo->m_noTemporalMvpConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_sbtmvp_constraint_flag");
    cinfo->m_noSbtmvpConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_amvr_constraint_flag");
    cinfo->m_noAmvrConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_bdof_constraint_flag");
    cinfo->m_noBdofConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_smvd_constraint_flag");
    cinfo->m_noSmvdConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_dmvr_constraint_flag");
    cinfo->m_noDmvrConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_mmvd_constraint_flag");
    cinfo->m_noMmvdConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_affine_motion_constraint_flag");
    cinfo->m_noAffineMotionConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_prof_constraint_flag");
    cinfo->m_noProfConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_bcw_constraint_flag");
    cinfo->m_noBcwConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_ciip_constraint_flag");
    cinfo->m_noCiipConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_gpm_constraint_flag");
    cinfo->m_noGeoConstraintFlag = symbol > 0 ? true : false;

    /* transform, quantization, residual */
    READ_FLAG(symbol, "gci_no_luma_transform_size_64_constraint_flag");
    cinfo->m_noLumaTransformSize64ConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_transform_skip_constraint_flag");
    cinfo->m_noTransformSkipConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_bdpcm_constraint_flag");
    cinfo->m_noBDPCMConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_mts_constraint_flag");
    cinfo->m_noMtsConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_lfnst_constraint_flag");
    cinfo->m_noLfnstConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_joint_cbcr_constraint_flag");
    cinfo->m_noJointCbCrConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_sbt_constraint_flag");
    cinfo->m_noSbtConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_act_constraint_flag");
    cinfo->m_noActConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_explicit_scaling_list_constraint_flag");
    cinfo->m_noExplicitScaleListConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_dep_quant_constraint_flag");
    cinfo->m_noDepQuantConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_sign_data_hiding_constraint_flag");
    cinfo->m_noSignDataHidingConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_cu_qp_delta_constraint_flag");
    cinfo->m_noCuQpDeltaConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_chroma_qp_offset_constraint_flag");
    cinfo->m_noChromaQpOffsetConstraintFlag = symbol > 0 ? true : false;

    /* loop filter */
    READ_FLAG(symbol, "gci_no_sao_constraint_flag");
    cinfo->m_noSaoConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_alf_constraint_flag");
    cinfo->m_noAlfConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_ccalf_constraint_flag");
    cinfo->m_noCCAlfConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_lmcs_constraint_flag");
    cinfo->m_noLmcsConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_ladf_constraint_flag");
    cinfo->m_noLadfConstraintFlag = symbol > 0 ? true : false;
    READ_FLAG(symbol, "gci_no_virtual_boundaries_constraint_flag");
    cinfo->m_noVirtualBoundaryConstraintFlag = symbol > 0 ? true : false;
    READ_CODE(8, symbol, "gci_num_additional_bits");
    uint32_t const numAdditionalBits = symbol;
    int numAdditionalBitsUsed;
    if (numAdditionalBits > 5)
    {
      READ_FLAG(symbol, "gci_all_rap_pictures_flag");
      cinfo->m_allRapPicturesFlag = symbol > 0 ? true : false;
      READ_FLAG(symbol, "gci_no_extended_precision_processing_constraint_flag");
      cinfo->m_noExtendedPrecisionProcessingConstraintFlag = symbol > 0 ? true : false;
      READ_FLAG(symbol, "gci_no_ts_residual_coding_rice_constraint_flag");
      cinfo->m_noTsResidualCodingRiceConstraintFlag = symbol > 0 ? true : false;
      READ_FLAG(symbol, "gci_no_rrc_rice_extension_constraint_flag");
      cinfo->m_noRrcRiceExtensionConstraintFlag = symbol > 0 ? true : false;
      READ_FLAG(symbol, "gci_no_persistent_rice_adaptation_constraint_flag");
      cinfo->m_noPersistentRiceAdaptationConstraintFlag = symbol > 0 ? true : false;
      READ_FLAG(symbol, "gci_no_reverse_last_sig_coeff_constraint_flag");
      cinfo->m_noReverseLastSigCoeffConstraintFlag = symbol > 0 ? true : false;
      numAdditionalBitsUsed = 6;
    }
    else if (numAdditionalBits > 0)
    {
      msg(vvc::ERROR, "Invalid bitstream: gci_num_additional_bits set to value %d (must be 0 or >= 6)\n", numAdditionalBits);
      numAdditionalBitsUsed = 0;
    }
    else
    {
      numAdditionalBitsUsed = 0;
    }
    for (int i = 0; i < (int)numAdditionalBits - numAdditionalBitsUsed; i++)
    {
      READ_FLAG(symbol, "gci_reserved_bit");
    }
  }
  while (!isByteAligned())
  {
    READ_FLAG(symbol, "gci_alignment_zero_bit");
    CHECK(symbol != 0, "gci_alignment_zero_bit not equal to zero");
  }
}

void parseNalH266::parseProfileTierLevel(vvc::ProfileTierLevel *ptl, bool profileTierPresentFlag, int maxNumSubLayersMinus1)
{
  uint32_t symbol;
  if (profileTierPresentFlag)
  {
    READ_CODE(7, symbol, "general_profile_idc");
    ptl->m_profileIdc = vvc::Profile::Name(symbol);
    READ_FLAG(symbol, "general_tier_flag");
    ptl->m_tierFlag = symbol ? vvc::Level::HIGH : vvc::Level::MAIN;
  }

  READ_CODE(8, symbol, "general_level_idc");
  ptl->m_levelIdc = vvc::Level::Name(symbol);
  CHECK(ptl->m_profileIdc != vvc::Profile::NONE && ptl->m_levelIdc < vvc::Level::LEVEL4 && ptl->m_tierFlag == vvc::Level::HIGH,
        "High tier not defined for levels below 4");

  READ_FLAG(symbol, "ptl_frame_only_constraint_flag");
  ptl->m_frameOnlyConstraintFlag = symbol;
  READ_FLAG(symbol, "ptl_multilayer_enabled_flag");
  ptl->m_multiLayerEnabledFlag = symbol;
  CHECK((ptl->m_profileIdc == vvc::Profile::MAIN_10 || ptl->m_profileIdc == vvc::Profile::MAIN_10_444 || ptl->m_profileIdc == vvc::Profile::MAIN_10_STILL_PICTURE || ptl->m_profileIdc == vvc::Profile::MAIN_10_444_STILL_PICTURE) && symbol,
        "ptl_multilayer_enabled_flag shall be equal to 0 for non-multilayer profiles");

  if (profileTierPresentFlag)
  {
    parseConstraintInfo(&ptl->m_constraintInfo, ptl);
  }

  for (int i = maxNumSubLayersMinus1 - 1; i >= 0; i--)
  {
    READ_FLAG(symbol, "sub_layer_level_present_flag[i]");
    ptl->m_subLayerLevelPresentFlag[i] = symbol;
  }

  while (!isByteAligned())
  {
    READ_FLAG(symbol, "ptl_reserved_zero_bit");
    CHECK(symbol != 0, "ptl_reserved_zero_bit not equal to zero");
  }

  for (int i = maxNumSubLayersMinus1 - 1; i >= 0; i--)
  {
    if (ptl->m_subLayerLevelPresentFlag[i])
    {
      READ_CODE(8, symbol, "sub_layer_level_idc");
      ptl->m_subLayerLevelIdc[i] = vvc::Level::Name(symbol);
    }
  }
  ptl->m_subLayerLevelIdc[maxNumSubLayersMinus1] = ptl->m_levelIdc;
  for (int i = maxNumSubLayersMinus1 - 1; i >= 0; i--)
  {
    if (!ptl->m_subLayerLevelPresentFlag[i])
    {
      ptl->m_subLayerLevelIdc[i] = ptl->m_subLayerLevelIdc[i + 1];
    }
  }

  if (profileTierPresentFlag)
  {
    READ_CODE(8, symbol, "ptl_num_sub_profiles");
    uint8_t numSubProfiles = symbol;
    ptl->m_numSubProfile = numSubProfiles;
    for (int i = 0; i < numSubProfiles; i++)
    {
      READ_CODE(32, symbol, "general_sub_profile_idc[i]");
      ptl->m_subProfileIdc[i] = symbol;
    }
  }
}

void parseNalH266::parseTerminatingBit(uint32_t &ruiBit)
{
  ruiBit = false;
  int iBitsLeft = m_pcBitstream->getNumBitsLeft();
  if (iBitsLeft <= 8)
  {
    uint32_t uiPeekValue = m_pcBitstream->peekBits(iBitsLeft);
    if ((int)uiPeekValue == (1 << (iBitsLeft - 1)))
    {
      ruiBit = true;
    }
  }
}

void parseNalH266::parseRemainingBytes(bool noTrailingBytesExpected)
{
  if (noTrailingBytesExpected)
  {
    CHECK(0 != m_pcBitstream->getNumBitsLeft(), "Bits left although no bits expected");
  }
  else
  {
    while (m_pcBitstream->getNumBitsLeft())
    {
      uint32_t trailingNullByte = m_pcBitstream->readByte();
      if (trailingNullByte != 0)
      {
        msg(vvc::ERROR, "Trailing byte should be 0, but has value %02x\n", trailingNullByte);
        THROW("Invalid trailing '0' byte");
      }
    }
  }
}

void parseNalH266::parseScalingList(vvc::ScalingList *scalingList, bool aps_chromaPrsentFlag)
{
  uint32_t code;
  bool scalingListCopyModeFlag;
  scalingList->m_chromaScalingListPresentFlag = aps_chromaPrsentFlag;
  for (int scalingListId = 0; scalingListId < 28; scalingListId++)
  {
    if (aps_chromaPrsentFlag || scalingList->isLumaScalingList(scalingListId))
    {
      READ_FLAG(code, "scaling_list_copy_mode_flag");
      scalingListCopyModeFlag = (code) ? true : false;
      scalingList->m_scalingListPredModeFlagIsCopy[scalingListId] = scalingListCopyModeFlag;
      scalingList->m_scalingListPreditorModeFlag[scalingListId] = false;
      if (!scalingListCopyModeFlag)
      {
        READ_FLAG(code, "scaling_list_predictor_mode_flag");
        scalingList->m_scalingListPredModeFlagIsCopy[scalingListId] = code;
      }

      if ((scalingListCopyModeFlag || scalingList->m_scalingListPreditorModeFlag[scalingListId]) && scalingListId != vvc::SCALING_LIST_1D_START_2x2 && scalingListId != vvc::SCALING_LIST_1D_START_4x4 && scalingListId != vvc::SCALING_LIST_1D_START_8x8) // Copy Mode
      {
        READ_UVLC(code, "scaling_list_pred_matrix_id_delta");
        scalingList->m_refMatrixId[scalingListId] = (uint32_t)((int)(scalingListId) - (code));
      }
      else if (scalingListCopyModeFlag || scalingList->m_scalingListPreditorModeFlag[scalingListId])
      {
        scalingList->m_refMatrixId[scalingListId] = (uint32_t)((int)(scalingListId));
      }
      if (scalingListCopyModeFlag) // copy
      {
        if (scalingListId >= vvc::SCALING_LIST_1D_START_16x16)
        {
          scalingList->m_scalingListDC[scalingListId] = (scalingListId == (int)scalingList->m_refMatrixId[scalingListId]) ? 16
                                                        : (scalingList->m_refMatrixId[scalingListId] < vvc::SCALING_LIST_1D_START_16x16)
                                                            ? scalingList->getScalingListAddress(scalingList->m_refMatrixId[scalingListId])[0]
                                                            : scalingList->getScalingListDC(scalingList->m_refMatrixId[scalingListId]);
        }
        scalingList->processRefMatrix(scalingListId, scalingList->m_refMatrixId[scalingListId]);
      }
      else
      {
        decodeScalingList(scalingList, scalingListId, scalingList->m_scalingListPreditorModeFlag[scalingListId]);
      }
    }
    else
    {
      scalingListCopyModeFlag = true;
      scalingList->m_scalingListPredModeFlagIsCopy[scalingListId] = scalingListCopyModeFlag;
      scalingList->m_refMatrixId[scalingListId] = (uint32_t)((int)(scalingListId));
      if (scalingListId >= vvc::SCALING_LIST_1D_START_16x16)
      {
        scalingList->m_scalingListDC[scalingListId] = 16;
      }
      scalingList->processRefMatrix(scalingListId, scalingList->m_refMatrixId[scalingListId]);
    }
  }

  return;
}

void parseNalH266::decodeScalingList(vvc::ScalingList *scalingList, uint32_t scalingListId, bool isPredictor)
{
  int matrixSize = (scalingListId < vvc::SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < vvc::SCALING_LIST_1D_START_8x8) ? 4
                                                                                                                           : 8;
  int i, coefNum = matrixSize * matrixSize;
  int data;
  int scalingListDcCoefMinus8 = 0;
  int nextCoef = (isPredictor) ? 0 : vvc::SCALING_LIST_START_VALUE;
  // vvc::ScanElement *scan = vvc::g_scanOrder[vvc::SCAN_UNGROUPED][vvc::SCAN_DIAG][0][0];
  // int *dst = scalingList->getScalingListAddress(scalingListId);

  int PredListId = scalingList->m_refMatrixId[scalingListId];
  CHECK(isPredictor && PredListId > (int)scalingListId, "Scaling List error predictor!");
  const int *srcPred = (isPredictor)
                           ? (((int)scalingListId == PredListId) ? scalingList->getScalingListDefaultAddress(scalingListId)
                                                                 : scalingList->getScalingListAddress(PredListId))
                           : nullptr;
  if (isPredictor && (int)scalingListId == PredListId)
  {
    scalingList->m_scalingListDC[PredListId] = vvc::SCALING_LIST_DC;
  }
  int predCoef = 0;

  if (scalingListId >= vvc::SCALING_LIST_1D_START_16x16)
  {
    READ_SVLC(scalingListDcCoefMinus8, "scaling_list_dc_coef_minus8");
    nextCoef += scalingListDcCoefMinus8;
    if (isPredictor)
    {
      predCoef = (PredListId >= vvc::SCALING_LIST_1D_START_16x16) ? scalingList->getScalingListDC(PredListId) : srcPred[0];
    }
    scalingList->m_scalingListDC[scalingListId] = (nextCoef + predCoef + 256) & 255;
  }

  for (i = 0; i < coefNum; i++)
  {
    // if (scalingListId >= vvc::SCALING_LIST_1D_START_64x64 && scan[i].x >= 4 && scan[i].y >= 4)
    // {
    //   dst[scan[i].idx] = 0;
    //   continue;
    // }
    READ_SVLC(data, "scaling_list_delta_coef");
    nextCoef += data;
    // predCoef = (isPredictor) ? srcPred[scan[i].idx] : 0;
    // dst[scan[i].idx] = (nextCoef + predCoef + 256) & 255;
  }
}

bool parseNalH266::xMoreRbspData()
{
  int bitsLeft = m_pcBitstream->getNumBitsLeft();

  // if there are more than 8 bits, it cannot be rbsp_trailing_bits
  if (bitsLeft > 8)
  {
    return true;
  }

  uint8_t lastByte = m_pcBitstream->peekBits(bitsLeft);
  int cnt = bitsLeft;

  // remove trailing bits equal to zero
  while ((cnt > 0) && ((lastByte & 1) == 0))
  {
    lastByte >>= 1;
    cnt--;
  }
  // remove bit equal to one
  cnt--;

  // we should not have a negative number of bits
  CHECK(cnt < 0, "Negative number of bits");

  // we have more data, if cnt is not zero
  return (cnt > 0);
}

void parseNalH266::alfFilter(vvc::AlfParam &alfParam, const bool isChroma, const int altIdx)
{
  uint32_t code;

  // derive maxGolombIdx
  vvc::AlfFilterShape alfShape(isChroma ? 5 : 7);
  const int numFilters = isChroma ? 1 : alfParam.numLumaFilters;
  short *coeff = isChroma ? alfParam.chromaCoeff[altIdx] : alfParam.lumaCoeff;
  uint16_t *clipp = isChroma ? alfParam.chromaClipp[altIdx] : alfParam.lumaClipp;

  // Filter coefficients
  for (int ind = 0; ind < numFilters; ++ind)
  {
    for (int i = 0; i < alfShape.numCoeff - 1; i++)
    {
      READ_UVLC(code, isChroma ? "alf_chroma_coeff_abs" : "alf_luma_coeff_abs");
      coeff[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i] = code;
      if (coeff[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i] != 0)
      {
        READ_FLAG(code, isChroma ? "alf_chroma_coeff_sign" : "alf_luma_coeff_sign");
        coeff[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i] = (code) ? -coeff[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i] : coeff[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i];
      }
      CHECK(isChroma &&
                (coeff[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i] > 127 || coeff[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i] < -128),
            "AlfCoeffC shall be in the range of -128 to 127, inclusive");
    }
  }

  // Clipping values coding
  if (alfParam.nonLinearFlag[isChroma])
  {
    // Filter coefficients
    for (int ind = 0; ind < numFilters; ++ind)
    {
      for (int i = 0; i < alfShape.numCoeff - 1; i++)
      {
        READ_CODE(2, code, isChroma ? "alf_chroma_clip_idx" : "alf_luma_clip_idx");
        clipp[ind * vvc::MAX_NUM_ALF_LUMA_COEFF + i] = code;
      }
    }
  }
  else
  {
    for (int ind = 0; ind < numFilters; ++ind)
    {
      std::fill_n(clipp + ind * vvc::MAX_NUM_ALF_LUMA_COEFF, alfShape.numCoeff, 0);
    }
  }
}


void parseNalH266::sei_parse(unsigned char *nal_bitstream, nal_info &nal, int curLen)
{
  /*
  int payloadType = 0;
  uint32_t val = 0;

  do
  {
    sei_read_scode(8, val, "payload_type");
    payloadType += val;
  } while (val == 0xFF);

  uint32_t payloadSize = 0;
  do
  {
    sei_read_scode(8, val, "payload_size");
    payloadSize += val;
  } while (val == 0xFF);

  InputBitstream *bs = getBitstream();
  setBitstream(bs->extractSubstream(payloadSize * 8));

  SEI *sei = nullptr;
  const SEIBufferingPeriod *bp = nullptr;

  if (nal.nal_unit_type == vvc::NAL_UNIT_PREFIX_SEI)
  {
    switch (payloadType)
    {
    case SEI::USER_DATA_UNREGISTERED:
      sei = new SEIuserDataUnregistered;
      xParseSEIuserDataUnregistered((SEIuserDataUnregistered &)*sei, payloadSize, pDecodedMessageOutputStream);
      break;
    // case SEI::DECODING_UNIT_INFO:
    //   bp = hrd.getBufferingPeriodSEI();
    //   if (!bp)
    //   {
    //     msg(WARNING, "Warning: Found Decoding unit information SEI message, but no active buffering period is available. Ignoring.");
    //   }
    //   else
    //   {
    //     sei = new SEIDecodingUnitInfo;
    //     xParseSEIDecodingUnitInfo((SEIDecodingUnitInfo &)*sei, payloadSize, *bp, temporalId, pDecodedMessageOutputStream);
    //   }
    //   break;
    // case SEI::BUFFERING_PERIOD:
    //   sei = new SEIBufferingPeriod;
    //   xParseSEIBufferingPeriod((SEIBufferingPeriod &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   hrd.setBufferingPeriodSEI((SEIBufferingPeriod *)sei);
    //   break;
    case SEI::PICTURE_TIMING:
    {
      bp = hrd.getBufferingPeriodSEI();
      if (!bp)
      {
        vvc::msg(vvc::WARNING, "Warning: Found Picture timing SEI message, but no active buffering period is available. Ignoring.");
      }
      else
      {
        sei = new SEIPictureTimingH266;
        xParseSEIPictureTiming((SEIPictureTimingH266 &)*sei, payloadSize, temporalId, *bp, pDecodedMessageOutputStream);
        hrd.setPictureTimingSEI((SEIPictureTimingH266 *)sei);
      }
    }
    break;
    // case SEI::SCALABLE_NESTING:
    //   sei = new SEIScalableNesting;
    //   xParseSEIScalableNesting((SEIScalableNesting &)*sei, nalUnitType, nuh_layer_id, payloadSize, vps, sps, hrd, pDecodedMessageOutputStream);
    //   break;
    // case SEI::FRAME_FIELD_INFO:
    //   sei = new SEIFrameFieldInfo;
    //   xParseSEIFrameFieldinfo((SEIFrameFieldInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::DEPENDENT_RAP_INDICATION:
    //   sei = new SEIDependentRAPIndication;
    //   xParseSEIDependentRAPIndication((SEIDependentRAPIndication &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::EXTENDED_DRAP_INDICATION:
    //   sei = new SEIExtendedDrapIndication;
    //   xParseSEIExtendedDrapIndication((SEIExtendedDrapIndication &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::FRAME_PACKING:
    //   sei = new SEIFramePacking;
    //   xParseSEIFramePacking((SEIFramePacking &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::DISPLAY_ORIENTATION:
    //   sei = new SEIDisplayOrientation;
    //   xParseSEIDisplayOrientation((SEIDisplayOrientation &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::ANNOTATED_REGIONS:
    //   sei = new SEIAnnotatedRegions;
    //   xParseSEIAnnotatedRegions((SEIAnnotatedRegions &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::PARAMETER_SETS_INCLUSION_INDICATION:
    //   sei = new SEIParameterSetsInclusionIndication;
    //   xParseSEIParameterSetsInclusionIndication((SEIParameterSetsInclusionIndication &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    case SEI::MASTERING_DISPLAY_COLOUR_VOLUME:
      sei = new SEIMasteringDisplayColourVolume;
      xParseSEIMasteringDisplayColourVolume((SEIMasteringDisplayColourVolume &)*sei, payloadSize, pDecodedMessageOutputStream);
      break;
    // case SEI::EQUIRECTANGULAR_PROJECTION:
    //   sei = new SEIEquirectangularProjection;
    //   xParseSEIEquirectangularProjection((SEIEquirectangularProjection &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::SPHERE_ROTATION:
    //   sei = new SEISphereRotation;
    //   xParseSEISphereRotation((SEISphereRotation &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::OMNI_VIEWPORT:
    //   sei = new SEIOmniViewport;
    //   xParseSEIOmniViewport((SEIOmniViewport &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::REGION_WISE_PACKING:
    //   sei = new SEIRegionWisePacking;
    //   xParseSEIRegionWisePacking((SEIRegionWisePacking &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::GENERALIZED_CUBEMAP_PROJECTION:
    //   sei = new SEIGeneralizedCubemapProjection;
    //   xParseSEIGeneralizedCubemapProjection((SEIGeneralizedCubemapProjection &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::SCALABILITY_DIMENSION_INFO:
    //   sei = new SEIScalabilityDimensionInfo;
    //   xParseSEIScalabilityDimensionInfo((SEIScalabilityDimensionInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::GREEN_METADATA:
    //   sei = new SEIGreenMetadataInfo;
    //   xParseSEIGreenMetadataInfo((SEIGreenMetadataInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::MULTIVIEW_ACQUISITION_INFO:
    //   sei = new SEIMultiviewAcquisitionInfo;
    //   xParseSEIMultiviewAcquisitionInfo((SEIMultiviewAcquisitionInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::MULTIVIEW_VIEW_POSITION:
    //   sei = new SEIMultiviewViewPosition;
    //   xParseSEIMultiviewViewPosition((SEIMultiviewViewPosition &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::ALPHA_CHANNEL_INFO:
    //   sei = new SEIAlphaChannelInfo;
    //   xParseSEIAlphaChannelInfo((SEIAlphaChannelInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::DEPTH_REPRESENTATION_INFO:
    //   sei = new SEIDepthRepresentationInfo;
    //   xParseSEIDepthRepresentationInfo((SEIDepthRepresentationInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::SUBPICTURE_LEVEL_INFO:
    //   sei = new SEISubpicureLevelInfo;
    //   xParseSEISubpictureLevelInfo((SEISubpicureLevelInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::SAMPLE_ASPECT_RATIO_INFO:
    //   sei = new SEISampleAspectRatioInfo;
    //   xParseSEISampleAspectRatioInfo((SEISampleAspectRatioInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    case SEI::USER_DATA_REGISTERED_ITU_T_T35:
      sei = new SEIUserDataRegistered;
      xParseSEIUserDataRegistered((SEIUserDataRegistered &)*sei, payloadSize, pDecodedMessageOutputStream);
      break;
    // case SEI::FILM_GRAIN_CHARACTERISTICS:
    //   sei = new SEIFilmGrainCharacteristics;
    //   xParseSEIFilmGrainCharacteristics((SEIFilmGrainCharacteristics &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    case SEI::CONTENT_LIGHT_LEVEL_INFO:
      sei = new SEIContentLightLevelInfo;
      xParseSEIContentLightLevelInfo((SEIContentLightLevelInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
      break;
    // case SEI::AMBIENT_VIEWING_ENVIRONMENT:
    //   sei = new SEIAmbientViewingEnvironment;
    //   xParseSEIAmbientViewingEnvironment((SEIAmbientViewingEnvironment &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::CONTENT_COLOUR_VOLUME:
    //   sei = new SEIContentColourVolume;
    //   xParseSEIContentColourVolume((SEIContentColourVolume &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::COLOUR_TRANSFORM_INFO:
    //   sei = new SEIColourTransformInfo;
    //   xParseSEIColourTransformInfo((SEIColourTransformInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::CONSTRAINED_RASL_ENCODING:
    //   sei = new SEIConstrainedRaslIndication;
    //   xParseSEIConstrainedRaslIndication((SEIConstrainedRaslIndication &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::SHUTTER_INTERVAL_INFO:
    //   sei = new SEIShutterIntervalInfo;
    //   xParseSEIShutterInterval((SEIShutterIntervalInfo &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::NEURAL_NETWORK_POST_FILTER_CHARACTERISTICS:
    //   sei = new SEINeuralNetworkPostFilterCharacteristics;
    //   xParseSEINNPostFilterCharacteristics((SEINeuralNetworkPostFilterCharacteristics &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::NEURAL_NETWORK_POST_FILTER_ACTIVATION:
    //   sei = new SEINeuralNetworkPostFilterActivation;
    //   xParseSEINNPostFilterActivation((SEINeuralNetworkPostFilterActivation &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    default:
      for (uint32_t i = 0; i < payloadSize; i++)
      {
        uint32_t seiByte;
        sei_read_scode(8, seiByte, "unknown prefix SEI payload byte");
      }
      vvc::msg(vvc::WARNING, "Unknown prefix SEI message (payloadType = %d) was found!\n", payloadType);
      if (pDecodedMessageOutputStream)
      {
        (*pDecodedMessageOutputStream) << "Unknown prefix SEI message (payloadType = " << payloadType << ") was found!\n";
      }
      break;
    }
  }
  else
  {
    switch (payloadType)
    {
    case SEI::USER_DATA_UNREGISTERED:
      sei = new SEIuserDataUnregistered;
      xParseSEIuserDataUnregistered((SEIuserDataUnregistered &)*sei, payloadSize, pDecodedMessageOutputStream);
      break;
    // case SEI::DECODED_PICTURE_HASH:
    //   sei = new SEIDecodedPictureHash;
    //   xParseSEIDecodedPictureHash((SEIDecodedPictureHash &)*sei, payloadSize, pDecodedMessageOutputStream);
    //   break;
    // case SEI::SCALABLE_NESTING:
    //   sei = new SEIScalableNesting;
    //   xParseSEIScalableNesting((SEIScalableNesting &)*sei, nalUnitType, nuh_layer_id, payloadSize, vps, sps, hrd, pDecodedMessageOutputStream);
    //   break;
    default:
      for (uint32_t i = 0; i < payloadSize; i++)
      {
        uint32_t seiByte;
        sei_read_scode(8, seiByte, "unknown suffix SEI payload byte");
      }
      vvc::msg(vvc::WARNING, "Unknown suffix SEI message (payloadType = %d) was found!\n", payloadType);
      break;
    }
  }

  if (sei != nullptr)
  {
    seis.push_back(sei);
  }

  int payloadBitsRemaining = getBitstream()->getNumBitsLeft();
  if (payloadBitsRemaining)
  {
    for (; payloadBitsRemaining > 9; payloadBitsRemaining--)
    {
      uint32_t reservedPayloadExtensionData;
      sei_read_scode(1, reservedPayloadExtensionData, "reserved_payload_extension_data");
    }

    int finalBits = getBitstream()->peekBits(payloadBitsRemaining);
    int finalPayloadBits = 0;
    for (int mask = 0xff; finalBits & (mask >> finalPayloadBits); finalPayloadBits++)
    {
      continue;
    }

    for (; payloadBitsRemaining > 9 - finalPayloadBits; payloadBitsRemaining--)
    {
      uint32_t reservedPayloadExtensionData;
      sei_read_flag(0, reservedPayloadExtensionData, "reserved_payload_extension_data");
    }

    uint32_t dummy;
    sei_read_flag(0, dummy, "payload_bit_equal_to_one");
    payloadBitsRemaining--;
    while (payloadBitsRemaining)
    {
      sei_read_flag(0, dummy, "payload_bit_equal_to_zero");
      payloadBitsRemaining--;
    }
  }

  delete getBitstream();
  setBitstream(bs);
  */
}