/****************************************************************************
 *
 *  Motiontest benchmark.
 *
 ***************************************************************************/

int  gWidth = 32;
char gComp[16384];
char gRef[16384];

void main()
{
  int data;
  int i;
  int j;
  char *bptr; /* pointer to start row of 16x16 pixel block being compressed.*/
  char *cptr; /* pointer to start row of 16x16 pixel reference block.*/
  int val=0;

  for (i = 0; i < 16384; i++) {
    gComp[i] = rand() & 0xff;
    gRef[i] = rand() & 0xff;
  }
  bptr = gComp;
  cptr = gRef;

  for(i=0;i<16;i++)
    {
      for(j=0;j<16;j++)
        {
          data=(*(bptr++)-*(cptr++));
          if (data<0) {val-=data;}
          else    {val+=data;}
        }

      /* Fast out after this row if we've exceeded best match*/
      if (val > 0xffff) break;
      /* update pointer to next row*/
      bptr += (gWidth - 16);
      /* update pointer to next row */
      cptr += (gWidth - 16);
    }
}
