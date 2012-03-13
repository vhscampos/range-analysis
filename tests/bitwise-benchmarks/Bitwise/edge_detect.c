/****************************************************************************
 *
 *  This benchmark was taken from Intel's MMX website.
 *
 ***************************************************************************/

/* ISIZE was 128, reduced to 4 to speed simulation time */
#define ISIZE      4
#define JSIZE    128

#define SIZE    ISIZE * JSIZE
#define WIDTH   8
#define ANS 33150
/* note: WIDTH shouldn't be changed */

#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif

int main() {

  int temp1, temp2, temp3, temp4;
  int i;
  int j;
  int checksum, iter;

  int r;
  int c;
  int dead_rows;
  int dead_cols;
  int sum;
  int normal_factor;
  
  int image_buffer1[ISIZE][JSIZE];
  int image_buffer2[ISIZE][JSIZE];
  int image_buffer3[ISIZE][JSIZE];
  int filter[3][3];
  
#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif

  for (i = 0; i < ISIZE; i++)
    for (j = 0; j < JSIZE; j++) {
      image_buffer1[i][j] = (i<ISIZE/2) ? 255 : 0; /* should create an edge */
      image_buffer2[i][j] = 0;
      image_buffer3[i][j] = 0;
    }

  /* performs three convolutions without procedure calls */
  for(iter=0;iter<3; iter++) {

  /* if we got rid of this, then macros would work */
    if(iter==0) {
      filter[0][0] = 1;
      filter[0][1] = 2;
      filter[0][2] = 1;
      filter[1][0] = 2;
      filter[1][1] = 4;
      filter[1][2] = 2;
      filter[2][0] = 1;
      filter[2][1] = 2;
      filter[2][2] = 1;
    }
    if(iter==1) {
      filter[0][0] = 1;
      filter[0][1] = 0;
      filter[0][2] = -1;
      filter[1][0] = 2;
      filter[1][1] = 0;
      filter[1][2] = -2;
      filter[2][0] = 1;
      filter[2][1] = 0;
      filter[2][2] = -1;
    }
    if(iter==2) {
      filter[0][0] = 1;
      filter[0][1] = 2;
      filter[0][2] = 1;
      filter[1][0] = 0;
      filter[1][1] = 0;
      filter[1][2] = 0;
      filter[2][0] = -1;
      filter[2][1] = -2;
      filter[2][2] = -1;
    }
    

    dead_rows = 1;
    dead_cols = 1;
    normal_factor = 0;
    for (r = 0; r < 3; r++)
      for (c = 0; c < 3; c++) {
	int tmp = filter[r][c];
	if (tmp < 0) tmp = -tmp;
	normal_factor = normal_factor + tmp;
      }

      if (normal_factor == 0) normal_factor = 1;
      
      for (r = 0; r < ISIZE-2; r++)
	for (c = 0; c < JSIZE-2; c++) {
	  int out, in0, in1, in2, in3, in4, in5, in6, in7, in8;
	  
	  in0 = (iter == 0) ?
	    image_buffer1[r+0][c+0] : image_buffer3[r+0][c+0];
	  in1 = (iter == 0) ?
	    image_buffer1[r+0][c+1] : image_buffer3[r+0][c+1];
	  in2 = (iter == 0) ?
	    image_buffer1[r+0][c+2] : image_buffer3[r+0][c+2];
	  in3 = (iter == 0) ?
	    image_buffer1[r+1][c+0] : image_buffer3[r+1][c+0];
	  in4 = (iter == 0) ?
	    image_buffer1[r+1][c+1] : image_buffer3[r+1][c+1];
	  in5 = (iter == 0) ?
	    image_buffer1[r+1][c+2] : image_buffer3[r+1][c+2];
	  in6 = (iter == 0) ?
	    image_buffer1[r+2][c+0] : image_buffer3[r+2][c+0];
	  in7 = (iter == 0) ?
	    image_buffer1[r+2][c+1] : image_buffer3[r+2][c+1];
	  in8 = (iter == 0) ?
	    image_buffer1[r+2][c+2] : image_buffer3[r+2][c+2];
	  
	  sum = in0 * filter[0][0]
	    + in1 * filter[0][1]
	    + in2 * filter[0][2]
	    + in3 * filter[1][0]
	    + in4 * filter[1][1]
	    + in5 * filter[1][2]
	    + in6 * filter[2][0]
	    + in7 * filter[2][1]
	    + in8 * filter[2][2];
	  out = sum / normal_factor;

	  if(iter==0) 
	    image_buffer3[r + dead_rows][c + dead_cols] = out;
	  else if(iter ==  1)
	    image_buffer1[r + dead_rows][c + dead_cols] = out;
	  else
	    image_buffer2[r + dead_rows][c + dead_cols] = out;	  
	}
  }

  checksum = 0;
  for (i = 0; i < ISIZE; i++)
    for (j = 0; j < JSIZE; j++) {		
      temp1 = image_buffer1[i][j];
      if (temp1 < 0) temp1 = -temp1;
      temp2 = image_buffer2[i][j];
      if (temp2 < 0) temp2 = -temp2;
      temp3 = (temp2<temp1) ? temp1 : temp2;
      temp4 = (127 < temp3) ? 255 : 0;
      
      image_buffer3[i][j] = temp4;
      
      checksum += temp4;
    }

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
