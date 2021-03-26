//
//  VoodooGPIOCannonLakeH.h
//  VoodooGPIO
//
//  Created by Alexandre Daoud on 15/11/18.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "../VoodooGPIO.hpp"

#ifndef VoodooGPIOCannonLakeH_h
#define VoodooGPIOCannonLakeH_h

#define CNL_PAD_OWN         0x020
#define CNL_PADCFGLOCK      0x080
#define CNL_H_HOSTSW_OWN    0x0c0
#define CNL_GPI_IE          0x120

#define CNL_GPP(r, s, e, g)     \
{                               \
    .reg_num = (r),             \
    .base = (s),                \
    .size = ((e) - (s) + 1),    \
    .gpio_base = (g),           \
}

#define CNL_NO_GPIO     -1

#define CNL_COMMUNITY(b, s, e, o, g)        \
{                                           \
    .barno = (b),                           \
    .padown_offset = CNL_PAD_OWN,           \
    .padcfglock_offset = CNL_PADCFGLOCK,    \
    .hostown_offset = (o),                  \
    .ie_offset = CNL_GPI_IE,                \
    .pin_base = (s),                        \
    .npins = ((e) - (s) + 1),               \
    .gpps = (g),                            \
    .ngpps = ARRAY_SIZE(g),                 \
}

#define CNLH_COMMUNITY(b, s, e, g)          \
        CNL_COMMUNITY(b, s, e, CNL_H_HOSTSW_OWN, g)

/* Cannon Lake-H */
static struct pinctrl_pin_desc cnlh_pins[] = {
    /* GPP_A */
    PINCTRL_PIN(0, (char *)"RCINB"),
    PINCTRL_PIN(1, (char *)"LAD_0"),
    PINCTRL_PIN(2, (char *)"LAD_1"),
    PINCTRL_PIN(3, (char *)"LAD_2"),
    PINCTRL_PIN(4, (char *)"LAD_3"),
    PINCTRL_PIN(5, (char *)"LFRAMEB"),
    PINCTRL_PIN(6, (char *)"SERIRQ"),
    PINCTRL_PIN(7, (char *)"PIRQAB"),
    PINCTRL_PIN(8, (char *)"CLKRUNB"),
    PINCTRL_PIN(9, (char *)"CLKOUT_LPC_0"),
    PINCTRL_PIN(10, (char *)"CLKOUT_LPC_1"),
    PINCTRL_PIN(11, (char *)"PMEB"),
    PINCTRL_PIN(12, (char *)"BM_BUSYB"),
    PINCTRL_PIN(13, (char *)"SUSWARNB_SUSPWRDNACK"),
    PINCTRL_PIN(14, (char *)"SUS_STATB"),
    PINCTRL_PIN(15, (char *)"SUSACKB"),
    PINCTRL_PIN(16, (char *)"CLKOUT_48"),
    PINCTRL_PIN(17, (char *)"SD_VDD1_PWR_EN_B"),
    PINCTRL_PIN(18, (char *)"ISH_GP_0"),
    PINCTRL_PIN(19, (char *)"ISH_GP_1"),
    PINCTRL_PIN(20, (char *)"ISH_GP_2"),
    PINCTRL_PIN(21, (char *)"ISH_GP_3"),
    PINCTRL_PIN(22, (char *)"ISH_GP_4"),
    PINCTRL_PIN(23, (char *)"ISH_GP_5"),
    PINCTRL_PIN(24, (char *)"ESPI_CLK_LOOPBK"),
    /* GPP_B */
    PINCTRL_PIN(25, (char *)"GSPI0_CS1B"),
    PINCTRL_PIN(26, (char *)"GSPI1_CS1B"),
    PINCTRL_PIN(27, (char *)"VRALERTB"),
    PINCTRL_PIN(28, (char *)"CPU_GP_2"),
    PINCTRL_PIN(29, (char *)"CPU_GP_3"),
    PINCTRL_PIN(30, (char *)"SRCCLKREQB_0"),
    PINCTRL_PIN(31, (char *)"SRCCLKREQB_1"),
    PINCTRL_PIN(32, (char *)"SRCCLKREQB_2"),
    PINCTRL_PIN(33, (char *)"SRCCLKREQB_3"),
    PINCTRL_PIN(34, (char *)"SRCCLKREQB_4"),
    PINCTRL_PIN(35, (char *)"SRCCLKREQB_5"),
    PINCTRL_PIN(36, (char *)"SSP_MCLK"),
    PINCTRL_PIN(37, (char *)"SLP_S0B"),
    PINCTRL_PIN(38, (char *)"PLTRSTB"),
    PINCTRL_PIN(39, (char *)"SPKR"),
    PINCTRL_PIN(40, (char *)"GSPI0_CS0B"),
    PINCTRL_PIN(41, (char *)"GSPI0_CLK"),
    PINCTRL_PIN(42, (char *)"GSPI0_MISO"),
    PINCTRL_PIN(43, (char *)"GSPI0_MOSI"),
    PINCTRL_PIN(44, (char *)"GSPI1_CS0B"),
    PINCTRL_PIN(45, (char *)"GSPI1_CLK"),
    PINCTRL_PIN(46, (char *)"GSPI1_MISO"),
    PINCTRL_PIN(47, (char *)"GSPI1_MOSI"),
    PINCTRL_PIN(48, (char *)"SML1ALERTB"),
    PINCTRL_PIN(49, (char *)"GSPI0_CLK_LOOPBK"),
    PINCTRL_PIN(50, (char *)"GSPI1_CLK_LOOPBK"),
    /* GPP_C */
    PINCTRL_PIN(51, (char *)"SMBCLK"),
    PINCTRL_PIN(52, (char *)"SMBDATA"),
    PINCTRL_PIN(53, (char *)"SMBALERTB"),
    PINCTRL_PIN(54, (char *)"SML0CLK"),
    PINCTRL_PIN(55, (char *)"SML0DATA"),
    PINCTRL_PIN(56, (char *)"SML0ALERTB"),
    PINCTRL_PIN(57, (char *)"SML1CLK"),
    PINCTRL_PIN(58, (char *)"SML1DATA"),
    PINCTRL_PIN(59, (char *)"UART0_RXD"),
    PINCTRL_PIN(60, (char *)"UART0_TXD"),
    PINCTRL_PIN(61, (char *)"UART0_RTSB"),
    PINCTRL_PIN(62, (char *)"UART0_CTSB"),
    PINCTRL_PIN(63, (char *)"UART1_RXD"),
    PINCTRL_PIN(64, (char *)"UART1_TXD"),
    PINCTRL_PIN(65, (char *)"UART1_RTSB"),
    PINCTRL_PIN(66, (char *)"UART1_CTSB"),
    PINCTRL_PIN(67, (char *)"I2C0_SDA"),
    PINCTRL_PIN(68, (char *)"I2C0_SCL"),
    PINCTRL_PIN(69, (char *)"I2C1_SDA"),
    PINCTRL_PIN(70, (char *)"I2C1_SCL"),
    PINCTRL_PIN(71, (char *)"UART2_RXD"),
    PINCTRL_PIN(72, (char *)"UART2_TXD"),
    PINCTRL_PIN(73, (char *)"UART2_RTSB"),
    PINCTRL_PIN(74, (char *)"UART2_CTSB"),
    /* GPP_D */
    PINCTRL_PIN(75, (char *)"SPI1_CSB"),
    PINCTRL_PIN(76, (char *)"SPI1_CLK"),
    PINCTRL_PIN(77, (char *)"SPI1_MISO_IO_1"),
    PINCTRL_PIN(78, (char *)"SPI1_MOSI_IO_0"),
    PINCTRL_PIN(79, (char *)"ISH_I2C2_SDA"),
    PINCTRL_PIN(80, (char *)"SSP2_SFRM"),
    PINCTRL_PIN(81, (char *)"SSP2_TXD"),
    PINCTRL_PIN(82, (char *)"SSP2_RXD"),
    PINCTRL_PIN(83, (char *)"SSP2_SCLK"),
    PINCTRL_PIN(84, (char *)"ISH_SPI_CSB"),
    PINCTRL_PIN(85, (char *)"ISH_SPI_CLK"),
    PINCTRL_PIN(86, (char *)"ISH_SPI_MISO"),
    PINCTRL_PIN(87, (char *)"ISH_SPI_MOSI"),
    PINCTRL_PIN(88, (char *)"ISH_UART0_RXD"),
    PINCTRL_PIN(89, (char *)"ISH_UART0_TXD"),
    PINCTRL_PIN(90, (char *)"ISH_UART0_RTSB"),
    PINCTRL_PIN(91, (char *)"ISH_UART0_CTSB"),
    PINCTRL_PIN(92, (char *)"DMIC_CLK_1"),
    PINCTRL_PIN(93, (char *)"DMIC_DATA_1"),
    PINCTRL_PIN(94, (char *)"DMIC_CLK_0"),
    PINCTRL_PIN(95, (char *)"DMIC_DATA_0"),
    PINCTRL_PIN(96, (char *)"SPI1_IO_2"),
    PINCTRL_PIN(97, (char *)"SPI1_IO_3"),
    PINCTRL_PIN(98, (char *)"ISH_I2C2_SCL"),
    /* GPP_G */
    PINCTRL_PIN(99, (char *)"SD3_CMD"),
    PINCTRL_PIN(100, (char *)"SD3_D0"),
    PINCTRL_PIN(101, (char *)"SD3_D1"),
    PINCTRL_PIN(102, (char *)"SD3_D2"),
    PINCTRL_PIN(103, (char *)"SD3_D3"),
    PINCTRL_PIN(104, (char *)"SD3_CDB"),
    PINCTRL_PIN(105, (char *)"SD3_CLK"),
    PINCTRL_PIN(106, (char *)"SD3_WP"),
    /* AZA */
    PINCTRL_PIN(107, (char *)"HDA_BCLK"),
    PINCTRL_PIN(108, (char *)"HDA_RSTB"),
    PINCTRL_PIN(109, (char *)"HDA_SYNC"),
    PINCTRL_PIN(110, (char *)"HDA_SDO"),
    PINCTRL_PIN(111, (char *)"HDA_SDI_0"),
    PINCTRL_PIN(112, (char *)"HDA_SDI_1"),
    PINCTRL_PIN(113, (char *)"SSP1_SFRM"),
    PINCTRL_PIN(114, (char *)"SSP1_TXD"),
    /* vGPIO */
    PINCTRL_PIN(115, (char *)"CNV_BTEN"),
    PINCTRL_PIN(116, (char *)"CNV_GNEN"),
    PINCTRL_PIN(117, (char *)"CNV_WFEN"),
    PINCTRL_PIN(118, (char *)"CNV_WCEN"),
    PINCTRL_PIN(119, (char *)"CNV_BT_HOST_WAKEB"),
    PINCTRL_PIN(120, (char *)"vCNV_GNSS_HOST_WAKEB"),
    PINCTRL_PIN(121, (char *)"vSD3_CD_B"),
    PINCTRL_PIN(122, (char *)"CNV_BT_IF_SELECT"),
    PINCTRL_PIN(123, (char *)"vCNV_BT_UART_TXD"),
    PINCTRL_PIN(124, (char *)"vCNV_BT_UART_RXD"),
    PINCTRL_PIN(125, (char *)"vCNV_BT_UART_CTS_B"),
    PINCTRL_PIN(126, (char *)"vCNV_BT_UART_RTS_B"),
    PINCTRL_PIN(127, (char *)"vCNV_MFUART1_TXD"),
    PINCTRL_PIN(128, (char *)"vCNV_MFUART1_RXD"),
    PINCTRL_PIN(129, (char *)"vCNV_MFUART1_CTS_B"),
    PINCTRL_PIN(130, (char *)"vCNV_MFUART1_RTS_B"),
    PINCTRL_PIN(131, (char *)"vCNV_GNSS_UART_TXD"),
    PINCTRL_PIN(132, (char *)"vCNV_GNSS_UART_RXD"),
    PINCTRL_PIN(133, (char *)"vCNV_GNSS_UART_CTS_B"),
    PINCTRL_PIN(134, (char *)"vCNV_GNSS_UART_RTS_B"),
    PINCTRL_PIN(135, (char *)"vUART0_TXD"),
    PINCTRL_PIN(136, (char *)"vUART0_RXD"),
    PINCTRL_PIN(137, (char *)"vUART0_CTS_B"),
    PINCTRL_PIN(138, (char *)"vUART0_RTSB"),
    PINCTRL_PIN(139, (char *)"vISH_UART0_TXD"),
    PINCTRL_PIN(140, (char *)"vISH_UART0_RXD"),
    PINCTRL_PIN(141, (char *)"vISH_UART0_CTS_B"),
    PINCTRL_PIN(142, (char *)"vISH_UART0_RTSB"),
    PINCTRL_PIN(143, (char *)"vISH_UART1_TXD"),
    PINCTRL_PIN(144, (char *)"vISH_UART1_RXD"),
    PINCTRL_PIN(145, (char *)"vISH_UART1_CTS_B"),
    PINCTRL_PIN(146, (char *)"vISH_UART1_RTS_B"),
    PINCTRL_PIN(147, (char *)"vCNV_BT_I2S_BCLK"),
    PINCTRL_PIN(148, (char *)"vCNV_BT_I2S_WS_SYNC"),
    PINCTRL_PIN(149, (char *)"vCNV_BT_I2S_SDO"),
    PINCTRL_PIN(150, (char *)"vCNV_BT_I2S_SDI"),
    PINCTRL_PIN(151, (char *)"vSSP2_SCLK"),
    PINCTRL_PIN(152, (char *)"vSSP2_SFRM"),
    PINCTRL_PIN(153, (char *)"vSSP2_TXD"),
    PINCTRL_PIN(154, (char *)"vSSP2_RXD"),
    /* GPP_K */
    PINCTRL_PIN(155, (char *)"FAN_TACH_0"),
    PINCTRL_PIN(156, (char *)"FAN_TACH_1"),
    PINCTRL_PIN(157, (char *)"FAN_TACH_2"),
    PINCTRL_PIN(158, (char *)"FAN_TACH_3"),
    PINCTRL_PIN(159, (char *)"FAN_TACH_4"),
    PINCTRL_PIN(160, (char *)"FAN_TACH_5"),
    PINCTRL_PIN(161, (char *)"FAN_TACH_6"),
    PINCTRL_PIN(162, (char *)"FAN_TACH_7"),
    PINCTRL_PIN(163, (char *)"FAN_PWM_0"),
    PINCTRL_PIN(164, (char *)"FAN_PWM_1"),
    PINCTRL_PIN(165, (char *)"FAN_PWM_2"),
    PINCTRL_PIN(166, (char *)"FAN_PWM_3"),
    PINCTRL_PIN(167, (char *)"GSXDOUT"),
    PINCTRL_PIN(168, (char *)"GSXSLOAD"),
    PINCTRL_PIN(169, (char *)"GSXDIN"),
    PINCTRL_PIN(170, (char *)"GSXSRESETB"),
    PINCTRL_PIN(171, (char *)"GSXCLK"),
    PINCTRL_PIN(172, (char *)"ADR_COMPLETE"),
    PINCTRL_PIN(173, (char *)"NMIB"),
    PINCTRL_PIN(174, (char *)"SMIB"),
    PINCTRL_PIN(175, (char *)"CORE_VID_0"),
    PINCTRL_PIN(176, (char *)"CORE_VID_1"),
    PINCTRL_PIN(177, (char *)"IMGCLKOUT_0"),
    PINCTRL_PIN(178, (char *)"IMGCLKOUT_1"),
    /* GPP_H */
    PINCTRL_PIN(179, (char *)"SRCCLKREQB_6"),
    PINCTRL_PIN(180, (char *)"SRCCLKREQB_7"),
    PINCTRL_PIN(181, (char *)"SRCCLKREQB_8"),
    PINCTRL_PIN(182, (char *)"SRCCLKREQB_9"),
    PINCTRL_PIN(183, (char *)"SRCCLKREQB_10"),
    PINCTRL_PIN(184, (char *)"SRCCLKREQB_11"),
    PINCTRL_PIN(185, (char *)"SRCCLKREQB_12"),
    PINCTRL_PIN(186, (char *)"SRCCLKREQB_13"),
    PINCTRL_PIN(187, (char *)"SRCCLKREQB_14"),
    PINCTRL_PIN(188, (char *)"SRCCLKREQB_15"),
    PINCTRL_PIN(189, (char *)"SML2CLK"),
    PINCTRL_PIN(190, (char *)"SML2DATA"),
    PINCTRL_PIN(191, (char *)"SML2ALERTB"),
    PINCTRL_PIN(192, (char *)"SML3CLK"),
    PINCTRL_PIN(193, (char *)"SML3DATA"),
    PINCTRL_PIN(194, (char *)"SML3ALERTB"),
    PINCTRL_PIN(195, (char *)"SML4CLK"),
    PINCTRL_PIN(196, (char *)"SML4DATA"),
    PINCTRL_PIN(197, (char *)"SML4ALERTB"),
    PINCTRL_PIN(198, (char *)"ISH_I2C0_SDA"),
    PINCTRL_PIN(199, (char *)"ISH_I2C0_SCL"),
    PINCTRL_PIN(200, (char *)"ISH_I2C1_SDA"),
    PINCTRL_PIN(201, (char *)"ISH_I2C1_SCL"),
    PINCTRL_PIN(202, (char *)"TIME_SYNC_0"),
    /* GPP_E */
    PINCTRL_PIN(203, (char *)"SATAXPCIE_0"),
    PINCTRL_PIN(204, (char *)"SATAXPCIE_1"),
    PINCTRL_PIN(205, (char *)"SATAXPCIE_2"),
    PINCTRL_PIN(206, (char *)"CPU_GP_0"),
    PINCTRL_PIN(207, (char *)"SATA_DEVSLP_0"),
    PINCTRL_PIN(208, (char *)"SATA_DEVSLP_1"),
    PINCTRL_PIN(209, (char *)"SATA_DEVSLP_2"),
    PINCTRL_PIN(210, (char *)"CPU_GP_1"),
    PINCTRL_PIN(211, (char *)"SATA_LEDB"),
    PINCTRL_PIN(212, (char *)"USB2_OCB_0"),
    PINCTRL_PIN(213, (char *)"USB2_OCB_1"),
    PINCTRL_PIN(214, (char *)"USB2_OCB_2"),
    PINCTRL_PIN(215, (char *)"USB2_OCB_3"),
    /* GPP_F */
    PINCTRL_PIN(216, (char *)"SATAXPCIE_3"),
    PINCTRL_PIN(217, (char *)"SATAXPCIE_4"),
    PINCTRL_PIN(218, (char *)"SATAXPCIE_5"),
    PINCTRL_PIN(219, (char *)"SATAXPCIE_6"),
    PINCTRL_PIN(220, (char *)"SATAXPCIE_7"),
    PINCTRL_PIN(221, (char *)"SATA_DEVSLP_3"),
    PINCTRL_PIN(222, (char *)"SATA_DEVSLP_4"),
    PINCTRL_PIN(223, (char *)"SATA_DEVSLP_5"),
    PINCTRL_PIN(224, (char *)"SATA_DEVSLP_6"),
    PINCTRL_PIN(225, (char *)"SATA_DEVSLP_7"),
    PINCTRL_PIN(226, (char *)"SATA_SCLOCK"),
    PINCTRL_PIN(227, (char *)"SATA_SLOAD"),
    PINCTRL_PIN(228, (char *)"SATA_SDATAOUT1"),
    PINCTRL_PIN(229, (char *)"SATA_SDATAOUT0"),
    PINCTRL_PIN(230, (char *)"EXT_PWR_GATEB"),
    PINCTRL_PIN(231, (char *)"USB2_OCB_4"),
    PINCTRL_PIN(232, (char *)"USB2_OCB_5"),
    PINCTRL_PIN(233, (char *)"USB2_OCB_6"),
    PINCTRL_PIN(234, (char *)"USB2_OCB_7"),
    PINCTRL_PIN(235, (char *)"L_VDDEN"),
    PINCTRL_PIN(236, (char *)"L_BKLTEN"),
    PINCTRL_PIN(237, (char *)"L_BKLTCTL"),
    PINCTRL_PIN(238, (char *)"DDPF_CTRLCLK"),
    PINCTRL_PIN(239, (char *)"DDPF_CTRLDATA"),
    /* SPI */
    PINCTRL_PIN(240, (char *)"SPI0_IO_2"),
    PINCTRL_PIN(241, (char *)"SPI0_IO_3"),
    PINCTRL_PIN(242, (char *)"SPI0_MOSI_IO_0"),
    PINCTRL_PIN(243, (char *)"SPI0_MISO_IO_1"),
    PINCTRL_PIN(244, (char *)"SPI0_TPM_CSB"),
    PINCTRL_PIN(245, (char *)"SPI0_FLASH_0_CSB"),
    PINCTRL_PIN(246, (char *)"SPI0_FLASH_1_CSB"),
    PINCTRL_PIN(247, (char *)"SPI0_CLK"),
    PINCTRL_PIN(248, (char *)"SPI0_CLK_LOOPBK"),
    /* CPU */
    PINCTRL_PIN(249, (char *)"HDACPU_SDI"),
    PINCTRL_PIN(250, (char *)"HDACPU_SDO"),
    PINCTRL_PIN(251, (char *)"HDACPU_SCLK"),
    PINCTRL_PIN(252, (char *)"PM_SYNC"),
    PINCTRL_PIN(253, (char *)"PECI"),
    PINCTRL_PIN(254, (char *)"CPUPWRGD"),
    PINCTRL_PIN(255, (char *)"THRMTRIPB"),
    PINCTRL_PIN(256, (char *)"PLTRST_CPUB"),
    PINCTRL_PIN(257, (char *)"PM_DOWN"),
    PINCTRL_PIN(258, (char *)"TRIGGER_IN"),
    PINCTRL_PIN(259, (char *)"TRIGGER_OUT"),
    /* JTAG */
    PINCTRL_PIN(260, (char *)"JTAG_TDO"),
    PINCTRL_PIN(261, (char *)"JTAGX"),
    PINCTRL_PIN(262, (char *)"PRDYB"),
    PINCTRL_PIN(263, (char *)"PREQB"),
    PINCTRL_PIN(264, (char *)"CPU_TRSTB"),
    PINCTRL_PIN(265, (char *)"JTAG_TDI"),
    PINCTRL_PIN(266, (char *)"JTAG_TMS"),
    PINCTRL_PIN(267, (char *)"JTAG_TCK"),
    PINCTRL_PIN(268, (char *)"ITP_PMODE"),
    /* GPP_I */
    PINCTRL_PIN(269, (char *)"DDSP_HPD_0"),
    PINCTRL_PIN(270, (char *)"DDSP_HPD_1"),
    PINCTRL_PIN(271, (char *)"DDSP_HPD_2"),
    PINCTRL_PIN(272, (char *)"DDSP_HPD_3"),
    PINCTRL_PIN(273, (char *)"EDP_HPD"),
    PINCTRL_PIN(274, (char *)"DDPB_CTRLCLK"),
    PINCTRL_PIN(275, (char *)"DDPB_CTRLDATA"),
    PINCTRL_PIN(276, (char *)"DDPC_CTRLCLK"),
    PINCTRL_PIN(277, (char *)"DDPC_CTRLDATA"),
    PINCTRL_PIN(278, (char *)"DDPD_CTRLCLK"),
    PINCTRL_PIN(279, (char *)"DDPD_CTRLDATA"),
    PINCTRL_PIN(280, (char *)"M2_SKT2_CFG_0"),
    PINCTRL_PIN(281, (char *)"M2_SKT2_CFG_1"),
    PINCTRL_PIN(282, (char *)"M2_SKT2_CFG_2"),
    PINCTRL_PIN(283, (char *)"M2_SKT2_CFG_3"),
    PINCTRL_PIN(284, (char *)"SYS_PWROK"),
    PINCTRL_PIN(285, (char *)"SYS_RESETB"),
    PINCTRL_PIN(286, (char *)"MLK_RSTB"),
    /* GPP_J */
    PINCTRL_PIN(287, (char *)"CNV_PA_BLANKING"),
    PINCTRL_PIN(288, (char *)"CNV_GNSS_FTA"),
    PINCTRL_PIN(289, (char *)"CNV_GNSS_SYSCK"),
    PINCTRL_PIN(290, (char *)"CNV_RF_RESET_B"),
    PINCTRL_PIN(291, (char *)"CNV_BRI_DT"),
    PINCTRL_PIN(292, (char *)"CNV_BRI_RSP"),
    PINCTRL_PIN(293, (char *)"CNV_RGI_DT"),
    PINCTRL_PIN(294, (char *)"CNV_RGI_RSP"),
    PINCTRL_PIN(295, (char *)"CNV_MFUART2_RXD"),
    PINCTRL_PIN(296, (char *)"CNV_MFUART2_TXD"),
    PINCTRL_PIN(297, (char *)"CNV_MODEM_CLKREQ"),
    PINCTRL_PIN(298, (char *)"A4WP_PRESENT"),
};

static unsigned int cnlh_spi0_pins[] = { 40, 41, 42, 43 };
static unsigned int cnlh_spi1_pins[] = { 44, 45, 46, 47 };
static unsigned int cnlh_spi2_pins[] = { 84, 85, 86, 87 };

static unsigned int cnlh_uart0_pins[] = { 59, 60, 61, 62 };
static unsigned int cnlh_uart1_pins[] = { 63, 64, 65, 66 };
static unsigned int cnlh_uart2_pins[] = { 71, 72, 73, 74 };

static unsigned int cnlh_i2c0_pins[] = { 67, 68 };
static unsigned int cnlh_i2c1_pins[] = { 69, 70 };
static unsigned int cnlh_i2c2_pins[] = { 88, 89 };
static unsigned int cnlh_i2c3_pins[] = { 79, 98 };

static struct intel_pingroup cnlh_groups[] = {
    PIN_GROUP((char *)"spi0_grp", cnlh_spi0_pins, 1),
    PIN_GROUP((char *)"spi1_grp", cnlh_spi1_pins, 1),
    PIN_GROUP((char *)"spi2_grp", cnlh_spi2_pins, 3),
    PIN_GROUP((char *)"uart0_grp", cnlh_uart0_pins, 1),
    PIN_GROUP((char *)"uart1_grp", cnlh_uart1_pins, 1),
    PIN_GROUP((char *)"uart2_grp", cnlh_uart2_pins, 1),
    PIN_GROUP((char *)"i2c0_grp", cnlh_i2c0_pins, 1),
    PIN_GROUP((char *)"i2c1_grp", cnlh_i2c1_pins, 1),
    PIN_GROUP((char *)"i2c2_grp", cnlh_i2c2_pins, 3),
    PIN_GROUP((char *)"i2c3_grp", cnlh_i2c3_pins, 2),
};

static char * const cnlh_spi0_groups[] = { (char *)"spi0_grp" };
static char * const cnlh_spi1_groups[] = { (char *)"spi1_grp" };
static char * const cnlh_spi2_groups[] = { (char *)"spi2_grp" };
static char * const cnlh_uart0_groups[] = { (char *)"uart0_grp" };
static char * const cnlh_uart1_groups[] = { (char *)"uart1_grp" };
static char * const cnlh_uart2_groups[] = { (char *)"uart2_grp" };
static char * const cnlh_i2c0_groups[] = { (char *)"i2c0_grp" };
static char * const cnlh_i2c1_groups[] = { (char *)"i2c1_grp" };
static char * const cnlh_i2c2_groups[] = { (char *)"i2c2_grp" };
static char * const cnlh_i2c3_groups[] = { (char *)"i2c3_grp" };

static struct intel_function cnlh_functions[] = {
    FUNCTION((char *)"spi0", cnlh_spi0_groups),
    FUNCTION((char *)"spi1", cnlh_spi1_groups),
    FUNCTION((char *)"spi2", cnlh_spi2_groups),
    FUNCTION((char *)"uart0", cnlh_uart0_groups),
    FUNCTION((char *)"uart1", cnlh_uart1_groups),
    FUNCTION((char *)"uart2", cnlh_uart2_groups),
    FUNCTION((char *)"i2c0", cnlh_i2c0_groups),
    FUNCTION((char *)"i2c1", cnlh_i2c1_groups),
    FUNCTION((char *)"i2c2", cnlh_i2c2_groups),
    FUNCTION((char *)"i2c3", cnlh_i2c3_groups),
};

static struct intel_padgroup cnlh_community0_gpps[] = {
    CNL_GPP(0, 0, 24, 0),               /* GPP_A */
    CNL_GPP(1, 25, 50, 32),             /* GPP_B */
};

static struct intel_padgroup cnlh_community1_gpps[] = {
    CNL_GPP(0, 51, 74, 64),             /* GPP_C */
    CNL_GPP(1, 75, 98, 96),             /* GPP_D */
    CNL_GPP(2, 99, 106, 128),           /* GPP_G */
    CNL_GPP(3, 107, 114, CNL_NO_GPIO),  /* AZA */
    CNL_GPP(4, 115, 146, 160),          /* vGPIO_0 */
    CNL_GPP(5, 147, 154, CNL_NO_GPIO),  /* vGPIO_1 */
};

static struct intel_padgroup cnlh_community3_gpps[] = {
    CNL_GPP(0, 155, 178, 192),          /* GPP_K */
    CNL_GPP(1, 179, 202, 224),          /* GPP_H */
    CNL_GPP(2, 203, 215, 256),          /* GPP_E */
    CNL_GPP(3, 216, 239, 288),          /* GPP_F */
    CNL_GPP(4, 240, 248, CNL_NO_GPIO),  /* SPI */
};

static struct intel_padgroup cnlh_community4_gpps[] = {
    CNL_GPP(0, 249, 259, CNL_NO_GPIO),  /* CPU */
    CNL_GPP(1, 260, 268, CNL_NO_GPIO),  /* JTAG */
    CNL_GPP(2, 269, 286, 320),          /* GPP_I */
    CNL_GPP(3, 287, 298, 352),          /* GPP_J */
};

static struct intel_community cnlh_communities[] = {
    CNLH_COMMUNITY(0, 0, 50, cnlh_community0_gpps),
    CNLH_COMMUNITY(1, 51, 154, cnlh_community1_gpps),
    CNLH_COMMUNITY(2, 155, 248, cnlh_community3_gpps),
    CNLH_COMMUNITY(3, 249, 298, cnlh_community4_gpps),
};

class VoodooGPIOCannonLakeH : public VoodooGPIO {
    OSDeclareDefaultStructors(VoodooGPIOCannonLakeH);

    bool start(IOService *provider) override;
};

#endif /* VoodooGPIOCannonLakeH_h */
