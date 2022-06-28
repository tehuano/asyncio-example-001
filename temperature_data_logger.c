//******************************************************************************
// The facilities team is in need of a temperature data logger to determine
// if we need to upgrade the air conditioning at our new facility. Using your
// favorite embedded platform, set up a project and write the code necessary
// to measure temperature using TI’s TMP100 I2C temperature sensor and log the
// temperature every 10 minutes to a Microchip 24FC256 eeprom. State any
// assumptions you need to make, you can use whatever libraries are available
// for your platform of choice from the platform vendor.
// (for example I2C libraries are ok, drivers for the TMP100 are not)
//
//******************************************************************************
//   MSP430F552x Demo - USCI_B0, I2C Master multiple byte TX/RX
//
//   Description: I2C master communicates to I2C slave sending and receiving
//   3 different messages of different length. I2C master will enter LPM0 mode
//   while waiting for the messages to be sent/receiving using I2C interrupt.
//   ACLK = NA, MCLK = SMCLK = DCO 16MHz.
//
//                                     /|\ /|\
//                   MSP430F5529       4.7k |
//                 -----------------    |  4.7k
//            /|\ |             P3.1|---+---|-- I2C Clock (UCB0SCL)
//             |  |                 |       |
//             ---|RST          P3.0|-------+-- I2C Data (UCB0SDA)
//                |                 |
//                |                 |
//                |                 |
//                |                 |
//                |                 |
//                |                 |
//
//******************************************************************************
//         /|\           /|\ /|\
//          |   TMP100   10k 10k     MSP430F5529
//          |   -------   |   |   -------------------
//          +--|Vcc SDA|<-|---+->|P3.1/UCB0SDA    XIN|-
//          |  |       |  |      |                   |
//          +--|A1,A0  |  |      |               XOUT|-
//             |       |  |      |                   |
//          +--|Vss SCL|<-+------|P3.2/UCB0SCL   P1.0|---> LED
//         \|/  -------          |                   |
//
//******************************************************************************

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#define TMP100_ADDR 0x4F
#define EEPROM24FC256_ADDR 0x57

#define MAX_BUFFER_SIZE 20

uint8_t temperature_reg_addr = 0x00;
uint8_t temperature_read_buffer[2] = {0, 0};
uint8_t addr_and_data_eeprom [4] = {0, 0, 0, 0};
uint8_t add_stop = 1;

//******************************************************************************
// General I2C State Machine ***************************************************
//******************************************************************************

typedef enum I2C_ModeEnum{
    IDLE_MODE,
    NACK_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITHC_TO_TX_MODE,
    TIMEOUT_MODE
} I2C_Mode;

/* Used to track the state of the software state machine*/
I2C_Mode MasterMode = IDLE_MODE;

/* The Register Address/Command to use*/
uint8_t TransmitRegAddr = 0;

/* ReceiveBuffer: Buffer used to receive data in the ISR
 * RXByteCtr: Number of bytes left to receive
 * ReceiveIndex: The index of the next byte to be received in ReceiveBuffer
 * TransmitBuffer: Buffer used to transmit data in the ISR
 * TXByteCtr: Number of bytes left to transfer
 * TransmitIndex: The index of the next byte to be transmitted in TransmitBuffer
 * */
uint8_t ReceiveBuffer[MAX_BUFFER_SIZE] = {0};
uint8_t RXByteCtr = 0;
uint8_t ReceiveIndex = 0;
uint8_t TransmitBuffer[MAX_BUFFER_SIZE] = {0};
uint8_t TXByteCtr = 0;
uint8_t TransmitIndex = 0;

/* I2C Write and Read Functions */

/* For slave device with dev_addr, writes the data specified in *reg_data
 *
 * dev_addr: The slave device address.
 *           Example: SLAVE_ADDR
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_MASTER
 * *reg_data: The buffer to write
 *           Example: MasterType0
 * count: The length of *reg_data
 *           Example: TYPE_0_LENGTH
 */

/* For slave device with dev_addr, read the data specified in slaves reg_addr.
 * The received data is available in ReceiveBuffer
 *
 * dev_addr: The slave device address.
 *           Example: SLAVE_ADDR
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_SLAVE
 * count: The length of data to read
 *           Example: TYPE_0_LENGTH
 */

I2C_Mode I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
I2C_Mode I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count);
void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count);
bool increaseVCoreToLevel2();
void initClockTo16MHz();
void initGPIO();
void initI2C();

//******************************************************************************
// Main ************************************************************************
// Send and receive three messages containing the example commands *************
//******************************************************************************

int main(void) {
    unsigned char adr_hi;
    unsigned char adr_lo;
    unsigned int Address = 0x0000;
    unsigned int i, j;

    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

    increaseVCoreToLevel2();
    initClockTo16MHz();
    initGPIO();
    initI2C();

    while (1) {
      for (j = 10; j > 0; j--) {
        for (i = 60; i > 0; i--) {
          __delay_cycles(16000000); // since clock is 16MHz, this delay is one sec
        }
      }
      // temperature_reg_addr = 0x00
      add_stop = 0;
      I2C_Master_WriteReg(TMP100_ADDR, temperature_reg_addr, temperature_read_buffer, 0);
      add_stop = 1;
      I2C_Master_ReadReg(TMP100_ADDR, temperature_reg_addr, 2);
      CopyArray(ReceiveBuffer, temperature_read_buffer, 2);

      adr_hi = Address >> 8;                    // calculate high byte
      adr_lo = Address & 0xFF;                  // and low byte of address

      addr_and_data_eeprom[2] = temperature_read_buffer[0];
      addr_and_data_eeprom[1] = temperature_read_buffer[1];
      addr_and_data_eeprom[0] = adr_lo;

      add_stop = 1;
      I2C_Master_WriteReg(EEPROM24FC256_ADDR, adr_hi, addr_and_data_eeprom, 3);

      Address = Address + 1;
    }

    __bis_SR_register(LPM0_bits + GIE);
    return 0;
}

//******************************************************************************
// I2C Interrupt ***************************************************************
//******************************************************************************

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCI_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  //Must read from UCB0RXBUF
  uint8_t rx_val = 0;

  switch(__even_in_range(UCB0IV,0xC)) {
    case USCI_NONE:break;                             // Vector 0 - no interrupt
    case USCI_I2C_UCALIFG:break;                      // Interrupt Vector: I2C Mode: UCALIFG
    case USCI_I2C_UCNACKIFG:break;                    // Interrupt Vector: I2C Mode: UCNACKIFG
    case USCI_I2C_UCSTTIFG:break;                     // Interrupt Vector: I2C Mode: UCSTTIFG
    case USCI_I2C_UCSTPIFG:break;                     // Interrupt Vector: I2C Mode: UCSTPIFG
    case USCI_I2C_UCRXIFG:
        rx_val = UCB0RXBUF;
        if (RXByteCtr) {
          ReceiveBuffer[ReceiveIndex++] = rx_val;
          RXByteCtr--;
        }
        if (RXByteCtr == 1) {
          UCB0CTL1 |= UCTXSTP;
        } else if (RXByteCtr == 0) {
          UCB0IE &= ~UCRXIE;
          MasterMode = IDLE_MODE;
          __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
        }
        break;                      // Interrupt Vector: I2C Mode: UCRXIFG
    case USCI_I2C_UCTXIFG:
        switch (MasterMode) {
          case TX_REG_ADDRESS_MODE:
              UCB0TXBUF = TransmitRegAddr;
              if (RXByteCtr) {
                  MasterMode = SWITCH_TO_RX_MODE;   // Need to start receiving now
              } else {
                  MasterMode = TX_DATA_MODE;        // Continue to transmission with the data in Transmit Buffer
              }
              break;
          case SWITCH_TO_RX_MODE:
              UCB0IE |= UCRXIE;             // Enable RX interrupt
              UCB0IE &= ~UCTXIE;            // Disable TX interrupt
              UCB0CTL1 &= ~UCTR;            // Switch to receiver
              MasterMode = RX_DATA_MODE;    // State state is to receive data
              UCB0CTL1 |= UCTXSTT;          // Send repeated start
              if (RXByteCtr == 1) {
                  //Must send stop since this is the N-1 byte
                  while((UCB0CTL1 & UCTXSTT));
                  UCB0CTL1 |= UCTXSTP;      // Send stop condition
              }
              break;
          case TX_DATA_MODE:
              if (TXByteCtr) {
                  UCB0TXBUF = TransmitBuffer[TransmitIndex++];
                  TXByteCtr--;
              } else {
                  //Done with transmission
                  if (add_stop == 1) {
                    UCB0CTL1 |= UCTXSTP;                // Send stop condition
                    add_stop = 0;
                  }
                  MasterMode = IDLE_MODE;
                  UCB0IE &= ~UCTXIE;                  // disable TX interrupt
                  __bic_SR_register_on_exit(CPUOFF);  // Exit LPM0
              }
              break;
          default:
              __no_operation();
              break;
        }
        break;                      // Interrupt Vector: I2C Mode: UCTXIFG
    default: break;
  }
}

I2C_Mode I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count) {
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;
    RXByteCtr = count;
    TXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB0I2CSA = dev_addr;
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);         // Clear any pending interrupts
    UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB0IE |= UCTXIE;                        // Enable TX interrupt

    UCB0CTL1 |= UCTR + UCTXSTT;              // I2C TX, start condition
    __bis_SR_register(LPM0_bits + GIE);      // Enter LPM0 w/ interrupts

    return MasterMode;
}

I2C_Mode I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count) {
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;

    //Copy register data to TransmitBuffer
    CopyArray(reg_data, TransmitBuffer, count);

    TXByteCtr = count;
    RXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB0I2CSA = dev_addr;
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);         // Clear any pending interrupts
    UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB0IE |= UCTXIE;                        // Enable TX interrupt

    UCB0CTL1 |= UCTR + UCTXSTT;              // I2C TX, start condition
    __bis_SR_register(LPM0_bits + GIE);      // Enter LPM0 w/ interrupts

    return MasterMode;
}

void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count) {
    uint8_t copyIndex = 0;
    for (copyIndex = 0; copyIndex < count; copyIndex++) {
        dest[copyIndex] = source[copyIndex];
    }
}

//******************************************************************************
// Device Initialization *******************************************************
//******************************************************************************

void initClockTo16MHz() {
    UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO
    UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
    UCSCTL2 = FLLD_0 + 487;                   // Set DCO Multiplier for 16MHz
                                              // (N + 1) * FLLRef = Fdco
                                              // (487 + 1) * 32768 = 16MHz
                                              // Set FLL Div = fDCOCLK
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // UG for optimization.
    // 32 x 32 x 16 MHz / 32,768 Hz = 500000 = MCLK cycles for DCO to settle
    __delay_cycles(500000);//
    // Loop until XT1,XT2 & DCO fault flag is cleared
    do {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG); // Clear XT2,XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                          // Clear fault flags
    } while (SFRIFG1&OFIFG);                         // Test oscillator fault flag
}

uint16_t setVCoreUp(uint8_t level) {
    uint32_t PMMRIE_backup, SVSMHCTL_backup, SVSMLCTL_backup;

    //The code flow for increasing the Vcore has been altered to work around
    //the erratum FLASH37.
    //Please refer to the Errata sheet to know if a specific device is affected
    //DO NOT ALTER THIS FUNCTION

    //Open PMM registers for write access
    PMMCTL0_H = 0xA5;

    //Disable dedicated Interrupts
    //Backup all registers
    PMMRIE_backup = PMMRIE;
    PMMRIE &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE |
                SVSLPE | SVMHVLRIE | SVMHIE |
                SVSMHDLYIE | SVMLVLRIE | SVMLIE |
                SVSMLDLYIE
                );
    SVSMHCTL_backup = SVSMHCTL;
    SVSMLCTL_backup = SVSMLCTL;

    //Clear flags
    PMMIFG = 0;

    //Set SVM highside to new level and check if a VCore increase is possible
    SVSMHCTL = SVMHE | SVSHE | (SVSMHRRL0 * level);

    //Wait until SVM highside is settled
    while((PMMIFG & SVSMHDLYIFG) == 0)
    {
        ;
    }

    //Clear flag
    PMMIFG &= ~SVSMHDLYIFG;

    //Check if a VCore increase is possible
    if((PMMIFG & SVMHIFG) == SVMHIFG)
    {
        //-> Vcc is too low for a Vcore increase
        //recover the previous settings
        PMMIFG &= ~SVSMHDLYIFG;
        SVSMHCTL = SVSMHCTL_backup;

        //Wait until SVM highside is settled
        while((PMMIFG & SVSMHDLYIFG) == 0)
        {
            ;
        }

        //Clear all Flags
        PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                     SVMLVLRIFG | SVMLIFG |
                     SVSMLDLYIFG
                     );

        //Restore PMM interrupt enable register
        PMMRIE = PMMRIE_backup;
        //Lock PMM registers for write access
        PMMCTL0_H = 0x00;
        //return: voltage not set
        return false;
    }

    //Set also SVS highside to new level
    //Vcc is high enough for a Vcore increase
    SVSMHCTL |= (SVSHRVL0 * level);

    //Wait until SVM highside is settled
    while((PMMIFG & SVSMHDLYIFG) == 0)
    {
        ;
    }

    //Clear flag
    PMMIFG &= ~SVSMHDLYIFG;

    //Set VCore to new level
    PMMCTL0_L = PMMCOREV0 * level;

    //Set SVM, SVS low side to new level
    SVSMLCTL = SVMLE | (SVSMLRRL0 * level) |
               SVSLE | (SVSLRVL0 * level);

    //Wait until SVM, SVS low side is settled
    while((PMMIFG & SVSMLDLYIFG) == 0)
    {
        ;
    }

    //Clear flag
    PMMIFG &= ~SVSMLDLYIFG;
    //SVS, SVM core and high side are now set to protect for the new core level

    //Restore Low side settings
    //Clear all other bits _except_ level settings
    SVSMLCTL &= (SVSLRVL0 + SVSLRVL1 + SVSMLRRL0 +
                 SVSMLRRL1 + SVSMLRRL2
                 );

    //Clear level settings in the backup register,keep all other bits
    SVSMLCTL_backup &=
        ~(SVSLRVL0 + SVSLRVL1 + SVSMLRRL0 + SVSMLRRL1 + SVSMLRRL2);

    //Restore low-side SVS monitor settings
    SVSMLCTL |= SVSMLCTL_backup;

    //Restore High side settings
    //Clear all other bits except level settings
    SVSMHCTL &= (SVSHRVL0 + SVSHRVL1 +
                 SVSMHRRL0 + SVSMHRRL1 +
                 SVSMHRRL2
                 );

    //Clear level settings in the backup register,keep all other bits
    SVSMHCTL_backup &=
        ~(SVSHRVL0 + SVSHRVL1 + SVSMHRRL0 + SVSMHRRL1 + SVSMHRRL2);

    //Restore backup
    SVSMHCTL |= SVSMHCTL_backup;

    //Wait until high side, low side settled
    while(((PMMIFG & SVSMLDLYIFG) == 0) &&
          ((PMMIFG & SVSMHDLYIFG) == 0))
    {
        ;
    }

    //Clear all Flags
    PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG
                );

    //Restore PMM interrupt enable register
    PMMRIE = PMMRIE_backup;

    //Lock PMM registers for write access
    PMMCTL0_H = 0x00;

    return true;
}

bool increaseVCoreToLevel2() {
    uint8_t level = 2;
    uint8_t actlevel;
    bool status = true;

    //Set Mask for Max. level
    level &= PMMCOREV_3;

    //Get actual VCore
    actlevel = PMMCTL0 & PMMCOREV_3;

    //step by step increase or decrease
    while((level != actlevel) && (status == true))
    {
        if(level > actlevel)
        {
            status = setVCoreUp(++actlevel);
        }
    }

    return (status);
}

void initGPIO() {
    //LEDs
    P1OUT = 0x00;                             // P1 setup for LED & reset output
    P1DIR |= BIT0;

    P4DIR |= BIT7;
    P4OUT &= ~(BIT7);

    //I2C Pins
    P3SEL |= BIT0 + BIT1;                     // P3.0,1 option select

}

void initI2C() {
    UCB0CTL1 |= UCSWRST;                      // Enable SW reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
    UCB0BR0 = 160;                            // fSCL = SMCLK/160 = ~100kHz
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
    UCB0IE |= UCNACKIE;
}
