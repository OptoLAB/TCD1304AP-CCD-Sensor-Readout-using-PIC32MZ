/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    usbcdc.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "usbcdc.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
uint8_t CACHE_ALIGN cdcReadBuffer[USBCDC_READ_BUFFER_SIZE];
uint8_t CACHE_ALIGN cdcWriteBuffer[USBCDC_READ_BUFFER_SIZE];
uint8_t setupData[SETUP_DATA_SIZE];

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the USBCDC_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

USBCDC_DATA usbcdcData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************
 * USB CDC Device Events - Application Event Handler
 *******************************************************/

USB_DEVICE_CDC_EVENT_RESPONSE USBCDC_USBDeviceCDCEventHandler
(
    USB_DEVICE_CDC_INDEX index,
    USB_DEVICE_CDC_EVENT event,
    void * pData,
    uintptr_t userData
)
{
    USBCDC_DATA * usbcdcDataObject;
    USB_CDC_CONTROL_LINE_STATE * controlLineStateData;
    USB_DEVICE_CDC_EVENT_DATA_READ_COMPLETE * eventDataRead;
    
    usbcdcDataObject = (USBCDC_DATA *)userData;

    switch(event)
    {
        case USB_DEVICE_CDC_EVENT_GET_LINE_CODING:

            /* This means the host wants to know the current line
             * coding. This is a control transfer request. Use the
             * USB_DEVICE_ControlSend() function to send the data to
             * host.  */

            USB_DEVICE_ControlSend(usbcdcDataObject->deviceHandle,
                    &usbcdcDataObject->getLineCodingData, sizeof(USB_CDC_LINE_CODING));

            break;

        case USB_DEVICE_CDC_EVENT_SET_LINE_CODING:

            /* This means the host wants to set the line coding.
             * This is a control transfer request. Use the
             * USB_DEVICE_ControlReceive() function to receive the
             * data from the host */

            USB_DEVICE_ControlReceive(usbcdcDataObject->deviceHandle,
                    &usbcdcDataObject->setLineCodingData, sizeof(USB_CDC_LINE_CODING));

            break;

        case USB_DEVICE_CDC_EVENT_SET_CONTROL_LINE_STATE:

            /* This means the host is setting the control line state.
             * Read the control line state. We will accept this request
             * for now. */

            controlLineStateData = (USB_CDC_CONTROL_LINE_STATE *)pData;
            usbcdcDataObject->controlLineStateData.dtr = controlLineStateData->dtr;
            usbcdcDataObject->controlLineStateData.carrier = controlLineStateData->carrier;

            USB_DEVICE_ControlStatus(usbcdcDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);

            break;

        case USB_DEVICE_CDC_EVENT_SEND_BREAK:

            /* This means that the host is requesting that a break of the
             * specified duration be sent. Read the break duration */

            usbcdcDataObject->breakData = ((USB_DEVICE_CDC_EVENT_DATA_SEND_BREAK *)pData)->breakDuration;
            
            /* Complete the control transfer by sending a ZLP  */
            USB_DEVICE_ControlStatus(usbcdcDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);
            
            break;

        case USB_DEVICE_CDC_EVENT_READ_COMPLETE:

            /* This means that the host has sent some data*/
            eventDataRead = (USB_DEVICE_CDC_EVENT_DATA_READ_COMPLETE *)pData;
            
            if(eventDataRead->status != USB_DEVICE_CDC_RESULT_ERROR)
            {
                usbcdcDataObject->isReadComplete = true;
                
                usbcdcDataObject->numBytesRead = eventDataRead->length; 
            }
            break;

        case USB_DEVICE_CDC_EVENT_CONTROL_TRANSFER_DATA_RECEIVED:

            /* The data stage of the last control transfer is
             * complete. For now we accept all the data */

            USB_DEVICE_ControlStatus(usbcdcDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);
            break;

        case USB_DEVICE_CDC_EVENT_CONTROL_TRANSFER_DATA_SENT:

            /* This means the GET LINE CODING function data is valid. We don't
             * do much with this data in this demo. */
            break;

        case USB_DEVICE_CDC_EVENT_WRITE_COMPLETE:

            /* This means that the data write got completed. We can schedule
             * the next read. */

            usbcdcDataObject->isWriteComplete = true;
            break;

        default:
            break;
    }

    return USB_DEVICE_CDC_EVENT_RESPONSE_NONE;
}

/***********************************************
 * Application USB Device Layer Event Handler.
 ***********************************************/
void USBCDC_USBDeviceEventHandler 
(
    USB_DEVICE_EVENT event, 
    void * eventData, 
    uintptr_t context 
)
{
    USB_DEVICE_EVENT_DATA_CONFIGURED *configuredEventData;

    switch(event)
    {
        case USB_DEVICE_EVENT_RESET:

            /* Update LED to show reset state */
            STAT_LED_Clear();

            usbcdcData.isConfigured = false;

            break;

        case USB_DEVICE_EVENT_CONFIGURED:

            /* Check the configuration. We only support configuration 1 */
            configuredEventData = (USB_DEVICE_EVENT_DATA_CONFIGURED*)eventData;
            
            if ( configuredEventData->configurationValue == 1)
            {
                /* Update LED to show configured state */
                STAT_LED_Set();
                
                /* Register the CDC Device application event handler here.
                 * Note how the usbcdcData object pointer is passed as the
                 * user data */

                USB_DEVICE_CDC_EventHandlerSet(USB_DEVICE_CDC_INDEX_0, USBCDC_USBDeviceCDCEventHandler, (uintptr_t)&usbcdcData);

                /* Mark that the device is now configured */
                usbcdcData.isConfigured = true;
            }
            
            break;

        case USB_DEVICE_EVENT_POWER_DETECTED:

            /* VBUS was detected. We can attach the device */
            USB_DEVICE_Attach(usbcdcData.deviceHandle);
            
            break;

        case USB_DEVICE_EVENT_POWER_REMOVED:
            
            /* VBUS is not available. We can detach the device */
            USB_DEVICE_Detach(usbcdcData.deviceHandle);
            
            usbcdcData.isConfigured = false;
            
            STAT_LED_Clear();
            
            break;

        case USB_DEVICE_EVENT_SUSPENDED:

            /* Switch LED to show suspended state */
            STAT_LED_Clear();
            
            break;

        case USB_DEVICE_EVENT_RESUMED:
        case USB_DEVICE_EVENT_ERROR:
        default:
            
            break;
    }
}
// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/*****************************************************
 * This function is called in every step of the
 * application state machine.
 *****************************************************/

bool USBCDC_StateReset(void)
{
    /* This function returns true if the device
     * was reset  */

    bool retVal;

    if(usbcdcData.isConfigured == false)
    {
        usbcdcData.state = USBCDC_STATE_WAIT_FOR_CONFIGURATION;
        usbcdcData.readTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
        usbcdcData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
        usbcdcData.isReadComplete = true;
        usbcdcData.isWriteComplete = true;
        retVal = true;
    }
    else
    {
        retVal = false;
    }

    return(retVal);
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void USBCDC_Initialize ( void )

  Remarks:
    See prototype in usbcdc.h.
 */

void USBCDC_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    usbcdcData.state = USBCDC_STATE_INIT;
    
    /* Device Layer Handle  */
    usbcdcData.deviceHandle = USB_DEVICE_HANDLE_INVALID ;

    /* Device configured status */
    usbcdcData.isConfigured = false;

    /* Initial get line coding state */
    usbcdcData.getLineCodingData.dwDTERate = 256000;
    usbcdcData.getLineCodingData.bParityType = 0;
    usbcdcData.getLineCodingData.bCharFormat= 0; 
    usbcdcData.getLineCodingData.bDataBits = 8;

    /* Read Transfer Handle */
    usbcdcData.readTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

    /* Write Transfer Handle */
    usbcdcData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

    /* Initialize the read complete flag */
    usbcdcData.isReadComplete = true;

    /*Initialize the write complete flag*/
    usbcdcData.isWriteComplete = true;
    
    /* Set up the read buffer */
    usbcdcData.cdcReadBuffer = &cdcReadBuffer[0];

    /* Set up the read buffer */
    usbcdcData.cdcWriteBuffer = &cdcWriteBuffer[0];    
    
    /* Initialize number of bytes to send to Host */ 
    usbcdcData.numBytesToWrite = 0;
    
    /* Initialize the read request flag */
    usbcdcData.readRequest = false; 
    
    /* Initialize the setup request flag */ 
    usbcdcData.setupRequest = false;     
    
    /* Initialize the data ready  flag */ 
    usbcdcData.dataReady = false; 
    
    /* Initialize setup data */ 
    usbcdcData.setupData = &setupData[0]; 
}
/******************************************************************************/
void USBCDC_GetSetupData(uint8_t *data)
{
    for(uint8_t i=0;i<SETUP_DATA_SIZE;i++)  //transfer received setup data 
        data[i]=usbcdcData.setupData[i];
    usbcdcData.setupRequest=0;              //setup request is processed, clear flag
    usbcdcData.dataReady=0;                 //invalidate data in cdcWriteBuffer
}
/******************************************************************************/
uint8_t USBCDC_SetupRequest(void)
{
    return usbcdcData.setupRequest;
}
/******************************************************************************/
uint8_t USBCDC_ReadRequest(void)
{
    return usbcdcData.readRequest;
}
/******************************************************************************/
void USBCDC_TrasferData(uint16_t *data, uint16_t len, uint8_t h_res, uint16_t v_res)
{
    switch(v_res)
    {
        case 0:
            usbcdcData.numBytesToWrite=len>>h_res;
            for(int i=0;i<usbcdcData.numBytesToWrite;i++)
            usbcdcData.cdcWriteBuffer[i]=(uint8_t)(data[i<<h_res]>>6);
            break;
        case 1:
            usbcdcData.numBytesToWrite=len>>h_res;
            for(int i=0;i<usbcdcData.numBytesToWrite;i++)
            usbcdcData.cdcWriteBuffer[i]=(uint8_t)(data[i<<h_res]>>4);
            break;
        case 2:
            usbcdcData.numBytesToWrite=(len>>h_res)<<1;
            for(int i=0;i<(usbcdcData.numBytesToWrite>>1);i++)
            {
                usbcdcData.cdcWriteBuffer[(i<<1)]=(uint8_t)(data[i<<h_res]>>10);
                usbcdcData.cdcWriteBuffer[(i<<1)+1]=(uint8_t)(data[i<<h_res]>>2);
            }
            break;
        case 3:
            usbcdcData.numBytesToWrite=(len>>h_res)<<1;
            for(int i=0;i<(usbcdcData.numBytesToWrite>>1);i++)
            {
                usbcdcData.cdcWriteBuffer[(i<<1)]=(uint8_t)(data[i<<h_res]>>8);
                usbcdcData.cdcWriteBuffer[(i<<1)+1]=(uint8_t)(data[i<<h_res]);
            }
            break;         
    }
    usbcdcData.dataReady=1;
}

/******************************************************************************
  Function:
    void USBCDC_Tasks ( void )

  Remarks:
    See prototype in usbcdc.h.
 */
void USBCDC_Tasks ( void )
{
    /* Check the application's current state. */
    switch ( usbcdcData.state )
    {
        /* Application's initial state. */
        case USBCDC_STATE_INIT:
        {
            /* Open the device layer */
            usbcdcData.deviceHandle = USB_DEVICE_Open( USB_DEVICE_INDEX_0, DRV_IO_INTENT_READWRITE );

            if(usbcdcData.deviceHandle != USB_DEVICE_HANDLE_INVALID)
            {
                /* Register a callback with device layer to get event notification (for end point 0) */
                USB_DEVICE_EventHandlerSet(usbcdcData.deviceHandle, USBCDC_USBDeviceEventHandler, 0);

                usbcdcData.state = USBCDC_STATE_WAIT_FOR_CONFIGURATION;
            }
            break;
        }
        case USBCDC_STATE_WAIT_FOR_CONFIGURATION:

            /* Check if the device was configured */
            if(usbcdcData.isConfigured)
            {
                /* If the device is configured then lets start reading */
                usbcdcData.state = USBCDC_STATE_SCHEDULE_READ;
            }
            
            break;
        case USBCDC_STATE_SCHEDULE_READ:

            if(USBCDC_StateReset())
            {
                break;
            }

            /* If a read is complete, then schedule a read
             * else wait for the current read to complete */

            usbcdcData.state = USBCDC_STATE_WAIT_FOR_READ_COMPLETE;
            if(usbcdcData.isReadComplete == true)
            {
                usbcdcData.isReadComplete = false;
                usbcdcData.readTransferHandle =  USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

                USB_DEVICE_CDC_Read (USB_DEVICE_CDC_INDEX_0,
                        &usbcdcData.readTransferHandle, usbcdcData.cdcReadBuffer,
                        USBCDC_READ_BUFFER_SIZE);
                
                if(usbcdcData.readTransferHandle == USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID)
                {
                    usbcdcData.state = USBCDC_STATE_ERROR;
                    break;
                }
            }

            break;
        case USBCDC_STATE_WAIT_FOR_READ_COMPLETE:
       // case APP_STATE_CHECK_SWITCH_PRESSED:

            if(USBCDC_StateReset())
            {
                break;
            }

            /* Check if a character was received or a switch was pressed.
             * The isReadComplete flag gets updated in the CDC event handler. */

            if(usbcdcData.isReadComplete)
            {
                usbcdcData.state = USBCDC_STATE_SCHEDULE_WRITE;
            }

            break;

        case USBCDC_STATE_SCHEDULE_WRITE:

            if(USBCDC_StateReset())
            {
                break;
            }

            /* GET -> read command */
            if(usbcdcData.cdcReadBuffer[0]=='G'&&usbcdcData.cdcReadBuffer[1]=='E'&&usbcdcData.cdcReadBuffer[2]=='T')
            {
                usbcdcData.readRequest=1;       //initiate CCD data transfer to cdcWriteBuffer 
                if(usbcdcData.dataReady)        //wait until CCD data transfer is finished
                {
                    usbcdcData.dataReady=0;
                    usbcdcData.cdcReadBuffer[0]=0;

                    usbcdcData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
                    usbcdcData.isWriteComplete = false;
                    usbcdcData.state = USBCDC_STATE_WAIT_FOR_WRITE_COMPLETE;

                    USB_DEVICE_CDC_Write(USB_DEVICE_CDC_INDEX_0,
                    &usbcdcData.writeTransferHandle,
                    usbcdcData.cdcWriteBuffer, usbcdcData.numBytesToWrite,
                    USB_DEVICE_CDC_TRANSFER_FLAGS_DATA_COMPLETE);
                }
            }
            /* SET -> setup command */
            else if(usbcdcData.cdcReadBuffer[0]=='S'&&usbcdcData.cdcReadBuffer[1]=='E'&&usbcdcData.cdcReadBuffer[2]=='T')
            {

                usbcdcData.setupRequest=1;

                usbcdcData.cdcReadBuffer[0]=0;

                usbcdcData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
                usbcdcData.isWriteComplete = false;
                usbcdcData.state = USBCDC_STATE_WAIT_FOR_WRITE_COMPLETE;

                for(uint8_t i=0;i<SETUP_DATA_SIZE;i++) //extract setup data from cdcReadBuffer
                {
                    usbcdcData.setupData[i]=usbcdcData.cdcReadBuffer[i+3];
                    usbcdcData.cdcWriteBuffer[i]=usbcdcData.setupData[i];
                }

                USB_DEVICE_CDC_Write(USB_DEVICE_CDC_INDEX_0,
                &usbcdcData.writeTransferHandle,
                usbcdcData.cdcWriteBuffer, 4,
                USB_DEVICE_CDC_TRANSFER_FLAGS_DATA_COMPLETE);

            }
            else usbcdcData.state = USBCDC_STATE_SCHEDULE_READ;

            break;

        case USBCDC_STATE_WAIT_FOR_WRITE_COMPLETE:

            if(USBCDC_StateReset())
            {
                break;
            }

            /* Check if a character was sent. The isWriteComplete
             * flag gets updated in the CDC event handler */

            if(usbcdcData.isWriteComplete == true)
            {
                usbcdcData.readRequest=0; // clear read request flag
                usbcdcData.state = USBCDC_STATE_SCHEDULE_READ;
            }

            break;

        case USBCDC_STATE_ERROR:
        default:
            
            break;
    }
}


/*******************************************************************************
 End of File
 */
