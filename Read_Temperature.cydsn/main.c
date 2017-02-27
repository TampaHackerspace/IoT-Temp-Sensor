/******************************************************************************
* Project Name      : CE211321_Temperature_Sensing
* Version           : 1.0
* Device Used       : CY8C4A45LQI-L483
* Software Used     : PSoC Creator 3.3 CP3
* Compiler Used     : ARM GCC 4.9.3 
* Related Hardware  : CY8CKIT-048 PSoC Analog Coprocessor Pioneer Kit 
*******************************************************************************
* Copyright (2016), Cypress Semiconductor Corporation.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software") is owned by Cypress Semiconductor Corporation (Cypress) and is
* protected by and subject to worldwide patent protection (United States and 
* foreign), United States copyright laws and international treaty provisions. 
* Cypress hereby grants to licensee a personal, non-exclusive, non-transferable
* license to copy, use, modify, create derivative works of, and compile the 
* Cypress source code and derivative works for the sole purpose of creating 
* custom software in support of licensee product, such licensee product to be
* used only in conjunction with Cypress's integrated circuit as specified in the
* applicable agreement. Any reproduction, modification, translation, compilation,
* or representation of this Software except as specified above is prohibited 
* without the express written permission of Cypress.
* 
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, 
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* Cypress reserves the right to make changes to the Software without notice. 
* Cypress does not assume any liability arising out of the application or use
* of Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use as critical components in any products 
* where a malfunction or failure may reasonably be expected to result in 
* significant injury or death ("ACTIVE Risk Product"). By including Cypress's 
* product in a ACTIVE Risk Product, the manufacturer of such system or application
* assumes all risk of such use and in doing so indemnifies Cypress against all
* liability. Use of this Software may be limited by and subject to the applicable
* Cypress software license agreement.
*******************************************************************************/
/*******************************************************************************
* Theory of Operation: This code example demonstrates how to implement an analog 
* front end (AFE) for a thermistor using the PSoC Analog Coprocessor. The measured 
* thermistor resistance and the calculated temperature are sent over I2C. The RGB 
* LED is controlled based on the calculated temperature value.
*******************************************************************************/

/* Header File Includes */
#include <project.h>

#define ADC_CHANNEL_VREF			(0u)
#define ADC_CHANNEL_VTH				(1u)
#define LED_ON						(0u)
#define LED_OFF						(1u)
#define TEMPERATURE_THRESHOLD_HIGH	(3000)
#define TEMPERATURE_THRESHOLD_LOW	(2500)
/* IIR Filter coefficients for each signal */
/* Cut off frequency = fs/(2 * pi * iir_filter_constant).  In this project fs ~= 1 ksps.
This results in a cut-off frequency of 4.97 Hz.  We are using IIR filter as FIR requires 
more order of filter to get the same cut-off frequency*/
#define FILTER_COEFFICIENT_TEMPERATURE	(32u)
/* EzI2C Read/Write Boundary */
#define READ_WRITE_BOUNDARY         (0u)

/* Structure that holds the temperature sensor (thermistor) data                 */
/* Use __attribute__((packed)) for GCC and MDK compilers to pack structures      */
/* For other compilers use the corresponding directive.                          */
/* For example, for IAR use the following directive                              */
/* typedef __packed struct {..}struct_name;                                      */
typedef struct __attribute__((packed))
{
	int16 Vth;					/* Voltage across thermistor */
	uint16 Rth;					/* Thermistor resistance */
	int16 temperature;			/* Measured temperature */
}temperature_sensor_data;

/* Function Prototypes */
void InitResources(void);

/* Declare the i2cBuffer to exchange sensor data between Bridge Control 
Panel (BCP) and PSoC Analog Coprocessor */
temperature_sensor_data i2cBuffer = {0, 0, 0};

/*******************************************************************************
* Function Name: main
********************************************************************************
*
* Summary:
*  This function initializes all the resources, and in an infinite loop, measures the temperature from the sensor 
*  readings and to send the data over I2C.
*
* Parameters:
*  None
*
* Return:
*  int
*
* Side Effects:
*   None
*******************************************************************************/
int main()
{

    /* Variables to hold the the ADC readings */
    int16 adcResultVREF, adcResultVTH;
    
    /* Filter input and output variables for Vref and Vth measurements */
    int16 filterOutputVref=0;
    int16 filterOutputVth=0;
    
    /* Variables to hold calculated resistance and temperature */
    int16 thermistorResistance, temperature;
    
    /* Variable to store the status returned by CyEnterCriticalSection()*/
    uint8 interruptState = 0;
    
    /* Enable global interrupts */
    CyGlobalIntEnable;
    
    /* Initialize hardware resources */
    InitResources();

    /* Infinite Loop */
    for(;;)
    {
        /* Check if the ADC data is ready */
        if(ADC_IsEndConversion(ADC_RETURN_STATUS))
        {
            /* Read the ADC result for reference and thermistor voltages */
            adcResultVREF = ADC_GetResult16(ADC_CHANNEL_VREF);
            adcResultVTH = ADC_GetResult16(ADC_CHANNEL_VTH);
            
            /* Low pass filter the measured ADC counts of Vref */            
            filterOutputVref = (adcResultVREF + (FILTER_COEFFICIENT_TEMPERATURE - 1) * filterOutputVref) / FILTER_COEFFICIENT_TEMPERATURE;
                    
            /* Low pass filter the measured ADC counts of Vth */         
            filterOutputVth = (adcResultVTH + (FILTER_COEFFICIENT_TEMPERATURE - 1) * filterOutputVth) / FILTER_COEFFICIENT_TEMPERATURE;
                        
            /* Calculate thermistor resistance */
            thermistorResistance = Thermistor_GetResistance(filterOutputVref, filterOutputVth);           
            
            /* Calculate temperature in degree Celsius using the Component API */
            temperature = Thermistor_GetTemperature(thermistorResistance);
            
            /* Turn ON Blue LED if Temperature is <= 25°C */ 
            if (temperature <= TEMPERATURE_THRESHOLD_LOW)
            {
                Pin_LED_Blue_Write(LED_ON);
                Pin_LED_Red_Write(LED_OFF);
            }
            /* Turn ON both Blue and Red LEDs if the temperature is >25°C and <=30°C */
            else if ((temperature > TEMPERATURE_THRESHOLD_LOW) && (temperature < TEMPERATURE_THRESHOLD_HIGH)) 
            {
                Pin_LED_Blue_Write(LED_ON);
                Pin_LED_Red_Write(LED_ON);
            }
            /* Turn ON Red LED if temperature is >30°C */
            else 
            {
                Pin_LED_Blue_Write(LED_OFF);
                Pin_LED_Red_Write(LED_ON);
            }   
            
			/* Enter critical section to check if I2C bus is busy or not */
            interruptState = CyEnterCriticalSection();
            
        	if(!(EZI2C_EzI2CGetActivity() & EZI2C_EZI2C_STATUS_BUSY))
        	{
                /* Update I2C Buffer */
                i2cBuffer.Rth = thermistorResistance;
                i2cBuffer.temperature = temperature;
                i2cBuffer.Vth = filterOutputVth;
            }
			
			/* Exit critical section */
            CyExitCriticalSection(interruptState);
        }
    }
}

/*******************************************************************************
* Function Name: void InitResources(void)
********************************************************************************
*
* Summary:
*  This function initializes all the hardware resources
*
* Parameters:
*  None
*
* Return:
*  None
*
* Side Effects:
*   None
*******************************************************************************/
void InitResources(void)
{
    /* Start EZI2C Slave Component and initialize buffer */
    EZI2C_Start();
    EZI2C_EzI2CSetBuffer1(sizeof(i2cBuffer), READ_WRITE_BOUNDARY, (uint8 *)&i2cBuffer);
        
    /* Start the Scanning SAR ADC Component and start conversion */
    ADC_Start();
    ADC_StartConvert();
    
    /* Start Reference buffer */
    VrefBuffer_Start();
    
    /* Start Programmable Voltage Reference */
    PVref_Start();
    
    /* Enable Programmable Voltage Reference */
    PVref_Enable();
}

/* [] END OF FILE */
