/*************************************************************************
* LOCAL BOARD
*
* The local control system within a networked heating system, recieves
* values inputted by a remote and uses them to heat or cool the system
* accordingly by using the flight board motor and heater
*
*Lloyd Wilson
*Andrew Perkins
**************************************************************************/
#include "includes.h"
#include <math.h>
#include <stdbool.h>

/************************************
* constants, prototypes, globals
***********************************/
const int WINWDTH = 131, WINHGT = 131, WIDGETFS = 170, DSPOFFSET=14;

//CAN MESSAGING CONSTANTS

void canHandler (void);
canMessage_t can1RxBuf, can2RxBuf, canTxBuf;
bool port1 = false, port2 = false;
const int ENABLED=1, THRESHSET=2, TEMP=3 , DISABLED=4;
int txCt = 0, rxCt = 0;


//TEMPERATURE SETTING CONSTANTS
int tempSetting = 0, temp = 0, tempThresh, initTemp=0;
char tmDsp[10];   //string for displaying time
extern int runningTm;   //elapsed seconds, incremented by timer interrupt handler
bool enabled = false;

/***************************************
* DELAY
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

/*
*Refresh the display in order to update the values on screen
*Also takes the temperature reading from the flight board
*/

void refreshSettingsAndDisplay (void)
{
  temp = appBoardReadADC ();
  formatTime (runningTm);
  LCDTextSetPos (1,1);
  printf ("T=%d  S=%d(%d)  ", temp, tempSetting, tempThresh);
  LCDTextSetPos(1,2);
  printf (tmDsp);
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
  canTxBuf.id = mtype;       //message type
  canTxBuf.dataA = paramDataA;
  canTxBuf.dataB = paramDataB;
  canTxBuf.len = sizeof(paramDataA) + sizeof(paramDataB); //message length
  written = canWrite (2, &canTxBuf);

  if (written)
  {

    txCt++;
  }
  return written;
}

/*
* Function used when message is received through CAN
*/
void canHandler (void)
{
  if (canReady(1))
  {
    canRead (1, &can1RxBuf);
    port1 = true;
  }

  if (canReady(2))
  {
    canRead (2, &can2RxBuf);
    port2 = true;
  }

  VICADDRESS = 0; // clear interrupt ///////////
}

/******************* main ********************/
int main (void)
{
  //Initialises devices at start up
  InitClock();
  GPIOInit();
  canInit();
  buttonsInit();
  potentiometerInit();
  appBoardInit();

  // LCD
  LCDPowerUpInit ((pInt8U)0);
  LCDSetBacklight (BACKLIGHT_ON);
  Dly100us ((void*)5000);
  LCDSetFont ( &Terminal_6_8_6, BLACK, WHITE );
  LCDSetWindow ( 0, 0, WINWDTH, WINHGT );
  printf( "\f" );   // Clear screen
  //Interrupt
  InitTimer0Interrupt();
 //Sets initial variable values

 //Sets up function used when interrupt is raised by message received
  canRxInterrupt(canHandler);

  while (true)
  {

    refreshSettingsAndDisplay ();
    graphicDisplay (WHITE, RED, WIDGETFS, temp);



 /**********************************************************
 *CAN Messages, this allows the two ARM boards to communicate
 ********************************************************/

/*
*Recieve from remote
*/

    //Checks if the remote has sent a new message
    if (port2)
    {
      //Starts or stops master control
      if (can2RxBuf.id == ENABLED)
        enabled = true ;


      if (can2RxBuf.id==DISABLED)
        enabled=false;


      //Updates set point and threshold from remote value
      if(can2RxBuf.id == THRESHSET)
      {
        tempSetting = can2RxBuf.dataA;
        tempThresh = can2RxBuf.dataB;
      }

      //If the temperature is lower than setting and the system is enabled
      // turn the heater on and fan off
      if ((temp < tempSetting-tempThresh)&&enabled )
      {
        appBoardHeaterState (ON);
        appBoardMotorState (OFF);
      }
       //If the temperature is higher than setting and the system is enabled
      // turn the fan on and heater off
      else if ((temp > tempSetting+tempThresh)&& enabled)
      {
        appBoardHeaterState (OFF);
        appBoardMotorState (ON);

      }
      //If the system is disabled turn fan and heater off
      else if (!enabled)
      {
        appBoardHeaterState (OFF);
        appBoardMotorState (OFF);
      }


/*
*Send to remote
*/

      //Sends temperature to remote board (for display)
      if (initTemp != temp)
      {
        initTemp = temp;
        send (TEMP, temp, 0);
      }



      Dly100us ((void*) 1);
    }
  }



}
