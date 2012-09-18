#include <assert.h>
#include <string>
#include <stdio.h>

#include "mpeg2frame.hh"

using namespace std;

int main( void )
{
  MPEG2Frame one( 640, 480, 16 ), two( 640, 480, 235 );

  const string diff( two.diff_from( one, 10000 ) );

  fwrite( diff.data(), diff.size(), 1, stdout );
}
