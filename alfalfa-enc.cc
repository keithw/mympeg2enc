#include <assert.h>
#include <string>
#include <stdio.h>

#include "mpeg2frame.hh"

using namespace std;

int main( void )
{
  MPEG2Frame frone( 640, 480, 16 ), two( 640, 480, 235 ), three( 640, 480, 192 ),
    four( 640, 480, 16 );

  const string diff( two.diff_from( frone, 2000 ) );
  const string diff2( three.diff_from( two, 2000 ) );
  string diff3( four.diff_from( three, 2000 ) );

  uint8_t pic[ 4 ] = { 0, 0, 1, 183 };

  diff3 += string( (char *)pic, 4 );

  fwrite( diff.data(), diff.size(), 1, stdout );
  fwrite( diff2.data(), diff2.size(), 1, stdout );
  fwrite( diff3.data(), diff3.size(), 1, stdout );
}
