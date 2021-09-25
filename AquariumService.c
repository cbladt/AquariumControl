#include <AquariumService.h>
#include "math.h"

static void SetHeaterState(AquariumServiceContext_t* context, RelayState_t relayState);

static void SetWaterPumpState(AquariumServiceContext_t* context, RelayState_t relayState)
{
    context->setWaterPumpState(relayState);
    context->waterPumpIsRunning = relayState;
}

static void SetAirPumpState(AquariumServiceContext_t* context, RelayState_t relayState)
{
    context->setAirPumpState(relayState);
    context->airPumpIsRunning = relayState;
}

static void SetHeaterState(AquariumServiceContext_t* context, RelayState_t relayState)
{
    context->setHeaterState(relayState);
    context->heaterIsRunning = relayState;
}

static void SetLightState(AquariumServiceContext_t* context, RelayState_t relayState)
{
    context->setLightState(relayState);
    context->lightIsRunning = relayState;
}


void HeaterService(AquariumServiceContext_t* context)
{
    context->getWaterTemperature(&context->waterTemperatureActual);
    context->getHeaterTemperature(&context->heaterTemperatureActual);

    // Heater needed?
    if (context->waterTemperatureActual > (context->waterTemperatureSetpoint + context->waterTemperatureHysteresis))
    {
        context->heaterEnabled = 0;
    }
    else if (context->waterTemperatureActual < (context->waterTemperatureSetpoint - context->waterTemperatureHysteresis))
    {
        context->heaterEnabled = 1;
    }

    // Temperature diff withing allowed range?
    context->waterHeaterTemperatureDiffActual = context->heaterTemperatureActual - context->waterTemperatureActual;
    if (context->heaterEnabled)
    {
        if (context->waterHeaterTemperatureDiffActual > (context->waterHeaterTemperatureDiffSetpoint + context->waterHeaterTemperatureDiffHysteresis))
        {
            context->heaterEnabled = 0;
        }
        else if (context->waterHeaterTemperatureDiffActual < (context->waterHeaterTemperatureDiffSetpoint - context->waterHeaterTemperatureDiffHysteresis))
        {
            context->heaterEnabled = 1;
        }
    }

    // Start/Stop heater.
    if (context->onlyRunHeaterAlongWithWaterPump)
    {
        if (context->heaterEnabled && context->waterPumpIsRunning)
        {
            SetHeaterState(context, 1);
        }
        else
        {
            SetHeaterState(context, 0);
        }
    }
    else
    {
        if (context->heaterEnabled)
        {
            SetHeaterState(context, 1);
        }
        else
        {
            SetHeaterState(context, 0);
        }
    }
}

Boolean_t NowIsBetweenTimestamps(AquariumServiceContext_t* context, Hour startHour, Minute startMinute, Hour stopHour, Minute stopMinute)
{
    Timestamp_t now;
    context->getTime(255, 255, &now);

    Timestamp_t start;
    context->getTime(startHour, startMinute, &start);

    Timestamp_t stop;
    context->getTime(stopHour, stopMinute, &stop);


    return (now >= start) && (now <= stop);
}

void AquariumService_Service(AquariumServiceContext_t* context)
{
    context->getExternStartSignal(&context->externStartSignal);
    HeaterService(context);

    // Water Pump.
    if (context->externStartSignal || NowIsBetweenTimestamps(context, context->waterPumpBeginHour, context->waterPumpBeginMinute, context->waterPumpStopHour, context->waterPumpStopMinute))
    {
        SetWaterPumpState(context, 1);
    }
    else
    {
        SetWaterPumpState(context, 0);
    }

    // Air Pump.
    if (NowIsBetweenTimestamps(context, context->airPumpBeginHour, context->airPumpBeginMinute, context->airPumpStopHour, context->airPumpStopHour))
    {
        SetAirPumpState(context, 1);
    }
    else
    {
        SetAirPumpState(context, 0);
    }

    // Light.
    if (context->externStartSignal || NowIsBetweenTimestamps(context, context->lightBeginHour, context->lightBeginMinute, context->lightStopHour, context->lightStopMinute))
    {
        SetLightState(context, 1);
    }
    else
    {
        SetLightState(context, 0);
    }
}
