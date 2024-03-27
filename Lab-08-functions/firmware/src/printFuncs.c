/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */



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
#include "testFuncs.h" // lab test structs
#include "printFuncs.h"  // lab print funcs

#define MAX_PRINT_LEN 1000

#define USING_HW 1

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */
static uint8_t txBuffer[MAX_PRINT_LEN] = {0};


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

// *****************************************************************************

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
void printAndWait(char *txBuffer, volatile bool *txCompletePtr)
{
    *txCompletePtr = false;

#if USING_HW 
    DMAC_ChannelTransfer(DMAC_CHANNEL_0, txBuffer, \
        (const void *)&(SERCOM5_REGS->USART_INT.SERCOM_DATA), \
        strlen((const char*)txBuffer));
    // spin here, waiting for timer and UART to complete
    while (*txCompletePtr == false); // wait for print to finish
    /* reset it for the next print */
    *txCompletePtr = false;
#endif

}


/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
void printUnpack(int32_t testNum, 
        char * desc,
        uint32_t packedVal, 
        int32_t msbA, 
        int32_t lsbB,
        volatile bool * txCompletePtr) 
{
        // build the string to be sent out over the serial lines
    snprintf((char*)txBuffer, MAX_PRINT_LEN,
            "========= asmUnpack %s test number: %ld\r\n"
            "packed value:                           0x%08lx\r\n"
            "unpacked A (multiplicand) value: %11ld; 0x%08lx\r\n"
            "unpacked B (multiplier) value:   %11ld; 0x%08lx\r\n"
            "========= END -- UNPACK debug output\r\n"
            "\r\n",
            desc,
            testNum,
            packedVal, 
            msbA,msbA,
            lsbB,lsbB
            ); 

#if USING_HW 
    printAndWait((char *)txBuffer, txCompletePtr);
#endif

    return ;
} // end -- printUnpack

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
void printAbs(int32_t testNum,
        char * desc,
        int32_t signedVal, 
        int32_t absVal, 
        int32_t signBit,
        volatile bool * txCompletePtr) 
{
        // build the string to be sent out over the serial lines
    snprintf((char*)txBuffer, MAX_PRINT_LEN,
            "========= asmAbs %s test number: %ld\r\n"
            "signed value:   %11ld; 0x%08lx\r\n"
            "unsigned value: %11ld; 0x%08lx\r\n"
            "sign bit:       %ld\r\n"
            "========= END -- ABS debug output\r\n"
            "\r\n",
            desc,
            testNum,
            signedVal, signedVal,
            absVal, absVal,
            signBit
            ); 

#if USING_HW 
    printAndWait((char *)txBuffer, txCompletePtr);
#endif

    return ;
} // end -- printAbs

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
    void printInitialProduct(int32_t testNum, 
            char * desc,
            int32_t absA, 
            int32_t absB, 
            int32_t result,
            volatile bool * txCompletePtr)
{
        // build the string to be sent out over the serial lines
    snprintf((char*)txBuffer, MAX_PRINT_LEN,
            "========= asmMult %s test number: %ld\r\n"
            "abs(A): %11ld; 0x%08lx\r\n"
            "abs(B): %11ld; 0x%08lx\r\n"
            "result: %11ld; 0x%08lx\r\n"
            "========= END -- INITIAL PRODUCT debug output\r\n"
            "\r\n",
            desc,
            testNum,
            absA, absA,
            absB, absB,
            result, result
            ); 

#if USING_HW 
    printAndWait((char *)txBuffer, txCompletePtr);
#endif

    return ;
} // end -- printInitialProduct

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
    void printFinalProduct(int32_t testNum, 
            char * desc,
            int32_t initialProduct,
            int32_t signBitA,
            int32_t signBitB,
            int32_t signCorrectedProduct, 
            volatile bool * txCompletePtr)    
{
        // build the string to be sent out over the serial lines
    snprintf((char*)txBuffer, MAX_PRINT_LEN,
            "========= asmFixSign %s test number: %ld\r\n"
            "unsigned initial product: %11ld; 0x%08lx\r\n"
            "sign bit A:               %ld\r\n"
            "sign bit B:               %ld\r\n"
            "signed final product:     %11ld; 0x%08lx\r\n"
            "========= END -- FINAL PRODUCT debug output\r\n"
            "\r\n",
            desc,
            testNum,
            initialProduct, initialProduct,
            signBitA,
            signBitB,
            signCorrectedProduct, signCorrectedProduct
            ); 

#if USING_HW 
    printAndWait((char *)txBuffer, txCompletePtr);
#endif

    return ;
} // end -- printFinalProduct

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
void printIntegratedProduct(int32_t testNum,
        char * desc,
        uint32_t packedVal, 
        int32_t finalValue ,
        volatile bool * txCompletePtr) 
{
        // build the string to be sent out over the serial lines
    snprintf((char*)txBuffer, MAX_PRINT_LEN,
            "========= asmMain %s test number: %ld\r\n"
            "packed value:  %11ld; 0x%08lx\r\n"
            "final product: %11ld; 0x%08lx\r\n"
            "========= END -- INTEGRATED (MAIN) debug output\r\n"
            "\r\n",
            desc,
            testNum,
            packedVal, packedVal,
            finalValue, finalValue
            ); 

#if USING_HW 
    printAndWait((char *)txBuffer, txCompletePtr);
#endif

    return ;
} // end -- PrintIntegratedProduct


/* *****************************************************************************
 End of File
 */
