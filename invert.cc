#include "invert.h"

#include <stdio.h>

double inverter::x( double f1 )
{
  double f2, x1 = maxx / 2.0;
  double w = maxx / 4.0;

  // no point in doing more than 32 iterationd for double (precission)
  for( int i = 0; i < 32; i++ )
  {
    f2 = f(x1) / maxf;
    if( fabs( f2 - f1 ) <= tol ) break;
    if( f2 > f1 ) 
      x1 -= w;
    else
      x1 += w;
    w *= 0.5;
  }

  return x1;
}

double massInverter::f( double x )
{
  return (   100.088 
           + 0.112798 * erff(-5.56257 + 0.0471405 * x) 
           + 37.4781 * erff(-19.3772 + 0.137386 * x) 
           + 37.4781 * erff(-13.0462 + 0.137386 * x) 
           + 12.5094 * erff(-30.8853 + 0.229537 * x) 
           + 12.5094 * erff(-23.2853 + 0.229537 * x) 
         ) / 200.1756;
}

double energyInverter::f( double x )
{
  double x1 = x / ( 1.0 - A / 234.0 );
  return ( -0.00014122 + (0.00014122 -7.12299E-7 * x1 ) * exp(0.0886603 * x1) ) / 127.216;
}