#include "h264_vlc.h"

static void linfo_ue(int len, int info, int *value1, int *dummy)
{
  // assert ((len >> 1) < 32);
  *value1 = (int)(((unsigned int)1 << (len >> 1)) + (unsigned int)(info)-1);
}

static void linfo_se(int len, int info, int *value1, int *dummy)
{
  // assert ((len >> 1) < 32);
  unsigned int n = ((unsigned int)1 << (len >> 1)) + (unsigned int)info - 1;
  *value1 = (n + 1) >> 1;
  if ((n & 0x01) == 0) // lsb is signed bit
    *value1 = -*value1;
}

int Bitstream::GetBits(unsigned char buffer[], int totbitoffset, int *info, int bitcount, int numbits)
{
  if ((totbitoffset + numbits) > bitcount)
  {
    return -1;
  }
  else
  {
    int bitoffset = 7 - (totbitoffset & 0x07); // bit from start of byte
    int byteoffset = (totbitoffset >> 3);      // byte from start of buffer
    int bitcounter = numbits;
    unsigned char *curbyte = &(buffer[byteoffset]);
    int inf = 0;

    while (numbits--)
    {
      inf <<= 1;
      inf |= ((*curbyte) >> (bitoffset--)) & 0x01;
      if (bitoffset == -1)
      { // Move onto next byte to get all of numbits
        curbyte++;
        bitoffset = 7;
      }
      // Above conditional could also be avoided using the following:
      // curbyte   -= (bitoffset >> 3);
      // bitoffset &= 0x07;
    }
    *info = inf;

    return bitcounter; // return absolute offset in bit from start of frame
  }
}

int Bitstream::readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream)
{
  int BitstreamLengthInBits = (currStream->bitstream_length << 3) + 7;

  if ((GetBits(currStream->streamBuffer, currStream->frame_bitoffset, &(sym->inf), BitstreamLengthInBits, sym->len)) < 0)
    return -1;

  sym->value1 = sym->inf;
  currStream->frame_bitoffset += sym->len; // move bitstream pointer

  return 1;
}

int Bitstream::GetVLCSymbol(unsigned char buffer[], int totbitoffset, int *info, int bytecount)
{
  long byteoffset = (totbitoffset >> 3);       // byte from start of buffer
  int bitoffset = (7 - (totbitoffset & 0x07)); // bit from start of byte
  int bitcounter = 1;
  int len = 0;
  unsigned char *cur_byte = &(buffer[byteoffset]);
  int ctr_bit = ((*cur_byte) >> (bitoffset)) & 0x01; // control bit for current bit posision

  while (ctr_bit == 0)
  { // find leading 1 bit
    len++;
    bitcounter++;
    bitoffset--;
    bitoffset &= 0x07;
    cur_byte += (bitoffset == 7);
    byteoffset += (bitoffset == 7);
    ctr_bit = ((*cur_byte) >> (bitoffset)) & 0x01;
  }

  if (byteoffset + ((len + 7) >> 3) > bytecount)
    return -1;
  else
  {
    // make infoword
    int inf = 0; // shortest possible code is 1, then info is always 0

    while (len--)
    {
      bitoffset--;
      bitoffset &= 0x07;
      cur_byte += (bitoffset == 7);
      bitcounter++;
      inf <<= 1;
      inf |= ((*cur_byte) >> (bitoffset)) & 0x01;
    }

    *info = inf;
    return bitcounter; // return absolute offset in bit from start of frame
  }
}

int Bitstream::readSyntaxElement_VLC(SyntaxElement *sym, Bitstream *currStream)
{
  sym->len = GetVLCSymbol(currStream->streamBuffer, currStream->frame_bitoffset, &(sym->inf), currStream->bitstream_length);
  if (sym->len == -1)
    return -1;

  currStream->frame_bitoffset += sym->len;
  sym->mapping(sym->len, sym->inf, &(sym->value1), &(sym->value2));

  return 1;
}

int Bitstream::read_u_v(int LenInBits, Bitstream *bitstream, int *used_bits)
{
  SyntaxElement symbol;
  symbol.inf = 0;

  symbol.type = avc::SE_HEADER;
  symbol.mapping = linfo_ue; // Mapping rule
  symbol.len = LenInBits;
  readSyntaxElement_FLC(&symbol, bitstream);
  *used_bits += symbol.len;

  return symbol.inf;
}

bool Bitstream::read_u_1(Bitstream *bitstream, int *used_bits)
{
  return (bool)read_u_v(1, bitstream, used_bits);
}

int Bitstream::read_ue_v(Bitstream *bitstream, int *used_bits)
{
  SyntaxElement symbol;

  symbol.type = avc::SE_HEADER;
  symbol.mapping = linfo_ue; // Mapping rule
  readSyntaxElement_VLC(&symbol, bitstream);
  *used_bits += symbol.len;
  return symbol.value1;
}

int Bitstream::read_se_v(Bitstream *bitstream, int *used_bits)
{
  SyntaxElement symbol;

  symbol.type = avc::SE_HEADER;
  symbol.mapping = linfo_se; // Mapping rule: signed integer
  readSyntaxElement_VLC(&symbol, bitstream);
  *used_bits += symbol.len;
  return symbol.value1;
}

int Bitstream::read_i_v(int LenInBits, Bitstream *bitstream, int *used_bits)
{
  SyntaxElement symbol;
  symbol.inf = 0;

  symbol.type = avc::SE_HEADER;
  symbol.mapping = linfo_ue; // Mapping rule
  symbol.len = LenInBits;
  readSyntaxElement_FLC(&symbol, bitstream);
  *used_bits += symbol.len;

  // can be negative
  symbol.inf = -(symbol.inf & (1 << (LenInBits - 1))) | symbol.inf;

  return symbol.inf;
}

int Bitstream::more_rbsp_data(unsigned char buffer[], int totbitoffset, int bytecount)
{
  long byteoffset = (totbitoffset >> 3); // byte from start of buffer
  // there is more until we're in the last byte
  if (byteoffset < (bytecount - 1))
    return 1;
  else
  {
    int bitoffset = (7 - (totbitoffset & 0x07)); // bit from start of byte
    unsigned char *cur_byte = &(buffer[byteoffset]);
    // read one bit
    int ctr_bit = ((*cur_byte) >> (bitoffset--)) & 0x01; // control bit for current bit posision

    // assert (byteoffset<bytecount);

    // a stop bit has to be one
    if (ctr_bit == 0)
      return 1;
    else
    {
      int cnt = 0;

      while (bitoffset >= 0 && !cnt)
      {
        cnt |= ((*cur_byte) >> (bitoffset--)) & 0x01; // set up control bit
      }

      return (cnt);
    }
  }
}