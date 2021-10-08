#include <iostream>
#include <time.h>
#include <thread>

#include "AquariumService.h"

#define Print(exp) (std::cout << #exp << "\t" << std::to_string(exp) << std::endl)

static const constexpr auto TickMs = 100;
static const constexpr auto HeatGain = 1.001;
static const constexpr auto HeatDiffGain = 1.002;
static const constexpr auto TimeGain = 5;
static const constexpr auto RoomTemp = 21;

Temperature_t _waterTemp;
Temperature_t _heaterTemp;

unsigned short _currentYear;
unsigned char _currentMonth;
unsigned char _currentDay;
Hour _currentHour;
Minute _currentMinute;

Boolean_t GetExternStartSignal()
{
    if (_currentHour == 2 || _currentHour == 3 || _currentHour == 4)
    {
      return 1;
    }
    else
    {
      return 0;
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
        info.tm_mday = _currentDay;
        info.tm_mon = _currentMonth-1;
        info.tm_year = _currentYear-1900;
        *timestamp = mktime(&info);
    }
    else
    {
        struct tm info;
        info.tm_sec = 0;
        info.tm_min = minute;
        info.tm_hour = hour;
        info.tm_mday = _currentDay;
        info.tm_mon = _currentMonth-1;
        info.tm_year = _currentYear-1900;
        *timestamp = mktime(&info);
    }    
}

void RewriteScreen(AquariumServiceContext_t& ctx)
{
  std::cout << "Time" << std::endl;
  std::cout << "Hour\t" << std::to_string(_currentHour) << std::endl;
  std::cout << "Minute\t" << std::to_string(_currentMinute) << std::endl;
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
        _currentDay += 1;
        _currentHour = 0;
    }

    if (_currentDay > 30)
    {
        _currentMonth += 1;
        _currentDay = 0;
    }

    if (_currentMonth > 12)
    {
        _currentYear += 1;
        _currentMonth = 0;
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

  context.Time.getTime = &GetTime;
  context.Time.currentHour = _currentHour;
  context.Time.currentMinute = _currentMinute;
  context.Time.currentSecond = 0;
}

int main()
{
    AquariumServiceContext_t context;

    _waterTemp = RoomTemp;
    _heaterTemp = RoomTemp;

    _currentYear = 2021;
    _currentMonth = 10;
    _currentDay = 8;
    _currentHour = 0;
    _currentMinute = 0;

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
