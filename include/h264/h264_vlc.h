#pragma once

#include "nal_parse.h"

typedef struct syntaxelement_dec
{
public:
  int type;
  int value1;
  int value2;
  int len;
  int inf;
  unsigned int bitpattern;
  int context;
  int k;

  void (*mapping)(int len, int info, int *value1, int *value2);
} SyntaxElement;

struct Bitstream
{
public:
  int read_len;
  int code_len;
  int frame_bitoffset;
  int bitstream_length;
  unsigned char *streamBuffer;
  int ei_flag;

  int GetBits(unsigned char buffer[], int totbitoffset, int *info, int bitcount, int numbits);
  int readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream);
  int GetVLCSymbol(unsigned char buffer[], int totbitoffset, int *info, int bytecount);
  int readSyntaxElement_VLC(SyntaxElement *sym, Bitstream *currStream);
  int read_u_v(int LenInBits, Bitstream *bitstream, int *used_bits);
  bool read_u_1(Bitstream *bitstream, int *used_bits);
  int read_ue_v(Bitstream *bitstream, int *used_bits);
  int read_se_v(Bitstream *bitstream, int *used_bits);
  int read_i_v(int LenInBits, Bitstream *bitstream, int *used_bits);
  int more_rbsp_data(unsigned char buffer[], int totbitoffset, int bytecount);
};

typedef struct decoder_params
{
public:
  int UsedBits;
  int bitcounter;
} DecoderParams;

typedef struct decoding_environment
{
public:
  unsigned int Drange;
  unsigned int Dvalue;
  int DbitsLeft;
  unsigned char *Dcodestrm;
  int *Dcodestrm_len;
} DecodingEnvironment;

typedef struct datapartition_dec : public Bitstream, public DecodingEnvironment
{
public:
  Bitstream *bitstream;
  DecodingEnvironment de_cabac;
  int (*readSyntaxElement)(struct macroblock_dec *currMB, struct syntaxelement_dec *, struct datapartition_dec *);
} DataPartition;