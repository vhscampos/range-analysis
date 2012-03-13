/****************************************************************************
 *
 *  This benchmark was taken from Intel's MMX website.
 *
 ***************************************************************************/

#define RED   0
#define GREEN 1
#define BLUE  2

#define BYTE       unsigned char
#define RGBQWORD   unsigned
#define WORD       unsigned
#define DWORD      unsigned
#define LPRGBQWORD unsigned

void main ()
{
  BYTE       TextureMap[4][128];
  RGBQWORD   ColorLookupTable[4][4];
  DWORD      dwUVal;
  DWORD      dwVVal;
  LPRGBQWORD lpRGBOut[4];

  DWORD dwUDelta;
  DWORD dwVDelta;
  DWORD dwUInt;
  DWORD dwVInt;  

  RGBQWORD *RGB1;
  RGBQWORD *RGB2;
  RGBQWORD *RGB3;
  RGBQWORD *RGB4;

  LPRGBQWORD lpRGBOutR1;
  LPRGBQWORD lpRGBOutR2;
  LPRGBQWORD lpRGBOutR3;
  LPRGBQWORD lpRGBOutR4;
  LPRGBQWORD lpRGBOutG1;
  LPRGBQWORD lpRGBOutG2;
  LPRGBQWORD lpRGBOutG3;
  LPRGBQWORD lpRGBOutG4;
  LPRGBQWORD lpRGBOutB1;
  LPRGBQWORD lpRGBOutB2;
  LPRGBQWORD lpRGBOutB3;
  LPRGBQWORD lpRGBOutB4;

  WORD w1, w2, w3, w4;
  DWORD dwTemp1, dwTemp2, dwTemp3, dwTemp4;
  unsigned tmp;

  /* provide some fake initialization. */
  dwUVal = rand ();
  dwVVal = rand ();

  tmp = (dwUVal & 0x003fffffL);
  dwUDelta = tmp >> 6;
  dwVDelta = (dwVVal & 0x003fffffL) >> 6;
  dwUInt   = dwUVal >> 22;
  dwVInt   = dwVVal >> 22;

  RGB1 = ColorLookupTable[TextureMap[dwVInt][dwUInt]];
  RGB2 = ColorLookupTable[TextureMap[dwVInt][dwUInt + 1]];
  RGB3 = ColorLookupTable[TextureMap[dwVInt + 1][dwUInt]];
  RGB4 = ColorLookupTable[TextureMap[dwVInt + 1][dwUInt + 1]];

  dwTemp1 = (0x10000L - dwUDelta) * (0x10000L - dwVDelta);
  dwTemp2 = dwUDelta * (0x10000L - dwVDelta);
  dwTemp3 = (0x10000L - dwUDelta) * dwVDelta;
  dwTemp4 = dwUDelta * dwVDelta;

  dwTemp1 = dwTemp1 >> 16;
  dwTemp2 = dwTemp2 >> 16;
  dwTemp3 = dwTemp3 >> 16;
  dwTemp4 = dwTemp4 >> 16;

  /* compute B */
  w1 = (RGB1[BLUE] >> 7) & 0xff;
  w2 = (RGB2[BLUE] >> 7) & 0xff;
  w3 = (RGB3[BLUE] >> 7) & 0xff;
  w4 = (RGB4[BLUE] >> 7) & 0xff;

  lpRGBOutB1 = ((w1 * dwTemp1) >> 16);
  lpRGBOutB2 = ((w2 * dwTemp2) >> 16);
  lpRGBOutB3 = ((w3 * dwTemp3) >> 16);
  lpRGBOutB4 = ((w4 * dwTemp4) >> 16);

  lpRGBOut[BLUE] = lpRGBOutB1 + lpRGBOutB2 + lpRGBOutB3 + lpRGBOutB4;

  /* compute G */
  w1 = (RGB1[GREEN] >> 7) & 0xff;
  w2 = (RGB2[GREEN] >> 7) & 0xff;
  w3 = (RGB3[GREEN] >> 7) & 0xff;
  w4 = (RGB4[GREEN] >> 7) & 0xff;
  lpRGBOutG1 = ((w1 * dwTemp1) >> 16);
  lpRGBOutG2 = ((w2 * dwTemp2) >> 16);
  lpRGBOutG3 = ((w3 * dwTemp3) >> 16);
  lpRGBOutG4 = ((w4 * dwTemp4) >> 16);
  
  lpRGBOut[GREEN] = lpRGBOutG1 + lpRGBOutG2 + lpRGBOutG3 + lpRGBOutG4;
  
  /* compute R */
  w1 = (RGB1[RED] >> 7) & 0xff;
  w2 = (RGB2[RED] >> 7) & 0xff;
  w3 = (RGB3[RED] >> 7) & 0xff;
  w4 = (RGB4[RED] >> 7) & 0xff;
  lpRGBOutR1 = ((w1 * dwTemp1) >> 16);
  lpRGBOutR2 = ((w2 * dwTemp2) >> 16);
  lpRGBOutR3 = ((w2 * dwTemp3) >> 16);
  lpRGBOutR4 = ((w2 * dwTemp4) >> 16);
  
  lpRGBOut[RED] = lpRGBOutG1 + lpRGBOutG2 + lpRGBOutG3 + lpRGBOutG4;
}
