
/* ======================================================
    serialPort: OpenSDA Virtual Serial Port using UART0 
   
   Interface
     * sendMsg
       - Queue a message for transmission over the virtual serial port
       - Non-blocking: small circular queue 
       - Returns immediately without queuing message if queue full
       - Message test not copied from buffer in user thread        

     * readLine
       - Single outstanding request 
       - Blocking: does not return until end of line read
       - Reads characters until LF; CR ignored; use with local echo
       - Message text written to buffer in user thread
         
   The implememtation is interrupt driven
       - Single ISR
       - Interrupt when transmit buffer empty
       - Interrupt when character received
       
   Section 1: Transmission data structure and functions
   Section 2: Receiving data structure and functions
   Section 3: Initialisation
   Section 4: ISR
    ========================================================= */


#include "cmsis_os2.h"
#include <MKL28Z7.h>
#include <stdbool.h>
#include "..\inc\serialPort.h"

// ================ Section 1: Transmission ==================

/* --------------------------------
     Circular buffer for transmisison of messages

   -------------------------------- */
#define QMAXSIZE (4)    // queue entries - power of 2
#define QMASK (3)       // mask for modulo arithmetic: entries - 1 

// Send request type
typedef struct {
    char* buffer ;  // pointer to null terminated string
    int line ;      // NOLINE, LFONLY, CRLF 
} SendReq_t ;

// Circular queue of send requests
typedef struct {
    SendReq_t requests[QMAXSIZE] ;  // queue of request
    unsigned int head ;             // first occupied
    unsigned int tail ;             // first UNOCCUPIED
    unsigned int size ;             // need to distingush empty from full
} volatile MsgQ_t ;

// Declare the message queue 
MsgQ_t msgQueue ;

// Initialisation of the message queue
void initSendMsg() {
    msgQueue.size = 0 ;
    msgQueue.head = 0 ;
    msgQueue.tail = 0 ;
}

/* --------------------------------
     Send a message

    * Queue message for transmission on UART0
    * The call does not block
    * False is returned if the queue is full; true otherwise

   Concurrency:
       - May be called from multiple threads
       - Queue fields head and size also accessed by ISR
       - Interrupts disabled for access to queue
   -------------------------------- */

bool sendMsg(char *msg, int eol) {
    
    // start critical region
    int currentMask = __get_PRIMASK() ; 
    __disable_irq() ;
    
    if (msgQueue.size == QMAXSIZE) {
        // queue full
        __set_PRIMASK(currentMask) ;
        return false ; 
    }
    msgQueue.requests[msgQueue.tail].buffer = msg ;
    msgQueue.requests[msgQueue.tail].line = eol ;
    msgQueue.tail = (msgQueue.tail + 1) & QMASK ;
    msgQueue.size++ ;

    // ensure interrupt enabled
    //UART0->C2 |= UART0_C2_TIE(1) ;
    LPUART0->CTRL |= LPUART_CTRL_TIE(1) ;

    __set_PRIMASK(currentMask) ;
    // end critical region
    
    return true ;        
}

/* --------------------------------
     Remove transmitted message

   * Queue cannot be empty
   * Return true if queue still non-empty

   Called from ISR when request handled (all transmitted)
   -------------------------------- */
bool removeMsg() {
    msgQueue.head = (msgQueue.head + 1) & QMASK  ;
    msgQueue.size-- ;
    return msgQueue.size ;
}

/* --------------------------------
     Get next charater to transmit from request at head

   -------------------------------- */
#define CRCHAR (13)
#define LFCHAR (10)

char getNextChar() {
    // Case 1: current request not finished
    char c = *msgQueue.requests[msgQueue.head].buffer ;
    if (c != 0) {
        msgQueue.requests[msgQueue.head].buffer++ ;
        return c ;
    }
    
    // Case 2: sending a CR 
    if (msgQueue.requests[msgQueue.head].line == CRLF) {
        msgQueue.requests[msgQueue.head].line-- ;
        return CRCHAR;
    }
    // Case 3: sending a LF 
    if (msgQueue.requests[msgQueue.head].line == LFONLY) {
        msgQueue.requests[msgQueue.head].line-- ;
        return LFCHAR ;
    }
    return 0 ;
}


// =============Section 2: Receive Message =============================

// Read request type
typedef struct {
    char* buffer ;  // pointer to null terminated string
    int maxIndex ;  // maximum index: num chars - 1; buffer must be +1 in length, for null 
    int index ;     // current buffer index
} readReq_t ;

// data structures
volatile readReq_t readReq ;

#define READCOMPLETE (0x1) 
osEventFlagsId_t readFlags ;          // event flag used to signal read completion


void initReadReq() {
    readReq.buffer = NULL ;
    readReq.index = 0 ;
    readFlags = osEventFlagsNew(NULL) ;
}


/* ------------------------------------------
     Read a line

    Reading continues until a LF is encountered. The 
      call does not return until this happens.

      msg - pointer to string buffer for characters read
      maxChars - maximum number of message characters written
                 to the buffer
      
    The buffer must have space for maxChar+1 as a string termination
    character is written. Additional characters received after maxChar and 
    before LF are discarded

    Concurrency: 
       1. Two threads could attempt to call this at the same time, 
          causing a race condition on the read request buffer and index. We disable interrupts
          on the buffer test and update.
       2. The ISR updates both buffer and index, but guarded by buffer not null 
   ------------------------------------------ */
bool readLine (char *msg, int maxChars) {
    
    // start critical region
    int currentMask = __get_PRIMASK() ; 
    __disable_irq() ;
    
    // check if an outstanding request
    if (readReq.buffer != NULL) {
        __set_PRIMASK(currentMask) ;
        return false ;
    }
    readReq.buffer = msg ;
    __set_PRIMASK(currentMask) ;
    // end critical region
    
    readReq.index = 0 ;
    readReq.maxIndex = maxChars ;
    
    // buffer is filled in the interrupt handler 
    
    // wait for completion
    osEventFlagsWait (readFlags, READCOMPLETE, osFlagsWaitAny, osWaitForever);
    return true ;
}

/* -------------------------------------
      Update with received character

   Called from ISR

   A character has been received. Write it to the buffer.
    - Detect end of message
    - Prevent buffer overflow

   End of message: signalled on a LF. CR is ignored.
   When the end of the buffer provided by the is reached, further
     characters are dropped
   Return true to signal line complete
------------------------------------- */
bool setNextChar(char c) {
    // ignore if no request active
    if (readReq.buffer == NULL) return false ; 
    // ignore a CR
    if (c == CRCHAR) return false ;
    
    // LF or buffer full ends the read
    if (c == LFCHAR) {
        readReq.buffer[readReq.index] = 0 ;
        readReq.buffer = NULL ;
        return true ;
    }
    
    // write character to buffer if not full
    if (readReq.index < readReq.maxIndex) {
        // buffer not full
        readReq.buffer[readReq.index++] = c ;
    }
    // drop character if buffer full
    return false;
}

// ============= Section 3: Initialisation =======================

/* ----------------------------------------
   Configure UART0 for serial I/O via the debug 
   system to the PC, using interrupts
 * ---------------------------------------- */
#define PERI_CLOCK (8000000)    // default system clock is 8 MHz - the slow IRC
// Pins are PTB16 and PTB17
#define RxPIN (16)             // Pin PTB16 fixed by development board design (ALT3)
#define TxPIN (17)             // Pin PTB17 fixed by development board design (ALT3)

void init_UART0(uint32_t baud_rate) {
    
    // LPUART Clock information
    // ------------------------
    // Page 118:
    //   System core clock (DIVCORE_CLK is SIRC - 8 MHz)
    //   Bus interface clock is DIVSLOW_CLK (probably 4 MHz, but not relevant)
    //   Clock grating available is Yes 
    //   Peripheral function clock is SCG DIV3 - this is 'diabled by default:
    //     it must be set to divide by one to work
   
    // Enable funtion clock - page 959 
    //   Set PCS to 010 - SCGIRCLK - Slow IRC Clock
    //   Set CGC to 1 

    SCG->SIRCDIV |= SCG_SIRCDIV_SIRCDIV3(1) ;
    
    PCC_LPUART0 &= ~PCC_CLKCFG_CGC(1) ; // Ensure UART0 clock is off - one bit field
    PCC_LPUART0 |= PCC_CLKCFG_PCS(2) ;  // Set clock source SCGIRCLK
    PCC_LPUART0 |= PCC_CLKCFG_CGC(1) ;  // Turn UART0 clock on
    
    // The pins are on port B
    PCC_PORTB |= PCC_CLKCFG_CGC(1) ;

    // Disable transmitter and receiver
    //   Required for configuration
    // UART0->C2 &= ~UART0_C2_TE_MASK & ~UART0_C2_RE_MASK ;
    LPUART0->CTRL &= ~LPUART_CTRL_TE_MASK & ~LPUART_CTRL_RE_MASK ;
    
    // Set UART0 clock to CPU clock
    //SIM->SOPT2 |= SIM_SOPT2_UART0SRC(1) ;
    //SIM->SOPT2 |= SIM_SOPT2_PLLFLLSEL_MASK ; // this does not work
    
    // Set pin multiplexing to UART0 Rx and Tx. Write 1 (w1c) to clear ISF
    PORTB->PCR[RxPIN] = PORT_PCR_ISF_MASK | PORT_PCR_MUX(3) ; 
    PORTB->PCR[TxPIN] = PORT_PCR_ISF_MASK | PORT_PCR_MUX(3) ; 
    
    // Set baud rate and over-sampling ratio
    //    Receiver clock multiplied by over-sampling 
    
    
    // The 13 bits in SBR[12:0] set the modulo divide rate for the baud rate generator. 
    // When SBR is 1 - 8191, the baud rate equals "baud clock / ((OSR+1) × SBR)".
    //  SBR = clock / ((OSR+1) * baud_rate)
    //
    // The following table shows a suitable OSR at 115200 baud
    // baud_rate  clock  OSR  SBR  Rounded    baud_rate  error
    // ---------------------------------------------------
    //  115200  8000000   5  11.574  12      111111.11    4088.89
    //  115200  8000000   6   9.921  10      114285.71     914.29 <------
    //  115200  8000000   7   8.681   9      111111.11    4088.89
    //  115200  8000000   8   7.716   8      111111.11    4088.89
    //  115200  8000000   9   6.944   7      114285.71     914.29
    //  115200  8000000  10   6.313   6      121212.12   -6012.12
    //
    // This table shows effect of OSR=6 at different baud rate
    // baud_rate  clock  OSR  SDR  Rounded  baud_rate  error 
    // ---------------------------------------------------
    //   9600   8000000   6  119.05  119      9603.84   -3.84
    //  19200   8000000   6   59.52   60     19047.62  152.38
    //  38400   8000000   6   29.76   30     38095.24  304.76
    //  57600   8000000   6   19.84   20     57142.86  457.14
    // 115200   8000000   6    9.92   10    114285.71  914.29 

#define OSR (6)                 // chosen to minimise rounding error at 115200

    // calculate sbr with rounding
    int sbr10, sbr10t ;
    sbr10  = (10 * PERI_CLOCK) / (baud_rate * (OSR + 1)) ;
    sbr10t = 10 * (PERI_CLOCK / (baud_rate * (OSR + 1))) ;
    uint16_t sbr = (uint16_t) (PERI_CLOCK / (baud_rate * (OSR + 1))) ;
    if (sbr10 - sbr10t > 5) sbr++ ;
    
    //UART0->BDH &= ~UART0_BDH_SBR_MASK ;
    //UART0->BDH |= UART0_BDH_SBR(sbr >> 8) ;
    //UART0->BDL = UART0_BDL_SBR(sbr) ;
    LPUART0->BAUD &= ~LPUART_BAUD_SBR_MASK ;
    LPUART0->BAUD |= LPUART_BAUD_SBR(sbr) ;
    
    // Set oversample rate
    //    Zero in invalid and causes 0xF to be written (page 803)
    //    This is a five bit field. The reset value is 0x0F.
    //    The minimum is 0x03

    // Clear bits of OSR set at reset but not needed to give the OSR required
    // This works only because bits of the desired OSR and with reset value
    LPUART0->BAUD &= (0xFFFFFFFF & ~LPUART_BAUD_OSR(15-OSR)) ;
    LPUART0->BAUD |= LPUART_BAUD_BOTHEDGE(1) ; // required - see page 803
    
    // Select one stop bit
    // Disable interrupts for Rx active edge and LIN break detect
    //UART0->BDH |= UART0_BDH_RXEDGIE(0) | UART0_BDH_SBNS(0) | UART0_BDH_LBKDIE(0) ; 
    // Code to clear these fields
    LPUART0->BAUD &= (~LPUART_BAUD_RXEDGIE_MASK) & (~LPUART_BAUD_SBNS_MASK) & (~LPUART_BAUD_LBKDIE_MASK) ;
    
    // No loop back; 8 data bits; No parity
    //UART0->C1 = UART0_C1_LOOPS(0) | UART0_C1_M(0) | UART0_C1_PE(0) ;
    LPUART0->CTRL &= (~LPUART_CTRL_LOOPS_MASK) & (~LPUART_CTRL_M_MASK) & (~LPUART_CTRL_PE_MASK) ;
    
       
    // No transmit invert; Enable interrupts for errors (why?)
    //UART0->C3 = UART0_C3_TXINV(0) | UART0_C3_ORIE(1) | UART0_C3_NEIE(1) | 
    //            UART0_C3_FEIE(1) | UART0_C3_PEIE(1) ;
    LPUART0->CTRL &= ~LPUART_CTRL_TXINV_MASK ;
    LPUART0->CTRL |= LPUART_CTRL_ORIE(1) | LPUART_CTRL_NEIE(1) | LPUART_CTRL_FEIE(1) | LPUART_CTRL_PEIE(1) ;

    // Clear error flags - these fields are w1c in a read register
    //UART0->S1 = UART0_S1_OR(1) | UART0_S1_NF(1) | UART0_S1_FE(1) | UART0_S1_PF(1)  ;
    LPUART0->STAT = LPUART_STAT_OR(1) | LPUART_STAT_NF(1) | LPUART_STAT_FE(1) | LPUART_STAT_PF(1) 
    
    // Transmit LSB first; Received data not inverted
    //UART0->S2 = UART0_S2_MSBF(0) | UART0_S2_RXINV(0) ;
                   | LPUART_STAT_MSBF(0) | LPUART_STAT_RXINV(0) ;  
    
    // Enable the interrupt
    NVIC_SetPriority(LPUART0_IRQn, 128) ;
    NVIC_ClearPendingIRQ(LPUART0_IRQn) ;
    NVIC_EnableIRQ(LPUART0_IRQn) ;
    
    // Enable the data received interrupt
    //UART0->C2 |= UART0_C2_RIE(1) ;
    LPUART0->CTRL |= LPUART_CTRL_RIE(1) ;

    // Enable transmitter and receiver
    //UART0->C2 |= UART0_C2_TE(1) | UART0_C2_RE(1) ;
    LPUART0->CTRL |= LPUART_CTRL_TE(1) | LPUART_CTRL_RE(1) ;
}

/* --------------------------------------
     Initialisation of tyhe serial port
        Call after the kernel initialisation
   -------------------------------------- */
void initSerialPort() {
    initSendMsg() ;
    initReadReq() ;
}

// =============Section 4: ISR =============================

/* --------------------------------
      UART0 Interrupt handler

   Interrupt sources
      - Receipt of a byte - receive data register full (RDRF)
      - Read error conditions
      - Transmit ready: data register empty (TDRE) 

   The transmit ready interrupt is disabled when there is nothing to send.
   To ensure that the interrupt is not repeated called, we need to clear the
     associated flags:
   
   -------------------------------- */

void LPUART0_IRQHandler(void) {
    char c ;
    
    // handle errors by reading character and discarding 
//    if (UART0->S1 & (UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK)) {
    if (LPUART0->STAT & (LPUART_STAT_OR_MASK | LPUART_STAT_NF_MASK | LPUART_STAT_FE_MASK | LPUART_STAT_PF_MASK)) {
        // read the character to clear RDRF
        c = LPUART0->DATA ; // resets the RDRF flag
        
        // reset the error flags
        //UART0->S1 |= UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK ;
        LPUART0->STAT |= LPUART_STAT_OR_MASK | LPUART_STAT_NF_MASK | LPUART_STAT_FE_MASK | LPUART_STAT_PF_MASK ;
    }
    
    // handle ready to transmit request
//    if ((UART0->C2 & UART0_C2_TIE_MASK) && (UART0->S1 & UART0_S1_TDRE_MASK)) {
    if ((LPUART0->CTRL & LPUART_CTRL_TIE_MASK) && (LPUART0->STAT & LPUART_STAT_TDRE_MASK)) {
        // Case 1: this request not finished
        c = getNextChar() ;
        if (c) {
            LPUART0->DATA = c ;
        
        // Case 2: next request            
        } else if (removeMsg()) {
            LPUART0->DATA = getNextChar() ;  // next request must have something

        // Case 3: request queue empty: disable transmission interrupt
        } else {
            // UART0->C2 &= ~UART0_C2_TIE_MASK ;
            LPUART0->CTRL &= ~LPUART_CTRL_TIE_MASK ;
        }
    }
    
    // handle character received 
    //if (UART0->S1 & UART0_S1_RDRF_MASK) {
    if (LPUART0->STAT & LPUART_STAT_RDRF_MASK) {
        c = LPUART0->DATA  ; // resets the RDRF flag
        
        // update buffer or discard char
        if (setNextChar(c)) {
            osEventFlagsSet(readFlags, READCOMPLETE);
        }
    }
}


