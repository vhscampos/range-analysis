/* Changes from original: 9/15/99 JWB
 * removed struct
 * changed all types to int
 * replaced I/O with dummy data setup
 * added result checksum
 * reversed direction of for loop
 * removed decoder
 * replaced some control flow with logic
 * inlined coder into test code
 * got rid of global variables
 * 
 * optimized for compiling with -fmacro to muxify if's
 *
 * This version has broken input and initialization data but should give
 * approximately correct hardware cycle counts and area for adpcm
 *
 * stepSizeTable needs to be initialized correctly for true result 
 */

/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.0, 7-Jul-92.
*/

#define NSAMPLES 2407 
#define MAXITER 2
#define ANS 123608

#define SIZE NSAMPLES
#define WIDTH 8

#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif


int stepsizeTable[89] = {
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
  19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
  130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
  337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
  2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
  5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


main() {

  int indexTable[16];
  
  int iter, checksum, i, j;
  
  int	abuf[(NSAMPLES/2)+1];
  int	sbuf[NSAMPLES];
  
  int val;			/* Current input sample value */
  int sign;			/* Current adpcm sign bit */
  int delta;			/* Current adpcm output value */
  int step;			/* Stepsize */
  int vpdiff;			/* Current change to valprev */
  int index;			/* Current step change index */
  int outputbuffer;		/* place to keep previous 4-bit value */
  int valprev;
  int tmp;

#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif


  /* Intel ADPCM step variation table */
  indexTable[0] = -1;
  indexTable[1] = -1;
  indexTable[2] = -1;
  indexTable[3] = -1;
  indexTable[4] = 2;
  indexTable[5] = 4;
  indexTable[6] = 6;
  indexTable[7] = 8;

  /* bogus stepsize table because I don't want to type it in */
  /*   for(i=0;i<88;i++) */
  /*     stepsizeTable[i]=7; */
  /*   stepsizeTable[88] = 32767; */
  
  checksum = 0; 
  for(i=0;i<NSAMPLES;i++) { 
    sbuf[i]=i & 0xFFFF; 
  } 

  for(iter=0;iter<MAXITER;iter++) {

    valprev = 1;
    index = 1;
    step = stepsizeTable[index];
    
    j = 0;
    for ( i = 0; i < NSAMPLES; i++ ) {
      val = sbuf[i];

      /* Step 1 - compute difference with previous value */
      delta = val - valprev;
      sign = (delta < 0) & 8;
      if ( delta < 0 ) delta = (-delta);

      /* Step 2 - Divide and clamp */

      vpdiff = 0;
      tmp = 0;
      if ( delta > step ) {
	tmp = 4;
	delta -= step;
	vpdiff = step;
      }
      step >>= 1;
      if ( delta > step  ) {
	tmp |= 2;
	delta -= step;
	vpdiff += step;
      }
      step >>= 1;
      if ( delta > step ) {
	tmp |= 1;
	vpdiff += step;
      }
      delta = tmp;
	  
      /* Step 3 - Update previous value */
      if ( sign )
	valprev -= vpdiff;
      else
	valprev += vpdiff;

      /* Step 4 - Clamp previous value to 16 bits */
      if ( valprev > 32767 )
	valprev = 32767;
      else if ( valprev < -32768 )
	valprev = -32768;

      /* Step 5 - Assemble value, update index and step values */
      delta |= sign;
	
      index += indexTable[delta & 7];
      if ( index < 0 ) index = 0;
      if ( index > 88 ) index = 88;
      step = stepsizeTable[index];

      /* Step 6 - Output value */
      /* this only has meaning every other cycle:
       * shouldn't we unroll this loop twice? */
      if(i & 1)
	abuf[j] = (delta & 0x0f) | (outputbuffer << 4) &0xf0;
      outputbuffer = delta;
      j+= i & 1;
    }
    
    /* Output last step, if needed */
    if ( NSAMPLES & 1 ) {
      abuf[j] = outputbuffer;
    }
  }      

  for(j=0;j<(NSAMPLES/2)+1;j++)
    checksum += abuf[j];

#ifdef TIME_SOFTWARE
#include "../../../include/time.end"
#endif

#ifdef PRINTOK
  printf("RESULT: Checksum %d\n", checksum);
#endif
  
  if(checksum==ANS) {
#ifdef PRINTOK
    printf("RESULT: Success!\n");
    printf("RESULT: Return-value %d (%x) \n", 0xbadbeef, 0xbadbeef);
#endif
    return 0xbadbeef;
  }
  else {
#ifdef PRINTOK
    printf("RESULT: Failure!\n");
    printf("RESULT: Return-value %d (%x) \n", 0xbad, 0x0bad);
#endif
    return 0xbad;
  }
}
