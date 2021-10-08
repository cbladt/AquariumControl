#include <iostream>
#include <time.h>
#include <thread>

#include "AquariumService.h"

#define Print(exp) (std::cout << #exp << "\t" << std::to_string(exp) << std::endl)

static const constexpr auto TickMs = 100;
static const constexpr auto HeatGain = 1.001;
static const constexpr auto HeatDiffGain = 1.0015;
static const constexpr auto TimeGain = 10;
static const constexpr auto RoomTemp = 21;

Temperature_t _waterTemp;
Temperature_t _heaterTemp;
Hour _currentHour;
Minute _currentMinute;

Boolean_t GetExternStartSignal()
{
    return 0;
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
        info.tm_mday = 8;
        info.tm_mon = 10-1;
        info.tm_year = 2021;
        *timestamp = mktime(&info);
    }
}

void RewriteScreen(AquariumServiceContext_t& ctx)
{  
  std::cout << "Time" << std::endl;
  Print(ctx.Time.currentHour);
  Print(ctx.Time.currentMinute);
  std::cout << std::endl;

  std::cout << "Temperatures" << std::endl;
  Print(ctx.Input.waterT1);
  Print(ctx.Input.waterT2);
  Print(ctx.Input.waterTHeat);
  Print(ctx.Input.externStartSignal);
  std::cout << std::endl;

  std::cout << "Output" << std::endl;
  Print(ctx.Output.waterPumpIsRunning);
  Print(ctx.Output.airPumpIsRunning);
  Print(ctx.Output.heaterIsRunning);
  Print(ctx.Output.lightIsRunning);
  std::cout << std::endl;


  float waterTfiltered = (ctx.Input.waterT1 + ctx.Input.waterT2) / 2;
  float waterTerror = ctx.Parameter.waterTSetpoint - waterTfiltered;

  std::cout << "nice" << std::endl;
  Print(ctx.Input.waterTHeat);
  std::cout << "waterTfiltered\t\t" << std::to_string(waterTfiltered) << std::endl;
  std::cout << "waterTerror\t\t" << std::to_string(waterTerror) << std::endl;
  Print(ctx.Output.heaterIsRunning);
  std::cout << std::endl;
}

void SimulateWater(AquariumServiceContext_t& context)
{
    // Actively heating the water.
    if (context.Output.heaterIsRunning)
    {
        _waterTemp *= HeatGain;
        _heaterTemp *= HeatDiffGain;
    }

    // Heater is off, but element still warmer than the water.
    else if (_heaterTemp > _waterTemp)
    {
        _waterTemp *= HeatGain;
        _heaterTemp *= 0.99f;
    }

    // Nominal heat dissipation.
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

void PrepareAquariumService(AquariumServiceContext_t& context)
{
  context.Input.waterT1 = (_waterTemp);
  context.Input.waterT2 = (_waterTemp - 0.356f);
  context.Input.waterTHeat = _heaterTemp;
  context.Input.externStartSignal = GetExternStartSignal();

  context.Parameter.enabled = 1;
  context.Parameter.waterTSetpoint = 25;
  context.Parameter.waterTHysteresis = 0.5;
  context.Parameter.heaterTDiffMax = 1;
  context.Parameter.heaterTDiffHysteresis = 0.1;
  context.Parameter.onlyRunHeaterAlongWithWaterPump = 1;

  context.Parameter.waterPumpBeginHour = 9;
  context.Parameter.waterPumpBeginMinute = 0;
  context.Parameter.waterPumpStopHour = 21;
  context.Parameter.waterPumpStopMinute = 30;

  context.Parameter.airPumpBeginHour = 9;
  context.Parameter.airPumpBeginMinute = 0;
  context.Parameter.airPumpStopHour = 15;
  context.Parameter.airPumpStopMinute = 0;

  context.Parameter.lightBeginHour = 6;
  context.Parameter.lightBeginMinute = 0;
  context.Parameter.lightStopHour = 21;
  context.Parameter.lightStopMinute = 30;

  context.Time.currentHour = _currentHour;
  context.Time.currentMinute = _currentMinute;
  context.Time.currentSecond = 0;
}

int main()
{
    AquariumServiceContext_t context;

    _waterTemp = RoomTemp;
    _heaterTemp = RoomTemp;

    _currentHour = 6;
    _currentMinute = 31;

    PrepareAquariumService(context);

    while (true)
    {
        SimulateWater(context);

        SimulateTime();

        PrepareAquariumService(context);
        AquariumService_Service(&context);

        RewriteScreen(context);

        std::this_thread::sleep_for(std::chrono::milliseconds(TickMs));
    }
}
