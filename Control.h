#pragma once

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

static float ControlPI(float Kp, float Ki, float reference, float feedback, float* integrator, float antiIntegratorWindup, float limitMin, float limitMax)
{
  float error = reference - feedback;

  float proportional = error * Kp;

  (*integrator) = ControlLimit((*integrator) + (error * Ki), antiIntegratorWindup/-1, antiIntegratorWindup);

  return ControlLimit(((*integrator) + proportional), limitMin, limitMax);
}
