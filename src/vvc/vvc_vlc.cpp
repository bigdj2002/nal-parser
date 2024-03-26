#include "vvc_type.h"
#include "vvc_vlc.h"
#include "vvc_nal.h"

#include <assert.h>

InputBitstream::InputBitstream()
: m_fifo()
, m_emulationPreventionByteLocation()
, m_fifo_idx(0)
, m_num_held_bits(0)
, m_held_bits(0)
, m_numBitsRead(0)
{ }

InputBitstream::InputBitstream(const InputBitstream &src)
: m_fifo(src.m_fifo)
, m_emulationPreventionByteLocation(src.m_emulationPreventionByteLocation)
, m_fifo_idx(src.m_fifo_idx)
, m_num_held_bits(src.m_num_held_bits)
, m_held_bits(src.m_held_bits)
, m_numBitsRead(src.m_numBitsRead)
{ }

void InputBitstream::resetToStart()
{
  m_fifo_idx=0;
  m_num_held_bits=0;
  m_held_bits=0;
  m_numBitsRead=0;
}

void InputBitstream::pseudoRead(uint32_t numberOfBits, uint32_t &ruiBits)
{
  uint32_t saved_num_held_bits = m_num_held_bits;
  uint8_t saved_held_bits = m_held_bits;
  uint32_t saved_fifo_idx = m_fifo_idx;

  uint32_t num_bits_to_read = std::min(numberOfBits, getNumBitsLeft());
  read(num_bits_to_read, ruiBits);
  ruiBits <<= (numberOfBits - num_bits_to_read);

  m_fifo_idx = saved_fifo_idx;
  m_held_bits = saved_held_bits;
  m_num_held_bits = saved_num_held_bits;
}

void InputBitstream::read(uint32_t numberOfBits, uint32_t &ruiBits)
{
  CHECK(numberOfBits > 32, "Too many bits read");

  m_numBitsRead += numberOfBits;

  /* NB, bits are extracted from the MSB of each byte. */
  uint32_t retval = 0;
  if (numberOfBits <= m_num_held_bits)
  {
    /* n=1, len(H)=7:   -VHH HHHH, shift_down=6, mask=0xfe
     * n=3, len(H)=7:   -VVV HHHH, shift_down=4, mask=0xf8
     */
    retval = m_held_bits >> (m_num_held_bits - numberOfBits);
    retval &= ~(0xff << numberOfBits);
    m_num_held_bits -= numberOfBits;
    ruiBits = retval;
    return;
  }

  /* all num_held_bits will go into retval
   *   => need to mask leftover bits from previous extractions
   *   => align retval with top of extracted word */
  /* n=5, len(H)=3: ---- -VVV, mask=0x07, shift_up=5-3=2,
   * n=9, len(H)=3: ---- -VVV, mask=0x07, shift_up=9-3=6 */
  numberOfBits -= m_num_held_bits;
  retval = m_held_bits & ~(0xff << m_num_held_bits);
  retval <<= numberOfBits;

  /* number of whole bytes that need to be loaded to form retval */
  /* n=32, len(H)=0, load 4bytes, shift_down=0
   * n=32, len(H)=1, load 4bytes, shift_down=1
   * n=31, len(H)=1, load 4bytes, shift_down=1+1
   * n=8,  len(H)=0, load 1byte,  shift_down=0
   * n=8,  len(H)=3, load 1byte,  shift_down=3
   * n=5,  len(H)=1, load 1byte,  shift_down=1+3
   */
  uint32_t aligned_word = 0;
  uint32_t num_bytes_to_load = (numberOfBits - 1) >> 3;
  CHECK(m_fifo_idx + num_bytes_to_load >= m_fifo.size(), "Exceeded FIFO size");

  switch (num_bytes_to_load)
  {
  case 3: aligned_word  = m_fifo[m_fifo_idx++] << 24;
  case 2: aligned_word |= m_fifo[m_fifo_idx++] << 16;
  case 1: aligned_word |= m_fifo[m_fifo_idx++] <<  8;
  case 0: aligned_word |= m_fifo[m_fifo_idx++];
  }

  /* resolve remainder bits */
  uint32_t next_num_held_bits = (32 - numberOfBits) % 8;

  /* copy required part of aligned_word into retval */
  retval |= aligned_word >> next_num_held_bits;

  /* store held bits */
  m_num_held_bits = next_num_held_bits;
  m_held_bits = aligned_word;

  ruiBits = retval;
}

uint32_t InputBitstream::readOutTrailingBits ()
{
  uint32_t count=0;
  uint32_t bits = 0;

  while ( ( getNumBitsLeft() > 0 ) && (getNumBitsUntilByteAligned()!=0) )
  {
    count++;
    read(1, bits);
  }
  return count;
}

InputBitstream *InputBitstream::extractSubstream( uint32_t uiNumBits )
{
  uint32_t uiNumBytes = uiNumBits/8;
  InputBitstream *pResult = new InputBitstream;

  std::vector<uint8_t> &buf = pResult->getFifo();
  buf.reserve((uiNumBits+7)>>3);

  if (m_num_held_bits == 0)
  {
    std::size_t currentOutputBufferSize=buf.size();
    const uint32_t uiNumBytesToReadFromFifo = std::min<uint32_t>(uiNumBytes, (uint32_t)m_fifo.size() - m_fifo_idx);
    buf.resize(currentOutputBufferSize+uiNumBytes);
    memcpy(&(buf[currentOutputBufferSize]), &(m_fifo[m_fifo_idx]), uiNumBytesToReadFromFifo); m_fifo_idx+=uiNumBytesToReadFromFifo;
    if (uiNumBytesToReadFromFifo != uiNumBytes)
    {
      memset(&(buf[currentOutputBufferSize+uiNumBytesToReadFromFifo]), 0, uiNumBytes - uiNumBytesToReadFromFifo);
    }
  }
  else
  {
    for (uint32_t ui = 0; ui < uiNumBytes; ui++)
    {
      uint32_t uiByte;
      read(8, uiByte);
      buf.push_back(uiByte);
    }
  }
  if (uiNumBits&0x7)
  {
    uint32_t uiByte = 0;
    read(uiNumBits&0x7, uiByte);
    uiByte <<= 8-(uiNumBits&0x7);
    buf.push_back(uiByte);
  }
  return pResult;
}

uint32_t InputBitstream::readByteAlignment()
{
  uint32_t code = 0;
  read( 1, code );
  CHECK(code != 1, "Code is not '1'");

  uint32_t numBits = getNumBitsUntilByteAligned();
  if(numBits)
  {
    CHECK(numBits > getNumBitsLeft(), "More bits available than left");
    read( numBits, code );
    CHECK(code != 0, "Code not '0'");
  }
  return numBits+1;
}

void VLCReader::xReadSCode (uint32_t length, int& value)
{
  uint32_t val;
  assert ( length > 0 && length<=32);
  m_pcBitstream->read (length, val);
  value= length>=32 ? int(val) : ( (-int( val & (uint32_t(1)<<(length-1)))) | int(val) );
}

void VLCReader::xReadCode(uint32_t length, uint32_t &ruiCode)
{
  CHECK(length == 0, "Reading a code of length '0'");
  m_pcBitstream->read(length, ruiCode);
}

void VLCReader::xReadUvlc( uint32_t& ruiVal)
{
  uint32_t uiVal = 0;
  uint32_t uiCode = 0;
  uint32_t length;
  m_pcBitstream->read( 1, uiCode );

  if( 0 == uiCode )
  {
    length = 0;

    while( ! ( uiCode & 1 ))
    {
      m_pcBitstream->read( 1, uiCode );
      length++;
    }

    m_pcBitstream->read(length, uiVal);

    uiVal += (1 << length) - 1;
  }

  ruiVal = uiVal;
}

void VLCReader::xReadSvlc( int& riVal)
{
  uint32_t bits = 0;
  m_pcBitstream->read(1, bits);
  if (0 == bits)
  {
    uint32_t length = 0;

    while (!(bits & 1))
    {
      m_pcBitstream->read(1, bits);
      length++;
    }

    m_pcBitstream->read(length, bits);

    bits += (1 << length);
    riVal = (bits & 1) ? -(int) (bits >> 1) : (int) (bits >> 1);
  }
  else
  {
    riVal = 0;
  }
}

void VLCReader::xReadFlag (uint32_t& ruiCode)
{
  m_pcBitstream->read( 1, ruiCode );

}

void VLCReader::xReadRbspTrailingBits()
{
  uint32_t bit;
  READ_FLAG( bit, "rbsp_stop_one_bit");
  CHECK(bit!=1, "Trailing bit not '1'");
  int cnt = 0;
  while (m_pcBitstream->getNumBitsUntilByteAligned())
  {
    READ_FLAG( bit, "rbsp_alignment_zero_bit");
    CHECK(bit!=0, "Alignment bit is not '0'");
    cnt++;
  }
  CHECK(cnt >= 8, "Read more than '8' trailing bits");
}

void AUDReader::parseAccessUnitDelimiter(InputBitstream* bs, uint32_t &audIrapOrGdrAuFlag, uint32_t &picType)
{
  setBitstream(bs);

  READ_FLAG (audIrapOrGdrAuFlag, "aud_irap_or_gdr_au_flag");
  READ_CODE (3, picType, "pic_type");
  xReadRbspTrailingBits();
}

void FDReader::parseFillerData(InputBitstream* bs, uint32_t &fdSize)
{
  setBitstream(bs);
  uint32_t ffByte;
  fdSize = 0;
  while( m_pcBitstream->getNumBitsLeft() >8 )
  {
    READ_CODE (8, ffByte, "ff_byte");
    CHECK(ffByte!=0xff, "Invalid filler data : not '0xff'");
    fdSize++;
  }
  xReadRbspTrailingBits();
}

void parseSeiH266::sei_read_scode(uint32_t length, int& code, const char *pSymbolName)
{
  READ_SCODE(length, code, pSymbolName);
}

void parseSeiH266::sei_read_code(uint32_t length, uint32_t &ruiCode, const char *pSymbolName)
{
  READ_CODE(length, ruiCode, pSymbolName);
}

void parseSeiH266::sei_read_uvlc(uint32_t& ruiCode, const char *pSymbolName)
{
  READ_UVLC(ruiCode, pSymbolName);
}

void parseSeiH266::sei_read_svlc(int& ruiCode, const char *pSymbolName)
{
  READ_SVLC(ruiCode, pSymbolName);
}

void parseSeiH266::sei_read_flag(uint32_t& ruiCode, const char *pSymbolName)
{
  READ_FLAG(ruiCode, pSymbolName);
}