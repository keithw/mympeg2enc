#include <assert.h>
#include <string>

#include "picture.hh"
#include "encoderparams.hh"
#include "mpeg2encoptions.hh"
#include "yuv4mpeg.h"
#include "elemstrmwriter.hh"
#include "quantize.hh"
#include "format_codes.h"
#include "motionsearch.h"
#include "transfrm_ref.h"
#include "predict_ref.h"

using namespace std;

class StringWriter : public ElemStrmWriter
{
private:
  string _buf;

public:
  StringWriter() : _buf() {}

  void WriteOutBufferUpto( const uint8_t *buffer, const uint32_t flush_upto )
  {
    _buf += string( (char *)buffer, flush_upto );
    flushed += flush_upto;
  }

  uint64_t BitCount( void ) { return flushed * 8LL; }
};

int main( void )
{
  init_motion_search();
  init_transform();
  init_predict();

  MPEG2EncOptions my_options;

  MPEG2EncInVidParams my_vid_params;
  my_vid_params.horizontal_size = 640;
  my_vid_params.vertical_size = 480;
  my_vid_params.aspect_ratio_code = 1; /* square pixels */
  my_vid_params.frame_rate_code = 5;
  my_vid_params.interlacing_code = Y4M_ILACE_NONE;

  my_options.bitrate = 1;
  my_options.video_buffer_size = 4000;
  my_options.format = MPEG_FORMAT_MPEG2;
  assert( my_options.SetFormatPresets( my_vid_params ) == false );

  EncoderParams my_params( my_options );

  StringWriter output;
  Quantizer quantizer( my_params );

  my_params.Init( my_options );
  quantizer.Init();

  Picture allblack( my_params, output, quantizer );

  return 0;
}
