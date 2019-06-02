/* *************************************************************
    
     File        : main.c
     Created by  : Reneas Project Generator
     Created date: June 27, 2003 : 18:27:50
	 Copyright   : 2003 Renesas Technology Europe Ltd.

   ************************************************************* */
           
//-----------------Include Common Header Files--------------------------
#include <machine.h>
#include "iodefine.h"
#include "edk3687def.h"
#include "drivers.h"

//-----------------Include Header Files for Stand Alone Operation-------
#ifdef Release
#include "sci.h"
struct SCI_Init_Params SCI_Init_Data={B57600,P_NONE,1,8};  
#endif  

//------------------ M A I N ---------------------------------------------
void main(void)						/* Initialise the serial port and display the menu */
{ 


#ifdef Release         			/* Menu output text on serial port when used without the monitor */
    InitSCI(SCI_Init_Data); 

	InitDrivers();

	ChessMain();

#endif
	
#ifdef HMON_Debug

	while(1);

#endif
}         
