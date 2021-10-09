#pragma once
#include "stdio.h"

static float ControlLimit(float in, float min, float max)
{
  if (in < min)
  {
    return min;
  }

  if (in > max)
  {
    return max;
  }

  return in;
}

static float ControlProportional(float input, float minIn, float maxIn, float minOut, float maxOut)
{
  float result;

  if (input < minIn)
  {
    result = minOut;
  }

  else if (input > maxIn)
  {
    result = maxOut;
  }

  else
  {
      if (minIn < maxIn)
      {
        result =  minOut + (input-minIn)*(maxOut-minOut) / (maxIn - minIn);
      }
      else
      {
        result = minOut;
      }
  }

  return result;
}

static float _GetKp(float error, float maxKp)
{
  if (error < 0)
  {
      error /= -1;
  }

  return ControlProportional(error, 0, 5, 0.1, maxKp);
}

static float _GetKi(float error, float maxTn)
{
  if (error < 0)
  {
      error /= -1;
  }

  float Tn = maxTn-ControlProportional(error, 0, 5, 0.1, maxTn);

  return 1/Tn;
}

static float ControlPI(float maxKp, float maxTn, float reference, float feedback, float* integrator, float antiIntegratorWindup, float limitMin, float limitMax)
{  
  float error = reference - feedback; 

  float Kp = _GetKp(error, maxKp);

  float Ki = _GetKi(error, maxTn);

  float proportional = error * Kp;

  (*integrator) = ControlLimit((*integrator) + (error * Ki), antiIntegratorWindup/-1, antiIntegratorWindup);

  return ControlLimit(((*integrator) + proportional), limitMin, limitMax);
}
