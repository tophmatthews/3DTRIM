#ifndef SAMPLE_WIRE_H
#define SAMPLE_WIRE_H 1

#include "sample.h"

struct sampleWire : sampleBase {
  sampleWire( double x = 10000.0, double y = 10000.0, double z = 10000.0 );

  virtual materialBase* lookupMaterial( double* pos );
};

#endif