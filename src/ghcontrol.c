/** @brief Gh control functions
 *  @file ghcontrol.c
 */
#include "ghcontrol.h"
#include "sensehat.h"
#include <cstring>
#include <string.h>

#if SENSEHAT
SenseHat Sh;
#endif

int GhSetVerticalBar(int bar, COLOR_SENSEHAT pxc, uint8_t value)
{

	int i;

	if (value > 7)
	{
		value = 7;
	}
	if (bar >= 0 && bar < 8 && value >= 0 && value < 8)
	{
		for (i = 0; i <= value; i++)
		{
			Sh.LightPixel(i, bar, pxc);
		}
		for (i = value + 1; i < 8; i++)
		{
			Sh.LightPixel(i, bar, BLACK);
		}
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

void GhDisplayAll(reading_s rd, setpoint_s sd)
{

	int rv, sv, avh, avl;

	// Clear the display
	Sh.WipeScreen();

	// Temperature
	rv = (int)(8.0 * (((rd.temperature - LSTEMP) / (USTEMP - LSTEMP)) + 0.05)) - 1;
	sv = (int)(8.0 * (((sd.temperature - LSTEMP) / (USTEMP - LSTEMP)) + 0.05)) - 1;
	GhSetVerticalBar(TBAR, GREEN, rv);
	Sh.LightPixel(sv, TBAR, MAGENTA);

	// HUmidity
	rv = (int)(8.0 * (((rd.humidity - LSHUMID) / (USHUMID - LSHUMID)) + 0.05)) - 1;
	sv = (int)(8.0 * (((sd.humidity - LSHUMID) / (USHUMID - LSHUMID)) + 0.05)) - 1;
	GhSetVerticalBar(HBAR, GREEN, rv);
	Sh.LightPixel(sv, HBAR, MAGENTA);

	// Pressure
	rv = (int)(8.0 * (((rd.pressure - LSPRESS) / (USPRESS - LSPRESS)) + 0.05)) - 1;

	GhSetVerticalBar(PBAR, GREEN, rv);
}

//This function saves the set points.
int GhSaveSetpoints(const char *fname, setpoint_s spts)
{
	FILE *fp;

	fp = fopen(fname, "w");
	if (fp == NULL)
	{
		return 0;
	}
	else
	{
		fwrite(&spts, sizeof(spts), 1, fp);
	}
	fclose(fp);
	return 1;
}

setpoint_s GhRetrieveSetpoints(const char *fname)
{
	setpoint_s spts = {0, 0};

	FILE *fp;
	fp = fopen(fname, "r");
	if (fp == NULL)
	{
		return spts;
	}
	else
	{
		fread(&spts, sizeof(spts), 1, fp);
	}

	fclose(fp);
	return spts;
}

int GhLogData(const char *fname, reading_s ghdata)
{
	FILE *fp;
	char ltime[CTIMESTRSZ];

	fp = fopen(fname, "a");
	if (fp == NULL)
	{
		fprintf(stdout, "\nCan't open file, data not retrived!\n");
		return 0;
	}
	strcpy(ltime, ctime(&ghdata.rtime));
	ltime[3] = ',';
	ltime[7] = ',';
	ltime[10] = ',';
	ltime[19] = ',';
	fprintf(fp, "\n%.24s,%5.1lf,%5.1lf,%6.1lf", ltime, ghdata.temperature, ghdata.humidity, ghdata.pressure);

	fclose(fp);
	return 1;
}

void GhDisplayHeader(const char *sname)
{
	fprintf(stdout, "%s's CENG153 Greenhouse Controller\n", sname);
}

u_int64_t GhGetSerial(void)
{
	static u_int64_t serial = 0;
	FILE *fp;
	char buf[SYSINFOBUFSZ];
	char searchstring[] = SEARCHSTR;
	fp = fopen("/proc/cpuinfo", "r");

	if (fp != NULL)
	{
		while (fgets(buf, sizeof(buf), fp) != NULL)
		{
			if (!strncasecmp(searchstring, buf, strlen(searchstring)))
			{
				sscanf(buf + strlen(searchstring), "%Lx", &serial);
			}
		}
		fclose(fp);
	}

	if (serial == 0)
	{
		system("uname -a");
		system("ls --fu /user/lib/codeblocks | grep -Po '\\.\\K[^ ]+'>stamp.txt");
		fp = fopen("stamp.txt", "r");

		if (fp != NULL)
		{
			while (fgets(buf, sizeof(buf), fp) != NULL)
			{
				sscanf(buf, "%Lx", &serial);
			}
			fclose(fp);
		}
	}

	return serial;
}

int GhGetRandom(int range)
{
	return rand() % range;
}

void GhDelay(int milliseconds)
{
	long wait;
	clock_t now, start;

	wait = milliseconds * (CLOCKS_PER_SEC / 1000);
	start = clock();
	now = start;
	while ((now - start) < wait)
	{
		now = clock();
	}
}

void GhControllerInit(void)
{
	srand((unsigned)time(NULL));
}

void GhDisplayReadings(reading_s rdata)
{
	fprintf(stdout, "\n%s Readings\tT: %5.1fC\tH: %5.1f%%\tP: %6.1fmB\n",
			ctime(&rdata.rtime), rdata.temperature, rdata.humidity, rdata.pressure);
}

void GhDisplayControls(control_s ctrl)
{
	fprintf(stdout, " Controls\tHeater: %d\tHumidifier: %d\n", ctrl.heater, ctrl.humidifier);
}

void GhDisplayTargets(setpoint_s spts)
{
	fprintf(stdout, " Setpoints\tT: %5.1fC\tH: %5.1f%\n", spts.temperature, spts.humidity);
}

void GhSetControls(int *heater, int *humidifier, reading_s rdata)
{
	if (rdata.temperature < STEMP)
	{
		*heater = ON;
	}
	else
	{
		*heater = OFF;
	}

	if (rdata.humidity < SHUMID)
	{
		*humidifier = ON;
	}
	else
	{
		*humidifier = OFF;
	}
}

float GhGetHumidity(void)
{
#if SIMULATE
	return GhGetRandom(USHUMID - LSHUMID) + LSHUMID;
#else
	return Sh.GetHumidity();
#endif
}

float GhGetPressure(void)
{
#if SIMULATE
	return GhGetRandom(USPRESS - LSPRESS) + LSPRESS;
#else
	return Sh.GetPressure();
#endif
}

float GhGetTemperature(void)
{
#if SIMULATE
	return GhGetRandom(USTEMP - LSTEMP) + LSTEMP;
#else
	return Sh.GetTemperature();
#endif
}

reading_s GhGetReadings(void)
{
	reading_s now;

	now.rtime = time(NULL);
	now.temperature = GhGetTemperature();
	now.humidity = GhGetHumidity();
	now.pressure = GhGetPressure();
	return now;
}

setpoint_s GhSetTargets(void)
{
	setpoint_s cpoints;

	cpoints = GhRetrieveSetpoints("setpoints.dat");

	if (cpoints.temperature == 0)
	{
		cpoints.temperature = STEMP;
		cpoints.humidity = SHUMID;
		GhSaveSetpoints("setpoints.dat", cpoints);
	}
	return cpoints;
}

control_s GhSetControls(setpoint_s target, reading_s rdata)
{
	control_s cset;
	if (rdata.temperature < target.temperature)
	{
		cset.heater = ON;
	}
	else
	{
		cset.heater = OFF;
	}

	if (rdata.humidity < target.humidity)
	{
		cset.humidifier = ON;
	}
	else
	{
		cset.humidifier = OFF;
	}

	return cset;
}
