/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach uh@alumni.caltech edu */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */

/*****************************************************************************
 *
 * And further modified by Matt Frank for RAW
 * mfrank@lcs.mit.edu Wed Jun  3 19:16:01 1998
 *
 ****************************************************************************/

/* IF YOU CHANGE NUM_BLOCKS YOU'LL ALSO NEED TO RERUN THE PROGRAM
 * AND CALCULATE A NEW VALUE FOR RESULT
 */

#define NUM_BLOCKS 512
#define RESULT 0x289a69fd

/* SHA f()-functions */

#define f0(x,y,z)	((x & y) | (~x & z))
#define f1(x,y,z)	(x ^ y ^ z)
#define f2(x,y,z)	((x & y) | (x & z) | (y & z))
#define f3(x,y,z)	(x ^ y ^ z)

/* SHA constants */

#define CONST0		0x5a827999L
#define CONST1		0x6ed9eba1L
#define CONST2		0x8f1bbcdcL
#define CONST3		0xca62c1d6L

/* 32-bit rotate */

#define ROT32(x,n)	((x << n) | (x >> (32 - n)))

#define FUNC(n,i)						\
    temp = ROT32(A,5) + f##n(B,C,D) + E + W##i + CONST##n;	\
    E = D; D = C; C = ROT32(B,30); B = A; A = temp

int
main()
{
  int b;
  unsigned long temp;
  unsigned long Aout, Bout, Cout, Dout, Eout;
  unsigned long data[NUM_BLOCKS][16];

  Aout = 0x67452301L;
  Bout = 0xefcdab89L;
  Cout = 0x98badcfeL;
  Dout = 0x10325476L;
  Eout = 0xc3d2e1f0L;

  /* initialize the data to something random */
  for (b = 0; b < NUM_BLOCKS; b++)
  {
    int j;
    for (j = 0; j < 16; j++)
    {
      data[b][j] = b+j;
    }
  }

  /* here's the transformation */
  for (b = 0; b < NUM_BLOCKS; b++)
  {
    unsigned long A0 = Aout;
    unsigned long A1 = Bout;
    unsigned long A2 = Cout;
    unsigned long A3 = Dout;
    unsigned long A4 = Eout;
    unsigned long W0 = data[b][0];
    unsigned long W1 = data[b][1];
    unsigned long W2 = data[b][2];
    unsigned long W3 = data[b][3];
    unsigned long W4 = data[b][4];
    unsigned long W5 = data[b][5];
    unsigned long W6 = data[b][6];
    unsigned long W7 = data[b][7];
    unsigned long W8 = data[b][8];
    unsigned long W9 = data[b][9];
    unsigned long W10 = data[b][10];
    unsigned long W11 = data[b][11];
    unsigned long W12 = data[b][12];
    unsigned long W13 = data[b][13];
    unsigned long W14 = data[b][14];
    unsigned long W15 = data[b][15];
    unsigned long the_const;

    the_const = CONST0;

    A4 = ROT32(A0,5) + f0(A1,A2,A3) + A4 + W0 + the_const;
    A1 = ROT32(A1,30);

    A3 = ROT32(A4,5) + f0(A0,A1,A2) + A3 + W1 + the_const;
    A0 = ROT32(A0,30);

    A2 = ROT32(A3,5) + f0(A4,A0,A1) + A2 + W2 + the_const;
    A4 = ROT32(A4,30);

    A1 = ROT32(A2,5) + f0(A3,A4,A0) + A1 + W3 + the_const;
    A3 = ROT32(A3,30);

    A0 = ROT32(A1,5) + f0(A2,A3,A4) + A0 + W4 + the_const;
    A2 = ROT32(A2,30);

    A4 = ROT32(A0,5) + f0(A1,A2,A3) + A4 + W5 + the_const;
    A1 = ROT32(A1,30);

    A3 = ROT32(A4,5) + f0(A0,A1,A2) + A3 + W6 + the_const;
    A0 = ROT32(A0,30);

    A2 = ROT32(A3,5) + f0(A4,A0,A1) + A2 + W7 + the_const;
    A4 = ROT32(A4,30);

    A1 = ROT32(A2,5) + f0(A3,A4,A0) + A1 + W8 + the_const;
    A3 = ROT32(A3,30);

    A0 = ROT32(A1,5) + f0(A2,A3,A4) + A0 + W9 + the_const;
    A2 = ROT32(A2,30);

    A4 = ROT32(A0,5) + f0(A1,A2,A3) + A4 + W10 + the_const;
    A1 = ROT32(A1,30);

    A3 = ROT32(A4,5) + f0(A0,A1,A2) + A3 + W11 + the_const;
    A0 = ROT32(A0,30);

    A2 = ROT32(A3,5) + f0(A4,A0,A1) + A2 + W12 + the_const;
    A4 = ROT32(A4,30);

    A1 = ROT32(A2,5) + f0(A3,A4,A0) + A1 + W13 + the_const;
    A3 = ROT32(A3,30);

    A0 = ROT32(A1,5) + f0(A2,A3,A4) + A0 + W14 + the_const;
    A2 = ROT32(A2,30);

    A4 = ROT32(A0,5) + f0(A1,A2,A3) + A4 + W15 + the_const;
    A1 = ROT32(A1,30);

    W0 ^= W13;
    W0 ^= W8;
    W0 ^= W2;
    W0 = ROT32(W0,1);
    A3 = ROT32(A4,5) + f0(A0,A1,A2) + A3 + W0 + the_const;
    A0 = ROT32(A0,30);

    W1 ^= W14;
    W1 ^= W9;
    W1 ^= W3;
    W1 = ROT32(W1,1);
    A2 = ROT32(A3,5) + f0(A4,A0,A1) + A2 + W1 + the_const;
    A4 = ROT32(A4,30);

    W2 ^= W15;
    W2 ^= W10;
    W2 ^= W4;
    W2 = ROT32(W2,1);
    A1 = ROT32(A2,5) + f0(A3,A4,A0) + A1 + W2 + the_const;
    A3 = ROT32(A3,30);

    W3 ^= W0;
    W3 ^= W11;
    W3 ^= W5;
    W3 = ROT32(W3,1);
    A0 = ROT32(A1,5) + f0(A2,A3,A4) + A0 + W3 + the_const;
    A2 = ROT32(A2,30);

    the_const = CONST1;

    W4 ^= W1;
    W4 ^= W12;
    W4 ^= W6;
    W4 = ROT32(W4,1);
    A4 = ROT32(A0,5) + f1(A1,A2,A3) + A4 + W4 + the_const;
    A1 = ROT32(A1,30);

    W5 ^= W2;
    W5 ^= W13;
    W5 ^= W7;
    W5 = ROT32(W5,1);
    A3 = ROT32(A4,5) + f1(A0,A1,A2) + A3 + W5 + the_const;
    A0 = ROT32(A0,30);

    W6 ^= W3;
    W6 ^= W14;
    W6 ^= W8;
    W6 = ROT32(W6,1);
    A2 = ROT32(A3,5) + f1(A4,A0,A1) + A2 + W6 + the_const;
    A4 = ROT32(A4,30);

    W7 ^= W4;
    W7 ^= W15;
    W7 ^= W9;
    W7 = ROT32(W7,1);
    A1 = ROT32(A2,5) + f1(A3,A4,A0) + A1 + W7 + the_const;
    A3 = ROT32(A3,30);

    W8 ^= W5;
    W8 ^= W0;
    W8 ^= W10;
    W8 = ROT32(W8,1);
    A0 = ROT32(A1,5) + f1(A2,A3,A4) + A0 + W8 + the_const;
    A2 = ROT32(A2,30);

    W9 ^= W6;
    W9 ^= W1;
    W9 ^= W11;
    W9 = ROT32(W9,1);
    A4 = ROT32(A0,5) + f1(A1,A2,A3) + A4 + W9 + the_const;
    A1 = ROT32(A1,30);

    W10 ^= W7;
    W10 ^= W2;
    W10 ^= W12;
    W10 = ROT32(W10,1);
    A3 = ROT32(A4,5) + f1(A0,A1,A2) + A3 + W10 + the_const;
    A0 = ROT32(A0,30);

    W11 ^= W8;
    W11 ^= W3;
    W11 ^= W13;
    W11 = ROT32(W11,1);
    A2 = ROT32(A3,5) + f1(A4,A0,A1) + A2 + W11 + the_const;
    A4 = ROT32(A4,30);

    W12 ^= W9;
    W12 ^= W4;
    W12 ^= W14;
    W12 = ROT32(W12,1);
    A1 = ROT32(A2,5) + f1(A3,A4,A0) + A1 + W12 + the_const;
    A3 = ROT32(A3,30);

    W13 ^= W10;
    W13 ^= W5;
    W13 ^= W15;
    W13 = ROT32(W13,1);
    A0 = ROT32(A1,5) + f1(A2,A3,A4) + A0 + W13 + the_const;
    A2 = ROT32(A2,30);

    W14 ^= W11;
    W14 ^= W6;
    W14 ^= W0;
    W14 = ROT32(W14,1);
    A4 = ROT32(A0,5) + f1(A1,A2,A3) + A4 + W14 + the_const;
    A1 = ROT32(A1,30);

    W15 ^= W12;
    W15 ^= W7;
    W15 ^= W1;
    W15 = ROT32(W15,1);
    A3 = ROT32(A4,5) + f1(A0,A1,A2) + A3 + W15 + the_const;
    A0 = ROT32(A0,30);

    W0 ^= W13;
    W0 ^= W8;
    W0 ^= W2;
    W0 = ROT32(W0,1);
    A2 = ROT32(A3,5) + f1(A4,A0,A1) + A2 + W0 + the_const;
    A4 = ROT32(A4,30);

    W1 ^= W14;
    W1 ^= W9;
    W1 ^= W3;
    W1 = ROT32(W1,1);
    A1 = ROT32(A2,5) + f1(A3,A4,A0) + A1 + W1 + the_const;
    A3 = ROT32(A3,30);

    W2 ^= W15;
    W2 ^= W10;
    W2 ^= W4;
    W2 = ROT32(W2,1);
    A0 = ROT32(A1,5) + f1(A2,A3,A4) + A0 + W2 + the_const;
    A2 = ROT32(A2,30);

    W3 ^= W0;
    W3 ^= W11;
    W3 ^= W5;
    W3 = ROT32(W3,1);
    A4 = ROT32(A0,5) + f1(A1,A2,A3) + A4 + W3 + the_const;
    A1 = ROT32(A1,30);

    W4 ^= W1;
    W4 ^= W12;
    W4 ^= W6;
    W4 = ROT32(W4,1);
    A3 = ROT32(A4,5) + f1(A0,A1,A2) + A3 + W4 + the_const;
    A0 = ROT32(A0,30);

    W5 ^= W2;
    W5 ^= W13;
    W5 ^= W7;
    W5 = ROT32(W5,1);
    A2 = ROT32(A3,5) + f1(A4,A0,A1) + A2 + W5 + the_const;
    A4 = ROT32(A4,30);

    W6 ^= W3;
    W6 ^= W14;
    W6 ^= W8;
    W6 = ROT32(W6,1);
    A1 = ROT32(A2,5) + f1(A3,A4,A0) + A1 + W6 + the_const;
    A3 = ROT32(A3,30);

    W7 ^= W4;
    W7 ^= W15;
    W7 ^= W9;
    W7 = ROT32(W7,1);
    A0 = ROT32(A1,5) + f1(A2,A3,A4) + A0 + W7 + the_const;
    A2 = ROT32(A2,30);

    the_const = CONST2;

    W8 ^= W5;
    W8 ^= W0;
    W8 ^= W10;
    W8 = ROT32(W8,1);
    A4 = ROT32(A0,5) + f2(A1,A2,A3) + A4 + W8 + the_const;
    A1 = ROT32(A1,30);

    W9 ^= W6;
    W9 ^= W1;
    W9 ^= W11;
    W9 = ROT32(W9,1);
    A3 = ROT32(A4,5) + f2(A0,A1,A2) + A3 + W9 + the_const;
    A0 = ROT32(A0,30);

    W10 ^= W7;
    W10 ^= W2;
    W10 ^= W12;
    W10 = ROT32(W10,1);
    A2 = ROT32(A3,5) + f2(A4,A0,A1) + A2 + W10 + the_const;
    A4 = ROT32(A4,30);

    W11 ^= W8;
    W11 ^= W3;
    W11 ^= W13;
    W11 = ROT32(W11,1);
    A1 = ROT32(A2,5) + f2(A3,A4,A0) + A1 + W11 + the_const;
    A3 = ROT32(A3,30);

    W12 ^= W9;
    W12 ^= W4;
    W12 ^= W14;
    W12 = ROT32(W12,1);
    A0 = ROT32(A1,5) + f2(A2,A3,A4) + A0 + W12 + the_const;
    A2 = ROT32(A2,30);

    W13 ^= W10;
    W13 ^= W5;
    W13 ^= W15;
    W13 = ROT32(W13,1);
    A4 = ROT32(A0,5) + f2(A1,A2,A3) + A4 + W13 + the_const;
    A1 = ROT32(A1,30);

    W14 ^= W11;
    W14 ^= W6;
    W14 ^= W0;
    W14 = ROT32(W14,1);
    A3 = ROT32(A4,5) + f2(A0,A1,A2) + A3 + W14 + the_const;
    A0 = ROT32(A0,30);

    W15 ^= W12;
    W15 ^= W7;
    W15 ^= W1;
    W15 = ROT32(W15,1);
    A2 = ROT32(A3,5) + f2(A4,A0,A1) + A2 + W15 + the_const;
    A4 = ROT32(A4,30);

    W0 ^= W13;
    W0 ^= W8;
    W0 ^= W2;
    W0 = ROT32(W0,1);
    A1 = ROT32(A2,5) + f2(A3,A4,A0) + A1 + W0 + the_const;
    A3 = ROT32(A3,30);

    W1 ^= W14;
    W1 ^= W9;
    W1 ^= W3;
    W1 = ROT32(W1,1);
    A0 = ROT32(A1,5) + f2(A2,A3,A4) + A0 + W1 + the_const;
    A2 = ROT32(A2,30);

    W2 ^= W15;
    W2 ^= W10;
    W2 ^= W4;
    W2 = ROT32(W2,1);
    A4 = ROT32(A0,5) + f2(A1,A2,A3) + A4 + W2 + the_const;
    A1 = ROT32(A1,30);

    W3 ^= W0;
    W3 ^= W11;
    W3 ^= W5;
    W3 = ROT32(W3,1);
    A3 = ROT32(A4,5) + f2(A0,A1,A2) + A3 + W3 + the_const;
    A0 = ROT32(A0,30);

    W4 ^= W1;
    W4 ^= W12;
    W4 ^= W6;
    W4 = ROT32(W4,1);
    A2 = ROT32(A3,5) + f2(A4,A0,A1) + A2 + W4 + the_const;
    A4 = ROT32(A4,30);

    W5 ^= W2;
    W5 ^= W13;
    W5 ^= W7;
    W5 = ROT32(W5,1);
    A1 = ROT32(A2,5) + f2(A3,A4,A0) + A1 + W5 + the_const;
    A3 = ROT32(A3,30);

    W6 ^= W3;
    W6 ^= W14;
    W6 ^= W8;
    W6 = ROT32(W6,1);
    A0 = ROT32(A1,5) + f2(A2,A3,A4) + A0 + W6 + the_const;
    A2 = ROT32(A2,30);

    W7 ^= W4;
    W7 ^= W15;
    W7 ^= W9;
    W7 = ROT32(W7,1);
    A4 = ROT32(A0,5) + f2(A1,A2,A3) + A4 + W7 + the_const;
    A1 = ROT32(A1,30);

    W8 ^= W5;
    W8 ^= W0;
    W8 ^= W10;
    W8 = ROT32(W8,1);
    A3 = ROT32(A4,5) + f2(A0,A1,A2) + A3 + W8 + the_const;
    A0 = ROT32(A0,30);

    W9 ^= W6;
    W9 ^= W1;
    W9 ^= W11;
    W9 = ROT32(W9,1);
    A2 = ROT32(A3,5) + f2(A4,A0,A1) + A2 + W9 + the_const;
    A4 = ROT32(A4,30);

    W10 ^= W7;
    W10 ^= W2;
    W10 ^= W12;
    W10 = ROT32(W10,1);
    A1 = ROT32(A2,5) + f2(A3,A4,A0) + A1 + W10 + the_const;
    A3 = ROT32(A3,30);

    W11 ^= W8;
    W11 ^= W3;
    W11 ^= W13;
    W11 = ROT32(W11,1);
    A0 = ROT32(A1,5) + f2(A2,A3,A4) + A0 + W11 + the_const;
    A2 = ROT32(A2,30);

    the_const = CONST3;

    W12 ^= W9;
    W12 ^= W4;
    W12 ^= W14;
    W12 = ROT32(W12,1);
    A4 = ROT32(A0,5) + f3(A1,A2,A3) + A4 + W12 + the_const;
    A1 = ROT32(A1,30);

    W13 ^= W10;
    W13 ^= W5;
    W13 ^= W15;
    W13 = ROT32(W13,1);
    A3 = ROT32(A4,5) + f3(A0,A1,A2) + A3 + W13 + the_const;
    A0 = ROT32(A0,30);

    W14 ^= W11;
    W14 ^= W6;
    W14 ^= W0;
    W14 = ROT32(W14,1);
    A2 = ROT32(A3,5) + f3(A4,A0,A1) + A2 + W14 + the_const;
    A4 = ROT32(A4,30);

    W15 ^= W12;
    W15 ^= W7;
    W15 ^= W1;
    W15 = ROT32(W15,1);
    A1 = ROT32(A2,5) + f3(A3,A4,A0) + A1 + W15 + the_const;
    A3 = ROT32(A3,30);

    W0 ^= W13;
    W0 ^= W8;
    W0 ^= W2;
    W0 = ROT32(W0,1);
    A0 = ROT32(A1,5) + f3(A2,A3,A4) + A0 + W0 + the_const;
    A2 = ROT32(A2,30);

    W1 ^= W14;
    W1 ^= W9;
    W1 ^= W3;
    W1 = ROT32(W1,1);
    A4 = ROT32(A0,5) + f3(A1,A2,A3) + A4 + W1 + the_const;
    A1 = ROT32(A1,30);

    W2 ^= W15;
    W2 ^= W10;
    W2 ^= W4;
    W2 = ROT32(W2,1);
    A3 = ROT32(A4,5) + f3(A0,A1,A2) + A3 + W2 + the_const;
    A0 = ROT32(A0,30);

    W3 ^= W0;
    W3 ^= W11;
    W3 ^= W5;
    W3 = ROT32(W3,1);
    A2 = ROT32(A3,5) + f3(A4,A0,A1) + A2 + W3 + the_const;
    A4 = ROT32(A4,30);

    W4 ^= W1;
    W4 ^= W12;
    W4 ^= W6;
    W4 = ROT32(W4,1);
    A1 = ROT32(A2,5) + f3(A3,A4,A0) + A1 + W4 + the_const;
    A3 = ROT32(A3,30);

    W5 ^= W2;
    W5 ^= W13;
    W5 ^= W7;
    W5 = ROT32(W5,1);
    A0 = ROT32(A1,5) + f3(A2,A3,A4) + A0 + W5 + the_const;
    A2 = ROT32(A2,30);

    W6 ^= W3;
    W6 ^= W14;
    W6 ^= W8;
    W6 = ROT32(W6,1);
    A4 = ROT32(A0,5) + f3(A1,A2,A3) + A4 + W6 + the_const;
    A1 = ROT32(A1,30);

    W7 ^= W4;
    W7 ^= W15;
    W7 ^= W9;
    W7 = ROT32(W7,1);
    A3 = ROT32(A4,5) + f3(A0,A1,A2) + A3 + W7 + the_const;
    A0 = ROT32(A0,30);

    W8 ^= W5;
    W8 ^= W0;
    W8 ^= W10;
    W8 = ROT32(W8,1);
    A2 = ROT32(A3,5) + f3(A4,A0,A1) + A2 + W8 + the_const;
    A4 = ROT32(A4,30);

    W9 ^= W6;
    W9 ^= W1;
    W9 ^= W11;
    W9 = ROT32(W9,1);
    A1 = ROT32(A2,5) + f3(A3,A4,A0) + A1 + W9 + the_const;
    A3 = ROT32(A3,30);

    W10 ^= W7;
    W10 ^= W2;
    W10 ^= W12;
    W10 = ROT32(W10,1);
    A0 = ROT32(A1,5) + f3(A2,A3,A4) + A0 + W10 + the_const;
    A2 = ROT32(A2,30);

    W11 ^= W8;
    W11 ^= W3;
    W11 ^= W13;
    W11 = ROT32(W11,1);
    A4 = ROT32(A0,5) + f3(A1,A2,A3) + A4 + W11 + the_const;
    A1 = ROT32(A1,30);

    W12 ^= W9;
    W12 ^= W4;
    W12 ^= W14;
    W12 = ROT32(W12,1);
    A3 = ROT32(A4,5) + f3(A0,A1,A2) + A3 + W12 + the_const;
    A0 = ROT32(A0,30);

    W13 ^= W10;
    W13 ^= W5;
    W13 ^= W15;
    W13 = ROT32(W13,1);
    A2 = ROT32(A3,5) + f3(A4,A0,A1) + A2 + W13 + the_const;
    A4 = ROT32(A4,30);

    W14 ^= W11;
    W14 ^= W6;
    W14 ^= W0;
    W14 = ROT32(W14,1);
    A1 = ROT32(A2,5) + f3(A3,A4,A0) + A1 + W14 + the_const;
    A3 = ROT32(A3,30);

    W15 ^= W12;
    W15 ^= W7;
    W15 ^= W1;
    W15 = ROT32(W15,1);
    A0 = ROT32(A1,5) + f3(A2,A3,A4) + A0 + W15 + the_const;
    A2 = ROT32(A2,30);

    Aout += A0;
    Bout += A1;
    Cout += A2;
    Dout += A3;
    Eout += A4;
  }

  if ((Aout ^ Bout ^ Cout ^ Dout ^ Eout) == RESULT) return 0xbadbeef;
  else return 0;

/*  printf("%x %x\n", (Aout ^ Bout ^ Cout ^ Dout ^ Eout), RESULT); */
}
