/*************************************************************************
* REMOTE BOARD
*
* The remote control system which is used to control the on/off buttons,
* the temperature threshold using the joystick and the temperature setting
* using the potentiometer. The data is sent across the CAN and adjusted on
* the local board.
*
*Lloyd Wilson
*Andrew Perkins
**************************************************************************/
#include "includes.h"
#include "can.h"
#include <math.h>
#include <stdbool.h>

/************************************
* constants, prototypes, globals
***********************************/
const int WINWDTH = 131, WINHGT = 131, THRESH = 5, WIDGETFS = 170, DSPOFFSET=14;

void canHandler (void);
canMessage_t can1RxBuf, can2RxBuf, canTxBuf;
bool port1 = false, port2 = false;
const int ENABLED=1, THRESHSET=2, TEMP=3, DISABLED=4;
int txCt = 0, rxCt = 0;

bool heating = false;
bool enabled = false;
bool disabled = true;
int tempSetting = 0, temp = 0, tempThresh;
char tmDsp[10];   //string for displaying time
extern int runningTm;   //elapsed seconds, incremented by timer interrupt handler

/***************************************
* Delay [100us] approximately!
* (Must execute InitClock();)
**************************************/
void Dly100us (void *arg)
{
  volatile Int32U Dly = (Int32U)arg, Dly100;
  for (;Dly;Dly--)
    for (Dly100 = 250; Dly100; Dly100--);
}

/*
* Generates a string that can be used to display time effectively
*/
void formatTime (int tm)
{
  int mins = tm / 60, secs = tm % 60;
  if (secs < 10)
    sprintf(tmDsp, "%d:0%d ", mins, secs);
  else
    sprintf(tmDsp, "%d:%d ", mins, secs);
}

void refreshSettingsAndDisplay (void)
{
  tempSetting = (1024 - potentiometerRead ()) / 10;
  formatTime (runningTm);
  LCDTextSetPos (1,1);
  printf ("T=%d  S=%d(%d)  ", temp, tempSetting, tempThresh);
  LCDTextSetPos(1,2);
  printf(tmDsp);
}

/*
* Adjusts threshold
*/
void adjThresh (int adj)
{
  int newThresh = tempThresh + adj;

  if (newThresh > THRESH*2)
    tempThresh =  THRESH*2;
  else
    if (newThresh <= 1)
      tempThresh =  1;
    else
      tempThresh = newThresh;
}

/*
* Display a vertical bar (1 pixel wide) at a height
* proportional to (value/range)
*/
void graphicDisplay (LdcPixel_t bgColour, LdcPixel_t fgColour, int range, int value)
{
  const int OFFS = 10;
  int y = runningTm % WINWDTH;
  int x = (int)(OFFS+(1-(float)value/range)*(WINHGT-OFFS));
  LCDDrawLine(0, y, x, y, bgColour);
  LCDDrawLine(x, y, WINHGT, y, fgColour);
  LCDDrawLine(OFFS, y+1, WINHGT, y+1, bgColour);
}

/*
* Function to send message over CAN
*/
bool send (int mtype, int paramDataA, int paramDataB)
{
  bool written;
  canTxBuf.id = mtype;                                      //message type
  canTxBuf.dataA = paramDataA;                              //message content
  canTxBuf.dataB = paramDataB;                              //message content
  canTxBuf.len = sizeof (paramDataA) + sizeof (paramDataB); //message length
  written = canWrite (2, &canTxBuf);

  // Records successful attempts
  if (written)
    txCt++;

  return written;
}

/*
* Function used when message is received through CAN
*/
void canHandler (void)
{
  if (canReady (1))
  {
    canRead (1, &can1RxBuf);
    port1 = true;
  }

  if (canReady (2))
  {
    canRead (2, &can2RxBuf);
    port2 = true;
  }

  VICADDRESS = 0; // clear interrupt ///////////
}

/******************* MAIN ********************/
int main (void)
{
  // Initialise the Clock and the I/O components
  InitClock();
  GPIOInit();
  LEDInit();
  canInit ();
  buttonsInit();
  potentiometerInit();

  //LCD initialisation
  LCDPowerUpInit ((pInt8U)0);
  LCDSetBacklight (BACKLIGHT_ON);
  Dly100us ((void*)5000);
  LCDSetFont ( &Terminal_6_8_6, BLACK, WHITE );
  LCDSetWindow ( 0, 0, WINWDTH, WINHGT );
  printf( "\f" );   // Clear screen

  LEDState(USB_LINK_LED, OFF);
  LEDState(USB_CONNECT_LED, OFF);

  InitTimer0Interrupt();

  tempThresh = THRESH;
  int localTempThresh = tempThresh;
  int localTempSetting = tempSetting;

  //Sets up function used when interrupt is raised by message received
  canRxInterrupt(canHandler);

  while (true)
  {
    /*****************************Display**********************************/
    refreshSettingsAndDisplay ();
    graphicDisplay (WHITE, RED, WIDGETFS, temp);


    /**********************CAN Messages Received***************************/
    //Checks if message has been received in port 2
    if (port2)
    {
      //Checks to see if message contains an new temperature value
      if (can2RxBuf.id == TEMP)
        temp = can2RxBuf.dataA;

      //Indicates message has been dealt with
      port2 = false;
    }


    /***************************Data Handling*******************************/
    if (buttonRead (JS_UP))
      adjThresh (1); // Increases the temperature threshold

    if (buttonRead (JS_DOWN))
      adjThresh (-1); // Decreases the temperature threshold

    if (runningTm >= 3600)
      runningTm = 0;


    /******************Communication with Local Board***********************/
    // Checks to see if button 1 has been pressed and starts/continues the heater/fan
    if (buttonRead (BUT_1) && enabled == false)
    {
      send (ENABLED, 0, 0);
      enabled = true;
      disabled = false;
      while (buttonRead (BUT_1));
    }
    // Checks to see if button 2 has been pressed and pauses the heater/fan
    if (buttonRead (BUT_2) && enabled == true)
    {
      send (DISABLED, 0, 0);
      enabled = false;
      disabled = true;
      while (buttonRead (BUT_2));
    }

    // Updates set point and threshold
    if (tempSetting != localTempSetting || tempThresh != localTempThresh)
    {
      send (THRESHSET, tempSetting, tempThresh);
      localTempSetting = tempSetting;
      localTempThresh = tempThresh;
    }

    Dly100us ((void*) 1);
  }
}
