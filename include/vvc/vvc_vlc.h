#pragma once

#include <stdint.h>
#include <vector>
#include <stdio.h>
#include <cmath>
#include <string>

#define READ_SCODE(length, code, name) xReadSCode(length, code)
#define READ_CODE(length, code, name) xReadCode(length, code)
#define READ_UVLC(code, name) xReadUvlc(code)
#define READ_SVLC(code, name) xReadSvlc(code)
#define READ_FLAG(code, name) xReadFlag(code)

#define THROW(x) throw(Exception("\nERROR: In function \"") << __FUNCTION__ << "\" in " << __FILE__ << ":" << __LINE__ << ": " << x)
#define CHECK(c, x) \
  if (c)            \
  {                 \
    THROW(x);       \
  }

class Exception : public std::exception
{
public:
  Exception(const std::string &_s) : m_str(_s) {}
  Exception(const Exception &_e) : std::exception(_e), m_str(_e.m_str) {}
  virtual ~Exception() noexcept {};
  virtual const char *what() const noexcept { return m_str.c_str(); }
  Exception &operator=(const Exception &_e)
  {
    std::exception::operator=(_e);
    m_str = _e.m_str;
    return *this;
  }
  template <typename T>
  Exception &operator<<(T t)
  {
    std::ostringstream oss;
    oss << t;
    m_str += oss.str();
    return *this;
  }

private:
  std::string m_str;
};

class InputBitstream
{
public:
  std::vector<uint8_t> m_fifo; /// FIFO for storage of complete bytes
  std::vector<uint32_t> m_emulationPreventionByteLocation;

  uint32_t m_fifo_idx{0}; /// Read index into m_fifo

  uint32_t m_num_held_bits{0};
  uint8_t m_held_bits{0};
  uint32_t m_numBitsRead{0};

public:
  /**
   * Create a new bitstream reader object that reads from buf.
   */
  InputBitstream();
  virtual ~InputBitstream() {}
  InputBitstream(const InputBitstream &src);

  void resetToStart();

  // interface for decoding
  void pseudoRead(uint32_t numberOfBits, uint32_t &ruiBits);
  void read(uint32_t numberOfBits, uint32_t &ruiBits);
  void readByte(uint32_t &ruiBits)
  {
    CHECK(m_fifo_idx >= m_fifo.size(), "FIFO exceeded");
    ruiBits = m_fifo[m_fifo_idx++];
#if ENABLE_TRACING
    m_numBitsRead += 8;
#endif
  }

  void peekPreviousByte(uint32_t &byte)
  {
    CHECK(m_fifo_idx == 0, "FIFO empty");
    byte = m_fifo[m_fifo_idx - 1];
  }

  uint32_t readOutTrailingBits();
  uint8_t getHeldBits() { return m_held_bits; }
  uint32_t getByteLocation() { return m_fifo_idx; }

  // Peek at bits in word-storage. Used in determining if we have completed reading of current bitstream and therefore slice in LCEC.
  uint32_t peekBits(uint32_t bits)
  {
    uint32_t tmp;
    pseudoRead(bits, tmp);
    return tmp;
  }

  // utility functions
  uint32_t read(uint32_t numberOfBits)
  {
    uint32_t tmp;
    read(numberOfBits, tmp);
    return tmp;
  }
  uint32_t readByte()
  {
    uint32_t tmp;
    readByte(tmp);
    return tmp;
  }
  uint32_t getNumBitsUntilByteAligned() { return m_num_held_bits & (0x7); }
  uint32_t getNumBitsLeft() { return 8 * ((uint32_t)m_fifo.size() - m_fifo_idx) + m_num_held_bits; }
  InputBitstream *extractSubstream(uint32_t uiNumBits); // Read the nominated number of bits, and return as a bitstream.
  uint32_t getNumBitsRead() { return m_numBitsRead; }
  uint32_t readByteAlignment();

  void pushEmulationPreventionByteLocation(uint32_t pos) { m_emulationPreventionByteLocation.push_back(pos); }
  uint32_t numEmulationPreventionBytesRead() { return (uint32_t)m_emulationPreventionByteLocation.size(); }
  const std::vector<uint32_t> &getEmulationPreventionByteLocation() const { return m_emulationPreventionByteLocation; }
  uint32_t getEmulationPreventionByteLocation(uint32_t idx) { return m_emulationPreventionByteLocation[idx]; }
  void clearEmulationPreventionByteLocation() { m_emulationPreventionByteLocation.clear(); }
  void setEmulationPreventionByteLocation(const std::vector<uint32_t> &vec) { m_emulationPreventionByteLocation = vec; }

  const std::vector<uint8_t> &getFifo() const { return m_fifo; }
  std::vector<uint8_t> &getFifo() { return m_fifo; }
};

class VLCReader
{
protected:
  InputBitstream *m_pcBitstream;

  VLCReader() : m_pcBitstream(nullptr){};
  virtual ~VLCReader(){};

  void xReadCode(uint32_t length, uint32_t &val);
  void xReadUvlc(uint32_t &val);
  void xReadSvlc(int &val);
  void xReadFlag(uint32_t &val);
  void xReadSCode(uint32_t length, int &val);

public:
  void setBitstream(InputBitstream *p) { m_pcBitstream = p; }
  InputBitstream *getBitstream() { return m_pcBitstream; }

protected:
  void xReadRbspTrailingBits();
  bool isByteAligned() { return (m_pcBitstream->getNumBitsUntilByteAligned() == 0); }
};

class AUDReader : public VLCReader
{
public:
  AUDReader(){};
  virtual ~AUDReader(){};
  void parseAccessUnitDelimiter(InputBitstream *bs, uint32_t &audIrapOrGdrAuFlag, uint32_t &picType);
};

class FDReader : public VLCReader
{
public:
  FDReader(){};
  virtual ~FDReader(){};
  void parseFillerData(InputBitstream *bs, uint32_t &fdSize);
};