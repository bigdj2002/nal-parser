#include "hevc_vlc.h"
#include <math.h>
#include <cstring>

TComInputBitstream::TComInputBitstream()
    : m_fifo(), m_emulationPreventionByteLocation(), m_fifo_idx(0), m_num_held_bits(0), m_held_bits(0), m_numBitsRead(0)
{
}

TComInputBitstream::TComInputBitstream(const TComInputBitstream &src)
    : m_fifo(src.m_fifo), m_emulationPreventionByteLocation(src.m_emulationPreventionByteLocation), m_fifo_idx(src.m_fifo_idx), m_num_held_bits(src.m_num_held_bits), m_held_bits(src.m_held_bits), m_numBitsRead(src.m_numBitsRead)
{
}

void TComInputBitstream::resetToStart()
{
  m_fifo_idx = 0;
  m_num_held_bits = 0;
  m_held_bits = 0;
  m_numBitsRead = 0;
}

void TComInputBitstream::pseudoRead(unsigned int uiNumberOfBits, unsigned int &ruiBits)
{
  unsigned int saved_num_held_bits = m_num_held_bits;
  unsigned char saved_held_bits = m_held_bits;
  unsigned int saved_fifo_idx = m_fifo_idx;

  unsigned int num_bits_to_read = std::min(uiNumberOfBits, getNumBitsLeft());
  read(num_bits_to_read, ruiBits);
  ruiBits <<= (uiNumberOfBits - num_bits_to_read);

  m_fifo_idx = saved_fifo_idx;
  m_held_bits = saved_held_bits;
  m_num_held_bits = saved_num_held_bits;
}

void TComInputBitstream::read(unsigned int uiNumberOfBits, unsigned int &ruiBits)
{
  assert(uiNumberOfBits <= 32);

  m_numBitsRead += uiNumberOfBits;

  unsigned int retval = 0;
  if (uiNumberOfBits <= m_num_held_bits)
  {
    retval = m_held_bits >> (m_num_held_bits - uiNumberOfBits);
    retval &= ~(0xff << uiNumberOfBits);
    m_num_held_bits -= uiNumberOfBits;
    ruiBits = retval;
    return;
  }

  uiNumberOfBits -= m_num_held_bits;
  retval = m_held_bits & ~(0xff << m_num_held_bits);
  retval <<= uiNumberOfBits;

  unsigned int aligned_word = 0;
  unsigned int num_bytes_to_load = (uiNumberOfBits - 1) >> 3;
  assert(m_fifo_idx + num_bytes_to_load < m_fifo.size());

  switch (num_bytes_to_load)
  {
  case 3:
    aligned_word = m_fifo[m_fifo_idx++] << 24;
  case 2:
    aligned_word |= m_fifo[m_fifo_idx++] << 16;
  case 1:
    aligned_word |= m_fifo[m_fifo_idx++] << 8;
  case 0:
    aligned_word |= m_fifo[m_fifo_idx++];
  }

  unsigned int next_num_held_bits = (32 - uiNumberOfBits) % 8;

  retval |= aligned_word >> next_num_held_bits;

  m_num_held_bits = next_num_held_bits;
  m_held_bits = aligned_word;

  ruiBits = retval;
}

TComInputBitstream *TComInputBitstream::extractSubstream(unsigned int uiNumBits)
{
  unsigned int uiNumBytes = uiNumBits / 8;
  TComInputBitstream *pResult = new TComInputBitstream;

  std::vector<unsigned char> &buf = pResult->getFifo();
  buf.reserve((uiNumBits + 7) >> 3);

  if (m_num_held_bits == 0)
  {
    std::size_t currentOutputBufferSize = buf.size();
    const unsigned int uiNumBytesToReadFromFifo = std::min<unsigned int>(uiNumBytes, (unsigned int)m_fifo.size() - m_fifo_idx);
    buf.resize(currentOutputBufferSize + uiNumBytes);
    memcpy(&(buf[currentOutputBufferSize]), &(m_fifo[m_fifo_idx]), uiNumBytesToReadFromFifo);
    m_fifo_idx += uiNumBytesToReadFromFifo;
    if (uiNumBytesToReadFromFifo != uiNumBytes)
    {
      memset(&(buf[currentOutputBufferSize + uiNumBytesToReadFromFifo]), 0, uiNumBytes - uiNumBytesToReadFromFifo);
    }
  }
  else
  {
    for (unsigned int ui = 0; ui < uiNumBytes; ui++)
    {
      unsigned int uiByte;
      read(8, uiByte);
      buf.push_back(uiByte);
    }
  }
  if (uiNumBits & 0x7)
  {
    unsigned int uiByte = 0;
    read(uiNumBits & 0x7, uiByte);
    uiByte <<= 8 - (uiNumBits & 0x7);
    buf.push_back(uiByte);
  }
  return pResult;
}

unsigned int TComInputBitstream::readByteAlignment()
{
  unsigned int code = 0;
  read(1, code);
  assert(code == 1);

  unsigned int numBits = getNumBitsUntilByteAligned();
  if (numBits)
  {
    assert(numBits <= getNumBitsLeft());
    read(numBits, code);
    assert(code == 0);
  }
  return numBits + 1;
}

void SyntaxElementParser::xReadSCode(unsigned int uiLength, int &rValue, const char *syntax)
{
  unsigned int val;
  assert(uiLength > 0 && uiLength <= 32);
  m_pcBitstream->read(uiLength, val);
  rValue = uiLength >= 32 ? int(val) : ((-(int)(val & ((unsigned int)(1) << (uiLength - 1)))) | (int)(val));
}

void SyntaxElementParser::xReadCode(unsigned int uiLength, unsigned int &rValue, const char *syntax)
{
  assert(uiLength > 0);
  m_pcBitstream->read(uiLength, rValue);
}

void SyntaxElementParser::xReadUvlc(unsigned int &rValue, const char *syntax)
{
  unsigned int uiVal = 0;
  unsigned int uiCode = 0;
  unsigned int uiLength;
  m_pcBitstream->read(1, uiCode);

  if (0 == uiCode)
  {
    uiLength = 0;

    while (!(uiCode & 1))
    {
      m_pcBitstream->read(1, uiCode);
      uiLength++;
    }

    m_pcBitstream->read(uiLength, uiVal);

    uiVal += (1 << uiLength) - 1;
  }

  rValue = uiVal;
}

void SyntaxElementParser::xReadSvlc(int &rValue, const char *syntax)
{
  unsigned int uiBits = 0;
  m_pcBitstream->read(1, uiBits);
  if (0 == uiBits)
  {
    unsigned int uiLength = 0;

    while (!(uiBits & 1))
    {
      m_pcBitstream->read(1, uiBits);
      uiLength++;
    }

    m_pcBitstream->read(uiLength, uiBits);

    uiBits += (1 << uiLength);
    rValue = (uiBits & 1) ? -(int)(uiBits >> 1) : (int)(uiBits >> 1);
  }
  else
  {
    rValue = 0;
  }
}

void SyntaxElementParser::xReadFlag(unsigned int &rValue, const char *syntax)
{
  m_pcBitstream->read(1, rValue);
}

void SyntaxElementParser::xReadRbspTrailingBits()
{
  unsigned int bit;
  xReadFlag(bit, "rbsp_stop_one_bit");
  assert(bit == 1);
  int cnt = 0;
  while (m_pcBitstream->getNumBitsUntilByteAligned())
  {
    xReadFlag(bit, "rbsp_alignment_zero_bit");
    assert(bit == 0);
    cnt++;
  }
  assert(cnt < 8);
}