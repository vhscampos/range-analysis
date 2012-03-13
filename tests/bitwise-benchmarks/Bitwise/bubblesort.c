/****************************************************************************
 *
 *  The bubblesort benchmark.
 *
 ***************************************************************************/

#define SIZE  512
#define WIDTH 16
#define ANS   10880

#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif

int main()
{
  int i,j,top,checksum,data,sortlist_odd[SIZE/2],sortlist_even[SIZE/2];
  int x, s1, s2, ie, io;

#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif

  for (i=0; i<SIZE/2; i++) {
    sortlist_even[i] = (SIZE-(i << 1)       ) | (1 << (WIDTH-1));
    sortlist_odd[i]  = (SIZE-((i << 1) | 1) ) | (1 << (WIDTH-1));
  }
    
  for(top=SIZE-1;top>0;top--) {
    for(i=0;i<top;i++) {
      io = i >> 1;
      ie = io + (i & 1);
      s1=sortlist_even[ie];
      s2=sortlist_odd[io];
      if(s1 > s2 ^ (i & 1)) {
	sortlist_even[ie] = s2;
	sortlist_odd[io]  = s1;
      }
    }
  }
  
  checksum = 0;
  for (i=0; i<SIZE/2; i++)
    checksum = (checksum + sortlist_even[i]*i) & 0xffff;

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
