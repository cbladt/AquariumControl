#include <iostream>
#include <iomanip>
#include <time.h>
#include <thread>

#include "AquariumService.h"

#define  Print(exp) (std::cout << #exp << "\t" << std::to_string(exp) << std::endl)
#define fPrint(exp) (std::cout << #exp << "\t\t" << std::fixed << std::setprecision(2) << exp << std::endl)

static const constexpr auto TickMs    = 100;
static const constexpr auto TimeStep  = 1;
static const constexpr auto Positive  = 1.002f;
static const constexpr auto Negative  = 0.998f;
static const constexpr auto RoomTemp  = 21.0f;

static const constexpr auto AquariumLengthM = 0.5f;
static const constexpr auto AquariumWidthM = 0.2f;
static const constexpr auto AquariumHeightM = 0.3f;
static const constexpr auto AquariumVolumeL = AquariumWidthM * AquariumLengthM * AquariumHeightM * 1000;
static const constexpr auto AquariumHeaterEffect = 400;

static const constexpr auto WaterSpecificHeat = 4186;

static Temperature_t _waterTemp;
static Temperature_t _heaterTemp;

static unsigned short _currentYear;
static unsigned char _currentMonth;
static unsigned char _currentDay;
static Hour _currentHour;
static Minute _currentMinute;

static unsigned char _counterLastHeatingState;
static unsigned long _counterHeatingTime;
static unsigned long _counterHeatingSwitch;
static unsigned long _counterRunTime;

template <typename T>
static T Limit(T in, T min, T max)
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

static auto SinusLikeFluctuation()
{
  return (_currentMinute-30.0f)/10000.0f;
}

static Boolean_t GetExternStartSignal()
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

static void RewriteScreen(AquariumServiceContext_t& ctx)
{
  std::cout << "Time" << std::endl;
  std::cout << "Hour\t\t\t\t" << std::to_string(_currentHour) << std::endl;
  std::cout << "Minute\t\t\t\t" << std::to_string(_currentMinute) << std::endl;
  std::cout << std::endl;

  std::cout << "Temperatures" << std::endl;
  fPrint(ctx.Input.waterT1);
  fPrint(ctx.Input.waterT2);
  fPrint(ctx.Input.waterTHeat);
  Print(ctx.Input.externStartSignal);
  std::cout << std::endl;

  std::cout << "Output" << std::endl;
  Print(ctx.Output.waterPumpIsRunning);
  Print(ctx.Output.airPumpIsRunning);
  Print(ctx.Output.heaterIsRunning);
  Print(ctx.Output.lightIsRunning);
  std::cout << std::endl;

  std::cout << "Counters" << std::endl;
  std::cout << "Runtime Hours\t\t\t" << std::to_string(_counterRunTime / 60) << std::endl;
  std::cout << "HeatingTime Minutes\t\t" << std::to_string(_counterHeatingTime) << std::endl;
  std::cout << "HeatingTime OnOff\t\t" << std::to_string(_counterHeatingSwitch) << std::endl;
  std::cout << std::endl;
}

static void SimulateWater(AquariumServiceContext_t& context)
{
  // Actively heating the water.
  if (context.Output.heaterIsRunning)
  {
    auto tWater = (_waterTemp + _heaterTemp) / 2;
    auto tTarget = context.Parameter.waterTSetpoint + context.Parameter.waterTHysteresis;
    auto totalMinutes = AquariumVolumeL * WaterSpecificHeat * (tTarget - tWater) / AquariumHeaterEffect / 60;
    auto step = TimeStep / totalMinutes;

    _waterTemp  += step * Negative;
    _heaterTemp += step * Positive;
  }

  // Heater is off, but element still warmer than the water.
  else if (_heaterTemp > _waterTemp)
  {
    _waterTemp  *= Positive;
    _heaterTemp *= Negative;
}

  // Nominal heat dissipation.
  else
  {
    // Calculate actual dissipation.
    auto tWater = (_waterTemp + _heaterTemp) / 2;
    if (tWater > RoomTemp)
    {
      auto powerW = 5.68f / (tWater * (AquariumWidthM * AquariumLengthM));
      auto totalMinutes = AquariumVolumeL * WaterSpecificHeat * (RoomTemp - tWater) / powerW / 60;
      auto step = TimeStep / totalMinutes;

      _waterTemp  += step;
      _heaterTemp += step;
    }

    else
    {
      _waterTemp  += SinusLikeFluctuation();
      _heaterTemp += SinusLikeFluctuation();
    }

    // Limit results.
    _waterTemp  = Limit<decltype(_waterTemp)> (_waterTemp,  RoomTemp*0.8, RoomTemp*1.2);
    _heaterTemp = Limit<decltype(_heaterTemp)>(_heaterTemp, RoomTemp*0.8, RoomTemp*1.2);
  }
}

static void SimulateTime()
{
  _currentMinute += TimeStep;

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

static void PrepareAquariumService(AquariumServiceContext_t& context)
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

static void ServiceCounters(AquariumServiceContext_t& context)
{
  if (context.Output.heaterIsRunning)
  {
    _counterHeatingTime += TimeStep;
  }

  if (context.Output.heaterIsRunning != _counterLastHeatingState)
  {
    _counterHeatingSwitch++;
    _counterLastHeatingState = context.Output.heaterIsRunning;
  }

  _counterRunTime += TimeStep;
}

int main()
{
    AquariumServiceContext_t context;

    _waterTemp = RoomTemp;
    _heaterTemp = RoomTemp;

    _currentYear = 2021;
    _currentMonth = 10;
    _currentDay = 8;
    _currentHour = 5;
    _currentMinute = 0;

    _counterLastHeatingState = 0;
    _counterHeatingTime = 0;
    _counterHeatingSwitch = 0;
    _counterRunTime = 0;

    PrepareAquariumService(context);

    while (true)
    {
      SimulateWater(context);

      SimulateTime();

      PrepareAquariumService(context);
      AquariumService_Service(&context);

      RewriteScreen(context);

      ServiceCounters(context);

      std::this_thread::sleep_for(std::chrono::milliseconds(TickMs));
    }
}
