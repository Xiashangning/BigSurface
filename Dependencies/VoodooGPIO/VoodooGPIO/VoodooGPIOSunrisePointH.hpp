//
//  VoodooGPIOSunrisePointH.h
//  VoodooGPIO
//
//  Created by CoolStar on 8/14/17.
//  Copyright © 2017 CoolStar. All rights reserved.
//

#include "VoodooGPIO.hpp"

#ifndef VoodooGPIOSunrisePointH_h
#define VoodooGPIOSunrisePointH_h

#define SPT_PAD_OWN     0x020
#define SPT_PADCFGLOCK  0x0a0
#define SPT_HOSTSW_OWN  0x0d0
#define SPT_GPI_IE      0x120

#define SPT_COMMUNITY(b, s, e)              \
{                                           \
    .barno = (b),                           \
    .padown_offset = SPT_PAD_OWN,           \
    .padcfglock_offset = SPT_PADCFGLOCK,    \
    .hostown_offset = SPT_HOSTSW_OWN,       \
    .ie_offset = SPT_GPI_IE,                \
    .gpp_size = 24,                         \
    .gpp_num_padown_regs = 4,               \
    .pin_base = (s),                        \
    .npins = ((e) - (s) + 1),               \
}

static struct pinctrl_pin_desc spth_pins[] = {
    /* GPP_A */
    PINCTRL_PIN(0, (char *)"RCINB"),
    PINCTRL_PIN(1, (char *)"LAD_0"),
    PINCTRL_PIN(2, (char *)"LAD_1"),
    PINCTRL_PIN(3, (char *)"LAD_2"),
    PINCTRL_PIN(4, (char *)"LAD_3"),
    PINCTRL_PIN(5, (char *)"LFRAMEB"),
    PINCTRL_PIN(6, (char *)"SERIQ"),
    PINCTRL_PIN(7, (char *)"PIRQAB"),
    PINCTRL_PIN(8, (char *)"CLKRUNB"),
    PINCTRL_PIN(9, (char *)"CLKOUT_LPC_0"),
    PINCTRL_PIN(10, (char *)"CLKOUT_LPC_1"),
    PINCTRL_PIN(11, (char *)"PMEB"),
    PINCTRL_PIN(12, (char *)"BM_BUSYB"),
    PINCTRL_PIN(13, (char *)"SUSWARNB_SUS_PWRDNACK"),
    PINCTRL_PIN(14, (char *)"SUS_STATB"),
    PINCTRL_PIN(15, (char *)"SUSACKB"),
    PINCTRL_PIN(16, (char *)"CLKOUT_48"),
    PINCTRL_PIN(17, (char *)"ISH_GP_7"),
    PINCTRL_PIN(18, (char *)"ISH_GP_0"),
    PINCTRL_PIN(19, (char *)"ISH_GP_1"),
    PINCTRL_PIN(20, (char *)"ISH_GP_2"),
    PINCTRL_PIN(21, (char *)"ISH_GP_3"),
    PINCTRL_PIN(22, (char *)"ISH_GP_4"),
    PINCTRL_PIN(23, (char *)"ISH_GP_5"),
    /* GPP_B */
    PINCTRL_PIN(24, (char *)"CORE_VID_0"),
    PINCTRL_PIN(25, (char *)"CORE_VID_1"),
    PINCTRL_PIN(26, (char *)"VRALERTB"),
    PINCTRL_PIN(27, (char *)"CPU_GP_2"),
    PINCTRL_PIN(28, (char *)"CPU_GP_3"),
    PINCTRL_PIN(29, (char *)"SRCCLKREQB_0"),
    PINCTRL_PIN(30, (char *)"SRCCLKREQB_1"),
    PINCTRL_PIN(31, (char *)"SRCCLKREQB_2"),
    PINCTRL_PIN(32, (char *)"SRCCLKREQB_3"),
    PINCTRL_PIN(33, (char *)"SRCCLKREQB_4"),
    PINCTRL_PIN(34, (char *)"SRCCLKREQB_5"),
    PINCTRL_PIN(35, (char *)"EXT_PWR_GATEB"),
    PINCTRL_PIN(36, (char *)"SLP_S0B"),
    PINCTRL_PIN(37, (char *)"PLTRSTB"),
    PINCTRL_PIN(38, (char *)"SPKR"),
    PINCTRL_PIN(39, (char *)"GSPI0_CSB"),
    PINCTRL_PIN(40, (char *)"GSPI0_CLK"),
    PINCTRL_PIN(41, (char *)"GSPI0_MISO"),
    PINCTRL_PIN(42, (char *)"GSPI0_MOSI"),
    PINCTRL_PIN(43, (char *)"GSPI1_CSB"),
    PINCTRL_PIN(44, (char *)"GSPI1_CLK"),
    PINCTRL_PIN(45, (char *)"GSPI1_MISO"),
    PINCTRL_PIN(46, (char *)"GSPI1_MOSI"),
    PINCTRL_PIN(47, (char *)"SML1ALERTB"),
    /* GPP_C */
    PINCTRL_PIN(48, (char *)"SMBCLK"),
    PINCTRL_PIN(49, (char *)"SMBDATA"),
    PINCTRL_PIN(50, (char *)"SMBALERTB"),
    PINCTRL_PIN(51, (char *)"SML0CLK"),
    PINCTRL_PIN(52, (char *)"SML0DATA"),
    PINCTRL_PIN(53, (char *)"SML0ALERTB"),
    PINCTRL_PIN(54, (char *)"SML1CLK"),
    PINCTRL_PIN(55, (char *)"SML1DATA"),
    PINCTRL_PIN(56, (char *)"UART0_RXD"),
    PINCTRL_PIN(57, (char *)"UART0_TXD"),
    PINCTRL_PIN(58, (char *)"UART0_RTSB"),
    PINCTRL_PIN(59, (char *)"UART0_CTSB"),
    PINCTRL_PIN(60, (char *)"UART1_RXD"),
    PINCTRL_PIN(61, (char *)"UART1_TXD"),
    PINCTRL_PIN(62, (char *)"UART1_RTSB"),
    PINCTRL_PIN(63, (char *)"UART1_CTSB"),
    PINCTRL_PIN(64, (char *)"I2C0_SDA"),
    PINCTRL_PIN(65, (char *)"I2C0_SCL"),
    PINCTRL_PIN(66, (char *)"I2C1_SDA"),
    PINCTRL_PIN(67, (char *)"I2C1_SCL"),
    PINCTRL_PIN(68, (char *)"UART2_RXD"),
    PINCTRL_PIN(69, (char *)"UART2_TXD"),
    PINCTRL_PIN(70, (char *)"UART2_RTSB"),
    PINCTRL_PIN(71, (char *)"UART2_CTSB"),
    /* GPP_D */
    PINCTRL_PIN(72, (char *)"SPI1_CSB"),
    PINCTRL_PIN(73, (char *)"SPI1_CLK"),
    PINCTRL_PIN(74, (char *)"SPI1_MISO_IO_1"),
    PINCTRL_PIN(75, (char *)"SPI1_MOSI_IO_0"),
    PINCTRL_PIN(76, (char *)"ISH_I2C2_SDA"),
    PINCTRL_PIN(77, (char *)"SSP0_SFRM"),
    PINCTRL_PIN(78, (char *)"SSP0_TXD"),
    PINCTRL_PIN(79, (char *)"SSP0_RXD"),
    PINCTRL_PIN(80, (char *)"SSP0_SCLK"),
    PINCTRL_PIN(81, (char *)"ISH_SPI_CSB"),
    PINCTRL_PIN(82, (char *)"ISH_SPI_CLK"),
    PINCTRL_PIN(83, (char *)"ISH_SPI_MISO"),
    PINCTRL_PIN(84, (char *)"ISH_SPI_MOSI"),
    PINCTRL_PIN(85, (char *)"ISH_UART0_RXD"),
    PINCTRL_PIN(86, (char *)"ISH_UART0_TXD"),
    PINCTRL_PIN(87, (char *)"ISH_UART0_RTSB"),
    PINCTRL_PIN(88, (char *)"ISH_UART0_CTSB"),
    PINCTRL_PIN(89, (char *)"DMIC_CLK_1"),
    PINCTRL_PIN(90, (char *)"DMIC_DATA_1"),
    PINCTRL_PIN(91, (char *)"DMIC_CLK_0"),
    PINCTRL_PIN(92, (char *)"DMIC_DATA_0"),
    PINCTRL_PIN(93, (char *)"SPI1_IO_2"),
    PINCTRL_PIN(94, (char *)"SPI1_IO_3"),
    PINCTRL_PIN(95, (char *)"ISH_I2C2_SCL"),
    /* GPP_E */
    PINCTRL_PIN(96, (char *)"SATAXPCIE_0"),
    PINCTRL_PIN(97, (char *)"SATAXPCIE_1"),
    PINCTRL_PIN(98, (char *)"SATAXPCIE_2"),
    PINCTRL_PIN(99, (char *)"CPU_GP_0"),
    PINCTRL_PIN(100, (char *)"SATA_DEVSLP_0"),
    PINCTRL_PIN(101, (char *)"SATA_DEVSLP_1"),
    PINCTRL_PIN(102, (char *)"SATA_DEVSLP_2"),
    PINCTRL_PIN(103, (char *)"CPU_GP_1"),
    PINCTRL_PIN(104, (char *)"SATA_LEDB"),
    PINCTRL_PIN(105, (char *)"USB2_OCB_0"),
    PINCTRL_PIN(106, (char *)"USB2_OCB_1"),
    PINCTRL_PIN(107, (char *)"USB2_OCB_2"),
    PINCTRL_PIN(108, (char *)"USB2_OCB_3"),
    /* GPP_F */
    PINCTRL_PIN(109, (char *)"SATAXPCIE_3"),
    PINCTRL_PIN(110, (char *)"SATAXPCIE_4"),
    PINCTRL_PIN(111, (char *)"SATAXPCIE_5"),
    PINCTRL_PIN(112, (char *)"SATAXPCIE_6"),
    PINCTRL_PIN(113, (char *)"SATAXPCIE_7"),
    PINCTRL_PIN(114, (char *)"SATA_DEVSLP_3"),
    PINCTRL_PIN(115, (char *)"SATA_DEVSLP_4"),
    PINCTRL_PIN(116, (char *)"SATA_DEVSLP_5"),
    PINCTRL_PIN(117, (char *)"SATA_DEVSLP_6"),
    PINCTRL_PIN(118, (char *)"SATA_DEVSLP_7"),
    PINCTRL_PIN(119, (char *)"SATA_SCLOCK"),
    PINCTRL_PIN(120, (char *)"SATA_SLOAD"),
    PINCTRL_PIN(121, (char *)"SATA_SDATAOUT1"),
    PINCTRL_PIN(122, (char *)"SATA_SDATAOUT0"),
    PINCTRL_PIN(123, (char *)"GPP_F_14"),
    PINCTRL_PIN(124, (char *)"USB_OCB_4"),
    PINCTRL_PIN(125, (char *)"USB_OCB_5"),
    PINCTRL_PIN(126, (char *)"USB_OCB_6"),
    PINCTRL_PIN(127, (char *)"USB_OCB_7"),
    PINCTRL_PIN(128, (char *)"L_VDDEN"),
    PINCTRL_PIN(129, (char *)"L_BKLTEN"),
    PINCTRL_PIN(130, (char *)"L_BKLTCTL"),
    PINCTRL_PIN(131, (char *)"GPP_F_22"),
    PINCTRL_PIN(132, (char *)"GPP_F_23"),
    /* GPP_G */
    PINCTRL_PIN(133, (char *)"FAN_TACH_0"),
    PINCTRL_PIN(134, (char *)"FAN_TACH_1"),
    PINCTRL_PIN(135, (char *)"FAN_TACH_2"),
    PINCTRL_PIN(136, (char *)"FAN_TACH_3"),
    PINCTRL_PIN(137, (char *)"FAN_TACH_4"),
    PINCTRL_PIN(138, (char *)"FAN_TACH_5"),
    PINCTRL_PIN(139, (char *)"FAN_TACH_6"),
    PINCTRL_PIN(140, (char *)"FAN_TACH_7"),
    PINCTRL_PIN(141, (char *)"FAN_PWM_0"),
    PINCTRL_PIN(142, (char *)"FAN_PWM_1"),
    PINCTRL_PIN(143, (char *)"FAN_PWM_2"),
    PINCTRL_PIN(144, (char *)"FAN_PWM_3"),
    PINCTRL_PIN(145, (char *)"GSXDOUT"),
    PINCTRL_PIN(146, (char *)"GSXSLOAD"),
    PINCTRL_PIN(147, (char *)"GSXDIN"),
    PINCTRL_PIN(148, (char *)"GSXRESETB"),
    PINCTRL_PIN(149, (char *)"GSXCLK"),
    PINCTRL_PIN(150, (char *)"ADR_COMPLETE"),
    PINCTRL_PIN(151, (char *)"NMIB"),
    PINCTRL_PIN(152, (char *)"SMIB"),
    PINCTRL_PIN(153, (char *)"GPP_G_20"),
    PINCTRL_PIN(154, (char *)"GPP_G_21"),
    PINCTRL_PIN(155, (char *)"GPP_G_22"),
    PINCTRL_PIN(156, (char *)"GPP_G_23"),
    /* GPP_H */
    PINCTRL_PIN(157, (char *)"SRCCLKREQB_6"),
    PINCTRL_PIN(158, (char *)"SRCCLKREQB_7"),
    PINCTRL_PIN(159, (char *)"SRCCLKREQB_8"),
    PINCTRL_PIN(160, (char *)"SRCCLKREQB_9"),
    PINCTRL_PIN(161, (char *)"SRCCLKREQB_10"),
    PINCTRL_PIN(162, (char *)"SRCCLKREQB_11"),
    PINCTRL_PIN(163, (char *)"SRCCLKREQB_12"),
    PINCTRL_PIN(164, (char *)"SRCCLKREQB_13"),
    PINCTRL_PIN(165, (char *)"SRCCLKREQB_14"),
    PINCTRL_PIN(166, (char *)"SRCCLKREQB_15"),
    PINCTRL_PIN(167, (char *)"SML2CLK"),
    PINCTRL_PIN(168, (char *)"SML2DATA"),
    PINCTRL_PIN(169, (char *)"SML2ALERTB"),
    PINCTRL_PIN(170, (char *)"SML3CLK"),
    PINCTRL_PIN(171, (char *)"SML3DATA"),
    PINCTRL_PIN(172, (char *)"SML3ALERTB"),
    PINCTRL_PIN(173, (char *)"SML4CLK"),
    PINCTRL_PIN(174, (char *)"SML4DATA"),
    PINCTRL_PIN(175, (char *)"SML4ALERTB"),
    PINCTRL_PIN(176, (char *)"ISH_I2C0_SDA"),
    PINCTRL_PIN(177, (char *)"ISH_I2C0_SCL"),
    PINCTRL_PIN(178, (char *)"ISH_I2C1_SDA"),
    PINCTRL_PIN(179, (char *)"ISH_I2C1_SCL"),
    PINCTRL_PIN(180, (char *)"GPP_H_23"),
    /* GPP_I */
    PINCTRL_PIN(181, (char *)"DDSP_HDP_0"),
    PINCTRL_PIN(182, (char *)"DDSP_HDP_1"),
    PINCTRL_PIN(183, (char *)"DDSP_HDP_2"),
    PINCTRL_PIN(184, (char *)"DDSP_HDP_3"),
    PINCTRL_PIN(185, (char *)"EDP_HPD"),
    PINCTRL_PIN(186, (char *)"DDPB_CTRLCLK"),
    PINCTRL_PIN(187, (char *)"DDPB_CTRLDATA"),
    PINCTRL_PIN(188, (char *)"DDPC_CTRLCLK"),
    PINCTRL_PIN(189, (char *)"DDPC_CTRLDATA"),
    PINCTRL_PIN(190, (char *)"DDPD_CTRLCLK"),
    PINCTRL_PIN(191, (char *)"DDPD_CTRLDATA"),
};

static unsigned spth_spi0_pins[] = { 39, 40, 41, 42 };
static unsigned spth_spi1_pins[] = { 43, 44, 45, 46 };
static unsigned spth_uart0_pins[] = { 56, 57, 58, 59 };
static unsigned spth_uart1_pins[] = { 60, 61, 62, 63 };
static unsigned spth_uart2_pins[] = { 68, 69, 71, 71 };
static unsigned spth_i2c0_pins[] = { 64, 65 };
static unsigned spth_i2c1_pins[] = { 66, 67 };
static unsigned spth_i2c2_pins[] = { 76, 95 };

static struct intel_pingroup spth_groups[] = {
    PIN_GROUP((char *)"spi0_grp", spth_spi0_pins, 1),
    PIN_GROUP((char *)"spi1_grp", spth_spi1_pins, 1),
    PIN_GROUP((char *)"uart0_grp", spth_uart0_pins, 1),
    PIN_GROUP((char *)"uart1_grp", spth_uart1_pins, 1),
    PIN_GROUP((char *)"uart2_grp", spth_uart2_pins, 1),
    PIN_GROUP((char *)"i2c0_grp", spth_i2c0_pins, 1),
    PIN_GROUP((char *)"i2c1_grp", spth_i2c1_pins, 1),
    PIN_GROUP((char *)"i2c2_grp", spth_i2c2_pins, 2),
};

static char * const spth_spi0_groups[] = { (char *)"spi0_grp" };
static char * const spth_spi1_groups[] = { (char *)"spi0_grp" };
static char * const spth_uart0_groups[] = { (char *)"uart0_grp" };
static char * const spth_uart1_groups[] = { (char *)"uart1_grp" };
static char * const spth_uart2_groups[] = { (char *)"uart2_grp" };
static char * const spth_i2c0_groups[] = { (char *)"i2c0_grp" };
static char * const spth_i2c1_groups[] = { (char *)"i2c1_grp" };
static char * const spth_i2c2_groups[] = { (char *)"i2c2_grp" };

static struct intel_function spth_functions[] = {
    FUNCTION((char *)"spi0", spth_spi0_groups),
    FUNCTION((char *)"spi1", spth_spi1_groups),
    FUNCTION((char *)"uart0", spth_uart0_groups),
    FUNCTION((char *)"uart1", spth_uart1_groups),
    FUNCTION((char *)"uart2", spth_uart2_groups),
    FUNCTION((char *)"i2c0", spth_i2c0_groups),
    FUNCTION((char *)"i2c1", spth_i2c1_groups),
    FUNCTION((char *)"i2c2", spth_i2c2_groups),
};

static struct intel_community spth_communities[] = {
    SPT_COMMUNITY(0, 0, 47),
    SPT_COMMUNITY(1, 48, 180),
    SPT_COMMUNITY(2, 181, 191),
};

class VoodooGPIOSunrisePointH : public VoodooGPIO {
    OSDeclareDefaultStructors(VoodooGPIOSunrisePointH);

    bool start(IOService *provider) override;
};

#endif /* VoodooGPIOSunrisePointH_h */
