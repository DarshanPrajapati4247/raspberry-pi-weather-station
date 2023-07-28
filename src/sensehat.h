/**
* @file SenseHat.h
* @date 4 mars 2018
* @version 1.2
* @authors Philippe SIMIER Philippe CRUCHET Christophe GRILLO
* @details Classe SenseHat : Gestion de la carte SenseHat
* @version 1.3
* @date 30 July 2019
* @authors Jon Dellaria bug fixes, method translation to English and Temperature Measurement adjustments required for the Raspberry for a true temperature reading.
* @version 1.3a
* @date 01 October 2020
* @authors Paul Moggach removed french to compact files and make it more readable. Made changes to conform with Allman style.
*/
#ifndef SENSEHAT_H
#define SENSEHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <linux/input.h>
#include <sstream>
#include <math.h>
#include <RTIMULib.h>
#include <iostream>
#include <iomanip>

// Constants
#define SENSEHAT_EMULATOR 0
#if SENSEHAT_EMULATOR
#include <python2.7/Python.h>
#endif
#define DEV_FB "/dev"
#define FB_DEV_NAME "fb"
#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

#define COLOR_SENSEHAT uint16_t
#define PI 3.14159265

#define RED 	0xF800
#define BLUE    0x001F
#define GREEN   0x07E0
#define WHITE   0xFFFF
#define BLACK   0x0000
#define ORANGE  0xFC00
#define CYAN    0x87FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0

// Structures
struct fb_t
{
	uint16_t pixel[8][8];
};


// Classes
class SenseHat
{
public:
    SenseHat(void);
    ~SenseHat(void);

    SenseHat& operator<<(SenseHat& (*)(SenseHat&));
    SenseHat& operator<<(const std::string&);
    SenseHat& operator<<(const int);
    SenseHat& operator<<(const double);
    SenseHat& operator<<(const char);
    SenseHat& operator<<(const char *);
    SenseHat& operator<<(const bool);


	void ViewMessage(const std::string message, int vitesseDefilement = 100, uint16_t colorText = BLUE, uint16_t colorBackground = BLACK);
	void ViewLetter(char lettre, uint16_t colorText = BLUE, uint16_t colorBackground = BLACK);
	void LightPixel(int row, int column, uint16_t color);
	uint16_t GetPixel(int row, int column);
	void ViewPattern(uint16_t pattern[][8]);
	void RotatePattern(int rotation);
    char ScannerJoystick(void);
    char ScanJoystick(void);
    COLOR_SENSEHAT ConvertRGB565(uint8_t red, uint8_t green,uint8_t blue);
	COLOR_SENSEHAT ConvertRGB565(uint8_t color[]);
	COLOR_SENSEHAT ConvertRGB565(std::string color);
	void WipeScreen(uint16_t color=BLACK);
	float GetTemperature(void);
	float correctTemperature(float senseHatTemp, float cpuTemp);
	float getRawTemperature(void);
	float getCpuTemperature(void);
	float GetPressure(void);
	float GetHumidity(void);
	void  GetOrientation(float &pitch, float &roll, float & yaw);
	void  GetAcceleration(float &x, float &y, float &z);
	void  GetMagnetism(float &x, float &y, float &z);
	void  GetSphericalMagnetism(float &ro, float &teta, float &delta);
    void  Version(void);
    void  Flush(void);
	void  SetColor(uint16_t);
	void  SetRotation(uint16_t);

private:
#if SENSEHAT_EMULATOR
#else
	void  InitializeLeds(void);
	void  InitializeJoystick(void);
	void  InitializePressure(void);
	void  InitializeHumidity(void);
	void  InitializeOrientation(void);
	void  InitializeAcceleration(void);
#endif
	void ConvertCharacterToPattern(char c, uint16_t image[8][8], uint16_t colorText, uint16_t colorBackground);
	bool EmptyColumn(int numcolumn, uint16_t image[8][8], uint16_t colorBackground);
	void ImageContainment(int numcolumn, uint16_t image[][8][8], int taille);

    struct fb_t *fb;
    int joystick;
#if SENSEHAT_EMULATOR
#else
    RTIMUSettings *settings;
    RTIMU *imu;
    RTPressure *pressure;
    RTHumidity *humidity;
#endif
    std::string buffer;
    uint16_t color;
    int rotation;
};

// surcharge des manipulators
SenseHat& endl(SenseHat& os);
SenseHat& flush(SenseHat& os);

// surcharge du manipulateur setcolor
struct _Setcolor { int val; };
_Setcolor setcolor(int n);
SenseHat&  operator<<(SenseHat& os, _Setcolor color);

// surcharge du manipulateur setrotation
struct _SetRotation { int val; };
_SetRotation setrotation(int n);
SenseHat&  operator<<(SenseHat& os, _SetRotation rotation);
#endif // SENSEHAT_H
