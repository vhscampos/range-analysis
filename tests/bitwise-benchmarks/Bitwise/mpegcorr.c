/********************************************************************
 *
 * Note: This is only a small portion of mpeg's kernel.  It is
 *       one of the benchmarks used in the RAW benchmark suite.
 *
 ********************************************************************/


#define ANS 6016

#define SIZE 16 * 16
#define WIDTH 16
#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif


#define DCTSIZE     8 

#define MOTION_TO_FRAME_COORD(bx1, bx2, mx1, mx2, x1, x2) { \
	x1 = (bx1)*DCTSIZE+(mx1);			    \
	x2 = (bx2)*DCTSIZE+(mx2);			    \
    }

#define ABS(x) (((x)<0)?-(x):(x))

int main () {  
  int currentBlock[16][16];
  int blockSoFar[16][16];
  int prev[32][32];
  int mx = 4;
  int my = 4;
  int bx = 4;
  int by = 4;
  int diff = 0;

  int localDiff;
  int y;
  int x;
  int yHalf;
  int xHalf;
  int fy;
  int fx;
  int lumMotionBlock[16][16];
  int checksum;

#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif

  for(y=0;y<16;y++) {
    for(x=0;x<16;x++) {
      currentBlock[x][y] = x + (1<<(WIDTH-2));
      blockSoFar[x][y] = x+y + (1<<(WIDTH-2));
    }
  }
  
  for(y=0;y<32;y++) {
    for(x=0;x<32;x++) {
      prev[x][y] = x+y  + (1<<(WIDTH-2));
    }
  }

  xHalf = (ABS(mx) % 2 == 1);
  yHalf = (ABS(my) % 2 == 1);

  MOTION_TO_FRAME_COORD(by, bx, my/2, mx/2, fy, fx);
  
  if ( xHalf ) {
    if ( mx < 0 ) {
      fx--;
    }

    if ( yHalf ) {
      if ( my < 0 ) {
	fy--;
      }
    }
    else if ( yHalf ) {
      if ( my < 0 ) {
	fy--;
      }
      
    }
  }
    
  diff = 0;
  
  /* this version was broken, at least have a correct C program */
  if(fx>16) fx = 16;
  if(fy>16) fy = 16;

  for ( y = 0; y < 16; y++)
    for ( x = 0; x < 16; x++)
      lumMotionBlock[y][x] = prev[y+fy][x+fx];

  for ( y = 0; y < 16; y++ )
    for ( x = 0; x < 16; x++ )
    {
      localDiff = ((lumMotionBlock[y][x] + blockSoFar[y][x] + 1) >> 1)
	- currentBlock[y][x];
	
      diff += ABS(localDiff);
    }

  checksum = diff;

#ifdef TIME_SOFTWARE
#include "../../../include/time.end"
#endif

#ifdef PRINTOK
  printf("Checksum = %d\n", checksum);
#endif
  
  if(checksum==ANS) {
#ifdef PRINTOK
    printf("RESULT: Success!\n");
    printf("RESULT: Return = %d (%x) \n", 0xbadbeef, 0xbadbeef);
#endif
    return 0xbadbeef;
  }
  else {
#ifdef PRINTOK
    printf("RESULT: Failure!\n");
    printf("RESULT: Return = %d (%x) \n", 0xbad, 0x0bad);
#endif
    return 0xbad;
  }
}
