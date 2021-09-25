#include <iostream>
#include <time.h>
#include <thread>

#include "AquariumService.h"

#define FORM100 100
static const constexpr auto WaterSetpoint = 25 * FORM100;
static const constexpr auto WaterHysteresis = 0.5 * FORM100;
static const constexpr auto WaterDiffSetpoint = 1 * FORM100;
static const constexpr auto WaterDiffHysteresis = 0.1 * FORM100;
static const constexpr auto HeatGain = 1.01;
static const constexpr auto HeatDiffGain = 1.015;
static const constexpr auto TimeGain = 10;
static const constexpr auto RoomTemp = 21 * FORM100;

Temperature_t _waterTemp;
Temperature_t _heaterTemp;
Hour _currentHour;
Minute _currentMinute;

static void GetWaterTemperature(Temperature_t* t)
{
    *t = _waterTemp;
}

static void GetHeaterTemperature(Temperature_t* t)
{
    *t = _heaterTemp;
}

void RelayStub(RelayState_t)
{}

void GetExternStartSignal(Boolean_t* signal)
{
    if (_currentHour == 2)
    {
        *signal = 0;
    }
    else
    {
        *signal = 0;
    }
}

static void GetTime(Hour hour, Minute minute, Timestamp_t* timestamp)
{
    if (hour == 255 && minute == 255)
    {
        struct tm info;
        info.tm_sec = 0;
        info.tm_min = _currentMinute;
        info.tm_hour = _currentHour;
        info.tm_mday = 25;
        info.tm_mon = 9-1;
        info.tm_year = 2021;
        *timestamp = mktime(&info);
    }
    else
    {
        struct tm info;
        info.tm_sec = 0;
        info.tm_min = minute;
        info.tm_hour = hour;
        info.tm_mday = 25;
        info.tm_mon = 9-1;
        info.tm_year = 2021;
        *timestamp = mktime(&info);
    }
}

void RewriteScreen(AquariumServiceContext_t& ctx)
{
    // Temperatures.
    std::cout << "\n\n\nWaterSetpoint:\t\t" << ctx.waterTemperatureSetpoint << std::endl;
    std::cout << "WaterActual:\t\t" << ctx.waterTemperatureActual << std::endl;
    std::cout << "HeaterActual:\t\t" << ctx.heaterTemperatureActual << std::endl;
    std::cout << "WaterHeaterDiff:\t\t" << ctx.waterHeaterTemperatureDiffActual << std::endl;

    // Heater.
    std::cout << "\nHeaterEnabled:\t\t" << (ctx.heaterEnabled ? "True" : "False") << std::endl;
    std::cout << "HeaterIsRunning:\t\t" << (ctx.heaterIsRunning ? "True" : "False") << std::endl;

    // Water Pump.
    std::cout << "\nWaterPumpIsRunning:\t" << (ctx.waterPumpIsRunning ? "True" : "False") << std::endl;
    std::cout << "WaterStartTime:\t\t" << std::to_string(ctx.waterPumpBeginHour) << ":" << std::to_string(ctx.waterPumpBeginMinute) << std::endl;
    std::cout << "WaterStopTime:\t\t" << std::to_string(ctx.waterPumpStopHour) << ":" << std::to_string(ctx.waterPumpStopMinute) << std::endl;

    // Air Pump.
    std::cout << "\nAirPumpIsRunning:\t\t" << (ctx.airPumpIsRunning ? "True" : "False") << std::endl;
    std::cout << "AirStartTime:\t\t" << std::to_string(ctx.airPumpBeginHour) << ":" << std::to_string(ctx.airPumpBeginMinute) << std::endl;
    std::cout << "AirStopTime:\t\t" << std::to_string(ctx.airPumpStopHour) << ":" << std::to_string(ctx.airPumpStopMinute) << std::endl;

    // Light.
    std::cout << "\nLightIsRunning:\t\t" << (ctx.lightIsRunning ? "True" : "False") << std::endl;
    std::cout << "LightStartTime:\t\t" << std::to_string(ctx.lightBeginHour) << ":" << std::to_string(ctx.lightBeginMinute) << std::endl;
    std::cout << "LightStopTime:\t\t" << std::to_string(ctx.lightStopHour) << ":" << std::to_string(ctx.lightStopMinute) << std::endl;

    // Time.
    std::cout << "\nCurrentTimeIs:\t\t" << std::to_string(_currentHour) << ":" << std::to_string(_currentMinute) << std::endl;

}

void SimulateWater(AquariumServiceContext_t& context)
{
    if (context.heaterIsRunning)
    {
        _waterTemp *= HeatGain;
        _heaterTemp *= HeatDiffGain;
    }
    else if (_heaterTemp > _waterTemp)
    {
        _waterTemp *= HeatGain;
    }
    else
    {
        _waterTemp /= HeatGain;
        _heaterTemp /= HeatDiffGain;

        if (_waterTemp < RoomTemp)
        {
            _waterTemp = RoomTemp;
        }

        if (_heaterTemp < _waterTemp)
        {
            _heaterTemp = _waterTemp;
        }
    }
}

void SimulateTime()
{
    _currentMinute += (1 * TimeGain);
    if (_currentMinute > 59)
    {
        _currentHour += 1;
        _currentMinute = 0;
    }

    if (_currentHour > 23)
    {
        _currentHour = 0;
    }
}

int main()
{
    AquariumServiceContext_t context;

    context.waterTemperatureSetpoint = WaterSetpoint;
    context.waterTemperatureHysteresis = WaterHysteresis;
    context.waterHeaterTemperatureDiffSetpoint = WaterDiffSetpoint;
    context.waterHeaterTemperatureDiffHysteresis = WaterDiffHysteresis;
    context.onlyRunHeaterAlongWithWaterPump = 0;

    context.setWaterPumpState = &RelayStub;
    context.setAirPumpState = &RelayStub;
    context.setHeaterState = &RelayStub;
    context.setLightState = &RelayStub;
    context.getWaterTemperature = &GetWaterTemperature;
    context.getHeaterTemperature = &GetHeaterTemperature;
    context.getTime = &GetTime;
    context.getExternStartSignal = &GetExternStartSignal;

    context.waterPumpBeginHour = 9;
    context.waterPumpBeginMinute = 0;
    context.waterPumpStopHour = 21;
    context.waterPumpStopMinute = 30;

    context.airPumpBeginHour = 9;
    context.airPumpBeginMinute = 0;
    context.airPumpStopHour = 15;
    context.airPumpStopMinute = 0;

    context.lightBeginHour = 6;
    context.lightBeginMinute = 0;
    context.lightStopHour = 21;
    context.lightStopMinute = 30;

    _waterTemp = RoomTemp;
    _heaterTemp = RoomTemp;

    _currentHour = 6;
    _currentMinute = 31;

    while (true)
    {
        SimulateWater(context);

        SimulateTime();

        AquariumService_Service(&context);

        RewriteScreen(context);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
