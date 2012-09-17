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
};

class OneShotRateState : public RateCtlState
{
  
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

  /* initialize picture */
  Picture allblack( my_params, output, quantizer );
  allblack.new_seq = allblack.end_seq = false;
  allblack.gop_decode = allblack.bgrp_decode = 0;
  allblack.decode = 0;
  allblack.present = 0;
  allblack.last_picture = false;
  allblack.finalfield = true;
  allblack.fwd_ref_frame = NULL;
  allblack.bwd_ref_frame = NULL;
  allblack.fwd_org = allblack.bwd_org = NULL;
  allblack.fwd_rec = allblack.bwd_rec = NULL;
  allblack.nb = allblack.np = 0;
  allblack.closed_gop = false;
  allblack.dc_prec = my_params.dc_prec;

  ImagePlanes allblack_orig( my_params );

  allblack.org_img = &allblack_orig;

  allblack.sxb = allblack.syb = 0;
  allblack.secondfield = false;
  allblack.ipflag = false;

  allblack.temp_ref = 0;
  allblack.frame_type = I_TYPE;
  allblack.gop_decode = 0;
  allblack.bgrp_decode = 0;
  allblack.pict_type = I_TYPE;
  allblack.vbv_delay = 0;
  allblack.pict_struct = FRAME_PICTURE;
  
  allblack.forw_hor_f_code = 15; //my_params.motion_data[0].forw_hor_f_code;
  allblack.forw_vert_f_code = 15; //my_params.motion_data[0].forw_vert_f_code;
  allblack.back_hor_f_code = 15;
  allblack.back_vert_f_code = 15;
  allblack.sxf = my_params.motion_data[0].sxf;
  allblack.syf = my_params.motion_data[0].syf;

  allblack.prog_frame = true;
  allblack.frame_pred_dct = true;
  allblack.q_scale_type = 1;
  allblack.intravlc = 0;
  allblack.altscan = false;
  allblack.scan_pattern = zig_zag_scan;

  allblack.unit_coeff_threshold = allblack.unit_coeff_first = 0;

  return 0;
}
