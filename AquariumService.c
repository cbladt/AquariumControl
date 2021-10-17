#include <AquariumService.h>
#include "math.h"

static float ControlMinimum(float a, float b)
{
  if (a < b)
  {
    return a;
  }
  else
  {
    return b;
  }
}

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

static Boolean_t NowIsBetweenTimestamps(AquariumServiceContext_t* context, Hour startHour, Minute startMinute, Hour stopHour, Minute stopMinute)
{
  Timestamp_t now;
  context->Time.getTime(255, 255, &now);

  Timestamp_t start;
  context->Time.getTime(startHour, startMinute, &start);

  Timestamp_t stop;
  context->Time.getTime(stopHour, stopMinute, &stop);

  return (now >= start) && (now <= stop);
}

static Boolean_t TemperatureOk(Temperature_t t)
{
  return (t > -50) && (t < 100);
}

static Boolean_t HeaterTempSignalsOk(AquariumServiceContext_t* context)
{
  Boolean_t ok = TemperatureOk(context->Input.waterT1);
  ok &= TemperatureOk(context->Input.waterT2);
  ok &= TemperatureOk(context->Input.waterTHeat);

  return ok;
}

static void HeaterService(AquariumServiceContext_t* context)
{
  Temperature_t tWater = (context->Input.waterT1 + context->Input.waterT2) / 2;
  Temperature_t tDiff = context->Input.waterTHeat - tWater;

  if (tDiff < 0)
  {
    tDiff = 0;
  }

  context->Output.heaterIsRunning = context->Parameter.enabled;
  context->Output.heaterIsRunning &= HeaterTempSignalsOk(context);

  if (context->Parameter.onlyRunHeaterAlongWithWaterPump)
  {
      context->Output.heaterIsRunning &= context->Output.waterPumpIsRunning;
  }

  if (context->Output.heaterIsRunning > 0)
  {
      float maybeMax = 100-ControlProportional(tDiff, 0, context->Parameter.heaterTDiffMax, 0, 100);
      float max = ControlMinimum(maybeMax, context->Regulator.MaxOutput);

      context->Output.heaterPercent = ControlPI(
            context->Regulator.MaxKp,
            context->Regulator.MaxTn,
            context->Parameter.waterTSetpoint,
            tWater,
            &context->Regulator.integrator,
            context->Regulator.antiIntegratorWindup,
            0,
            max
      );
  }
  else
  {
    context->Output.heaterPercent = 0;
  }
}

static void WaterPumpService(AquariumServiceContext_t* context)
{
  if (NowIsBetweenTimestamps(context, context->Parameter.waterPumpBeginHour, context->Parameter.waterPumpBeginMinute, context->Parameter.waterPumpStopHour, context->Parameter.waterPumpStopMinute))
  {
      context->Output.waterPumpIsRunning = 1;
  }
  else
  {
      context->Output.waterPumpIsRunning = 0;
  }

  context->Output.waterPumpIsRunning |= context->Input.externStartSignal;
  context->Output.waterPumpIsRunning &= context->Parameter.enabled;
}

static void AirPumpService(AquariumServiceContext_t* context)
{
  if (NowIsBetweenTimestamps(context, context->Parameter.airPumpBeginHour, context->Parameter.airPumpBeginMinute, context->Parameter.airPumpStopHour, context->Parameter.airPumpStopHour))
  {
      context->Output.airPumpIsRunning = 1;
  }
  else
  {
      context->Output.airPumpIsRunning = 0;
  }

  context->Output.airPumpIsRunning &= context->Parameter.enabled;
}

static void LightService(AquariumServiceContext_t* context)
{
  if (NowIsBetweenTimestamps(context, context->Parameter.lightBeginHour, context->Parameter.lightBeginMinute, context->Parameter.lightStopHour, context->Parameter.lightStopMinute))
  {
      context->Output.lightIsRunning = 1;
  }
  else
  {
      context->Output.lightIsRunning = 0;
  }

  context->Output.lightIsRunning |= context->Input.externStartSignal;
  context->Output.lightIsRunning &= context->Parameter.enabled;
}

void AquariumService_Service(AquariumServiceContext_t* context)
{
  HeaterService(context);

  WaterPumpService(context);

  AirPumpService(context);

  LightService(context);
}
