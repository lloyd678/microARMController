/***************************************************************************
 **
 **    Master include file
 **
 **    Used with ARM IAR C/C++ Compiler and Assembler.
 **
 **    (c) Copyright IAR Systems 2006
 **
 **    $Revision: #5 $
 **
 ***************************************************************************/

#ifndef  __INCLUDES_H
#define  __INCLUDES_H

#include    <stdio.h>
//#include    <string.h>
#include    <ctype.h>
#include    <stdlib.h>
#include    <stdbool.h>
//#include    <assert.h>

#include    <intrinsics.h>
#include    <nxp/iolpc2378.h>

#include "arm_comm.h"
#include "board.h"

#include "GPIO.h"
#include "LEDs.h"
#include "buttons.h"
#include "ADC.h"
#include "DAC.h"
#include "potentiometer.h"
#include "accelerometer.h"
#include "appBoard.h"
#include "can.h"

#include    "drv_glcd_cnfg.h"
#include    "drv_glcd.h"
#include    "glcd_ll.h"

extern FontType_t Terminal_6_8_6;
extern FontType_t Terminal_9_12_6;
extern FontType_t Terminal_18_24_12;

#include    "iar_logo.h"

#include    "LowLevelInit.h"
#include    "Timer0.h"

#endif // __INCLUDES_H
