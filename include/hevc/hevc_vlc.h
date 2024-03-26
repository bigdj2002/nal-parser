#pragma once

#include "nal_parse.h"

#include <vector>
#include <stdint.h>
#include <assert.h>

class TComInputBitstream
{
public:
  std::vector<uint8_t> m_fifo; /// FIFO for storage of complete bytes
  std::vector<unsigned int> m_emulationPreventionByteLocation;

  unsigned int m_fifo_idx; /// Read index into m_fifo

  unsigned int m_num_held_bits;
  unsigned char m_held_bits;
  unsigned int m_numBitsRead;

public:
  TComInputBitstream();
  virtual ~TComInputBitstream(){};
  TComInputBitstream(const TComInputBitstream &src);

  void resetToStart();

  // interface for decoding
  void pseudoRead(unsigned int uiNumberOfBits, unsigned int &ruiBits);
  void read(unsigned int uiNumberOfBits, unsigned int &ruiBits);
  void readByte(unsigned int &ruiBits)
  {
    assert(m_fifo_idx < m_fifo.size());
    ruiBits = m_fifo[m_fifo_idx++];
  }

  void peekPreviousByte(unsigned int &byte)
  {
    assert(m_fifo_idx > 0);
    byte = m_fifo[m_fifo_idx - 1];
  }

  unsigned int readOutTrailingBits();
  unsigned char getHeldBits() { return m_held_bits; }
  // TComOutputBitstream &operator=(const TComOutputBitstream &src);
  unsigned int getByteLocation() { return m_fifo_idx; }

  // Peek at bits in word-storage. Used in determining if we have completed reading of current bitstream and therefore slice in LCEC.
  unsigned int peekBits(unsigned int uiBits)
  {
    unsigned int tmp;
    pseudoRead(uiBits, tmp);
    return tmp;
  }

  // utility functions
  unsigned int read(unsigned int numberOfBits)
  {
    unsigned int tmp;
    read(numberOfBits, tmp);
    return tmp;
  }
  unsigned int readByte()
  {
    unsigned int tmp;
    readByte(tmp);
    return tmp;
  }
  unsigned int getNumBitsUntilByteAligned() { return m_num_held_bits & (0x7); }
  unsigned int getNumBitsLeft() { return 8 * ((unsigned int)m_fifo.size() - m_fifo_idx) + m_num_held_bits; }
  TComInputBitstream *extractSubstream(unsigned int uiNumBits); // Read the nominated number of bits, and return as a bitstream.
  unsigned int getNumBitsRead() { return m_numBitsRead; }
  unsigned int readByteAlignment();

  void pushEmulationPreventionByteLocation(unsigned int pos) { m_emulationPreventionByteLocation.push_back(pos); }
  unsigned int numEmulationPreventionBytesRead() { return (unsigned int)m_emulationPreventionByteLocation.size(); }
  const std::vector<unsigned int> &getEmulationPreventionByteLocation() const { return m_emulationPreventionByteLocation; }
  unsigned int getEmulationPreventionByteLocation(unsigned int idx) { return m_emulationPreventionByteLocation[idx]; }
  void clearEmulationPreventionByteLocation() { m_emulationPreventionByteLocation.clear(); }
  void setEmulationPreventionByteLocation(const std::vector<unsigned int> &vec) { m_emulationPreventionByteLocation = vec; }

  const std::vector<uint8_t> &getFifo() const { return m_fifo; }
  std::vector<uint8_t> &getFifo() { return m_fifo; }
};

class SyntaxElementParser
{
public:
  TComInputBitstream *m_pcBitstream;

  SyntaxElementParser()
      : m_pcBitstream(NULL){};
  virtual ~SyntaxElementParser(){};

  void xReadSCode(unsigned int length, int &val, const char *syntax);
  void xReadCode(unsigned int length, unsigned int &val, const char *syntax);
  void xReadUvlc(unsigned int &val, const char *syntax);
  void xReadSvlc(int &val, const char *syntax);
  void xReadFlag(unsigned int &val, const char *syntax);

protected:
  void xReadRbspTrailingBits();

public:
  void setBitstream(TComInputBitstream *p) { m_pcBitstream = p; }
  TComInputBitstream *getBitstream() { return m_pcBitstream; }
};