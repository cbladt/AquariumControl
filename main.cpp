#include <iostream>
#include <iomanip>
#include <time.h>
#include <thread>

#include <Control.h>
#include "AquariumService.h"

#define  Print(exp) (std::cout << #exp << "\t" << std::to_string(exp) << std::endl)
#define fPrint(exp) (std::cout << #exp << "\t\t" << std::fixed << std::setprecision(2) << exp << std::endl)

static const constexpr auto TickMs    = 100;
static const constexpr auto TimeStep  = 5;
static const constexpr auto Positive  = 1.002f;
static const constexpr auto Negative  = 0.998f;
static const constexpr auto RoomTemp  = 11.0f;

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

static unsigned char _counterScreenRewrite;
static float _counterHeatingTime;
static float _counterRunTime;

static auto SinusLikeFluctuation()
{
  return (_currentMinute-30.0f)/10000.0f;
}

static Boolean_t GetExternStartSignal()
{
  if (_currentHour == 2)
  {
    return 0;
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
  system("clear");

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
  Print(ctx.Output.heaterPercent);
  Print(ctx.Output.lightIsRunning);
  std::cout << std::endl;

  std::cout << "Counters" << std::endl;
  std::cout << "Runtime Hours\t\t\t" << std::to_string(_counterRunTime / 60) << std::endl;
  std::cout << "HeatingTime Minutes\t\t" << std::to_string(_counterHeatingTime) << std::endl;  
  std::cout << std::endl;
}

static void MaybeRewriteScreen(AquariumServiceContext_t& ctx)
{
  _counterScreenRewrite += TickMs;

  if (_counterScreenRewrite > 0)
  {
    RewriteScreen(ctx);
    _counterScreenRewrite = 0;
  }
}

static void SimulateWater(AquariumServiceContext_t& context)
{   
  // Actively heating the water.
  if (context.Output.heaterPercent > 0)
  {    
    auto power = (AquariumHeaterEffect * context.Output.heaterPercent) / 100;
    auto tWater = (_waterTemp + _heaterTemp) / 2;
    auto tTarget = tWater + 0.5;
    auto totalMinutes = AquariumVolumeL * WaterSpecificHeat * (tTarget - tWater) / AquariumHeaterEffect / 60;
    auto step = TimeStep / totalMinutes;

    auto newWaterT = _waterTemp + step * Negative;
    _waterTemp = (_waterTemp + newWaterT) / 2;

    auto newHeatT = _heaterTemp + step * Positive;
    _heaterTemp = (_heaterTemp + newHeatT) / 2;
  }

  // Heater is off, but element still warmer than the water.
  else if (_heaterTemp > _waterTemp)
  {    
    _waterTemp  *= Positive;
    _heaterTemp *= Negative;
  }

  else
  {    
    // Nominal heat dissipation.
    auto tWater = (_waterTemp + _heaterTemp) / 2;
    if (tWater > RoomTemp)
    {      
      auto powerW = 5.68f / (AquariumWidthM * AquariumLengthM);
      auto totalMinutes = AquariumVolumeL * WaterSpecificHeat * (RoomTemp - tWater) / powerW / 60;
      auto step = (TimeStep / totalMinutes);

      _waterTemp  += step;
      _heaterTemp += step;
    }

    else
    {      
      _waterTemp  = ControlLimit(_waterTemp + SinusLikeFluctuation(), RoomTemp*0.95f, RoomTemp*1.05f);
      _heaterTemp = ControlLimit(_heaterTemp+ SinusLikeFluctuation(), RoomTemp*0.95f, RoomTemp*1.05f);
    }
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
  context.Input.waterT2 = (_waterTemp);
  context.Input.waterTHeat = _heaterTemp;
  context.Input.externStartSignal = GetExternStartSignal();

  context.Parameter.enabled = 1;
  context.Parameter.waterTSetpoint = 25;
  context.Parameter.heaterTDiffMax = 0.5;
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

  context.Regulator.MaxKp = 1000;
  context.Regulator.MaxTn = 1000;
  context.Regulator.antiIntegratorWindup = 10;
  context.Regulator.MaxOutput = 100;

  context.Time.getTime = &GetTime;
  context.Time.currentHour = _currentHour;
  context.Time.currentMinute = _currentMinute;
  context.Time.currentSecond = 0;
}

static void ServiceCounters(AquariumServiceContext_t& context)
{
  if (context.Output.heaterPercent > 0)
  {
    _counterHeatingTime += TimeStep;
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
    _currentHour = 3;
    _currentMinute = 0;

    _counterHeatingTime = 0;
    _counterRunTime = 0;

    PrepareAquariumService(context);    

    while (true)
    {
      SimulateWater(context);

      SimulateTime();

      PrepareAquariumService(context);
      AquariumService_Service(&context);

      MaybeRewriteScreen(context);

      ServiceCounters(context);

      std::this_thread::sleep_for(std::chrono::milliseconds(TickMs));
    }
}
