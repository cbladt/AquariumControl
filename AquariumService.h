#ifndef H_AQUARIUMSERVICE_H_
#define H_AQUARIUMSERVICE_H_

typedef unsigned char Boolean_t;

typedef Boolean_t RelayState_t;

typedef signed short Temperature_t;

typedef unsigned char Percentage_t;

typedef unsigned int Milliseconds_t;

typedef unsigned char Hour;

typedef unsigned char Minute;

typedef unsigned int Timestamp_t;

typedef void (*SetRelayCallback_t)(RelayState_t);
typedef void (*GetTemperature_t)(Temperature_t*);
typedef void (*GetTimestamp_t)(Hour, Minute, Timestamp_t*);

typedef struct
{
	Boolean_t enabled;

	Temperature_t waterTemperatureSetpoint;
	Temperature_t waterTemperatureActual;
	Temperature_t waterTemperatureHysteresis;
	Temperature_t heaterTemperatureActual;
	Temperature_t waterHeaterTemperatureDiffSetpoint;
	Temperature_t waterHeaterTemperatureDiffActual;
	Temperature_t waterHeaterTemperatureDiffHysteresis;

	Boolean_t heaterEnabled;

	Boolean_t waterPumpIsRunning;
	SetRelayCallback_t setWaterPumpState;
	Hour waterPumpBeginHour;
	Minute waterPumpBeginMinute;
	Hour waterPumpStopHour;
	Minute waterPumpStopMinute;

	Boolean_t airPumpIsRunning;
	SetRelayCallback_t setAirPumpState;
	Hour airPumpBeginHour;
	Minute airPumpBeginMinute;
	Hour airPumpStopHour;
	Minute airPumpStopMinute;

	Boolean_t heaterIsRunning;
	SetRelayCallback_t setHeaterState;

	Boolean_t lightIsRunning;
	SetRelayCallback_t setLightState;
	Hour lightBeginHour;
	Minute lightBeginMinute;
	Hour lightStopHour;
	Minute lightStopMinute;

	GetTemperature_t getWaterTemperature;
	GetTemperature_t getHeaterTemperature;
	GetTimestamp_t getTime;

} AquariumServiceContext_t;

void AquariumService_Service(AquariumServiceContext_t* context);

#endif /* H_AQUARIUMSERVICE_H_ */
