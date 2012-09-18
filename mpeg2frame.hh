#ifndef MPEG2FRAME_HH
#define MPEG2FRAME_HH

#include <assert.h>
#include <string>

#include "elemstrmwriter.hh"
#include "quantize.hh"
#include "encoderparams.hh"
#include "mpeg2encoptions.hh"

class Picture;

void initialize_mpeg2( void );

class MPEG2Frame
{
private:
  MPEG2EncOptions _options;
  EncoderParams _params;
  Picture *_pic;
  Quantizer _quantizer;
  int _height, _width;

  class StringWriter : public ElemStrmWriter
  {
  private:
    std::string _buf;

  public:
    StringWriter() : _buf() {}

    void WriteOutBufferUpto( const uint8_t *buffer, const uint32_t flush_upto )
    {
      _buf += std::string( (char *)buffer, flush_upto );
      flushed += flush_upto;
    }

    uint64_t BitCount( void ) { return flushed * 8LL; }
    
    const std::string & str( void ) const { return _buf; }
  };

  StringWriter _output;

  static MPEG2EncOptions make_options( const int height, const int width );

public:
  MPEG2Frame( const int height, const int width );
  ~MPEG2Frame();

  const Picture *get_pic( void ) const { return _pic; }

  /* interface for Network::Transport */
  void subtract( const MPEG2Frame *prefix ) {}
  std::string diff_from( const MPEG2Frame &existing, const size_t len ) const; /* encode */
  void apply_string( const std::string diff ); /* reconstruct */
  bool operator==( const MPEG2Frame &x ) const { assert( false ); return false; }

  bool compare( const MPEG2Frame & ) const { assert( false ); return false; }
};

#endif
