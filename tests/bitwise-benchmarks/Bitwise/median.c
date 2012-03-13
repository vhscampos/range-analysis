/****************************************************************************
 *
 *  Median filter benchmark.
 *
 ***************************************************************************/

#define ANS 1536

#define SIZE 1024
#define WIDTH 31
#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif

int main(void)
{
  int colA[SIZE+2];
  int colB[SIZE+2];
  int colC[SIZE+2];
  int output[SIZE];
  int newArray[5];
  int sortMe[9];
  int i,j,k, temp, checksum;
  
#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif
  
  /* fool bitwidth analyzer since data is initialized here */
  colA[0]   =(1<<WIDTH)-1;
  colB[0]   =(1<<WIDTH)-1;
  colC[0]   =(1<<WIDTH)-1;
  sortMe[0] =(1<<WIDTH)-1;

  for (k = 0; k< SIZE+2; k++) {
    colA[k]=k;
    colB[k]=k+1;
    colC[k]=k+2;
  }

  for (k = 0; k < SIZE; k++)
  {
    sortMe[0] = colA[k];
    sortMe[1] = colA[k+1];
    sortMe[2] = colA[k+2];
    sortMe[3] = colB[k];
    sortMe[4] = colB[k+1];
    sortMe[5] = colB[k+2];
    sortMe[6] = colC[k];
    sortMe[7] = colC[k+1];
    sortMe[8] = colC[k+2];    
    
    for (i = 0; i < 5; i++)
      newArray[i] = 0;
    
    for (i = 0; i < 9; i++)
    {
      temp = sortMe[i];
      for (j = 0; j < 5; j++)
	{
	int temp1 = temp;
	int temp2 = newArray[j];
	if (temp1 > temp2) {
	  temp = temp2;
	  newArray[j] = temp1;
	}
      }
    }
    output[k] = newArray[4];
  }

  checksum=0;
  for (k = 0; k < SIZE; k++)
    checksum = (checksum + output[k]) & 32767;

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
