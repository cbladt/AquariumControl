#ifndef H_AQUARIUMSERVICE_H_
#define H_AQUARIUMSERVICE_H_

typedef unsigned char Boolean_t;

typedef Boolean_t RelayState_t;

typedef float Temperature_t;

typedef unsigned char Percentage_t;

typedef unsigned int Milliseconds_t;

typedef unsigned char Hour;

typedef unsigned char Minute;

typedef unsigned char Second;

typedef unsigned long Timestamp_t;

typedef void (*GetTimestamp_t)(Hour, Minute, Timestamp_t*);

typedef struct
{
  struct
  {
    Temperature_t waterT1;
    Temperature_t waterT2;
    Temperature_t waterTHeat;

    Boolean_t externStartSignal;
  } Input;

  struct
  {
    Boolean_t enabled;
    Temperature_t waterTSetpoint;    
    Temperature_t heaterTDiffMax;    
    Boolean_t onlyRunHeaterAlongWithWaterPump;

    Hour waterPumpBeginHour;
    Minute waterPumpBeginMinute;
    Hour waterPumpStopHour;
    Minute waterPumpStopMinute;

    Hour airPumpBeginHour;
    Minute airPumpBeginMinute;
    Hour airPumpStopHour;
    Minute airPumpStopMinute;

    Hour lightBeginHour;
    Minute lightBeginMinute;
    Hour lightStopHour;
    Minute lightStopMinute;
  } Parameter;

  struct
  {
    float MaxKp;
    float MaxTn;
    float integrator;
    float antiIntegratorWindup;
    Percentage_t MaxOutput;
  } Regulator;

  struct
  {
    GetTimestamp_t getTime;
    Hour currentHour;
    Minute currentMinute;
    Minute currentSecond;
  } Time;

  struct
  {
    Boolean_t waterPumpIsRunning;
    Boolean_t airPumpIsRunning;    
    Boolean_t lightIsRunning;
    Boolean_t heaterIsRunning;
    Percentage_t heaterPercent;
  } Output;
} AquariumServiceContext_t;

#ifdef __cplusplus
extern "C" {
#endif
void AquariumService_Service(AquariumServiceContext_t* context);
#ifdef __cplusplus
}
#endif

#endif /* H_AQUARIUMSERVICE_H_ */
