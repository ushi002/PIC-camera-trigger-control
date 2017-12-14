/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
    #include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */
#include <stdlib.h>        /* c standard library */

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp */

/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/

#define PRTC_ANAL    ANSELC

#define CAMTRIG         PORTCbits.RC5
/* i.e. uint8_t <variable_name>; */

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/
void main(void)
{
    unsigned int dacval, i;
    bool thiefdetected = 0;
    unsigned int startupDelay = 0;
    
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitApp();
    
    //turn off watchdog:
    WDTCONbits.SWDTEN = 1; //check WDTE in config word1, must be 01 to enable
                            //control
    WDTCONbits.WDTPS = 0xc; //0xC=4s
    
    //configure SPI pins:
    //0 = output, 1 = input    
    TRISAbits.TRISA4 = 0;
    TRISAbits.TRISA5 = 1;

    //TRISCbits.TRISC4 = 0; //UART TX
    //TRISCbits.TRISC5 = 1; //UART RX
    TRISCbits.TRISC5 = 0; //CAMTRIG output
    //enable weak pull ups
    //OPTION_REGbits.nWPUEN = 0;
    //weak pull up
    //WPUAbits.WPUA5 = 1;
    
    //set output value
    PORTAbits.RA4 = 0;
    //PORTAbits.RA5 = 0;
    
    //set output value
    CAMTRIG = 0;
    
    ANSELAbits.ANSA4 = 0; //enable digital input for PORTA4
    dacval = 0;
    
    //enable interrupts
    //GIE is for interrupt vectors. Else program continues just after the sleep
    //INTCONbits.GIE = 1;
    //enable external interrupt
    INTCONbits.INTE = 1;
    //enable interrupt on change
    INTCONbits.IOCIE = 1;

    //detect rising (P) falling (N) edge
    IOCAPbits.IOCAP5 = 1;
    //!! IOCAFbits.IOCAF5 should be set when interrupt arrives..
    //??OPTION_REGbits.INTEDG = 0; //falling edge interrupt
    
    INTCONbits.INTF = 0; //clear external interrupt flag
    while(startupDelay < 4)
    {
        for(i=0; i<200; i++) {} //delay before diode is turned off
        LATAbits.LATA4 = 0;
        asm("SLEEP");
        asm("NOP");
        if (!STATUSbits.nTO) //watchdog timeout
        {
            LATAbits.LATA4 = 1;
            startupDelay++;
        }
        INTCONbits.INTF = 0; //clear (watchdog) flag
    }

    //clear flags
    LATAbits.LATA4 = 0;
    INTCONbits.INTF = 0; //clear external interrupt flag
    IOCAFbits.IOCAF5 = 0;
    thiefdetected = false;

    while(1)
    {
        if (!thiefdetected)
        {
            INTCONbits.INTF = 0; //clear external interrupt flag
            while(1)
            {
                for(i=0; i<200; i++) {} //delay before diode is turned off
                LATAbits.LATA4 = 0;
                asm("SLEEP");
                asm("NOP");
                if (!STATUSbits.nTO) //watchdog timeout
                    LATAbits.LATA4 = 1;
                if (PCONbits.nRWDT)
                    LATAbits.LATA4 = 1; //watchdog reset has not occurred
                if(INTCONbits.IOCIF)
                    LATAbits.LATA4 = 1; //some external interrupt flag occurred
                INTCONbits.INTF = 0; //clear (watchdog) flag
                if(IOCAFbits.IOCAF5)    //PORTA.5 flag
                {
                    //IOCAFbits.IOCAF5 = 0; //moved to the very end of the cycle
                    thiefdetected = true;
                    break; //quit this loop and continue the program
                }
            }
        }
        asm("CLRWDT");
        //for 3500 x dacval>23 makes cca 8 seconds
        for(i=0; i<3500; i++) //higher then 20000 values does not work - watchdog! (OLD WATCHDOG)
        {
        }
        LATAbits.LATA4 = LATAbits.LATA4 ^ 1; //heart beat

        APFCONbits.P2SEL = 0;

        dacval++;
        CAMTRIG = 1; //turn on camera and take a picture
        
        //tune the value for one picture shot!
        //if (dacval > 3)  //speed up debugging
        if (dacval > 23)  //skip last 8 steps (31 steps)
        {
            dacval = 0;
            thiefdetected = 0; //end of camera sequence
            IOCAFbits.IOCAF5 = 0;
            CAMTRIG = 0;
        }
    }
    
    
}

