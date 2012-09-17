#include <assert.h>
#include <string>
#include <string.h>

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
#include "streamstate.h"
#include "tables.h"
#include "ratectl.hh"
#include "imageplanes.hh"

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

  const string & str( void ) const { return _buf; }
};

void setup_picture( Picture & pic, Picture *const old, PICTURE_CODING type, EncoderParams & my_params )
{
  pic.new_seq = pic.end_seq = true; /* XXX */
  pic.gop_start = true; /* XXX */
  pic.gop_decode = pic.bgrp_decode = 0;
  pic.decode = 0;
  pic.present = 0;
  pic.last_picture = false;
  pic.finalfield = true;
  pic.fwd_ref_frame = pic.bwd_ref_frame = NULL;
  pic.fwd_org = pic.bwd_org = NULL;
  pic.fwd_rec = pic.bwd_rec = NULL;
  pic.nb = pic.np = 0;
  pic.closed_gop = false;
  pic.dc_prec = my_params.dc_prec;

  ImagePlanes *pic_orig = new ImagePlanes( my_params );
  memset( pic_orig->Plane( 0 ), 16, my_params.lum_buffer_size );
  memset( pic_orig->Plane( 1 ), 128, my_params.chrom_buffer_size );
  memset( pic_orig->Plane( 2 ), 128, my_params.chrom_buffer_size );

  memset( pic.rec_img->Plane( 0 ), 16, my_params.lum_buffer_size );
  memset( pic.rec_img->Plane( 1 ), 128, my_params.chrom_buffer_size );
  memset( pic.rec_img->Plane( 2 ), 128, my_params.chrom_buffer_size );

  pic.org_img = pic_orig;

  pic.sxb = pic.syb = 0;
  pic.secondfield = false;
  pic.ipflag = false;

  pic.temp_ref = 0;
  pic.frame_type = type;
  pic.gop_decode = 0;
  pic.bgrp_decode = 0;
  pic.pict_type = type;
  pic.vbv_delay = 0;
  pic.pict_struct = FRAME_PICTURE;
  
  pic.forw_hor_f_code = (type == I_TYPE) ? 15 : my_params.motion_data[0].forw_hor_f_code;
  pic.forw_vert_f_code = (type == I_TYPE) ? 15 : my_params.motion_data[0].forw_vert_f_code;
  pic.back_hor_f_code = 15;
  pic.back_vert_f_code = 15;
  pic.sxf = my_params.motion_data[0].sxf;
  pic.syf = my_params.motion_data[0].syf;

  pic.prog_frame = true;
  pic.frame_pred_dct = true;
  pic.q_scale_type = 1;
  pic.intravlc = 0;
  pic.altscan = false;
  pic.scan_pattern = zig_zag_scan;

  pic.unit_coeff_threshold = pic.unit_coeff_first = 0;
}

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
  my_options.video_buffer_size = 0;
  my_options.format = MPEG_FORMAT_MPEG2;
  assert( my_options.SetFormatPresets( my_vid_params ) == false );

  EncoderParams my_params( my_options );

  StringWriter output;
  Quantizer quantizer( my_params );

  my_params.Init( my_options );
  quantizer.Init();

  /* initialize pictures */
  Picture old_pic( my_params, output, quantizer );
  Picture new_pic( my_params, output, quantizer );

  setup_picture( old_pic, nullptr, I_TYPE, my_params );
  setup_picture( new_pic, nullptr, P_TYPE, my_params );

  new_pic.fwd_ref_frame = &old_pic;
  new_pic.fwd_org = old_pic.org_img;
  new_pic.fwd_rec = old_pic.rec_img;

  new_pic.MotionSubSampledLum();

  for ( auto mbit = new_pic.mbinfo.begin();
	mbit != new_pic.mbinfo.end();
	mbit++ ) {
    mbit->MotionEstimateAndModeSelect();
  }

  for ( auto mbit = new_pic.mbinfo.begin();
	mbit != new_pic.mbinfo.end();
	mbit++ ) {
    mbit->Encode();
  }

  new_pic.PutHeaders();

  //  new_pic.CommitCoding();

  fprintf( stderr, "Writing %d bytes... ", (int)output.str().size() );
  fwrite( output.str().data(), output.str().size(), 1, stdout );
  fprintf( stderr, "done.\n" );

  return 0;
}
