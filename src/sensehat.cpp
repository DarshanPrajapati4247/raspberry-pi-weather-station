/**
 * @file SenseHat.cpp
 * @date 4 mars 2018
 * @version 1.2
 * @authors Philippe SIMIER Philippe CRUCHET Christophe GRILLO
 * @details Classe SenseHat : Gestion de la carte SenseHat
 * @version 1.3
 * @date 30 July 2019
 * @authors Jon Dellaria bug fixes, method translation to English and Temperature Measurement adjustments required for the Raspberry for a true temperature reading.
 * @version 1.3a
 * @date 05 May 2021
 * @authors Paul Moggach removed french to compact files and make it more readable. Made changes to conform with Allman style., Python Emulator support
 */
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include "sensehat.h"
#include "font.h"

#define NUMBER_OF_TRIES_BEFORE_FAILURE 1000
#if SENSEHAT_EMULATOR
int numReadings = 0;    // python threads maximum counter
#endif

static int is_framebuffer_device(const struct dirent *dir)
{
	return strncmp(FB_DEV_NAME, dir->d_name,strlen(FB_DEV_NAME)-1) == 0;
}

static int open_fbdev(const char *dev_name)
{
	struct dirent **namelist;
	int i, ndev;
	int fd = -1;
	struct fb_fix_screeninfo fix_info;

	ndev = scandir(DEV_FB, &namelist, is_framebuffer_device, versionsort);
	if (ndev <= 0) { return ndev; }

	for (i = 0; i < ndev; i++)
	{
		char fname[64];

		snprintf(fname, sizeof(fname),"%s/%s", DEV_FB, namelist[i]->d_name);
		fd = open(fname, O_RDWR);
		if (fd < 0) { continue; }
		ioctl(fd, FBIOGET_FSCREENINFO, &fix_info);
		if (strcmp(dev_name, fix_info.id) == 0){ break; }
		close(fd);
		fd = -1;
	}
	for (i = 0; i < ndev; i++) { free(namelist[i]); }
	return fd;
}

static int is_event_device(const struct dirent *dir)
{
	return strncmp(EVENT_DEV_NAME, dir->d_name,strlen(EVENT_DEV_NAME)-1) == 0;
}

static int open_evdev(const char *dev_name)
{
	int tries;
	struct dirent **namelist;
	int i, ndev;
	int fd = -1;
	int sortie = false;
	char fname[64];
	char name[256];

	tries = 0;
	while(true)
	{
		ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
		if (ndev <= 0)
		{
			tries++;
			usleep(100);
		}
		else
		{
			break;
		}
		if (tries > NUMBER_OF_TRIES_BEFORE_FAILURE)
		{
			exit (EXIT_FAILURE);
		}
	}

	i=0;
	do
	{
		snprintf(fname, sizeof(fname),"%s/%s", DEV_INPUT_EVENT, namelist[i++]->d_name);
		tries = 0;
		while(true)
		{
			fd = open(fname, O_RDONLY );
			if (fd < 0)
			{
				tries++;
				usleep(100);
			}
			else
			{
				break;
			}
			if (tries > NUMBER_OF_TRIES_BEFORE_FAILURE)
			{
				exit (EXIT_FAILURE);
			}
		}
		ioctl(fd, EVIOCGNAME(sizeof(name)), name);
		if (strcmp(dev_name, name) != 0) { close(fd); }
		else { sortie = true; }
		}
    while( i<ndev && sortie != true);

	for (i = 0; i < ndev; i++) { free(namelist[i]); }
	return fd;
}

uint16_t handle_events(int evfd)
{
	struct input_event ev;
	int rd;
	uint16_t retour = 0 ;
	int flag;

	flag = fcntl(evfd,F_GETFL,0);
	fcntl(evfd,F_SETFL,flag | O_NONBLOCK | O_NOCTTY);

	rd = read(evfd, &ev, sizeof(struct input_event));
	if(rd > 0)
	{
		if(ev.type == EV_KEY && ev.value == 1) { retour = ev.code; }
    }
    return retour;
}

/**
 * @brief SenseHat::SenseHat
 * @details Constructeur de la classe, initialise les attributs
 *          par défaut imu, leds, Joystick, buffer.
 */
SenseHat::SenseHat(void)
{
#if SENSEHAT_EMULATOR
	Py_Initialize();
#else
  int tries;
  settings = new RTIMUSettings("RTIMULib");

  tries = 0;
  while(true)
  {
    imu = RTIMU::createIMU(settings);
    if ((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL))
    {
      tries++;
      usleep(100);
    }
    else
    {
      break;
    }
    if (tries > NUMBER_OF_TRIES_BEFORE_FAILURE)
    {
      exit (EXIT_FAILURE);
    }
  }

  imu->IMUInit();
  imu->setSlerpPower(0.02);
  imu->setGyroEnable(true);
  imu->setAccelEnable(true);
  imu->setCompassEnable(true);
  InitializeLeds();
  InitializeJoystick();
  InitializeHumidity();
  InitializePressure();
#endif
  buffer=" ";
  color=BLUE;
  rotation = 0;
}

/**
 * @brief SenseHat::~SenseHat
 * @details Destructeur de la classe
 */
SenseHat::~SenseHat(void)
{
#if SENSEHAT_EMULATOR
	Py_Finalize();
#else
    delete settings;
#endif
}

/**
 * @brief SenseHat::operator<<
 * @details surcharge de l'opérateur << pour les modificateurs endl et flush
 */
SenseHat& SenseHat::operator<<(SenseHat& (*fp)(SenseHat&))
{
	return (*fp)(*this);
}

/**
 * @brief SenseHat::Version
 * @details affiche la version de la classe
 */
void SenseHat::Version(void)
{
	std::cout << "SenseHat PCT,PSR,CGO Version 1.2.0" << std::endl;
}

void SenseHat::SetColor(uint16_t _color)
{
	color = _color;
}

void SenseHat::SetRotation(uint16_t _rotation)
{
	rotation = _rotation;
}

/**
 * @brief SenseHat::AfficherLettre
 * @param lettre char
 * @param colorText  uint16_t une color au format 565
 * @param colorBackground   uint16_t une color au format 565
 * @details affiche un caractère (lettre ponctuation signe) sur l'afficheur
 */
void SenseHat::ViewLetter(char lettre, uint16_t colorText, uint16_t colorBackground)
{
	uint16_t chr[8][8];

	ConvertCharacterToPattern(lettre,chr,colorText,colorBackground);
	ViewPattern(chr);
}

/**
 * @brief SenseHat::LightPixel
 * @param row integer
 * @param column integer
 * @param color   uint16_t une color au format 565
 */
void SenseHat::LightPixel(int row, int column, uint16_t color)
{
#if SENSEHAT_EMULATOR
	char ltime [120];
	float conv;
	uint8_t red,green,blue = 0;

	red = ((color & RED) >> 11)/31.0*255.0;
	green = ((color & GREEN) >> 5)/63.0*255.0;
	blue = ((color & BLUE))/31.0*255.0;
	sprintf(ltime,
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"sense.set_pixel(%d,%d,%d,%d,%d)\n"
		,column,row,red,green,blue);
	PyRun_SimpleString(ltime);
#else
    if(row < 0)
	row = 0;
    if(column < 0)
	column = 0;

    fb->pixel[row%8][column%8] = color;
#endif
}

/**
 * @brief SenseHat::GetPixel
 * @param row integer
 * @param column integer
 * @return uint16_t color in 565 format
 */
uint16_t SenseHat::GetPixel(int row, int column)
{
	if(row < 0) { row = 0; }
	if(column < 0) { column = 0; }
	return fb->pixel[row%8][column%8] ;
}

/**
 * @brief SenseHat::ViewPattern
 * @param pattern uint16_t 8x8 arrays
 */
void SenseHat::ViewPattern(uint16_t pattern[][8])
{
	for(int row=0; row <8 ; row++)
	{
		for(int column=0 ; column <8 ; column++)
		{
			switch(this->rotation)
			{
				case   90:
				case -270:
					fb->pixel[7 - column][row] = pattern[row][column];
					break;
				case  180:
				case -180:
					fb->pixel[7 - row][7 - column] = pattern[row][column];
					break;
				case  270:
				case  -90:
					fb->pixel[column][7 - row] = pattern[row][column];
					break;
				default:
					fb->pixel[row][column] = pattern[row][column];
			}
		}
    }
}

/**
 * @brief SenseHat::RotatePattern
 * @param int angle de rotation 90, 180, 270, -90, -180, -270
 */
void SenseHat::RotatePattern(int angle)
{
    uint16_t tabAux[8][8];

    for(int row=0; row <8 ; row++)
    {
        for(int column=0 ; column <8 ; column++)
        {
            switch(angle)
            {
                case   90:
                case -270:
                    tabAux[7 - column][row] = fb->pixel[row][column];
                    break;
                case  180:
                case -180:
                    tabAux[7 - row][7 - column] = fb->pixel[row][column];
                    break;
                case  270:
                case  -90:
                    tabAux[column][7 - row] = fb->pixel[row][column];
                    break;
                default:
                    tabAux[row][column] = fb->pixel[row][column];
            }
        }
    }
    ViewPattern(tabAux);
}

/**
 * @brief SenseHat::WipeScreen
 * @param color uint16_t RG%565
 */
void SenseHat::WipeScreen(uint16_t color)
{
#if SENSEHAT_EMULATOR
	if (numReadings >=12)
	{
		numReadings=0;
		fprintf(stdout,"12 readings is about the limit for the emulator\n"
		     "the way that the current code is written since\n"
		     "it spawns too many threads and using Py_Finalize\n"
		     "causes a decref segmentation fault. In addition,\n"
		     "it doesn't respond to Ctrl-C thus exiting gracefully.\n");
     /* Note that if you want to exit sooner you can stop the ghc process
	by using Ctrl-Z, find the PID of ghc by using the command ps, and
	use kill -9 PID# to end the process. */
		exit(EXIT_FAILURE);
	}
	else
	{
		//printf("numReadings= %d\n",numReadings);
		numReadings++;
	}
    	PyRun_SimpleString(
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"sense.clear()\n"
		);
#else
	memset(fb, color, 128);
#endif
}

/**
 * @brief SenseHat::ScannerJoystick
 * @return le code équivalent aux touches du clavier enter,
 * fleche droite, gauche, haut et bas.
 */
char SenseHat::ScanJoystick(void)
{
	return handle_events(joystick);
}

/**
 * @brief SenseHat::ConvertRGB565
 * @param red uint8_t component
 * @param green uint8_t component
 * @param blue uint8_t component
 * @return uint16_t RGB565 color code
 */
uint16_t SenseHat::ConvertRGB565(uint8_t red, uint8_t green, uint8_t blue)
{
	blue &= 0xF8;
	green &= 0xFC;
	red &= 0xF8;
	return ((red<<8) + (green<<3) + (blue>>3));
}

/**
 * @brief surcharge de SenseHat::ConvertirRGB565
 * @param un tableau de trois uint8_t
 * @return uint16_t une color codée en RGB565
 * @details permet de convertir une color exprimer sous la forme
 *          d'un tableaude  trois entiers non signés sur 8 bits en un entier
 *          représentant la color codée en RGB565
 */
uint16_t SenseHat::ConvertRGB565(uint8_t color[])
{
    return ConvertRGB565(color[1],color[2],color[3]);
}

/**
 * @brief surcharge de SenseHat::ConvertirRGB565
 * @param string chaine de caratère représentant une color au format hexa
 * @return uint16_t une color codée en RGB565
 * @details permet de convertir une color exprimer sous la forme
 *          d'une chaîne de caractère au format hexa en un entier
 *          représentant la color codée en RGB565
 */
COLOR_SENSEHAT SenseHat::ConvertRGB565(std::string hexCode)
{
   int r, g, b;

   // Retire le hashtag ...
   if(hexCode.at(0) == '#')
   {
      hexCode = hexCode.erase(0, 1);
   }
   // puis extraction des valeurs r g b.
   std::istringstream(hexCode.substr(0,2)) >> std::hex >> r;
   std::istringstream(hexCode.substr(2,2)) >> std::hex >> g;
   std::istringstream(hexCode.substr(4,2)) >> std::hex >> b;

   return ConvertRGB565(r,g,b);
}

/**
 * @brief SenseHat::GetTemperature
 * @return float la valeur de la température exprimée en °C,
 */
float SenseHat::GetTemperature(void)
{
    float cpuTemp;
    float correctedTemp;
    float senseHatTemp;

    senseHatTemp = getRawTemperature();
    cpuTemp = getCpuTemperature();

    //temp_calibrated = temp - ((cpu_temp - temp)/FACTOR)
    correctedTemp = correctTemperature(senseHatTemp, cpuTemp);
    return (correctedTemp);
}

float SenseHat::correctTemperature(float senseHatTemp, float cpuTemp)
{
    float correctedTemp;
    float TEMPERATUREFACTOR = 1.2;

    //temp_calibrated = temp - ((cpu_temp - temp)/FACTOR)
    correctedTemp = senseHatTemp - ((cpuTemp - senseHatTemp)/TEMPERATUREFACTOR);
    return (correctedTemp);
}

/**
 * @brief SenseHat::getRawTemperature
 * @return float la valeur de la température exprimée en °C,
 */
float SenseHat::getRawTemperature(void)
{
	float senseHatTemp;
#if SENSEHAT_EMULATOR
	double reading=0;
	FILE *fp;

	PyRun_SimpleString(
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"temp=sense.temp\n"
		"f=open(\"tempfileforpython.txt\",\"w\")\n"
		"f.write(repr(temp))\n"
		"f.close()\n"
		);

	fp=fopen("tempfileforpython.txt","r");
	fscanf(fp, "%lf", &reading);
	fclose(fp);
    senseHatTemp = reading;
#else
	RTIMU_DATA data;

    pressure->pressureRead(data);
    senseHatTemp = data.temperature;
#endif
    return senseHatTemp;
}

/**
 * @brief SenseHat::getCpuTemperature
 * @return float la valeur de la température exprimée en °C,
 */
float SenseHat::getCpuTemperature(void)
{
    FILE *temperatureFile;
    double T;

    temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temperatureFile == NULL)
    {
//      printf ("Error getting Core Temperature!")  ; //print some message
      return (0);
    }
    fscanf (temperatureFile, "%lf", &T);
    T /= 1000;
//    printf ("The temperature is %6.3f C.\n", T);
    fclose (temperatureFile);
    return(T);
}


/**
 * @brief SenseHat::GetPressure
 * @return float la valeur de la Pression exprimée en hPa,
 */
float SenseHat::GetPressure(void)
{
	float pression = nan("");  // initialise la valeur à Not-A-Number
#if SENSEHAT_EMULATOR
	double reading=0;
	FILE *fp;

	PyRun_SimpleString(
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"temp=sense.pressure\n"
		"f=open(\"tempfileforpython.txt\",\"w\")\n"
		"f.write(repr(temp))\n"
		"f.close()\n"
		);

	fp=fopen("tempfileforpython.txt","r");
	fscanf(fp, "%lf", &reading);
	fclose(fp);
    pression = reading;
#else
    RTIMU_DATA data;

    if (pressure->pressureRead(data))
    {
    	if (data.pressureValid)
    	{
    	    pression = data.pressure;
        }
    }
#endif
    return pression;
}

/**
 * @brief SenseHat::GetHumidity
 * @return float la valeur de l'humidité relative exprimée en %,
 */
float SenseHat::GetHumidity(void)
{
	float humidi = nan("");  // initialise la valeur à Not-A-Number
#if SENSEHAT_EMULATOR
	double reading=0;
	FILE *fp;

	PyRun_SimpleString(
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"humid=sense.humidity\n"
		"f=open(\"humifileforpython.txt\",\"w\")\n"
		"f.write(repr(humid))\n"
		"f.close()\n"
		);

	fp=fopen("humifileforpython.txt","r");
	fscanf(fp, "%lf", &reading);
	fclose(fp);
	humidi = reading;
#else
	RTIMU_DATA data;

    if (humidity->humidityRead(data))
    {
        if (data.humidityValid)
        {
            humidi = data.humidity;
        }
    }
#endif
    return humidi;
}

/**
 * @brief SenseHat::GetOrientation
 * @return float la valeur de l'accélération angulaire suivant pitch roll et yaw
 */
void SenseHat::GetOrientation(float &pitch, float &roll, float &yaw)
{
#if SENSEHAT_EMULATOR
	FILE *fp;
    float p,r,y;

	PyRun_SimpleString
	(
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"p, r, y = sense.get_orientation().values()\n"
		"f=open(\"tempfileforpython.txt\",\"w\")\n"
		"f.write(\"%s %s %s\" % (p,r,y))\n"
		"f.close()\n"
	);

	fp=fopen("tempfileforpython.txt","r");
	fscanf(fp, "%f %f %f", &pitch,&roll,&yaw);
	fclose(fp);
#else
    while (imu->IMURead())
    {
        RTIMU_DATA imuData = imu->getIMUData();
        pitch = imuData.gyro.x();
        roll  = imuData.gyro.y();
        yaw   = imuData.gyro.z();
    }
#endif
}

/**
 * @brief SenseHat::GetAcceleration
 * @return float la valeur de l'accélération linéaire suivant X,Y,Z
 */
void SenseHat::GetAcceleration(float &x, float &y, float &z)
{
#if SENSEHAT_EMULATOR
	FILE *fp;
    float ax,ay,az;

	PyRun_SimpleString
	(
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"ax, ay, az = sense.get_accelerometer_raw().values()\n"
		"f=open(\"tempfileforpython.txt\",\"w\")\n"
		"f.write(\"%s %s %s\" % (ax,ay,az))\n"
		"f.close()\n"
	);

	fp=fopen("tempfileforpython.txt","r");
	fscanf(fp, "%f %f %f", &z,&y,&z);
	fclose(fp);
#else
    while (imu->IMURead())
    {
        RTIMU_DATA imuData = imu->getIMUData();
        x = imuData.accel.x();
        y = imuData.accel.y();
        z = imuData.accel.z();
    }
#endif
}

/**
 * @brief SenseHat::GetMagnetism
 * @return float la valeur du champ magnétique terrestre suivant X,Y,Z
 *
 */
void SenseHat::GetMagnetism(float &x, float &y, float &z)
{
#if SENSEHAT_EMULATOR
	FILE *fp;
    float mx,my,mz;

	PyRun_SimpleString
	(
		"from sense_emu import SenseHat\n"
		"sense=SenseHat()\n"
		"mx, my, mz = sense.get_compass_raw().values()\n"
		"f=open(\"tempfileforpython.txt\",\"w\")\n"
		"f.write(\"%s %s %s\" % (mx,my,mz))\n"
		"f.close()\n"
	);

	fp=fopen("tempfileforpython.txt","r");
	fscanf(fp, "%f %f %f", &z,&y,&z);
	fclose(fp);
#else
    while (imu->IMURead())
    {
        RTIMU_DATA imuData = imu->getIMUData();
        x = imuData.compass.x();
        y = imuData.compass.y();
        z = imuData.compass.z();
    }
#endif
}

/**
 * @brief SenseHat::GetSphericalMagnetism
 * @return la valeur du vecteur champ magnétique en coordonnées sphérique
 */
void SenseHat::GetSphericalMagnetism(float &ro, float &teta, float &delta)
{
    float x,y,z;
#if SENSEHAT_EMULATOR
	ro = teta = delta = 1.0;
#else
    GetMagnetism(x,y,z);
    teta = atan2 (y,x) * 180/PI;
    ro   = sqrt(x*x + y*y + z*z);
    delta =  atan2 (z,sqrt(x*x + y*y)) * 180/PI;
#endif
}

#if SENSEHAT_EMULATOR
#else
/**
 * @brief  SenseHat::InitializeLeds
 */
void SenseHat::InitializeLeds(void)
{
    int fbfd ;
    int tries;
    int tries2;

    tries = 0;
    tries2 = 0;
    while(true)
    {
      fbfd = open_fbdev("RPi-Sense FB");
      if (fbfd > 0)
      {
          while (true)
          {
            fb = (struct fb_t*)mmap(0, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
            if (!fb)
            {
                tries2++;
                usleep(100);
                if (tries2 > NUMBER_OF_TRIES_BEFORE_FAILURE)
                {
                  printf("Failed to mmap.\n");
                  exit(EXIT_FAILURE);
                }
            }
            else
            {

              memset(fb, 0, 128);
              return;
            }

          }
      }
      else
      {
        close(fbfd);
        tries++;
        usleep(100);
        if (tries > NUMBER_OF_TRIES_BEFORE_FAILURE)
        {
          printf("Error: cannot open framebuffer device.\n");
          exit (EXIT_FAILURE);
        }
      }

    }

}

/**
 * @brief  SenseHat::InitializeJoystick
 */
void SenseHat::InitializeJoystick(void)
{
	joystick = open_evdev("Raspberry Pi Sense HAT Joystick");
}

/**
 * @brief  SenseHat::InitialiserPression
 */
void SenseHat::InitializePressure(void)
{
	int tries;

	tries = 0;
	while(true)
	{
		pressure = RTPressure::createPressure(settings);
		if (pressure == NULL)
		{
			tries++;
			usleep(100);
		}
		else
		{
			break;
		}
		if (tries > NUMBER_OF_TRIES_BEFORE_FAILURE)
		{
			fprintf(stdout,"Pas de mesure de pression/température \n");
			exit (EXIT_FAILURE);
		}
	}
	pressure->pressureInit();
}

/**
 * @brief  SenseHat::Initialiserhumidity
 * @detail initialise le capteur d'humidité
 */
void SenseHat::InitializeHumidity(void)
{
	int tries;

	tries = 0;
	while(true)
	{
		humidity = RTHumidity::createHumidity(settings);
		if(humidity == NULL)
		{
			tries++;
			usleep(100);
		}
		else
		{
			break;
		}
		if (tries > NUMBER_OF_TRIES_BEFORE_FAILURE)
		{
			fprintf(stdout,"Pas de mesure de pression/température \n");
			exit (EXIT_FAILURE);
		}
	}
	humidity->humidityInit();
}

void SenseHat::InitializeOrientation(void)
{

}

void SenseHat::InitializeAcceleration(void)
{
	imu->setAccelEnable(true);
}
#endif

/**
 * @brief  SenseHat::ConvertCharactereToPattern
 * - Fait par Grilo Christophe
 */
void SenseHat::ConvertCharacterToPattern(char c, uint16_t image[8][8], uint16_t colorText, uint16_t colorBackground)
{
    int i=0;
    int j,k;
    int tailleTableDeConvertion=sizeof(font)/sizeof(Tfont);

    // Recherche si le caractere existe dans la table de convertion (cf font.h)
    while(c!=font[i].caractere && i < tailleTableDeConvertion )
	i++;

    // Si le caractere est dans la table on le converti
    if(i < tailleTableDeConvertion)
    {
        for (j=0;j<8;j++)
        {
            for(k=0;k<8;k++)
            {
                if(font[i].binarypattern[j][k]) { image[j][k]=colorText; }
                else { image[j][k]=colorBackground; }
            }
        }
    }
    else // caractère inexistant on le remplace par un glyphe inconnu
//	ConvertirCaractereEnpattern(255,image,colorText,colorBackground);
    {
        ConvertCharacterToPattern(255,image,colorText,colorBackground);
    }
}

bool SenseHat::EmptyColumn(int numcolumn,uint16_t image[8][8],uint16_t colorBackground)
{
    int i=0;

    for(i=0;i<8;i++)
    {
        if(image[i][numcolumn]!=colorBackground) { return false; }
    }
    return true;
}

void SenseHat::ImageContainment(int numcolumn,uint16_t image[][8][8], int taille)
{
    int i=0,j=0,k=0,l=0,isuivant,ksuivant;
    int nombredecolumns=taille*8; //8 columns par pattern

    for(l=numcolumn;l<nombredecolumns-1;l++)
    {
        i=l/8;
        k=l%8;
        isuivant=(l+1)/8;
        ksuivant=(l+1)%8;
        for(j=0;j<8;j++)
        {
            image[i][j][k]=image[isuivant][j][ksuivant];
        }
    }
}

void SenseHat::ViewMessage(const std::string message, int vitesseDefilement, uint16_t colorText, uint16_t colorBackground)
{
    int taille=message.length();
    uint16_t chaine[taille][8][8]; /* Le tableau de pattern (image/caractère) à afficher */
    int i=0,j=0,k=0,l=0,nombreDecolumnVide=0;
    int isuivant=0,ksuivant=0,nombreDecolumns=0;

    /* Convertion de tout le message en tableau de patterns
     * format caractère : 1 column vide + 5 columns réellement utilisées
     * + 2 columns vides */
    for( i=0,j=0; i<taille; i++,j++)
    {
        if(message[i]==195)  // les lettres accentuées sont codées sur deux octets  (195 167 pour ç)
        {
            i++;
            k++;
        }
  	// row suivante à décommenter pour obtenir le code des caractères UTF8
	// std::cout << "code : " << (int)message[i] << std::endl;
		ConvertCharacterToPattern(message[i],chaine[j],colorText,colorBackground);
    }
	taille = taille - k;
	nombreDecolumns=(taille)*8-2;
	k = 0;
	// Parcours de toutes les columns de tous les patterns qui compose
	// la chaine de caractères à afficher pour supprimer les columns vides sauf celle
	// qui sépare les patterns (caractères). + gestion du caractère espace.
	for( l=0;l<nombreDecolumns;l++)
	{
		i=l/8;
		k=l%8;

//		if(columnVide(k,chaine[i],colorBackground)) // Une column Vide avant chaque caractère à ne pas supprimer
		if(EmptyColumn(k,chaine[i],colorBackground)) // Une column Vide avant chaque caractère à ne pas supprimer
		{
			isuivant=(++l)/8;
			ksuivant=(l)%8;
			nombreDecolumnVide=1;

			// compter les columns vide après la première afin de les supprimer
			// si plus de 4 c'est le caractère espace que l'on doit garder
//			while(columnVide(ksuivant,chaine[isuivant],colorBackground) && nombreDecolumnVide++ < 6)
			while(EmptyColumn(ksuivant,chaine[isuivant],colorBackground) && nombreDecolumnVide++ < 6)
			{
				ImageContainment(l,chaine,taille);
				nombreDecolumns--;
			}

		}
	}

	// Parcours de toutes les columns de tous les patterns qui composent
	// la chaine de caractères à afficher (sans les columns vides superflues).
	for( l=0; l<nombreDecolumns; l++)
	{
		// Decalage des columns vers la gauche sur l'image Numero 0 (celle qu'on affiche sur la matrice de LED
		for( i=0;i<taille;i++)
		{
			// Cas Normal, les columns sont sur le même pattern
			for(j=0;j<8;j++)
			{
				for(k=0;k<7;k++)
					chaine[i][j][k]=chaine[i][j][k+1];

			}
			// Cas où l'on doit changer de pattern
			for(j=0;j<8;j++)
			{
				chaine[i][j][7]=chaine[i+1][j][0];
            }
		}
		usleep(1000*vitesseDefilement);
		ViewPattern(chaine[0]);
	}

}

SenseHat& SenseHat::operator<<(const std::string &message)
{
	buffer += message;
	return *this;
}

SenseHat& SenseHat::operator<<(const int valeur)
{
	buffer += std::to_string(valeur);
	return *this;
}

SenseHat& SenseHat::operator<<(const double valeur)
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << valeur;
	buffer += ss.str();
	return *this;
}

SenseHat& SenseHat::operator<<(const char caractere)
{
	buffer += std::string(1, caractere);
	return *this;
}

SenseHat& SenseHat::operator<<(const char * message)
{
	buffer += std::string(message);
	return *this;
}

SenseHat& SenseHat::operator<<(const bool valeur)
{
	buffer +=  std::to_string(valeur);
	return *this;
}
// Méthode Flush() Affiche le buffer puis le vide
void SenseHat::Flush(void)
{
	buffer += "  ";
	ViewMessage(buffer, 80, color);
	buffer = " ";
}

// Modificator endl
// (endl manipulator) effectue un flush du buffer
SenseHat& endl(SenseHat& os)
{
   os.Flush();
   return os;
}

SenseHat& flush(SenseHat& os)
{
	os.Flush();
	return os;
}

_Setcolor setcolor(int n)
{
	return { n };
}

SenseHat&  operator<<(SenseHat& os, _Setcolor color)
{
	os.SetColor(color.val);
	return os;
}

_SetRotation setrotation(int n)
{
	return { n };
}

SenseHat&  operator<<(SenseHat& os, _SetRotation rotation)
{
	os.SetRotation(rotation.val);
	return os;
}
