#include "vvc_param_set.h"
#include "vvc_vlc.h"
#include "vvc_type.h"

#include <limits>

static const vvc::LevelTierFeatures mainLevelTierInfo[] =
    {
        //  level, maxlumaps, maxcpb[tier], , maxSlicesPerAu, maxTilesPerAu, cols, maxLumaSr, maxBr[tier], , minCr[tier], ,
        {vvc::Level::LEVEL1, 36864, {350, 0}, 16, 1, 1, 552960ULL, {128, 0}, {2, 0}},
        {vvc::Level::LEVEL2, 122880, {1500, 0}, 16, 1, 1, 3686400ULL, {1500, 0}, {2, 0}},
        {vvc::Level::LEVEL2_1, 245760, {3000, 0}, 20, 1, 1, 7372800ULL, {3000, 0}, {2, 0}},
        {vvc::Level::LEVEL3, 552960, {6000, 0}, 30, 4, 2, 16588800ULL, {6000, 0}, {2, 0}},
        {vvc::Level::LEVEL3_1, 983040, {10000, 0}, 40, 9, 3, 33177600ULL, {10000, 0}, {2, 0}},
        {vvc::Level::LEVEL4, 2228224, {12000, 30000}, 75, 25, 5, 66846720ULL, {12000, 30000}, {4, 4}},
        {vvc::Level::LEVEL4_1, 2228224, {20000, 50000}, 75, 25, 5, 133693440ULL, {20000, 50000}, {4, 4}},
        {vvc::Level::LEVEL5, 8912896, {25000, 100000}, 200, 110, 10, 267386880ULL, {25000, 100000}, {6, 4}},
        {vvc::Level::LEVEL5_1, 8912896, {40000, 160000}, 200, 110, 10, 534773760ULL, {40000, 160000}, {8, 4}},
        {vvc::Level::LEVEL5_2, 8912896, {60000, 240000}, 200, 110, 10, 1069547520ULL, {60000, 240000}, {8, 4}},
        {vvc::Level::LEVEL6, 35651584, {80000, 240000}, 600, 440, 20, 1069547520ULL, {60000, 240000}, {8, 4}},
        {vvc::Level::LEVEL6_1, 35651584, {120000, 480000}, 600, 440, 20, 2139095040ULL, {120000, 480000}, {8, 4}},
        {vvc::Level::LEVEL6_2, 35651584, {180000, 800000}, 600, 440, 20, 4278190080ULL, {240000, 800000}, {8, 4}},
        {vvc::Level::LEVEL6_3, 80216064, {240000, 1600000}, 1000, 990, 30, 4812963840ULL, {320000, 1600000}, {8, 4}},
        {vvc::Level::LEVEL15_5, 0xFFFFFFFFU, {0xFFFFFFFFU, 0xFFFFFFFFU}, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, std::numeric_limits<uint64_t>::max(), {0xFFFFFFFFU, 0xFFFFFFFFU}, {0, 0}},
        {vvc::Level::NONE}};

static const vvc::ProfileFeatures validProfiles[] = {
    // profile, pNameString, maxBitDepth, maxChrFmt, lvl15.5, cpbvcl, cpbnal, fcf*1000, mincr*100, levelInfo
    // most constrained profiles must appear first.
    {vvc::Profile::MAIN_10_STILL_PICTURE, "Main_10_Still_Picture", 10, vvc::CHROMA_420, true, 1000, 1100, 1875, 100,
     mainLevelTierInfo, true},
    {vvc::Profile::MULTILAYER_MAIN_10_STILL_PICTURE, "Multilayer_Main_10_Still_Picture", 10, vvc::CHROMA_420, true, 1000, 1100,
     1875, 100, mainLevelTierInfo, true},
    {vvc::Profile::MAIN_10_444_STILL_PICTURE, "Main_444_10_Still_Picture", 10, vvc::CHROMA_444, true, 2500, 2750, 3750, 75,
     mainLevelTierInfo, true},
    {vvc::Profile::MULTILAYER_MAIN_10_444_STILL_PICTURE, "Multilayer_Main_444_10_Still_Picture", 10, vvc::CHROMA_444, true, 2500,
     2750, 3750, 75, mainLevelTierInfo, true},
    {vvc::Profile::MAIN_10, "Main_10", 10, vvc::CHROMA_420, true, 1000, 1100, 1875, 100, mainLevelTierInfo, false},
    {vvc::Profile::MULTILAYER_MAIN_10, "Multilayer_Main_10", 10, vvc::CHROMA_420, true, 1000, 1100, 1875, 100, mainLevelTierInfo,
     false},
    {vvc::Profile::MAIN_10_444, "Main_444_10", 10, vvc::CHROMA_444, true, 2500, 2750, 3750, 75, mainLevelTierInfo, false},
    {vvc::Profile::MULTILAYER_MAIN_10_444, "Multilayer_Main_444_10", 10, vvc::CHROMA_444, true, 2500, 2750, 3750, 75,
     mainLevelTierInfo, false},
    {vvc::Profile::MAIN_12, "Main_12", 12, vvc::CHROMA_420, true, 1200, 1320, 1875, 100, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_12_INTRA, "Main_12_Intra", 12, vvc::CHROMA_420, true, 2400, 2640, 1875, 100, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_12_STILL_PICTURE, "Main_12_Still_Picture", 12, vvc::CHROMA_420, true, 2400, 2640, 1875, 100, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_12_444, "Main_12_444", 12, vvc::CHROMA_444, true, 3000, 3300, 3750, 75, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_12_444_INTRA, "Main_12_444_Intra", 12, vvc::CHROMA_444, true, 6000, 6600, 3750, 75, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_12_444_STILL_PICTURE, "Main_12_444_Still_Picture", 12, vvc::CHROMA_444, true, 6000, 6600, 3750, 75, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_16_444, "Main_16_444", 16, vvc::CHROMA_444, true, 4000, 4400, 6000, 75, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_16_444_INTRA, "Main_16_444_Intra", 16, vvc::CHROMA_444, true, 8000, 8800, 6000, 75, mainLevelTierInfo, false},
    {vvc::Profile::MAIN_16_444_STILL_PICTURE, "Main_16_444_Still_Picture", 16, vvc::CHROMA_444, true, 8000, 8800, 6000, 75, mainLevelTierInfo, false},
    {vvc::Profile::NONE, 0},
};

const vvc::ProfileFeatures *vvc::ProfileFeatures::getProfileFeatures(const vvc::Profile::Name p)
{
  int i;
  for (i = 0; validProfiles[i].profile != vvc::Profile::NONE; i++)
  {
    if (validProfiles[i].profile == p)
    {
      return &validProfiles[i];
    }
  }

  return &validProfiles[i];
}

void vvc::PPS::resetTileSliceInfo()
{
  m_numExpTileCols = 0;
  m_numExpTileRows = 0;
  m_numTileCols = 0;
  m_numTileRows = 0;
  m_numSlicesInPic = 0;
  m_tileColWidth.clear();
  m_tileRowHeight.clear();
  m_tileColBd.clear();
  m_tileRowBd.clear();
  m_ctuToTileCol.clear();
  m_ctuToTileRow.clear();
  m_ctuToSubPicIdx.clear();
  m_rectSlices.clear();
  m_sliceMap.clear();
}

void vvc::PPS::initTiles()
{
  int colIdx, rowIdx;
  int ctuX, ctuY;

  // check explicit tile column sizes
  uint32_t remainingWidthInCtu = m_picWidthInCtu;

  for (colIdx = 0; colIdx < (int)m_numExpTileCols; colIdx++)
  {
    CHECK(m_tileColWidth[colIdx] > remainingWidthInCtu, "Tile column width exceeds picture width");
    remainingWidthInCtu -= m_tileColWidth[colIdx];
  }

  // divide remaining picture width into uniform tile columns
  uint32_t uniformTileColWidth = m_tileColWidth[colIdx - 1];
  while (remainingWidthInCtu > 0)
  {
    CHECK(colIdx >= MAX_TILE_COLS, "Number of tile columns exceeds valid range");
    uniformTileColWidth = std::min(remainingWidthInCtu, uniformTileColWidth);
    m_tileColWidth.push_back(uniformTileColWidth);
    remainingWidthInCtu -= uniformTileColWidth;
    colIdx++;
  }
  m_numTileCols = colIdx;

  // check explicit tile row sizes
  uint32_t remainingHeightInCtu = m_picHeightInCtu;

  for (rowIdx = 0; rowIdx < (int)m_numExpTileRows; rowIdx++)
  {
    CHECK(m_tileRowHeight[rowIdx] > remainingHeightInCtu, "Tile row height exceeds picture height");
    remainingHeightInCtu -= m_tileRowHeight[rowIdx];
  }

  // divide remaining picture height into uniform tile rows
  uint32_t uniformTileRowHeight = m_tileRowHeight[rowIdx - 1];
  while (remainingHeightInCtu > 0)
  {
    uniformTileRowHeight = std::min(remainingHeightInCtu, uniformTileRowHeight);
    m_tileRowHeight.push_back(uniformTileRowHeight);
    remainingHeightInCtu -= uniformTileRowHeight;
    rowIdx++;
  }
  m_numTileRows = rowIdx;

  // set left column bounaries
  m_tileColBd.push_back(0);
  for (colIdx = 0; colIdx < (int)m_numTileCols; colIdx++)
  {
    m_tileColBd.push_back(m_tileColBd[colIdx] + m_tileColWidth[colIdx]);
  }

  // set top row bounaries
  m_tileRowBd.push_back(0);
  for (rowIdx = 0; rowIdx < (int)m_numTileRows; rowIdx++)
  {
    m_tileRowBd.push_back(m_tileRowBd[rowIdx] + m_tileRowHeight[rowIdx]);
  }

  // set mapping between horizontal CTU address and tile column index
  colIdx = 0;
  for (ctuX = 0; ctuX <= (int)m_picWidthInCtu; ctuX++)
  {
    if (ctuX == (int)m_tileColBd[colIdx + 1])
    {
      colIdx++;
    }
    m_ctuToTileCol.push_back(colIdx);
  }

  // set mapping between vertical CTU address and tile row index
  rowIdx = 0;
  for (ctuY = 0; ctuY <= (int)m_picHeightInCtu; ctuY++)
  {
    if (ctuY == (int)m_tileRowBd[rowIdx + 1])
    {
      rowIdx++;
    }
    m_ctuToTileRow.push_back(rowIdx);
  }
}

void vvc::PPS::initRectSlices()
{
  CHECK(m_numSlicesInPic > MAX_SLICES, "Number of slices in picture exceeds valid range");
  m_rectSlices.resize(m_numSlicesInPic);
}

void vvc::PPS::setChromaQpOffsetListEntry(int cuChromaQpOffsetIdxPlus1, int cbOffset, int crOffset, int jointCbCrOffset)
{
  CHECK(cuChromaQpOffsetIdxPlus1 == 0 || cuChromaQpOffsetIdxPlus1 > MAX_QP_OFFSET_LIST_SIZE, "Invalid chroma QP offset");
  m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdxPlus1].u.comp.CbOffset = cbOffset; // Array includes entry [0] for the null offset used when cu_chroma_qp_offset_flag=0, and entries [cu_chroma_qp_offset_idx+1...] otherwise
  m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdxPlus1].u.comp.CrOffset = crOffset;
  m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdxPlus1].u.comp.JointCbCrOffset = jointCbCrOffset;
  m_chromaQpOffsetListLen = std::max(m_chromaQpOffsetListLen, cuChromaQpOffsetIdxPlus1);
}

void vvc::ReferencePictureList::setRefPicIdentifier(int idx, int identifier, bool isLongterm, bool isInterLayerRefPic, int interLayerIdx)
{
  m_refPicIdentifier[idx] = identifier;
  m_isLongtermRefPic[idx] = isLongterm;

  m_deltaPocMSBPresentFlag[idx] = false;
  m_deltaPOCMSBCycleLT[idx] = 0;

  m_isInterLayerRefPic[idx] = isInterLayerRefPic;
  m_interLayerRefPicIdx[idx] = interLayerIdx;
}

void vvc::AlfParam::reset()
{
  std::memset(enabledFlag, false, sizeof(enabledFlag));
  std::memset(nonLinearFlag, false, sizeof(nonLinearFlag));
  std::memset(lumaCoeff, 0, sizeof(lumaCoeff));
  std::memset(lumaClipp, 0, sizeof(lumaClipp));
  numAlternativesChroma = 1;
  std::memset(chromaCoeff, 0, sizeof(chromaCoeff));
  std::memset(chromaClipp, 0, sizeof(chromaClipp));
  std::memset(filterCoeffDeltaIdx, 0, sizeof(filterCoeffDeltaIdx));
  std::memset(alfLumaCoeffFlag, true, sizeof(alfLumaCoeffFlag));
  numLumaFilters = 1;
  alfLumaCoeffDeltaFlag = false;
  memset(newFilterFlag, 0, sizeof(newFilterFlag));
}

void vvc::ChromaQpMappingTable::setParams(const vvc::ChromaQpMappingTableParams &params, const int qpBdOffset)
{
  m_qpBdOffset = qpBdOffset;
  m_sameCQPTableForAllChromaFlag = params.m_sameCQPTableForAllChromaFlag;
  m_numQpTables = params.m_numQpTables;

  for (int i = 0; i < MAX_NUM_CQP_MAPPING_TABLES; i++)
  {
    m_numPtsInCQPTableMinus1[i] = params.m_numPtsInCQPTableMinus1[i];
    m_deltaQpInValMinus1[i] = params.m_deltaQpInValMinus1[i];
    m_qpTableStartMinus26[i] = params.m_qpTableStartMinus26[i];
    m_deltaQpOutVal[i] = params.m_deltaQpOutVal[i];
  }
}

void vvc::ChromaQpMappingTable::derivedChromaQPMappingTables()
{
  for (int i = 0; i < getNumQpTables(); i++)
  {
    const int qpBdOffsetC = m_qpBdOffset;
    const int numPtsInCQPTableMinus1 = getNumPtsInCQPTableMinus1(i);
    std::vector<int> qpInVal(numPtsInCQPTableMinus1 + 2), qpOutVal(numPtsInCQPTableMinus1 + 2);

    qpInVal[0] = getQpTableStartMinus26(i) + 26;
    qpOutVal[0] = qpInVal[0];
    for (int j = 0; j <= getNumPtsInCQPTableMinus1(i); j++)
    {
      qpInVal[j + 1] = qpInVal[j] + getDeltaQpInValMinus1(i, j) + 1;
      qpOutVal[j + 1] = qpOutVal[j] + getDeltaQpOutVal(i, j);
    }

    for (int j = 0; j <= getNumPtsInCQPTableMinus1(i); j++)
    {
      CHECK(qpInVal[j] < -qpBdOffsetC || qpInVal[j] > vvc::MAX_QP, "qpInVal out of range");
      CHECK(qpOutVal[j] < -qpBdOffsetC || qpOutVal[j] > vvc::MAX_QP, "qpOutVal out of range");
    }

    m_chromaQpMappingTables[i][qpInVal[0]] = qpOutVal[0];
    for (int k = qpInVal[0] - 1; k >= -qpBdOffsetC; k--)
    {
      m_chromaQpMappingTables[i][k] = Clip3(-qpBdOffsetC, MAX_QP, m_chromaQpMappingTables[i][k + 1] - 1);
    }
    for (int j = 0; j <= numPtsInCQPTableMinus1; j++)
    {
      int sh = (getDeltaQpInValMinus1(i, j) + 1) >> 1;
      for (int k = qpInVal[j] + 1, m = 1; k <= qpInVal[j + 1]; k++, m++)
      {
        m_chromaQpMappingTables[i][k] = m_chromaQpMappingTables[i][qpInVal[j]] + ((qpOutVal[j + 1] - qpOutVal[j]) * m + sh) / (getDeltaQpInValMinus1(i, j) + 1);
      }
    }
    for (int k = qpInVal[numPtsInCQPTableMinus1 + 1] + 1; k <= MAX_QP; k++)
    {
      m_chromaQpMappingTables[i][k] = Clip3(-qpBdOffsetC, MAX_QP, m_chromaQpMappingTables[i][k - 1] + 1);
    }
  }
}

void vvc::SPS::createRPLList0(int numRPL)
{
  m_RPLList0.destroy();
  m_RPLList0.create(numRPL);
  m_numRPL0 = numRPL;
  m_rpl1IdxPresentFlag = (m_numRPL0 != m_numRPL1) ? true : false;
}
void vvc::SPS::createRPLList1(int numRPL)
{
  m_RPLList1.destroy();
  m_RPLList1.create(numRPL);
  m_numRPL1 = numRPL;
  m_rpl1IdxPresentFlag = (m_numRPL0 != m_numRPL1) ? true : false;
}

void vvc::VPS::deriveOutputLayerSets()
{
  if (m_vpsEachLayerIsAnOlsFlag || m_vpsOlsModeIdc < 2)
  {
    m_totalNumOLSs = m_maxLayers;
  }
  else if (m_vpsOlsModeIdc == 2)
  {
    m_totalNumOLSs = m_vpsNumOutputLayerSets;
  }

  m_olsDpbParamsIdx.resize(m_totalNumOLSs);
  m_olsDpbPicSize.resize(m_totalNumOLSs, vvc::Size(0, 0));
  m_numOutputLayersInOls.resize(m_totalNumOLSs);
  m_numLayersInOls.resize(m_totalNumOLSs);
  m_outputLayerIdInOls.resize(m_totalNumOLSs, std::vector<int>(m_maxLayers, NOT_VALID));
  m_numSubLayersInLayerInOLS.resize(m_totalNumOLSs, std::vector<int>(m_maxLayers, NOT_VALID));
  m_layerIdInOls.resize(m_totalNumOLSs, std::vector<int>(m_maxLayers, NOT_VALID));
  m_olsDpbChromaFormatIdc.resize(m_totalNumOLSs);
  m_olsDpbBitDepthMinus8.resize(m_totalNumOLSs);

  std::vector<int> numRefLayers(m_maxLayers);
  std::vector<std::vector<int>> outputLayerIdx(m_totalNumOLSs, std::vector<int>(m_maxLayers, NOT_VALID));
  std::vector<std::vector<int>> layerIncludedInOlsFlag(m_totalNumOLSs, std::vector<int>(m_maxLayers, 0));
  std::vector<std::vector<int>> dependencyFlag(m_maxLayers, std::vector<int>(m_maxLayers, NOT_VALID));
  std::vector<std::vector<int>> refLayerIdx(m_maxLayers, std::vector<int>(m_maxLayers, NOT_VALID));
  std::vector<int> layerUsedAsRefLayerFlag(m_maxLayers, 0);
  std::vector<int> layerUsedAsOutputLayerFlag(m_maxLayers, NOT_VALID);

  for (int i = 0; i < (int)m_maxLayers; i++)
  {
    int r = 0;

    for (int j = 0; j < (int)m_maxLayers; j++)
    {
      dependencyFlag[i][j] = m_vpsDirectRefLayerFlag[i][j];

      for (int k = 0; k < i; k++)
      {
        if (m_vpsDirectRefLayerFlag[i][k] && dependencyFlag[k][j])
        {
          dependencyFlag[i][j] = 1;
        }
      }
      if (m_vpsDirectRefLayerFlag[i][j])
      {
        layerUsedAsRefLayerFlag[j] = 1;
      }

      if (dependencyFlag[i][j])
      {
        refLayerIdx[i][r++] = j;
      }
    }

    numRefLayers[i] = r;
  }

  m_numOutputLayersInOls[0] = 1;
  m_outputLayerIdInOls[0][0] = m_vpsLayerId[0];
  m_numSubLayersInLayerInOLS[0][0] = m_ptlMaxTemporalId[m_olsPtlIdx[0]] + 1;
  layerUsedAsOutputLayerFlag[0] = 1;
  for (int i = 1; i < (int)m_maxLayers; i++)
  {
    if (m_vpsEachLayerIsAnOlsFlag || m_vpsOlsModeIdc < 2)
    {
      layerUsedAsOutputLayerFlag[i] = 1;
    }
    else
    {
      layerUsedAsOutputLayerFlag[i] = 0;
    }
  }
  for (int i = 1; i < m_totalNumOLSs; i++)
  {
    if (m_vpsEachLayerIsAnOlsFlag || m_vpsOlsModeIdc == 0)
    {
      m_numOutputLayersInOls[i] = 1;
      m_outputLayerIdInOls[i][0] = m_vpsLayerId[i];
      if (m_vpsEachLayerIsAnOlsFlag)
      {
        m_numSubLayersInLayerInOLS[i][0] = m_ptlMaxTemporalId[m_olsPtlIdx[i]] + 1;
      }
      else
      {
        m_numSubLayersInLayerInOLS[i][i] = m_ptlMaxTemporalId[m_olsPtlIdx[i]] + 1;
        for (int k = i - 1; k >= 0; k--)
        {
          m_numSubLayersInLayerInOLS[i][k] = 0;
          for (int m = k + 1; m <= i; m++)
          {
            uint32_t maxSublayerNeeded = std::min((uint32_t)m_numSubLayersInLayerInOLS[i][m], m_vpsMaxTidIlRefPicsPlus1[m][k]);
            if (m_vpsDirectRefLayerFlag[m][k] && m_numSubLayersInLayerInOLS[i][k] < (int)maxSublayerNeeded)
            {
              m_numSubLayersInLayerInOLS[i][k] = maxSublayerNeeded;
            }
          }
        }
      }
    }
    else if (m_vpsOlsModeIdc == 1)
    {
      m_numOutputLayersInOls[i] = i + 1;

      for (int j = 0; j < m_numOutputLayersInOls[i]; j++)
      {
        m_outputLayerIdInOls[i][j] = m_vpsLayerId[j];
        m_numSubLayersInLayerInOLS[i][j] = m_ptlMaxTemporalId[m_olsPtlIdx[i]] + 1;
      }
    }
    else if (m_vpsOlsModeIdc == 2)
    {
      int j = 0;
      int highestIncludedLayer = 0;
      for (j = 0; j < (int)m_maxLayers; j++)
      {
        m_numSubLayersInLayerInOLS[i][j] = 0;
      }
      j = 0;
      for (int k = 0; k < (int)m_maxLayers; k++)
      {
        if (m_vpsOlsOutputLayerFlag[i][k])
        {
          layerIncludedInOlsFlag[i][k] = 1;
          highestIncludedLayer = k;
          layerUsedAsOutputLayerFlag[k] = 1;
          outputLayerIdx[i][j] = k;
          m_outputLayerIdInOls[i][j++] = m_vpsLayerId[k];
          m_numSubLayersInLayerInOLS[i][k] = m_ptlMaxTemporalId[m_olsPtlIdx[i]] + 1;
        }
      }
      m_numOutputLayersInOls[i] = j;

      for (j = 0; j < m_numOutputLayersInOls[i]; j++)
      {
        int idx = outputLayerIdx[i][j];
        for (int k = 0; k < numRefLayers[idx]; k++)
        {
          layerIncludedInOlsFlag[i][refLayerIdx[idx][k]] = 1;
        }
      }
      for (int k = highestIncludedLayer - 1; k >= 0; k--)
      {
        if (layerIncludedInOlsFlag[i][k] && !m_vpsOlsOutputLayerFlag[i][k])
        {
          for (int m = k + 1; m <= highestIncludedLayer; m++)
          {
            uint32_t maxSublayerNeeded = std::min((uint32_t)m_numSubLayersInLayerInOLS[i][m], m_vpsMaxTidIlRefPicsPlus1[m][k]);
            if (m_vpsDirectRefLayerFlag[m][k] && layerIncludedInOlsFlag[i][m] && m_numSubLayersInLayerInOLS[i][k] < (int)maxSublayerNeeded)
            {
              m_numSubLayersInLayerInOLS[i][k] = maxSublayerNeeded;
            }
          }
        }
      }
    }
  }
  for (int i = 0; i < (int)m_maxLayers; i++)
  {
    CHECK(layerUsedAsRefLayerFlag[i] == 0 && layerUsedAsOutputLayerFlag[i] == 0, "There shall be no layer that is neither an output layer nor a direct reference layer");
  }

  m_numLayersInOls[0] = 1;
  m_layerIdInOls[0][0] = m_vpsLayerId[0];
  m_numMultiLayeredOlss = 0;
  for (int i = 1; i < m_totalNumOLSs; i++)
  {
    if (m_vpsEachLayerIsAnOlsFlag)
    {
      m_numLayersInOls[i] = 1;
      m_layerIdInOls[i][0] = m_vpsLayerId[i];
    }
    else if (m_vpsOlsModeIdc == 0 || m_vpsOlsModeIdc == 1)
    {
      m_numLayersInOls[i] = i + 1;
      for (int j = 0; j < m_numLayersInOls[i]; j++)
      {
        m_layerIdInOls[i][j] = m_vpsLayerId[j];
      }
    }
    else if (m_vpsOlsModeIdc == 2)
    {
      int j = 0;
      for (int k = 0; k < (int)m_maxLayers; k++)
      {
        if (layerIncludedInOlsFlag[i][k])
        {
          m_layerIdInOls[i][j++] = m_vpsLayerId[k];
        }
      }

      m_numLayersInOls[i] = j;
    }
    if (m_numLayersInOls[i] > 1)
    {
      m_multiLayerOlsIdx[i] = m_numMultiLayeredOlss;
      m_numMultiLayeredOlss++;
    }
  }
  m_multiLayerOlsIdxToOlsIdx.resize(m_numMultiLayeredOlss);

  for (int i = 0, j = 0; i < m_totalNumOLSs; i++)
  {
    if (m_numLayersInOls[i] > 1)
    {
      m_multiLayerOlsIdxToOlsIdx[j++] = i;
    }
  }
}

void vvc::VPS::checkVPS()
{
  for (int multiLayerOlsIdx = 0; multiLayerOlsIdx < m_numMultiLayeredOlss; multiLayerOlsIdx++)
  {
    const int olsIdx = m_multiLayerOlsIdxToOlsIdx[multiLayerOlsIdx];
    const int olsTimingHrdIdx = getOlsTimingHrdIdx(multiLayerOlsIdx);
    const int olsPtlIdx = getOlsPtlIdx(olsIdx);
    CHECK(getHrdMaxTid(olsTimingHrdIdx) < getPtlMaxTemporalId(olsPtlIdx), "The value of vps_hrd_max_tid[vps_ols_timing_hrd_idx[m]] shall be greater than or equal to "
                                                                          "vps_ptl_max_tid[ vps_ols_ptl_idx[n]] for each m-th multi-layer OLS for m from 0 to "
                                                                          "NumMultiLayerOlss - 1, inclusive, and n being the OLS index of the m-th multi-layer OLS among all OLSs.");
    const int olsDpbParamsIdx = getOlsDpbParamsIdx(olsIdx);
    CHECK(m_dpbMaxTemporalId[olsDpbParamsIdx] < (int)getPtlMaxTemporalId(olsPtlIdx), "The value of vps_dpb_max_tid[vps_ols_dpb_params_idx[m]] shall be greater than or equal to "
                                                                                     "vps_ptl_max_tid[ vps_ols_ptl_idx[n]] for each m-th multi-layer OLS for m from 0 to "
                                                                                     "NumMultiLayerOlss - 1, inclusive, and n being the OLS index of the m-th multi-layer OLS among all OLSs.");
  }
}

bool vvc::ScalingList::isLumaScalingList(int scalingListId) const
{
  return (scalingListId % MAX_NUM_COMPONENT == SCALING_LIST_1D_START_4x4 || scalingListId == SCALING_LIST_1D_START_64x64 + 1);
}

void vvc::ScalingList::processRefMatrix(uint32_t scalinListId, uint32_t refListId)
{
  int matrixSize = (scalinListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalinListId < SCALING_LIST_1D_START_8x8) ? 4
                                                                                                               : 8;
  ::memcpy(getScalingListAddress(scalinListId), ((scalinListId == refListId) ? getScalingListDefaultAddress(refListId) : getScalingListAddress(refListId)), sizeof(int) * matrixSize * matrixSize);
}

const int *vvc::ScalingList::getScalingListDefaultAddress(uint32_t scalingListId)
{
  const int *src = 0;
  int sizeId = (scalingListId < SCALING_LIST_1D_START_8x8) ? 2 : 3;
  switch (sizeId)
  {
  case SCALING_LIST_1x1:
  case SCALING_LIST_2x2:
  case SCALING_LIST_4x4:
    src = vvc::g_quantTSDefault4x4;
    break;
  case SCALING_LIST_8x8:
  case SCALING_LIST_16x16:
  case SCALING_LIST_32x32:
  case SCALING_LIST_64x64:
  case SCALING_LIST_128x128:
    src = vvc::g_quantInterDefault8x8;
    break;
  default:
    THROW("Invalid scaling list");
    src = nullptr;
    break;
  }
  return src;
}