/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

//Author: J. Bajic, 2022 (REV1)

//WIRRING (mini-32 for PIC32MZ board from Mikroelektronika)
//  CLK ->  RE3
//  SH  ->  RE5
//  ICG ->  RG8
//  OS  ->  RB0

//  Built in STAT LED ->    RE1     (used for USB status)
//  Built in DATA LED ->    RE0     (used as data transfer indicator)

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes

#define CCD_DATA_SIZE 3694      //total number of outputs [32(dummy)+3648(signal)+14(dummy)]
#define ICG_PERIOD_MIN 18470    //Min. time between two ICG pulses (3694*5us, 5us is data rate)

uint16_t ccd_data[CCD_DATA_SIZE]={};
uint8_t rx_data[20]={};
uint16_t data_cnt=0, integration_cnt=0, ICG_period_cnt=0; //counters
uint16_t ICG_period=1847;       //x10us, this parameter is adjusted based on integration time so that ICG pulse is aligned with SH pulse

typedef struct
{
    uint16_t    integrationTime;
    uint8_t     horzontalResolution;
    uint8_t     verticalResolution;
    uint16_t    *data;
}CCD_t;

CCD_t ccd;
/*********INTEGRATION TIME**********/
//  affects sensitivity
//  MIN=1       ->  10us    (DEFAULT)
//  MAX=65535   ->  655.35ms 

/*********HORIZONTAL RESOLUTION**********/
//  affects number of data points
//  0 -> CCD_DATA_SIZE      (DEFAULT)
//  1 -> CCD_DATA_SIZE/2
//  2 -> CCD_DATA_SIZE/4
//  3 -> CCD_DATA_SIZE/8
//  4 -> CCD_DATA_SIZE/16
//  5 -> CCD_DATA_SIZE/32 

/*********VERTICAL RESOLUTION**********/
//  affects A/D convertor resolution (number of bits)
//  0 -> 6 bits
//  1 -> 8 bits             (DEFAULT)
//  2 -> 10 bits
//  3 -> 12 bits
         
void CCD_Initialize(void)
{
    ccd.integrationTime=1;      //10us
    ccd.horzontalResolution=0;  //3648 points
    ccd.verticalResolution=1;   //8 bits
    ccd.data=&ccd_data[0];      //data array
}

//Timer 5 triggers this interrupt
void ADC_ResultHandler(ADCHS_CHANNEL_NUM channel, uintptr_t context)
{
    /* Read the ADC result */
    ccd.data[data_cnt]=ADCHS_ChannelResultGet(ADCHS_CH0); 
    if(data_cnt<(CCD_DATA_SIZE))data_cnt++;
}

//Timer 3 generates SH pulses on pin RE5 and ICG pulse on pin RG8
//ICG low state duration is 10us
//SH period determines integration time
void TIMER3_InterruptSvcRoutine(uint32_t status, uintptr_t context)
{
    integration_cnt++;
    if(!ICG_period_cnt&&!ICG_Get()) //Reset ICG and readout counter (data_cnt)
    {
        ICG_Set();
        data_cnt=0;
    }
    if(integration_cnt>=ccd.integrationTime)//Generate SH pulse
    {
        ICG_period_cnt++;
        integration_cnt=0;
        OCMP4_Enable();
        
        if(ICG_period_cnt>=ICG_period) //Generate ICG pulse
        {
            ICG_period_cnt=0;
            ICG_Clear();
        }
    }
}
// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    CCD_Initialize();
    
    ADCHS_CallbackRegister(ADCHS_CH0, ADC_ResultHandler, (uintptr_t)NULL);
    TMR3_CallbackRegister(TIMER3_InterruptSvcRoutine, (uintptr_t) NULL);
  
    CORETIMER_Start();
    
    //Using Timer 2 and Output Compare 5 CLK is generated on pin RE3 
    //OCMP5 generates continuous pulses
    OCMP5_Enable();//CLK (RE3) -------------------> f_CLK=0.8MHz (T_CLK=1.25us))
    TMR2_Start();   

    //Using Timer 3 and Output Compare 4 SH pulse is generated on pin RE5
    //OCMP4 generates single pulse and it is restarted in Timer 3 interrupt based on integration time
    //ICG pulse is generated in Timer 3 interrupt (align with SH) on pin RG8
    OCMP4_Enable();//SH (RE5) -------------------> min T_SH=10us
    TMR3_Start();  //10us
    
    //Output compare 1 (Timer 5) triggers A/D conversion on pin RB0
    //CCD output data rate is f_CLK/4 -> T_ADC=4*T_CLK=5us
    TMR5_Start();
    OCMP1_Enable(); 
    
    while ( true )
    {
        CORETIMER_DelayMs(1);

        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( ); // USBCDC tasks
        
        //if "GET" command is received
        if(USBCDC_ReadRequest())
        {
            DATA_LED_Toggle();
            USBCDC_TrasferData(ccd.data,CCD_DATA_SIZE,ccd.horzontalResolution,ccd.verticalResolution);
        }
        //if "SET" command is received
        if(USBCDC_SetupRequest())
        {
            USBCDC_GetSetupData(rx_data);
            uint16_t temp=0;
            temp=rx_data[0]<<8;
            temp+=rx_data[1];
            ccd.horzontalResolution=rx_data[2];
            ccd.verticalResolution=rx_data[3];

            //recalculate ICG period to match SH and update integration time
            ICG_period=ICG_PERIOD_MIN/(temp*10)+1;
            ccd.integrationTime=temp;
        
            ADC0TIME =(0x00010001)|(ccd.verticalResolution<<24); 
        }
    }
    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}




/*******************************************************************************
 End of File
*/

