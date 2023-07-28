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

/** Sets the vertical bar on the display with the specified value
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param bar The position of the vertical bar on the display
 *  @param pxc The color to set the pixels of the vertical bar
 *  @param The value to set the vertical bar
 *  @return EXIT_SUCCESS if the vertical bar is set successfully, EXIT_FAILURE otherwise.
*/
int GhSetVerticalBar(int bar, COLOR_SENSEHAT pxc, uint8_t value){

    int i;

    if (value >7 ){value = 7;}
    if (bar >= 0 && bar < 8 && value >= 0 && value < 8 )
    {
        for(i=0; i<=value; i++)
        {
            Sh.LightPixel(i, bar, pxc);
        }
        for(i=value+1; i<8; i++)
        {
            Sh.LightPixel(i, bar, BLACK);
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}


/** Displays the Greenhouse readings and setpoints
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param rd the object of readings structure
 *  @param sd the object of setpoints structure
 *  @return void
*/
void GhDisplayAll(reading_s rd, setpoint_s sd) {

    int rv, sv, avh, avl;

    // Clear the display
    Sh.WipeScreen();

    //Temperature
    rv = (int)(8.0 * (((rd.temperature - LSTEMP) / (USTEMP - LSTEMP)) + 0.05)) - 1;
    sv = (int)(8.0 * (((sd.temperature - LSTEMP) / (USTEMP - LSTEMP)) + 0.05)) - 1;
    GhSetVerticalBar(TBAR, GREEN, rv);
    Sh.LightPixel(sv, TBAR, MAGENTA);

    // HUmidity
    rv = (int)(8.0 * (((rd.humidity - LSHUMID) / (USHUMID - LSHUMID)) + 0.05)) - 1;
    sv = (int)(8.0 * (((sd.humidity - LSHUMID) / (USHUMID - LSHUMID)) + 0.05)) - 1;
    GhSetVerticalBar(HBAR,GREEN, rv);
    Sh.LightPixel(sv, HBAR, MAGENTA);

    // Pressure
    rv = (int)(8.0 * (((rd.pressure - LSPRESS) / (USPRESS - LSPRESS)) + 0.05)) - 1;

    GhSetVerticalBar(PBAR, GREEN, rv);
}


/** Saves the setpoints to a file.
 * @param fname The name of the file to save the setpoints to.
 * @param spts The structure containing the setpoints to be saved.
 * @author Paul Moggach
 * @author Darshan Prajapati
 * @since 2021-05-07
 * @version CENG153, serial:6a86135c
 * @return 1 if the setpoints are saved successfully, 0 otherwise.
 */
int GhSaveSetpoints(const char * fname, setpoint_s spts)
{
	FILE *fp;

	fp = fopen(fname, "w");
	if(fp == NULL)
	{
		return 0;
	}
	else
	{
		fwrite(&spts, sizeof(spts),1,fp);
	}
	fclose(fp);
	return 1;

}


/** Retrieves the setpoints from a file.
 * @version CENG153, serial:6a86135c
 * @author Paul Moggach
 * @author Darshan Prajapati
 * @param fname The name of the file to retrieve the setpoints from.
 * @return The structure containing the retrieved setpoints. If the file cannot be opened or read, it returns a structure with both setpoints set to 0.
 * @since 2021-05-07
 */
setpoint_s GhRetrieveSetpoints(const char * fname)
{
	setpoint_s spts = {0,0};

	FILE *fp;
	fp = fopen(fname, "r");
	if(fp == NULL)
	{
		return spts;
	}
	else
	{
		fread(&spts, sizeof(spts),1,fp);
	}

	fclose(fp);
	return spts;

}


/**This function logs the greenhouse data, including temperature, humidity, and pressure, to the specified file.
 * @version CENG153, serial:6a86135c
 * @author Paul Moggach
 * @author Darshan Prajapati
 * @param fname The name of the file to log the data.
 * @param ghdata The structure containing the greenhouse data.
 * @return 1 if the data is logged successfully, 0 otherwise.
 * @since 2021-05-07
 */
int GhLogData(const char * fname, reading_s ghdata)
{
	FILE *fp;
	char ltime[CTIMESTRSZ];

	fp = fopen(fname,"a");
	if(fp == NULL)
	{
		fprintf(stdout,"\nCan't open file, data not retrived!\n");
		return 0;
	}
	strcpy(ltime, ctime(&ghdata.rtime));
	ltime[3] = ',';
        ltime[7] = ',';
    	ltime[10] = ',';
    	ltime[19] = ',';
	fprintf(fp,"\n%.24s,%5.1lf,%5.1lf,%6.1lf",ltime,ghdata.temperature,ghdata.humidity,ghdata.pressure);

	fclose(fp);
	return 1;

}

/** Prints Gh Controller Title
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param sname string with Operator's name
 *  @return void
*/
void GhDisplayHeader(const char * sname)
{
	fprintf(stdout,"%s's CENG153 Greenhouse Controller\n", sname);
}


/** Gets the serial number of PI
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param void
 *  @return serial number
*/
u_int64_t GhGetSerial(void)
{
	static u_int64_t serial = 0;
	FILE * fp;
	char buf[SYSINFOBUFSZ];
	char searchstring[] = SEARCHSTR;
	fp = fopen ("/proc/cpuinfo","r");

	if (fp != NULL )
	   {
		while (fgets(buf, sizeof(buf), fp)!= NULL)
		   {
			if(!strncasecmp(searchstring, buf, strlen(searchstring)))
			{
				sscanf(buf+strlen(searchstring), "%Lx", &serial);
			}
		   }
		fclose(fp);
	   }

	if (serial ==0)
	{
	    system("uname -a");
	    system("ls --fu /user/lib/codeblocks | grep -Po '\\.\\K[^ ]+'>stamp.txt");
	    fp = fopen("stamp.txt", "r");

	    if(fp != NULL)
	    {
		 while(fgets(buf, sizeof(buf), fp)!= NULL)
		 {
			 sscanf(buf, "%Lx", &serial);
		 }
		 fclose(fp);
	    }
	}

	return serial;

}


/** Generates a random number within the specified range
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param The upper limit for the range of random numbers
 *  @return A random number within the range [0, range)
*/
int GhGetRandom(int range)
{
	return rand() % range;
}


/** Delays the execution for the specified number of milliseconds
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param sname string with Operator's namesname string with Operator's name
 *  @return void
*/
void GhDelay(int milliseconds)
{
	long wait;
	clock_t now, start;

	wait = milliseconds*(CLOCKS_PER_SEC/1000);
	start = clock();
	now = start;
	while((now - start) < wait)
	{
		now = clock();
	}
}


/** Initializes the Greenhouse Controller
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param void
 *  @return void
*/
void GhControllerInit(void)
{
	srand((unsigned) time(NULL));
}


/** Displays the Greenhouse Sensor Readings.
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param The time when readings were recorded (rtime)
 *  @param dreads An array containing the sensor readings
 *  @return void
*/
void GhDisplayReadings(reading_s rdata)
{
	fprintf(stdout,"\n%s Readings\tT: %5.1fC\tH: %5.1f%%\tP: %6.1fmB\n",
		ctime(&rdata.rtime),rdata.temperature,rdata.humidity,rdata.pressure);

}


/** Displays the Greenhouse Controls.
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param heater A pointer to the integer representing the state of the heater
 *  @param humidifier a pointer to the integer representing the state of the humidifier
 *  @return void
*/
void GhDisplayControls(control_s ctrl)
{
	fprintf(stdout," Controls\tHeater: %d\tHumidifier: %d\n", ctrl.heater, ctrl.humidifier);
}


/** Displays the Greenhouse Setpoints.
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param void
 *  @return void
*/
void GhDisplayTargets(setpoint_s spts)
{
	fprintf(stdout," Setpoints\tT: %5.1fC\tH: %5.1f%\n", spts.temperature, spts.humidity);
}


/** Sets the greenhouse control based on sensor readings
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param heater A pointer to the integer representing the state of the heater
 *  @param humidifier a pointer to the integer representing the state of the humidifier
 *  @param readings An array containing the sensor readings
 *  @return void
*/
void GhSetControls(int *heater,int *humidifier,reading_s rdata)
{
	if(rdata.temperature<STEMP)
	{
		*heater = ON;
	}
	else
	{
		*heater = OFF;
	}


	if(rdata.humidity< SHUMID)
	{
		*humidifier = ON;
	}
	else
	{
		*humidifier = OFF;
	}

}


/** Retrieves the current humidity reading
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param void
 *  @return The current humidity reading
*/
float GhGetHumidity(void)
{
#if SIMULATE
	return GhGetRandom(USHUMID - LSHUMID) + LSHUMID;
#else
	return Sh.GetHumidity();
#endif
}


/** Retrieves the current Pressure reading
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param void
 *  @return The current Pressure reading
*/
float GhGetPressure(void)
{
#if SIMULATE
	return GhGetRandom(USPRESS - LSPRESS) + LSPRESS ;
#else
	return Sh.GetPressure();
#endif
}


/** Retrieves the current Temperature reading
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2021-05-07
 *  @param void
 *  @return The current Temperature reading
*/
float GhGetTemperature(void)
{
#if SIMULATE
	return GhGetRandom(USTEMP - LSTEMP) + LSTEMP;
#else
	return Sh.GetTemperature();
#endif
}


/** Get the current readings for temperature, humidity, and pressure
 *  @version CENG153, serial:6a86135c
 *  @author Paul Moggach
 *  @author Darshan Prajapati
 *  @since 2023-06-21
 *  @param readings a struct conaiting current readings of weather
 *  @return void
*/
reading_s GhGetReadings(void)
{
	reading_s now;

	now.rtime = time(NULL);
	now.temperature = GhGetTemperature();
	now.humidity = GhGetHumidity();
	now.pressure = GhGetPressure();
	return now;
}


/** Set the target values for temperature and humidity
 * @version CENG153, serial: 6a86135c
 * @author Paul Moggach
 * @author Darshan Prajapati
 * @since 2023-06-21
 * @param void
 * @return a struct containing the target values for temperature and humidity
*/

setpoint_s GhSetTargets(void)
{
	setpoint_s cpoints;

	cpoints = GhRetrieveSetpoints("setpoints.dat");

	if(cpoints.temperature == 0)
	{
		cpoints.temperature = STEMP;
		cpoints.humidity = SHUMID;
		GhSaveSetpoints("setpoints.dat", cpoints);
	}
	return cpoints;
}

/** Set the controls for heating and humidification based on target values and current readings
 * @version CENG153, serial: 6a86135c
 * @author Paul Moggach
 * @author Darshan Prajapati
 * @since 2023-06-21
 * @param target The struct containing the target values for temperature and humidity
 * @param rdata The struct containing the current readings for temperature and humidity
 * @return a struct containing the control values for the heater and humidifier
*/
control_s GhSetControls(setpoint_s target,reading_s rdata)
{
	 control_s cset;
	 if(rdata.temperature < target.temperature)
	 {
		 cset.heater = ON;
	 }
         else
	 {
		 cset.heater = OFF;
	 }

         if(rdata.humidity < target.humidity)
	 {
		 cset.humidifier = ON;
	 }
         else
	 {
		 cset.humidifier = OFF;
	 }

	return cset;
}
