/** @brief Gh control constant, structures, function prototypes
 *  @file ghcontrol.h
 */

#ifndef GHCONTROL_H
#define GHCONTROL_H

// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sensehat.h"

// Constants

#define SEARCHSTR "serial\t\t:"
#define SYSINFOBUFSZ 512
#define GHUPDATE 2000
#define SENSORS 3
#define TEMPERATURE 0
#define HUMIDITY 1
#define PRESSURE 2
#define SIMULATE 0
#define USTEMP 	50
#define LSTEMP -10
#define USHUMID 100
#define LSHUMID 0
#define USPRESS 1016
#define LSPRESS 975
#define STEMP 25.0
#define SHUMID 55.0
#define ON 1
#define OFF 0
#define CTIMESTRSZ 25
#define NUMBARS 8
#define NUMPTS 8.0
#define TBAR 7
#define HBAR 5
#define PBAR 3
#define SENSEHAT 1
#define SIMTEMPERATURE 0
#define SIMHUMIDITY 0
#define SIMPRESSURE 0

// Structures

typedef struct readings
{
	time_t rtime;
	float temperature;
	float humidity;
	float pressure;
}reading_s;


typedef struct setpoints
{
	float temperature;
	float humidity;
}setpoint_s;

typedef struct controls
{
	int heater;
	int humidifier;
}control_s;

///@cond INTERNAL
// Function prototypes

void GhDisplayHeader(const char * sname);
u_int64_t GhGetSerial(void);
int GhGetRandom(int range);
void GhDelay(int milliseconds);
int GhLogData(const char * fname, reading_s ghdata);
void GhControllerInit(void);
void GhDisplayControls(control_s ctrl);
void GhDisplayReadings(reading_s rdata);
void GhDisplayTargets(setpoint_s spts);
control_s GhSetControls(setpoint_s target,reading_s rdata);
setpoint_s GhSetTargets(void);
void GhDisplayAll(reading_s rd,setpoint_s sd);
int GhSetVerticalBar(int bar, COLOR_SENSEHAT pxc, uint8_t value);

float GhGetHumidity(void);
float GhGetPressure(void);
float GhGetTemperature(void);
reading_s GhGetReadings(void);
int GhSaveSetpoints(const char * fname, setpoint_s spts);
setpoint_s GhRetrieveSetpoints(const char * fname);

///@endcond
#endif
