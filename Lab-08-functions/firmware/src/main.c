/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project. It is intended to
    be used as the starting point for CISC-211 Curiosity Nano Board
    programming projects. After initializing the hardware, it will
    go into a 0.5s loop that calls an assembly function specified in a separate
    .s file. It will print the iteration number and the result of the assembly 
    function call to the serial port.
    As an added bonus, it will toggle the LED on each iteration
    to provide feedback that the code is actually running.
  
    NOTE: PC serial port MUST be set to 115200 rate.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>
#include <malloc.h>
#include <inttypes.h>   // required to print out pointers using PRIXPTR macro
#include "definitions.h"                // SYS function prototypes
#include "asmExterns.h"
#include "testFuncs.h" // lab test structs
#include "printFuncs.h"  // lab print funcs

// Define the global that gives access to the student's name
extern uint32_t nameStrPtr;

// set to false to skip tests when debugging
bool doUnpackTest  = true;
bool doAbsTest     = true;
bool doMultTest    = true;
bool doFixSignTest = true;
bool doAsmMainTest = true;

bool onlyPrintFails = true; // set this to false to see passing test cases, too

/* RTC Time period match values for input clock of 1 KHz */
#define PERIOD_10MS                             10
#define PERIOD_100MS                            102
#define PERIOD_500MS                            512
#define PERIOD_1S                               1024
#define PERIOD_2S                               2048
#define PERIOD_4S                               4096

#define MAX_PRINT_LEN 1000

static volatile bool isRTCExpired = false;
static volatile bool changeTempSamplingRate = false;
static volatile bool isUSARTTxComplete = true;
static uint8_t uartTxBuffer[MAX_PRINT_LEN] = {0};



// the following array defines pairs of {balance, transaction} values
// tc stands for test case
static int32_t tc[] = {
    0x00020003,
    0xFFFC0003,  // -,+
    0x00000000,  // 0,0
    0x00000005,  // 0,+
    0x0000FFFC,  // 0,-
    0xFFFD0000,  // -,0
    0x00020000,  // +,0
    0x80008000,  // -,-
    0xFFF3FFE0,  // -,-
    0x7FF38001,  // +,-
    0x7FF17FF2   // +,+
};

// static char * pass = "PASS";
// static char * fail = "FAIL";

// VB COMMENT:
// The ARM calling convention permits the use of up to 4 registers, r0-r3
// to pass data into a function. Only one value can be returned to the 
// C caller. The assembly language routine stores the return value
// in r0. The C compiler will automatically use it as the function's return
// value.
//
/* These functions will be implemented by students in asmMult.s */
/* asmUnpack: no return value */
extern void asmUnpack(uint32_t packedValue, int32_t* a, int32_t* b);

/* asmAbs: return abs value. Also store abs value at location absOut,
 * and sign bit at location signBit. Must b 0 for +, 1 for negative */
extern int32_t asmAbs(int32_t input, int32_t *absOut, int32_t *signBit);

/* return product of two positive integers guaranteed to be <= 2^16 */
extern int32_t asmMult(int32_t a, int32_t b);

/* return corrected product based on signs of two original input values */
extern int32_t asmFixSign(int32_t initProduct, int32_t signBitA, int32_t signBitB);

/* Executes student's asmMain() function that ties together all of the above */
extern int32_t asmMain(uint32_t packedValue);

#if 0
extern int32_t a_Multiplicand;
extern int32_t b_Multiplier;
extern int32_t rng_Error;
extern int32_t a_Sign;
extern int32_t b_Sign;
extern int32_t prod_Is_Neg;
extern int32_t a_Abs;
extern int32_t b_Abs;
extern int32_t init_Product;
extern int32_t final_Product;
#endif


// set this to 0 if using the simulator. BUT note that the simulator
// does NOT support the UART, so there's no way to print output.
#define USING_HW 1

#if USING_HW
static void rtcEventHandler (RTC_TIMER32_INT_MASK intCause, uintptr_t context)
{
    if (intCause & RTC_MODE0_INTENSET_CMP0_Msk)
    {            
        isRTCExpired    = true;
    }
}
static void usartDmaChannelHandler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
    if (event == DMAC_TRANSFER_EVENT_COMPLETE)
    {
        isUSARTTxComplete = true;
    }
}
#endif



// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
int main ( void )
{
    
 
#if USING_HW
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_0, usartDmaChannelHandler, 0);
    RTC_Timer32CallbackRegister(rtcEventHandler, 0);
    RTC_Timer32Compare0Set(PERIOD_10MS);
    RTC_Timer32CounterSet(0);
    RTC_Timer32Start();
#else // using the simulator
    isRTCExpired = true;
    isUSARTTxComplete = true;
#endif //SIMULATOR
    
    printGlobalAddresses((char *)uartTxBuffer, &isUSARTTxComplete);

    // initialize all the variables
    int32_t passCount = 0;
    int32_t failCount = 0;
    // int32_t x1 = sizeof(tc);
    // int32_t x2 = sizeof(tc[0]);
    uint32_t numTestCases = sizeof(tc)/sizeof(tc[0]);
    
    static expectedValues exp;

    // Loop forever
    while ( true )
    {
        // Do the tests for asmUnpack
        int32_t unpackTotalPassCount = 0;
        int32_t unpackTotalFailCount = 0;
        int32_t unpackTotalTests = 0;

        if (doUnpackTest == true)
        {

            for (int testCase = 0; testCase < numTestCases; ++testCase)
            {
                // Toggle the LED to show we're running a new test case
                LED0_Toggle();

                // reset the state variables for the timer and serial port funcs
                isRTCExpired = false;
                isUSARTTxComplete = false;
                passCount = 0;
                failCount = 0;
                
                // Get the packed value for this test case 
                int32_t packedValue = tc[testCase];  // multiplicand and multiplier
                calcExpectedValues(testCase,"",packedValue,&exp);
                
                int32_t unpackedA = 0;
                int32_t unpackedB = 0;
                
                // !!!! THIS IS WHERE YOUR ASSEMBLY LANGUAGE PROGRAM GETS CALLED!!!!
                // Call our assembly function defined in file asmMult.s
                // Send in the test case value, see if the results are correct
                asmUnpack(exp.packedVal, &unpackedA, &unpackedB);
                
                testAsmUnpack(testCase,
                        "",
                        exp.packedVal, // inputs
                        &unpackedA,     // outputs
                        &unpackedB,
                        exp.inputA,    // expected values
                        exp.inputB,
                        &passCount,
                        &failCount,
                        onlyPrintFails,
                        &isUSARTTxComplete
                        );
                // print summary of tests executed so far
                unpackTotalPassCount = unpackTotalPassCount + passCount;
                unpackTotalFailCount = unpackTotalFailCount + failCount;
                unpackTotalTests = unpackTotalPassCount + unpackTotalFailCount;

                isUSARTTxComplete = false;
                snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                        "========= asmUnpack In-progress test summary:\r\n"
                        "%ld of %ld tests passed so far...\r\n"
                        "\r\n",
                        unpackTotalPassCount, unpackTotalTests); 
                
                printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

                // spin here until the LED toggle timer has expired. This allows
                // the test cases to be spread out in time.
                while (isRTCExpired == false);
            } // end: loop on all test cases for asmUnpack
            isUSARTTxComplete = false;
            snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                    "========= asmUnpack TESTS COMPLETE: \r\n"
                    "Summary of tests: %ld of %ld tests passed\r\n"
                    "\r\n",
                    unpackTotalPassCount, unpackTotalTests); 
            printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

            // STUDENTS: put a breakpoint at the next instruction to see the 
            // results of the asmUnpack tests!
            isUSARTTxComplete = false;
        } // if doUnpackTest == true


        int32_t absTotalPassCount = 0;
        int32_t absTotalFailCount = 0;
        int32_t absTotalTests = 0;
        if (doAbsTest == true)
        {
            // Do the tests for asmAbs
            for (int testCase = 0; testCase < numTestCases; ++testCase)
            {
                // Toggle the LED to show we're running a new test case
                LED0_Toggle();

                // reset the state variables for the timer and serial port funcs
                isRTCExpired = false;
                isUSARTTxComplete = false;

                passCount = 0;
                failCount = 0;
                
                // Get the packed value for this test case 
                int32_t packedValue = tc[testCase];  // multiplicand and multiplier
                calcExpectedValues(testCase,"",packedValue,&exp);
                
                int32_t absA = 0;
                int32_t absB = 0;
                int32_t signBitA = 0;
                int32_t signBitB = 0;
                
                // test the absolute value of A
                int32_t r0_absValA = asmAbs(exp.inputA, &absA, &signBitA);

                testAsmAbs(testCase,
                        "",
                        exp.inputA,  //inputs
                        &absA,        // I/O
                        &signBitA,
                        r0_absValA,   // outputs
                        exp.absA,  // expected values
                        exp.signA,
                        &passCount,
                        &failCount,
                        onlyPrintFails,
                        &isUSARTTxComplete
                        );
     
                absTotalPassCount = absTotalPassCount + passCount;
                absTotalFailCount = absTotalFailCount + failCount;

                passCount = 0;
                failCount = 0;
                
                // test the absolute value of B
                int32_t r0_absValB = asmAbs(exp.inputB, &absB, &signBitB);

                testAsmAbs(testCase,
                        "",
                        exp.inputB,  //inputs
                        &absB,        // I/O
                        &signBitB,
                        r0_absValB,   // outputs
                        exp.absB,  // expected values
                        exp.signB,
                        &passCount,
                        &failCount,
                        onlyPrintFails,
                        &isUSARTTxComplete
                        );

                // print summary of tests executed so far. Sums both A and B tests
                absTotalPassCount = absTotalPassCount + passCount;
                absTotalFailCount = absTotalFailCount + failCount;
                absTotalTests = absTotalPassCount + absTotalFailCount;

                isUSARTTxComplete = false;
                snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                        "========= asmAbs In-progress test summary:\r\n"
                        "%ld of %ld tests passed so far...\r\n"
                        "\r\n",
                        absTotalPassCount, absTotalTests); 
                
                printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

                // spin here until the LED toggle timer has expired. This allows
                // the test cases to be spread out in time.
                while (isRTCExpired == false);
            } // end: loop on all test cases for asmAbs A and B
            
            isUSARTTxComplete = false;
            snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                    "========= asmAbs TESTS COMPLETE: \r\n"
                    "Summary of tests: %ld of %ld tests passed\r\n"
                    "\r\n",
                    absTotalPassCount, absTotalTests); 
            printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);
            
            // STUDENTS: put a breakpoint at the next instruction to see the 
            // results of the asmAbs tests!
            isUSARTTxComplete = false;
        } //  if doAbsTest == true
        
       
       
        // test cases for asmMult function
        int32_t multTotalPassCount = 0;
        int32_t multTotalFailCount = 0;
        int32_t multTotalTests = 0;
        if (doMultTest == true)
        {
            /* return product of two positive integers guaranteed to be <= 2^16 */
            for (int testCase = 0; testCase < numTestCases; ++testCase)
            {
                // Toggle the LED to show we're running a new test case
                LED0_Toggle();

                // reset the state variables for the timer and serial port funcs
                isRTCExpired = false;
                isUSARTTxComplete = false;
                
                passCount = 0;
                failCount = 0;
                    // Get the packed value for this test case 
                int32_t packedValue = tc[testCase];  // multiplicand and multiplier
                calcExpectedValues(testCase,"",packedValue,&exp);
                
                // !!!! THIS IS WHERE YOUR ASSEMBLY LANGUAGE PROGRAM GETS CALLED!!!!
                // Call our assembly function defined in file asmMult.s
                // initConservedRegs(conservedRegInitValues);            // set the input values 
                int32_t r0_initProd = asmMult(exp.absA, exp.absB);

                testAsmMult(testCase,
                        "",
                        exp.absA, // inputs
                        exp.absB,
                        r0_initProd, // outputs
                        exp.initProduct, // expected values
                        &passCount,
                        &failCount,
                        onlyPrintFails,
                        &isUSARTTxComplete
                        );
                
                multTotalPassCount = multTotalPassCount + passCount;
                multTotalFailCount = multTotalFailCount + failCount;
                multTotalTests = multTotalPassCount + multTotalFailCount;

                isUSARTTxComplete = false;
                snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                        "========= asmMult In-progress test summary:\r\n"
                        "%ld of %ld tests passed so far...\r\n"
                        "\r\n",
                        multTotalPassCount, multTotalTests); 
                
                printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

                // spin here until the LED toggle timer has expired. This allows
                // the test cases to be spread out in time.
                while (isRTCExpired == false);
            } // end: loop on all test cases for asmMult
            
            isUSARTTxComplete = false;
            snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                    "========= asmMult TESTS COMPLETE: \r\n"
                    "Summary of tests: %ld of %ld tests passed\r\n"
                    "\r\n",
                    multTotalPassCount, multTotalTests); 
            printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);
            
            // STUDENTS: put a breakpoint at the next instruction to see the 
            // results of the asmMult tests!
            isUSARTTxComplete = false;
            
        } // end -- if doMultTest == true


        
        // test cases for asmFixSign function
        int32_t fsTotalPassCount = 0;
        int32_t fsTotalFailCount = 0;
        int32_t fsTotalTests = 0;
        if (doFixSignTest == true)
        {
            for (int testCase = 0; testCase < numTestCases; ++testCase)
            {
                // Toggle the LED to show we're running a new test case
                LED0_Toggle();

                // reset the state variables for the timer and serial port funcs
                isRTCExpired = false;
                isUSARTTxComplete = false;
                
                passCount = 0;
                failCount = 0;
                // Get the packed value for this test case 
                int32_t packedValue = tc[testCase];  // multiplicand and multiplier
                calcExpectedValues(testCase,"",packedValue,&exp);
                
                // !!!! THIS IS WHERE YOUR ASSEMBLY LANGUAGE PROGRAM GETS CALLED!!!!
                /* return corrected product based on signs of two original input values */
                // provide the correct value as inputs,
                // see if the sign is adjusted correctly
                int32_t r0_finalProduct = asmFixSign(exp.initProduct, 
                        exp.signA, 
                        exp.signB);

                testAsmFixSign(testCase,
                        "DEBUG",
                        exp.initProduct, // inputs
                        exp.signA, 
                        exp.signB,
                        r0_finalProduct, // outputs
                        exp.finalProduct, // expected values
                        &passCount,
                        &failCount,
                        onlyPrintFails,
                        &isUSARTTxComplete
                        );
                fsTotalPassCount = fsTotalPassCount + passCount;
                fsTotalFailCount = fsTotalFailCount + failCount;
                fsTotalTests = fsTotalPassCount + fsTotalFailCount;

                isUSARTTxComplete = false;
                snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                        "========= asmFixSign In-progress test summary:\r\n"
                        "%ld of %ld tests passed so far...\r\n"
                        "\r\n",
                        fsTotalPassCount, fsTotalTests); 
                
                printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

                // spin here until the LED toggle timer has expired. This allows
                // the test cases to be spread out in time.
                while (isRTCExpired == false);
            } // end: loop on all test cases for asmFixSign
            
            isUSARTTxComplete = false;
            snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                    "========= asmFixSign TESTS COMPLETE: \r\n"
                    "Summary of tests: %ld of %ld tests passed\r\n"
                    "\r\n",
                    fsTotalPassCount, fsTotalTests); 
            printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);
            
            // STUDENTS: put a breakpoint at the next instruction to see the 
            // results of the asmFixSign tests!
            isUSARTTxComplete = false;
            
        } // end -- if doFixSignTest == true



       
        // test cases for asmMain function
        int32_t mainTotalPassCount = 0;
        int32_t mainTotalFailCount = 0;
        int32_t mainTotalTests = 0;
        if (doAsmMainTest == true)
        {
            // return product of two positive integers guaranteed to be <= 2^16 
            for (int testCase = 0; testCase < numTestCases; ++testCase)
            {
                // Toggle the LED to show we're running a new test case
                LED0_Toggle();

                // reset the state variables for the timer and serial port funcs
                isRTCExpired = false;
                isUSARTTxComplete = false;
                passCount = 0;
                failCount = 0;

                // Get the packed value for this test case 
                int32_t packedValue = tc[testCase];  // multiplicand and multiplier
                calcExpectedValues(testCase,"",packedValue,&exp);
                
                // !!!! THIS IS WHERE YOUR ASSEMBLY LANGUAGE PROGRAM GETS CALLED!!!!
                // Call our assembly function defined in file asmMult.s
                
                int32_t r0_mainFinalProd = asmMain(packedValue);
                testAsmMain(testCase,
                        "",
                        exp.packedVal, // inputs
                        r0_mainFinalProd, // outputs
                        a_Multiplicand, // val stored in mem
                        b_Multiplier,  // val stored in mem
                        a_Abs, a_Sign, b_Abs, b_Sign,
                        init_Product,
                        final_Product,
                        &exp, // expected values
                        &passCount,
                        &failCount,
                        onlyPrintFails,
                        &isUSARTTxComplete
                        );

                            
                mainTotalPassCount = mainTotalPassCount + passCount;
                mainTotalFailCount = mainTotalFailCount + failCount;
                mainTotalTests = mainTotalPassCount + mainTotalFailCount;

                isUSARTTxComplete = false;
                snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                        "========= asmMain In-progress test summary:\r\n"
                        "%ld of %ld tests passed so far...\r\n"
                        "\r\n",
                        mainTotalPassCount, mainTotalTests); 
                
                printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

                // spin here until the LED toggle timer has expired. This allows
                // the test cases to be spread out in time.
                while (isRTCExpired == false);
            } // end: loop on all test cases for asmMult
            
            isUSARTTxComplete = false;
            snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                    "========= asmMain TESTS COMPLETE: \r\n"
                    "Summary of tests: %ld of %ld tests passed\r\n"
                    "\r\n",
                    mainTotalPassCount, mainTotalTests); 
            printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

            // STUDENTS: put a breakpoint at the next instruction to see the 
            // results of the asmMain tests!
            isUSARTTxComplete = false;
        } // end -- if doAsmMainTest == true
        
        // When all test cases are complete, print the pass/fail statistics
        // Keep looping so that students can see code is still running.
        // We do this in case there are very few tests and they don't have the
        // terminal hooked up in time.
        uint32_t idleCount = 1;
        // uint32_t totalTests = totalPassCount + totalFailCount;
        bool firstTime = true;
        // float unpackPct, absPct, multPct, fsPct, mainPct;
        uint32_t unpackPts, absPts, multPts, fsPts, mainPts,totalPts;
        unpackPts = 5*unpackTotalPassCount/unpackTotalTests;
        absPts = 5*absTotalPassCount/absTotalTests;
        multPts = 5*multTotalPassCount/multTotalTests;
        fsPts = 5*fsTotalPassCount/fsTotalTests;
        mainPts = 5*mainTotalPassCount/mainTotalTests;
        totalPts = unpackPts + absPts + multPts + fsPts + mainPts;
        
        while(true)      // post-test forever loop
        {
            isRTCExpired = false;
            isUSARTTxComplete = false;
            if (doUnpackTest == true && 
                    doAbsTest == true && 
                    doMultTest == true &&
                    doFixSignTest == true &&
                    doAsmMainTest == true)
            {
                snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                    "========= %s: Functions Lab ALL TESTS COMPLETE: Post-test Idle Cycle Number: %ld\r\n"
                    "Summary of tests: asmUnpack:  %ld of %ld tests passed; %ld pts\r\n"
                    "Summary of tests: asmAbs:     %ld of %ld tests passed; %ld pts\r\n"
                    "Summary of tests: asmMult:    %ld of %ld tests passed; %ld pts\r\n"
                    "Summary of tests: asmFixSign: %ld of %ld tests passed; %ld pts\r\n"
                    "Summary of tests: asmMain:    %ld of %ld tests passed; %ld pts\r\n"
                    " Total point score: %ld\r\n"
                    "\r\n",
                    (char *) nameStrPtr, idleCount, 
                    unpackTotalPassCount, unpackTotalTests, unpackPts,
                    absTotalPassCount, absTotalTests, absPts,
                    multTotalPassCount, multTotalTests, multPts,
                    fsTotalPassCount, fsTotalTests, fsPts,
                    mainTotalPassCount, mainTotalTests, mainPts,
                    totalPts
                    ); 
            }
            else
            {
                snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
                    "========= %s: PARTIAL tests complete, make sure to rerun with all tests enabled!!!\r\n"
                    "Post-test Idle Cycle Number: %ld\r\n",
                    (char *) nameStrPtr, idleCount);
            }

#if USING_HW 
            isUSARTTxComplete = false;
            // spin here, waiting for UART to complete
            printAndWait((char*)uartTxBuffer, &isUSARTTxComplete);
            LED0_Toggle();
            ++idleCount;

            // spin here, waiting for LED toggle timer to complete
            while (isRTCExpired == false);

            // slow down the blink rate after the tests have been executed
            if (firstTime == true)
            {
                firstTime = false; // only execute this section once
                RTC_Timer32Compare0Set(PERIOD_4S); // set blink period to 4sec
                RTC_Timer32CounterSet(0); // reset timer to start at 0
            }
#endif
             return ( EXIT_FAILURE ); // stop after first iteration
        } // end - post-test forever loop
        
        // Should never get here...
        break;
    } // while ...
            
    /* Execution should not come here during normal operation */
    return ( EXIT_FAILURE );
}
/*******************************************************************************
 End of File
*/

