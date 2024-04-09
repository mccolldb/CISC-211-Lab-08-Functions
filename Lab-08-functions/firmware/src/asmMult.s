/*** asmMult.s   ***/
/* SOLUTION; used to test C test harness
 * VB 10/14/2023
 */
    
/* Tell the assembler to allow both 16b and 32b extended Thumb instructions */
.syntax unified

#include <xc.h>

/* Tell the assembler that what follows is in data memory    */
.data
.align
 
/* define and initialize global variables that C can access */

/* create a string */
.global nameStr
.type nameStr,%gnu_unique_object
    
/*** STUDENTS: Change the next line to your name!  **/
nameStr: .asciz "David McColl"  
 
/* initialize a global variable that C can access to print the nameStr */
.global nameStrPtr
.type nameStrPtr,%gnu_unique_object
nameStrPtr: .word nameStr   /* Assign the mem loc of nameStr to nameStrPtr */

.global a_Multiplicand,b_Multiplier,a_Sign,b_Sign,a_Abs,b_Abs,init_Product,final_Product
.type a_Multiplicand,%gnu_unique_object
.type b_Multiplier,%gnu_unique_object
.type rng_Error,%gnu_unique_object
.type a_Sign,%gnu_unique_object
.type b_Sign,%gnu_unique_object
.type prod_Is_Neg,%gnu_unique_object
.type a_Abs,%gnu_unique_object
.type b_Abs,%gnu_unique_object
.type init_Product,%gnu_unique_object
.type final_Product,%gnu_unique_object

/* NOTE! These are only initialized ONCE, right before the program runs.
 * If you want these to be 0 every time asmMult gets called, you must set
 * them to 0 at the start of your code!
 */
a_Multiplicand:  .word     0  
b_Multiplier:    .word     0  
rng_Error:       .word     0 
a_Sign:          .word     0  
b_Sign:          .word     0 
prod_Is_Neg:     .word     0 
a_Abs:           .word     0  
b_Abs:           .word     0 
init_Product:    .word     0
final_Product:   .word     0

 /* Tell the assembler that what follows is in instruction memory    */
.text
.align

.global asmUnpack, asmAbs, asmMult, asmFixSign, asmMain
.type asmUnpack,%function
.type asmAbs,%function
.type asmMult,%function
.type asmFixSign,%function
.type asmMain,%function

/* function: asmUnpack
 *    inputs:   r0: contains the packed value. 
 *                  MSB 16bits is signed multiplicand (a)
 *                  LSB 16bits is signed multiplier (b)
 *              r1: address where to store unpacked, 
 *                  sign-extended 32 bit a value
 *              r2: address where to store unpacked, 
 *                  sign-extended 32 bit b value
 *    outputs:  r0: No return value
 *              memory: 
 *                  1) store unpacked A value in location
 *                     specified by r1
 *                  2) store unpacked B value in location
 *                     specified by r2
 */
asmUnpack: 
/*** STUDENTS: Place your asmUnpack code BELOW this line!!! **************/
    /* void asmUnpack(uint packed, int* a, int* b) */
    /*  R0                  R0      R1      R2     */
    PUSH {r4-r11,LR}  // save caller registers
    MOV R4,R0, ASR 16 // a value
    STR R4,[R1]       // save to addr(a)
    SXTH R5,R0        // b value
    STR R5,[R2]       // save to addr(b)
    POP  {r4-r11,LR}  // restore caller registers
    BX LR             // return to caller
/*** STUDENTS: Place your asmUnpack code ABOVE this line!!! **************/
/***************  END ---- asmUnpack  ************/

 
/* function: asmAbs
 *    inputs:   r0: contains signed value
 *              r1: address where to store absolute value
 *              r2: address where to store sign bit 0 = "+", 1 = "-")
 *    outputs:  r0: Absolute value of r0 input. Same value as stored to location given in r1
 *              memory: store absolute value in location given by r1
 *                      store sign bit in location given by r2
 */    
asmAbs:  
/*** STUDENTS: Place your asmAbs code BELOW this line!!! **************/
    /* uint abs = asmAbs(int input, int* absOut, int* signBit); */
    /*      R0                 R0         R1           R2       */
    PUSH {r4-r11,LR}  // save caller registers
    CMP R0,0          // cmp input to 0
    NEGLT R0,R0       // if input<0 R0=-R0
    MOVLT R3,1        //            sign = 1
    MOVGE R3,0        // else       sign = 0
    STR R0,[R1]       // save value to addr(abs)
    STR R3,[R2]       // save sign  to addr(sign)
    POP  {r4-r11,LR}  // restore caller registers
    BX LR             // return to caller
/*** STUDENTS: Place your asmAbs code ABOVE this line!!! **************/
/***************  END ---- asmAbs  ************/

 
/* function: asmMult
 *    inputs:   r0: contains abs value of multiplicand (a)
 *              r1: contains abs value of multiplier (b)
 *    outputs:  r0: initial product: r0 * r1
 */ 
asmMult:   
/*** STUDENTS: Place your asmMult code BELOW this line!!! **************/
    /*   uint product = asmMult(uint a, uint b); */
    /*          R0                 R0    R1      */
    PUSH {r4-r11,LR}  // save caller registers
    /* do the shift&add multiply  R0 * R1 -> R3 */
    MOV r3,0          // initialize product accumulator */
1:  TST R0,1          // Z=1 if lobit of shifted A clear */
    ADDNE r3,r3,r1    // if Z==0 add copy of shifted B into accum  */
    LSL  R1,R1,1      // double B  */
    LSRS R0,R0,1      // halve A */
    BNE 1b            // loop if A not yet zero */
    MOV R0,R3         // move product to result=R0
    POP  {r4-r11,LR}  // restore caller registers
    BX LR             // return to caller
/*** STUDENTS: Place your asmMult code ABOVE this line!!! **************/
/***************  END ---- asmMult  ************/

    
/* function: asmFixSign
 *    inputs:   r0: initial product from previous step: 
 *              (abs value of A) * (abs value of B)
 *              r1: sign bit of originally unpacked value
 *                  of A
 *              r2: sign bit of originally unpacked value
 *                  of B
 *    outputs:  r0: final product:
 *                  sign-corrected version of initial product
 */ 
asmFixSign:   
/*** STUDENTS: Place your asmFixSign code BELOW this line!!! **************/
    /*  int final = asmFixSign(int initial, int signBitA, int signBitB) */
    /*        R0                     R0             R1            R2    */
    PUSH {r4-r11,LR}  // save caller registers
    ADD R1,R1,R2      // add sign bits
    TST R1,1          // check for odd
    NEGNE R0,R0       // if odd(tst!=0) final = -initial
    POP  {r4-r11,LR}  // restore caller registers
    BX LR             // return to caller
/*** STUDENTS: Place your asmFixSign code ABOVE this line!!! **************/
/***************  END ---- asmFixSign  ************/


/* function: asmMain
 *    inputs:   r0: contains packed value to be multiplied
 *                  using shift-and-add algorithm
 *           where: MSB 16bits is signed multiplicand (a)
 *                  LSB 16bits is signed multiplier (b)
 *    outputs:  r0: final product: sign-corrected product
 *                  of the two unpacked A and B input values
 *    NOTE TO STUDENTS: 
 *           To implement asmMain, follow the steps outlined
 *           in the comments in the body of the function
 *           definition below.
 */  
asmMain:   
/*** STUDENTS: Place your asmMain code BELOW this line!!! **************/
    /* int32 product = asmMain(uint packedValue) */
    /*          R0                    R0         */
    PUSH {r4-r11,LR}  // save caller registers
    /* Step 1: call asmUnpack. 
     * Have it store the output values in a_Multiplicand and b_Multiplier.
     */
    MOV R0,R0   // R0 already has packed value
    LDR R1,=a_Multiplicand
    LDR R2,=b_Multiplier
    BL asmUnpack
     /* Step 2a: call asmAbs for the multiplicand (a). 
      * Have it store the absolute value in a_Abs, and the sign in a_Sign.
      */
    LDR R0,=a_Multiplicand
    LDR R0,[R0]
    LDR R1,=a_Abs
    LDR R2,=a_Sign
    BL asmAbs
     /* Step 2b: call asmAbs for the multiplier (b). 
      * Have it store the absolute value in b_Abs, and the sign in b_Sign.
      */
    LDR R0,=b_Multiplier
    LDR R0,[R0]
    LDR R1,=b_Abs
    LDR R2,=b_Sign
    BL asmAbs
    /* Step 3: call asmMult. 
     * Pass a_Abs as the multiplicand, and b_Abs as the multiplier.
     * asmMult returns the initial (positive) product in r0.
     * In this function (asmMain), store the output value  
     * returned asmMult in r0 to mem location init_Product.
     */
    LDR R0,=a_Abs
    LDR R0,[R0]
    LDR R1,=b_Abs
    LDR R1,[R1]
    BL asmMult
    LDR R1,=init_Product
    STR R0,[R1]
    /* Step 4: call asmFixSign. 
     * Pass in the initial product, and the sign bits for a and b inputs. 
     * asmFixSign returns the final product with the correct sign.
     * Store the value returned in r0 to mem location final_Product.
     */
    LDR R1,=a_Sign
    LDR R1,[R1]
    LDR R2,=b_Sign
    LDR R2,[R2]
    BL asmFixSign
    LDR R1,=final_Product
    STR R0,[R1]
     /* Step 5: Return to caller. 
      * Make sure of the following:
      * 1) Stack has been correctly managed.
      * 2) the final answer is stored in r0, so that the C call can access it.
      */
    POP  {r4-r11,LR}  // restore caller registers
    BX LR             // return to caller    
/*** STUDENTS: Place your asmMain code ABOVE this line!!! **************/
/***************  END ---- asmMain  ************/
.end   /* the assembler will ignore anything after this line. */
