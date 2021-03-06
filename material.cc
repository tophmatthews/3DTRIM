#include "material.h"
#include "simconf.h"

#include "functions.h"

#include <cmath>
#include <iostream>


void materialBase::prepare()
{
  double tt = 0.0;

  // get total stoichiometry
  for( int i = 0; i < element.size(); i++ )
  {
    if( element[i]->t < 0.0 ) element[i]->t = 0.0;
    tt += element[i]->t;
  }

  // normalize relative probabilities to 1
  for( int i = 0; i < element.size(); i++ ) element[i]->t /= tt;

  // average
  am = 0.0;
  az = 0.0;
  for( int i = 0; i < element.size(); i++ )
  {
    am += element[i]->m * element[i]->t;
    az += double( element[i]->z ) * element[i]->t;
  }

  arho = rho * 0.6022 / am; //[TRI00310] atoms/Ang^3
}

// make sure layers are prepare'd first!
void materialBase::average( const ionBase *pka )
{
  mu = pka->m1 / am;

  // universal or firsov screening length
  a = .5292 * .8853 / ( pow( double(pka->z1), 0.23 ) + pow( az, 0.23 ) );
  //a = .5292 * .8853 / pow( pow( double(pka.z1), 0.5 ) + pow( az, 0.5 ), 2.0/3.0 );

  // mean flight path0
  f = a * am / ( az * double(pka->z1) * 14.4 * ( pka->m1 + am ) );
  //eps0 = e0 * f;
  epsdg = simconf->tmin * f * pow( 1.0 + mu, 2.0 ) / ( 4.0 * mu );

  // fd and kd determine how much recoil energy goes into el. loss and vaccancies
  fd = pow( 0.01 * az, -7.0 / 3.0 );
  kd = pow( 0.1334 * az, 2.0 / 3.0 ) / sqrtf( am );

  for( int i = 0; i < element.size(); i++ )
  {
    element[i]->my = pka->m1 / element[i]->m;
    element[i]->ec = 4.0 * element[i]->my / pow( 1.0 + element[i]->my, 2.0 );
    element[i]->ai = .5292 * .8853 / ( pow( double(pka->z1), 0.23 ) + pow( element[i]->z, 0.23 ) );
    //ai = .5292 * .8853 / pow( pow( double(pka.z1), 0.5 ) + pow( element[i].z, 0.5 ), 2.0/3.0 );
    element[i]->fi = element[i]->ai * element[i]->m /
                     ( double(pka->z1) * double(element[i]->z) * 14.4 * ( pka->m1 + element[i]->m ) );
  }

  dirty = false;
}

// make sure layers are prepare'd and averaged first!
double materialBase::getrstop( const ionBase *pka )
{
  double se = 0.0;
  for( int i = 0; i < element.size(); i++ )
    se += rstop( pka, element[i]->z ) * element[i]->t * arho;

  return se;
}

double materialBase::rpstop( int z2p, double e )
{
  double pe, pe0, sl, sh, sp, velpwr;
  int z2 = z2p-1;
  // velocity proportional stopping below pe0
  pe0 = 25.0;
  pe = fmax( pe0, e );

  // pcoef indices are one less than in the fortran version!
  sl = ( simconf->pcoef[z2][0] * pow( pe, simconf->pcoef[z2][1] ) ) +
       ( simconf->pcoef[z2][2] * pow( pe, simconf->pcoef[z2][3] ) );
  sh = simconf->pcoef[z2][4] / pow( pe, simconf->pcoef[z2][5] ) *
       logf( simconf->pcoef[z2][6] / pe + simconf->pcoef[z2][7] * pe );
  sp = sl * sh / (sl + sh );
  if( e <= pe0 )
  {
    // velpwr is the power of velocity stopping below pe0
    if( z2p <= 6 )
      velpwr = 0.25;
    else
      velpwr = 0.45;
    sp *= pow( e/pe0, velpwr );
  }
  return sp;
}

double materialBase::rstop( const ionBase *ion, int z2 )
{
  double e, vrmin, yrmin, v, vr, yr, vmin, m1;
  double a, b, q, q1, l, l0, l1;
  double zeta;
  int z1 = ion->z1;
  double fz1 = double(z1), fz2 = double(z2);
  double eee, sp, power;
  double se;

  // scoeff
  double lfctr = simconf->scoef[z1-1].lfctr;
  double mm1 = simconf->scoef[z1-1].mm1;
  double vfermi = simconf->scoef[z2-1].vfermi;
  double atrho = simconf->scoef[z2-1].atrho;

  if( ion->m1 == 0.0 )
    m1 = mm1;
  else
    m1 = ion->m1;

  e = 0.001 * ion->e / m1;

  if( z1 == 1 )
  {
    cerr << "proton stopping not yet implemented!\n";
  }
  else if( z1 == 2 )
  {
    cerr << "alpha stopping not yet implemented!\n";
  }
  else
  {
    yrmin = 0.13;
    vrmin = 1.0;
    v = sqrtf( e / 25.0) / vfermi;

    if( v >= 1.0 )
      vr = v * vfermi * ( 1.0 + 1.0 / ( 5.0 * v*v ) );
    else
      vr = ( 3.0 * vfermi / 4.0 ) * ( 1.0 + ( 2.0 * v*v / 3.0 ) - pow( v, 4.0 ) / 15.0 );

    yr = fmax( yrmin, vr / pow(fz1,0.6667) );
    yr = fmax( yr, vrmin / pow(fz1,0.6667) );
    a = -0.803 * pow( yr, 0.3 ) + 1.3167 * pow( yr, 0.6 ) + 0.38157 * yr +  0.008983 * yr*yr;

    // ionization level of the ion at velocity yr
    q = fmin( 1.0, fmax( 0.0, 1.0 - exp( -fmin( a, 50.0 ) ) ) );

    b = ( fmin( 0.43, fmax( 0.32, 0.12 + 0.025 * fz1 ) ) ) / pow( fz1, 0.3333 );
    l0 = ( 0.8 - q * fmin( 1.2, 0.6 + fz1 / 30.0) ) / pow( fz1, 0.3333 );
    if( q < 0.2 )
      l1 = 0.0;
    else if( q < fmax( 0.0, 0.9 - 0.025 * fz1 ) )
    {//210
      q1 = 0.2;
      l1 = b * ( q - 0.2 ) / fabs( fmax( 0.0, 0.9 - 0.025 * fz1 ) - 0.2000001 );
    }
    else if( q < fmax( 0.0, 1.0 - 0.025 * fmin( 16.0, fz1 ) ) )
      l1 = b;
    else
      l1 = b * ( 1.0 - q ) / ( 0.025 * fmin( 16.0, fz1 ) );

    l = fmax( l1, l0 * lfctr );
    zeta = q + ( 1.0 / ( 2.0 * vfermi*vfermi ) ) * ( 1.0 - q ) * logf( 1.0 + sqr( 4.0 * l * vfermi / 1.919  ) );

    // add z1^3 effect
    a = -sqr( 7.6 - fmax( 0.0, logf( e ) ) );
    zeta *= 1.0 + ( 1.0 / (fz1*fz1) ) * ( 0.18 + 0.0015 * fz2 ) * expf( a );

    if( yr <= fmax( yrmin, vrmin / pow( fz1, 0.6667 ) ) )
    {
      // calculate velocity stopping for  yr < yrmin
      vrmin = fmax( vrmin, yrmin * pow( fz1, 0.6667 ) );
      vmin = 0.5 * ( vrmin + sqrtf( fmax( 0.0, vrmin*vrmin - 0.8 * vfermi*vfermi ) ) );
      eee = 25.0 * vmin*vmin;
      sp = rpstop( z2, eee );

      if( z2 == 6 || ( ( z2 == 14 || z2 == 32 ) && z1 <= 19 ) )
        power = 0.375;
      else
        power = 0.5;

      se = sp * sqr( zeta * fz1 ) * pow( e/eee, power );
    }
    else
    {
      sp = rpstop( z2, e );
      se = sp * sqr( zeta * fz1 );
    }
  } // END: heavy-ions

  return se * 10.0;
}
