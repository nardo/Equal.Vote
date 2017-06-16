
#define TRUE 1
#define FALSE 0
#define uint unsigned int
#define bool char
#define uint32 unsigned int
#define uint64 unsigned long long
#define uchar unsigned char
#define schar signed char
#define real double
#define assert(x)

/****** convenient constants: *******/
#define BIGINT 0x7FFFFFFF
#define MAXUINT ((uint)((255<<48)|(255<<40)|(255<<32)|(255<<24)|(255<<16)|(255<<8)|(255)))
/* defn works on 8,16,32, and 64-bit machines */


uint32 BLC32x[60];  /* 32*60=1920 bits of state. Must be nonzero mod P. */
int BLC32NumLeft;

/********************************************************
Warren D. Smith 2001
**********************************************************
Linear congruential pseudo-random number generator mod P,
where P is the enormous prime (578 base-10 digits long; 
60 words long if each word is 32 bits)
  P = [2^(48*32) - 1] * 2^(12*32) + 1.
This prime can yield PRNGs suitable for use on 
computers with w-bit words, w=8,16,32,48,64,96,128.
The following code is intended for w=32.
The fact that 2^(60*32) = 2^(12*32) - 1 (mod P)
makes modular arithmetic mod P easy, and is the
reason this particular P was chosen.
The period of our generator is P-1.
***********************************************************
Although it is usually easy to detect the nonrandomness of
linear congruential generators because they generate d-tuples
lying on a lattice, in the present case the "grain size" of the
lattice is invisibly small (far less than a least significant bit),
for 32-bit words, if 1<=d<=180. If you want 64-bit words, then need
to concatenate two 32-bit words, and then grain size invisibly small
if 1<=d<=90. These bounds are conservative; in fact I suspect
one is ok, even for 64-bit words, even in up to 1000 dimensions.
***********************************************************
Some even more enormous primes which we have not used are:
[2^{59*32} - 1] * 2^{8 *32} + 1,
[2^{63*32} - 1] * 2^{24*32} + 1,
[2^{69*32} - 1] * 2^{14*32} + 1,
[2^{95*32} - 1] * 2^{67*32} + 1,
[2^{99*32} - 1] * 2^{35*32} + 1;
these would also be suitable for (8,16,32)-bit computers,
and the second of them would also be good for (48,96)-bit computers.
Unfortunately the factorization of P-1 is not known for the last 3 
I've listed here, preventing you from being certain you've found a
primitive root mod that P. A still more enormous prime is
  [2^4253 - 1] * 2^4580 + 1    [2659 digits long!]
(where note 2^4253-1 is also prime so that factorizing P-1 is
trivial) but doing arithmetic mod this P is (although still fairly
easy) less pleasant because bit-shifting is required.
*************************************************************/
uint32 BigLinCong32(){
   uint32 y[120];
   int i;
   uint64 u;

   if(BLC32NumLeft==0){
      /* Need to refill BLC32x[0..59] with 60 new random numbers: */

 /****************************************************************
 * If BLC32x[0..59] is the digits, LS..MS, of a number in base 2^w,
 * then the following code fragment puts A times that number 
 * in y[0..119].  Here
 *  A = 1284507170 * 2^(w*3) + 847441413 * 2^(w*44) + 650134147 * 2^(w*59)
 * is a "good" primitive root mod P, if w=32.
 *****************************************************************/
#define lohalf(x) (uint32)(x)
#define A1 (uint64)1284507170
#define A2 (uint64)847441413
#define A3 (uint64)650134147
      for(i=0; i<3; i++){
	 y[i] = 0;
      }
      u=0;
      for(/*i=3*/; i<44; i++){
	 u += A1 * BLC32x[i-3];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      for(/*i=44*/; i<59; i++){
	 u += A1 * BLC32x[i-3]; 
	 u += A2 * BLC32x[i-44];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      for(/*i=59*/; i<60+3; i++){
	 u += A1 * BLC32x[i-3]; 
	 u += A2 * BLC32x[i-44]; 
	 u += A3 * BLC32x[i-59];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      for(/*i=60+3*/; i<60+44; i++){
	 u += A2 * BLC32x[i-44]; 
	 u += A3 * BLC32x[i-59];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      for(/*i=60+44*/; i<60+59; i++){
	 u += A3 * BLC32x[i-59];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      /*i=60+59=119*/
      y[i] = lohalf(u);
#undef A1 
#undef A2 
#undef A3 
 /*************************************************************
 * If y[0..119] is the digits, LS..MS, of a number in base 2^w,
 * then the following code fragment replaces that number with
 * its remainder mod P in y[0..59]  (conceivably the result will
 * be >P, but this does not matter; it will never be >=2^(w*60)).
 **************************************************************/
      u=1; /*borrow*/
#define AllF 0xffffffff
      /* Step 1: y[0..72] = y[0..59] + y[60..119]shift12 - y[60..119]: */
      for(i=0; i<12; i++){
	 u += y[i]; 
	 u += (uint64)~y[60+i];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      for(/*i=12*/; i<60; i++){
	 u += y[i]; 
	 u += y[48+i]; 
	 u += (uint64)~y[60+i];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      for(/*i=60*/; i<72; i++){
	 u += AllF; 
	 u += y[48+i];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      assert(u>0);
      y[72] = (uint32)(u-1); /*unborrow*/

      /*  Step 2: y[0..60] = y[0..59] + y[60..72]shift12  - y[60..72]: */
      u=1; /*borrow*/
      for(i=0; i<12; i++){
	 u += y[i]; 
	 u += (uint64)~y[60+i];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      /*i=12*/
      u += y[i] + y[48+i]; 
      u += (uint64)~y[60+i];
      y[i] = lohalf(u);
      u = u>>32;
      i++;
      for(/*i=13*/; i<25; i++){
	 u += AllF; 
	 u += y[i]; 
	 u += y[48+i];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      for(/*i=25*/; i<60; i++){
	 u += AllF; 
	 u += y[i];
	 y[i] = lohalf(u);
	 u = u>>32;
      }
      /*i=60*/
      assert(u>0);
      y[i] = (uint32)(u-1); /*unborrow*/

     /*It is rare that any iterations of this loop are needed:*/
      while(y[60]!=0){ 
         printf("rare loop\n");
	 /*Step 3+:  y[0..60] = y[0..59] + y[60]shift12 - y[60]:*/
	 u=1; /*borrow*/
	 u += y[0]; 
	 u += (uint64)~y[60];
	 y[0] = lohalf(u);
	 u = u>>32;
	 for(i=1; i<12; i++){
	    u += AllF; 
	    u += y[i];
	    y[i] = lohalf(u);
	    u = u>>32;
	 }
	 /*i=12*/
	 u += AllF; 
	 u += y[i]; 
	 u += y[60];
	 y[i] = lohalf(u);
	 u = u>>32;
	 i++;
	 for(/*i=13*/; i<60; i++){
	    u += AllF; 
	    u += y[i];
	    y[i] = lohalf(u);
	    u = u>>32;
	 }
	 /*i=60*/
	 assert(u>0);
	 y[i] = (uint32)(u-1); /*unborrow*/
      }
#undef AllF 
#undef lohalf

      /* Copy y[0..59] into BLC32x[0..59]: */
      for(i=0; i<60; i++){ 
	 BLC32x[i] = y[i]; 
      }
      /*printf("%u\n", BLC32x[44]);*/
      BLC32NumLeft=60;
   }
   /* (Else) We have random numbers left, so return one: */
   BLC32NumLeft--;
   return BLC32x[BLC32NumLeft];
}

real Rand01(){ /* returns random uniform in [0,1] */
  return ((BigLinCong32()+0.5)/(1.0 + MAXUINT) + BigLinCong32())/(1.0 + MAXUINT);
}

real RandNormal(){ /* returns standard Normal (gaussian variance=1 mean=0) deviate */
  real w, x1;
  static real x2;
  static bool ready = FALSE;
  if(ready){
    ready = FALSE;
    return x2;
  }
  do{
    x1 = 2*Rand01() - 1.0;
    x2 = 2*Rand01() - 1.0;
    w = x1*x1 + x2*x2;
  }while ( w > 1.0 || w==0.0 );
  w = sqrt( (-2.0*log(w)) / w );
  x1 *= w;
  x2 *= w;  /* Now x1 and x2 are two indep normals (Box-Muller polar method) */
  ready = TRUE;
  return x1;
}

real RandRadialNormal(){ 
  real w;
  do{ w = Rand01(); }while(w==0.0);
  w = sqrt( -2.0*log(w) );
  return w;
}



void InitRand(uint seed){ /* initializes the randgen */
   int i;
   int seed_sec=0, processId=0;
   uint seed_usec=0;
#if     MSWINDOWS
   tm* locTime;
   _timeb currTime;
   time_t now;
#else
   struct timeval tv;
#endif
   if(seed==0){
     printf("using time of day and PID to generate seed");
#if MSWINDOWS
     now = time(NULL);
     locTime = localtime(&now);
     _ftime(&currTime);
     seed_usec = currTime.millitm*1001;
     seed_sec = locTime->tm_sec + 60*(locTime->tm_min + 60*locTime->tm_hour);
     processId = _getpid();
#else
     gettimeofday(&tv,0);
     seed_sec = tv.tv_sec;
     seed_usec = tv.tv_usec;
     processId = getpid();
#endif
     seed = 1000000*(uint)seed_sec + (uint)seed_usec + 
       (((uint)processId)<<20) + (((uint)processId)<<10);
     printf("=%u\n", seed);
   }
   for(i=0; i<60; i++){ BLC32x[i]=0; }
   BLC32x[0] = seed; 
   BLC32NumLeft = 0;
   for(i=0; i<599; i++){ BigLinCong32(); }
   printf("Random generator initialized with seed=%u:\n", seed);
   for(i=0; i<7; i++){ 
     printf("%.6f ", Rand01());
   }
   printf("\n");
}

real RandExpl(){ /* returns standard exponential (density=exp(-x) for x>0) random deviate */
  real x;
  do{ 
    x = Rand01();
  }while( x==0.0 );
  return -log(x);
}

