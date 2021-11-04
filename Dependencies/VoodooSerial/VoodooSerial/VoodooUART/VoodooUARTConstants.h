//
//  VoodooUARTConstants.h
//  BigSurface
//
//  Created by Xavier on 2021/9/23.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef VoodooUARTConstants_h
#define VoodooUARTConstants_h

// register length: 32 => 8*4 = 8<<2 = 8<<REG_SHIFT
#define REG_SHIFT 2
#define UART_CLOCK          64000000

#define LPSS_UART_CLK       0x00
#define UART_ENABLE_CLK     0x01
#define UART_UPDATE_CLK     BIT(31)
#define CALC_CLK(M, N)      (((N)&0x7F)<<16|((M)&0x7F)<<1)

/* Surface Pro 7 UART type 16550A specific register layout. See https://linux-sunxi.org/images/d/d2/Dw_apb_uart_db.pdf*/
#define DW_UART_RX  0x0                     /*receive buffer */
#define DW_UART_TX  0x0                     /*transmit buffer */
#define DW_UART_DLL 0x0

#define DW_UART_DLH (0x01<<REG_SHIFT)
#define DW_UART_IER (0x01<<REG_SHIFT)       /*Interrupt Enable Register*/

#define DW_UART_IIR (0x02<<REG_SHIFT)       /*Interrupt Identification Register READ!*/
#define DW_UART_FCR (0x02<<REG_SHIFT)       /*FIFO Control Register WRITE!*/

#define DW_UART_LCR (0x03<<REG_SHIFT)       /*Line Control Register */
#define DW_UART_MCR (0x04<<REG_SHIFT)       /*Modem Control Register */
#define DW_UART_LSR (0x05<<REG_SHIFT)       /*Line Status Register */
#define DW_UART_MSR (0x06<<REG_SHIFT)       /*Modem Status Register */

#define DW_UART_USR    (0x1F<<REG_SHIFT)    /* UART Status Register */
#define DW_UART_SRR    (0x22<<REG_SHIFT)    /* Software Reset Register */
#define DW_UART_CPR    (0x3D<<REG_SHIFT)    /* Component Parameter Register */
#define DW_UART_UCV    (0x3E<<REG_SHIFT)    /* UART Component Version */

/* Line Control Register bits*/
#define UART_LCR_DLAB        0x80 /* Divisor latch access bit */
#define UART_LCR_SBC        0x40 /* Set break control */
#define UART_LCR_SPAR        0x20 /* Stick parity (?) */
#define UART_LCR_EPAR        0x10 /* Even parity select */
#define UART_LCR_PARITY        0x08 /* Parity Enable */
#define UART_LCR_STOP        0x04 /* Stop bits: 0=1 bit, 1=2 bits */
#define UART_LCR_WLEN5        0x00 /* Wordlength: 5 bits */
#define UART_LCR_WLEN6        0x01 /* Wordlength: 6 bits */
#define UART_LCR_WLEN7        0x02 /* Wordlength: 7 bits */
#define UART_LCR_WLEN8        0x03 /* Wordlength: 8 bits */

/* Interrupt Enable Register bits */
#define UART_IER_ENABLE_THRE_INT_MODE   BIT(7)
#define UART_IER_ENABLE_MODEM_STA_INT   BIT(3) /* Enable Modem Status Interrupt */
#define UART_IER_ENABLE_ERR_INT         BIT(2) /* Enable Receiver Line Status Interrupt */
#define UART_IER_ENABLE_TX_EMPTY_INT    BIT(1) /* Enable Transmit Holding Register Empty Interrupt */
#define UART_IER_ENABLE_RX_AVAIL_INT    BIT(0) /* Enable Received Data Available Interrupt */
#define UART_IER_INT_MASK               0xFF

/* Interrupt Identification Register bits */
#define UART_IIR_XOFF_CHAR          BIT(4) /* OMAP XOFF/Special Character */
#define UART_IIR_CTS_RTS_DSR_CHANGE BIT(5) /* OMAP CTS/RTS/DSR Change */
#define UART_IIR_FIFO_ENABLED       (3<<6)
#define UART_IIR_CHAR_TIMEOUT_INT   0x0C /* Character timeout Interrupt*/
#define UART_IIR_BUSY_DETECT        0x07 /* DesignWare APB Busy Detect */
#define UART_IIR_RX_LINE_STA_INT    0x06 /* Receiver line status interrupt */
#define UART_IIR_DATA_AVAIL_INT     0x04 /* Receiver data interrupt */
#define UART_IIR_THR_EMPTY_INT      0x02 /* Transmitter holding register empty */
#define UART_IIR_NO_INT             0x01 /* No interrupts pending */
#define UART_IIR_MODEM_STA_INT      0x00 /* Modem status interrupt */
#define UART_IIR_INT_MASK           0x0F

/* FIFO Control Register bits */
#define UART_FCR_ENABLE_FIFO    BIT(0)
#define UART_FCR_CLEAR_RX_FIFO  BIT(1)
#define UART_FCR_CLEAR_TX_FIFO  BIT(2)

#define UART_FCR_R_TRIG_ONE_CHAR    (0x00<<6) /* 1 char in FIFO*/
#define UART_FCR_R_TRIG_QUARTER     (0x01<<6) /* FIFO 1/4 full*/
#define UART_FCR_R_TRIG_HALF        (0x10<<6) /* FIFO 1/2 full*/
#define UART_FCR_R_TRIG_TWO_LESS    (0x11<<6) /* FIFO 2 less than full*/
#define UART_FCR_T_TRIG_EMPTY       (0x00<<4) /* FIFO empty*/
#define UART_FCR_T_TRIG_TWO_CHAR    (0x01<<4) /* 2 char in FIFO*/
#define UART_FCR_T_TRIG_QUARTER     (0x10<<4) /* FIFO 1/4 full*/
#define UART_FCR_T_TRIG_HALF        (0x11<<4) /* FIFO 1/2 full*/

/* FIFO Control Register bits */
#define UART_MCR_AFC         BIT(5) /* Enable auto flow control mode */
#define UART_MCR_LOOP        BIT(4) /* Enable loopback test mode */
#define UART_MCR_OUT2        BIT(3) /* Out2 complement */
#define UART_MCR_OUT1        BIT(2) /* Out1 complement */
#define UART_MCR_RTS         BIT(1) /* RTS complement */
#define UART_MCR_DTR         BIT(0) /* DTR complement */

/* Line Status Register bits */
#define UART_LSR_RX_FIFO_ERR       BIT(7) /* Rx fifo error */
#define UART_LSR_TX_EMPTY          BIT(6) /* Transmitter empty */
#define UART_LSR_TX_FIFO_FULL      BIT(5) /* Transmit-hold-register empty */
#define UART_LSR_BREAK_INT         BIT(4) /* Break interrupt indicator */
#define UART_LSR_FRAME_ERR         BIT(3) /* Frame error indicator */
#define UART_LSR_PARITY_ERR        BIT(2) /* Parity error indicator */
#define UART_LSR_OVERRUN_ERR       BIT(1) /* Overrun error indicator */
#define UART_LSR_DATA_READY        BIT(0) /* Receiver data ready */
#define UART_LSR_BRK_ERROR_BITS    0x1E   /* BI, FE, PE, OE bits */

/* Modem Status Register bits */
#define UART_MSR_DCD         BIT(7) /* Data Carrier Detect */
#define UART_MSR_RI          BIT(6) /* Ring Indicator */
#define UART_MSR_DSR         BIT(5) /* Data Set Ready */
#define UART_MSR_CTS         BIT(4) /* Clear to Send */
#define UART_MSR_DDCD        BIT(3) /* Delta DCD */
#define UART_MSR_TERI        BIT(2) /* Trailing edge ring indicator */
#define UART_MSR_DDSR        BIT(1) /* Delta DSR */
#define UART_MSR_DCTS        BIT(0) /* Delta CTS */
#define UART_MSR_ANY_DELTA   0x0F /* Any of the delta bits! */

/* Component Parameter Register bits */
#define UART_CPR_ABP_DATA_WIDTH         0x03
#define UART_CPR_AFCE_MODE              BIT(4)
#define UART_CPR_THRE_MODE              BIT(5)
#define UART_CPR_SIR_MODE               BIT(6)
#define UART_CPR_SIR_LP_MODE            BIT(7)
#define UART_CPR_ADDITIONAL_FEATURES    BIT(8)
#define UART_CPR_FIFO_ACCESS            BIT(9)
#define UART_CPR_FIFO_STAT              BIT(10)
#define UART_CPR_SHADOW                 BIT(11)
#define UART_CPR_ENCODED_PARMS          BIT(12)
#define UART_CPR_DMA_EXTRA              BIT(13)
#define UART_CPR_FIFO_MODE              (0xff << 16)
/* Helper for FIFO size calculation */
#define DW_UART_CPR_FIFO_SIZE(a)    (((a >> 16) & 0xff) * 16)

/* UART Status Register bits */
#define UART_USR_BUSY               BIT(0)
#define UART_USR_TX_FIFO_NOT_FULL   BIT(1)
#define UART_USR_TX_FIFO_EMPTY      BIT(2)
#define UART_USR_RX_FIFO_NOT_EMPTY  BIT(3)
#define UART_USR_RX_FIFO_FULL       BIT(4)

#define TIMEOUT 20

#endif /* VoodooUARTConstants_h */
