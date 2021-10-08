#include <AquariumService.h>
#include "math.h"
#include "stdio.h"

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

static Boolean_t HeaterNeeded(AquariumServiceContext_t* context, Temperature_t tWater)
{
	float max = context->Parameter.waterTSetpoint + context->Parameter.waterTHysteresis;
	float min = context->Parameter.waterTSetpoint - context->Parameter.waterTHysteresis;

	// Stop if tWater is too high.
	if (tWater > max)
	{
	  return 0;
	}

	// Start if tWater is too low.
	else if (tWater < min)
	{
	  return 1;
	}

	// Otherwise just keep going.
	else
	{
	  return context->Output.heaterIsRunning;
	}
}

static Boolean_t HeaterAllowed(AquariumServiceContext_t* context, Temperature_t tDiff)
{
	float max = context->Parameter.heaterTDiffMax + context->Parameter.heaterTDiffHysteresis;
	float min = context->Parameter.heaterTDiffMax - context->Parameter.heaterTDiffHysteresis;

	// Stop if tDiff is too high.
	if (tDiff > max)
	{
	  return 0;
	}

	// Start if tDiff is too low.
	else if (tDiff < min)
	{
	  return 1;
	}

	// Otherwise just keep going.
	else
	{
	  return context->Output.heaterIsRunning;
	}

}

static void HeaterService(AquariumServiceContext_t* context)
{
	Temperature_t tWater = (context->Input.waterT1 + context->Input.waterT2) / 2;
	Temperature_t tDiff = context->Input.waterTHeat - tWater;

	Boolean_t run = context->Parameter.enabled;
	run &= HeaterTempSignalsOk(context);
	run &= HeaterNeeded(context, tWater);
	run &= HeaterAllowed(context, tDiff);

	if (context->Parameter.onlyRunHeaterAlongWithWaterPump)
	{
	  run &= context->Output.waterPumpIsRunning;
	}

	context->Output.heaterIsRunning = run;
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
