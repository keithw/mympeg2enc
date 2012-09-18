#include "mpeg2frame.hh"

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
#include "ontheflyratectlpass1.hh"
#include "ontheflyratectlpass2.hh"

void initialize_mpeg2( void )
{
  static bool initialized = false;
  if ( initialized ) {
    return;
  }

  init_motion_search();
  init_transform();
  init_predict();

  initialized = true;
}

static void setup_picture( Picture & pic, Picture *const old, const PICTURE_CODING type, const EncoderParams & my_params )
{
  pic.new_seq = pic.end_seq = true; /* XXX */
  pic.gop_start = true; /* XXX */
  pic.gop_decode = pic.bgrp_decode = 0;
  pic.decode = 1;
  pic.present = 1;
  pic.last_picture = false;
  pic.finalfield = true;
  pic.fwd_ref_frame = pic.bwd_ref_frame = NULL;
  pic.fwd_org = pic.bwd_org = NULL;
  pic.fwd_rec = pic.bwd_rec = NULL;
  pic.nb = pic.np = 0;
  pic.closed_gop = false;
  pic.dc_prec = my_params.dc_prec;

  ImagePlanes *pic_orig = new ImagePlanes( my_params );

  /* user will have to initialize image data */
  /*
  memset( pic_orig->Plane( 0 ), 128, my_params.lum_buffer_size );
  memset( pic_orig->Plane( 1 ), 128, my_params.chrom_buffer_size );
  memset( pic_orig->Plane( 2 ), 128, my_params.chrom_buffer_size );

  memset( pic.rec_img->Plane( 0 ), 128, my_params.lum_buffer_size );
  memset( pic.rec_img->Plane( 1 ), 128, my_params.chrom_buffer_size );
  memset( pic.rec_img->Plane( 2 ), 128, my_params.chrom_buffer_size );
  */

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

  pic.unit_coeff_threshold = 30;
  pic.unit_coeff_first = 0;
}

MPEG2EncOptions MPEG2Frame::make_options( const int height, const int width )
{
  /* set up MPEG-2 parameters */

  initialize_mpeg2();

  MPEG2EncOptions my_options;

  MPEG2EncInVidParams my_vid_params;
  my_vid_params.horizontal_size = height;
  my_vid_params.vertical_size = width;
  my_vid_params.aspect_ratio_code = 2;
  my_vid_params.frame_rate_code = 5;
  my_vid_params.interlacing_code = Y4M_ILACE_NONE;

  my_options.video_buffer_size = 4000;
  my_options.bitrate = -1;
  my_options.format = MPEG_FORMAT_SVCD_STILL;
  assert( my_options.SetFormatPresets( my_vid_params ) == false );

  my_options.stream_frames = 1;
  my_options.ignore_constraints = true;

  return my_options;
}


MPEG2Frame::MPEG2Frame( const int height, const int width )
  : _options( make_options( height, width ) ),
    _params( _options ),
    _pic( NULL ),
    _quantizer( _params ),
    _height( height ),
    _width( width ),
    _output()
{
  _params.Init( _options );
  _quantizer.Init();

  _params.coding_tolerance = 0;

  _pic = new Picture( _params, _output, _quantizer );

  /* set up picture */
  setup_picture( *_pic, NULL, P_TYPE, _params );
}

MPEG2Frame::MPEG2Frame( const int height, const int width, uint8_t shade )
  : _options( make_options( height, width ) ),
    _params( _options ),
    _pic( NULL ),
    _quantizer( _params ),
    _height( height ),
    _width( width ),
    _output()
{
  _params.Init( _options );
  _quantizer.Init();

  _params.coding_tolerance = 0;

  _pic = new Picture( _params, _output, _quantizer );

  /* set up picture */
  setup_picture( *_pic, NULL, P_TYPE, _params );

  memset( _pic->org_img->Plane( 0 ), shade, _params.lum_buffer_size );
  memset( _pic->org_img->Plane( 1 ), 128, _params.chrom_buffer_size );
  memset( _pic->org_img->Plane( 2 ), 128, _params.chrom_buffer_size );  
  memset( _pic->rec_img->Plane( 0 ), shade, _params.lum_buffer_size );
  memset( _pic->rec_img->Plane( 1 ), 128, _params.chrom_buffer_size );
  memset( _pic->rec_img->Plane( 2 ), 128, _params.chrom_buffer_size );  
}

MPEG2Frame::~MPEG2Frame()
{
  delete _pic;
}

std::string MPEG2Frame::diff_from( const MPEG2Frame &existing, const size_t len ) const
{
  /* encode */

  _output.clear();

  assert( _height == existing.height() );
  assert( _width == existing.width() );

  EncoderParams my_params( _params );

  my_params.still_size = len;
  my_params.coding_tolerance = 0;
  my_params.vbv_buffer_still_size = len;

  _pic->fwd_ref_frame = existing.get_pic();
  _pic->fwd_org = existing.get_pic()->rec_img; /* XXX motion vector search works better if we keep original frame around, but we won't do this */
  _pic->fwd_rec = _pic->fwd_org;

  _pic->MotionSubSampledLum();

  for ( auto mbit = _pic->mbinfo.begin();
	mbit != _pic->mbinfo.end();
	mbit++ ) {
    mbit->MotionEstimateAndModeSelect();
  }

  OnTheFlyPass1 rc1( my_params );
  rc1.Init();
  rc1.GopSetup( 1, 0 );
  rc1.PictSetup( *_pic );

  for ( auto mbit = _pic->mbinfo.begin();
	mbit != _pic->mbinfo.end();
	mbit++ ) {
    mbit->Encode();
  }

  _pic->PutHeaders();
  _pic->QuantiseAndCode( rc1 );
  int pad;
  rc1.PictUpdate( *_pic, pad );
  _pic->PutTrailers( 0 );
  _pic->Reconstruct();

  /* prepare for second pass */
  std::deque< Picture *> picture_deque;
  picture_deque.push_back( _pic );

  OnTheFlyPass2 rc2( my_params );
  rc2.Init();
  rc2.GopSetup( picture_deque.begin(), picture_deque.end() );
  rc2.PictSetup( *_pic );

  _pic->DiscardCoding();

  _pic->PutHeaders();
  _pic->QuantiseAndCode( rc2 );
  rc2.PictUpdate( *_pic, pad );
  _pic->PutTrailers( 0 );
  _pic->Reconstruct();

  _pic->CommitCoding();

  return _output.str();
}
