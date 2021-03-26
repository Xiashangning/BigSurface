//
//  VoodooGPIOCannonLakeLP.h
//  VoodooGPIO
//
//  Created by Alexandre Daoud on 15/11/18.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "../VoodooGPIO.hpp"

#ifndef VoodooGPIOCannonLakeLP_h
#define VoodooGPIOCannonLakeLP_h

#define CNL_PAD_OWN         0x020
#define CNL_PADCFGLOCK      0x080
#define CNL_LP_HOSTSW_OWN   0x0b0
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

#define CNLLP_COMMUNITY(b, s, e, g)         \
        CNL_COMMUNITY(b, s, e, CNL_LP_HOSTSW_OWN, g)

/* Cannon Lake-LP */
static struct pinctrl_pin_desc cnllp_pins[] = {
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
    PINCTRL_PIN(16, (char *)"SD_1P8_SEL"),
    PINCTRL_PIN(17, (char *)"SD_PWR_EN_B"),
    PINCTRL_PIN(18, (char *)"ISH_GP_0"),
    PINCTRL_PIN(19, (char *)"ISH_GP_1"),
    PINCTRL_PIN(20, (char *)"ISH_GP_2"),
    PINCTRL_PIN(21, (char *)"ISH_GP_3"),
    PINCTRL_PIN(22, (char *)"ISH_GP_4"),
    PINCTRL_PIN(23, (char *)"ISH_GP_5"),
    PINCTRL_PIN(24, (char *)"ESPI_CLK_LOOPBK"),
    /* GPP_B */
    PINCTRL_PIN(25, (char *)"CORE_VID_0"),
    PINCTRL_PIN(26, (char *)"CORE_VID_1"),
    PINCTRL_PIN(27, (char *)"VRALERTB"),
    PINCTRL_PIN(28, (char *)"CPU_GP_2"),
    PINCTRL_PIN(29, (char *)"CPU_GP_3"),
    PINCTRL_PIN(30, (char *)"SRCCLKREQB_0"),
    PINCTRL_PIN(31, (char *)"SRCCLKREQB_1"),
    PINCTRL_PIN(32, (char *)"SRCCLKREQB_2"),
    PINCTRL_PIN(33, (char *)"SRCCLKREQB_3"),
    PINCTRL_PIN(34, (char *)"SRCCLKREQB_4"),
    PINCTRL_PIN(35, (char *)"SRCCLKREQB_5"),
    PINCTRL_PIN(36, (char *)"EXT_PWR_GATEB"),
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
    /* GPP_G */
    PINCTRL_PIN(51, (char *)"SD3_CMD"),
    PINCTRL_PIN(52, (char *)"SD3_D0_SD4_RCLK_P"),
    PINCTRL_PIN(53, (char *)"SD3_D1_SD4_RCLK_N"),
    PINCTRL_PIN(54, (char *)"SD3_D2"),
    PINCTRL_PIN(55, (char *)"SD3_D3"),
    PINCTRL_PIN(56, (char *)"SD3_CDB"),
    PINCTRL_PIN(57, (char *)"SD3_CLK"),
    PINCTRL_PIN(58, (char *)"SD3_WP"),
    /* SPI */
    PINCTRL_PIN(59, (char *)"SPI0_IO_2"),
    PINCTRL_PIN(60, (char *)"SPI0_IO_3"),
    PINCTRL_PIN(61, (char *)"SPI0_MOSI_IO_0"),
    PINCTRL_PIN(62, (char *)"SPI0_MISO_IO_1"),
    PINCTRL_PIN(63, (char *)"SPI0_TPM_CSB"),
    PINCTRL_PIN(64, (char *)"SPI0_FLASH_0_CSB"),
    PINCTRL_PIN(65, (char *)"SPI0_FLASH_1_CSB"),
    PINCTRL_PIN(66, (char *)"SPI0_CLK"),
    PINCTRL_PIN(67, (char *)"SPI0_CLK_LOOPBK"),
    /* GPP_D */
    PINCTRL_PIN(68, (char *)"SPI1_CSB"),
    PINCTRL_PIN(69, (char *)"SPI1_CLK"),
    PINCTRL_PIN(70, (char *)"SPI1_MISO_IO_1"),
    PINCTRL_PIN(71, (char *)"SPI1_MOSI_IO_0"),
    PINCTRL_PIN(72, (char *)"IMGCLKOUT_0"),
    PINCTRL_PIN(73, (char *)"ISH_I2C0_SDA"),
    PINCTRL_PIN(74, (char *)"ISH_I2C0_SCL"),
    PINCTRL_PIN(75, (char *)"ISH_I2C1_SDA"),
    PINCTRL_PIN(76, (char *)"ISH_I2C1_SCL"),
    PINCTRL_PIN(77, (char *)"ISH_SPI_CSB"),
    PINCTRL_PIN(78, (char *)"ISH_SPI_CLK"),
    PINCTRL_PIN(79, (char *)"ISH_SPI_MISO"),
    PINCTRL_PIN(80, (char *)"ISH_SPI_MOSI"),
    PINCTRL_PIN(81, (char *)"ISH_UART0_RXD"),
    PINCTRL_PIN(82, (char *)"ISH_UART0_TXD"),
    PINCTRL_PIN(83, (char *)"ISH_UART0_RTSB"),
    PINCTRL_PIN(84, (char *)"ISH_UART0_CTSB"),
    PINCTRL_PIN(85, (char *)"DMIC_CLK_1"),
    PINCTRL_PIN(86, (char *)"DMIC_DATA_1"),
    PINCTRL_PIN(87, (char *)"DMIC_CLK_0"),
    PINCTRL_PIN(88, (char *)"DMIC_DATA_0"),
    PINCTRL_PIN(89, (char *)"SPI1_IO_2"),
    PINCTRL_PIN(90, (char *)"SPI1_IO_3"),
    PINCTRL_PIN(91, (char *)"SSP_MCLK"),
    PINCTRL_PIN(92, (char *)"GSPI2_CLK_LOOPBK"),
    /* GPP_F */
    PINCTRL_PIN(93, (char *)"CNV_GNSS_PA_BLANKING"),
    PINCTRL_PIN(94, (char *)"CNV_GNSS_FTA"),
    PINCTRL_PIN(95, (char *)"CNV_GNSS_SYSCK"),
    PINCTRL_PIN(96, (char *)"EMMC_HIP_MON"),
    PINCTRL_PIN(97, (char *)"CNV_BRI_DT"),
    PINCTRL_PIN(98, (char *)"CNV_BRI_RSP"),
    PINCTRL_PIN(99, (char *)"CNV_RGI_DT"),
    PINCTRL_PIN(100, (char *)"CNV_RGI_RSP"),
    PINCTRL_PIN(101, (char *)"CNV_MFUART2_RXD"),
    PINCTRL_PIN(102, (char *)"CNV_MFUART2_TXD"),
    PINCTRL_PIN(103, (char *)"GPP_F_10"),
    PINCTRL_PIN(104, (char *)"EMMC_CMD"),
    PINCTRL_PIN(105, (char *)"EMMC_DATA_0"),
    PINCTRL_PIN(106, (char *)"EMMC_DATA_1"),
    PINCTRL_PIN(107, (char *)"EMMC_DATA_2"),
    PINCTRL_PIN(108, (char *)"EMMC_DATA_3"),
    PINCTRL_PIN(109, (char *)"EMMC_DATA_4"),
    PINCTRL_PIN(110, (char *)"EMMC_DATA_5"),
    PINCTRL_PIN(111, (char *)"EMMC_DATA_6"),
    PINCTRL_PIN(112, (char *)"EMMC_DATA_7"),
    PINCTRL_PIN(113, (char *)"EMMC_RCLK"),
    PINCTRL_PIN(114, (char *)"EMMC_CLK"),
    PINCTRL_PIN(115, (char *)"EMMC_RESETB"),
    PINCTRL_PIN(116, (char *)"A4WP_PRESENT"),
    /* GPP_H */
    PINCTRL_PIN(117, (char *)"SSP2_SCLK"),
    PINCTRL_PIN(118, (char *)"SSP2_SFRM"),
    PINCTRL_PIN(119, (char *)"SSP2_TXD"),
    PINCTRL_PIN(120, (char *)"SSP2_RXD"),
    PINCTRL_PIN(121, (char *)"I2C2_SDA"),
    PINCTRL_PIN(122, (char *)"I2C2_SCL"),
    PINCTRL_PIN(123, (char *)"I2C3_SDA"),
    PINCTRL_PIN(124, (char *)"I2C3_SCL"),
    PINCTRL_PIN(125, (char *)"I2C4_SDA"),
    PINCTRL_PIN(126, (char *)"I2C4_SCL"),
    PINCTRL_PIN(127, (char *)"I2C5_SDA"),
    PINCTRL_PIN(128, (char *)"I2C5_SCL"),
    PINCTRL_PIN(129, (char *)"M2_SKT2_CFG_0"),
    PINCTRL_PIN(130, (char *)"M2_SKT2_CFG_1"),
    PINCTRL_PIN(131, (char *)"M2_SKT2_CFG_2"),
    PINCTRL_PIN(132, (char *)"M2_SKT2_CFG_3"),
    PINCTRL_PIN(133, (char *)"DDPF_CTRLCLK"),
    PINCTRL_PIN(134, (char *)"DDPF_CTRLDATA"),
    PINCTRL_PIN(135, (char *)"CPU_VCCIO_PWR_GATEB"),
    PINCTRL_PIN(136, (char *)"TIMESYNC_0"),
    PINCTRL_PIN(137, (char *)"IMGCLKOUT_1"),
    PINCTRL_PIN(138, (char *)"GPPC_H_21"),
    PINCTRL_PIN(139, (char *)"GPPC_H_22"),
    PINCTRL_PIN(140, (char *)"GPPC_H_23"),
    /* vGPIO */
    PINCTRL_PIN(141, (char *)"CNV_BTEN"),
    PINCTRL_PIN(142, (char *)"CNV_GNEN"),
    PINCTRL_PIN(143, (char *)"CNV_WFEN"),
    PINCTRL_PIN(144, (char *)"CNV_WCEN"),
    PINCTRL_PIN(145, (char *)"CNV_BT_HOST_WAKEB"),
    PINCTRL_PIN(146, (char *)"CNV_BT_IF_SELECT"),
    PINCTRL_PIN(147, (char *)"vCNV_BT_UART_TXD"),
    PINCTRL_PIN(148, (char *)"vCNV_BT_UART_RXD"),
    PINCTRL_PIN(149, (char *)"vCNV_BT_UART_CTS_B"),
    PINCTRL_PIN(150, (char *)"vCNV_BT_UART_RTS_B"),
    PINCTRL_PIN(151, (char *)"vCNV_MFUART1_TXD"),
    PINCTRL_PIN(152, (char *)"vCNV_MFUART1_RXD"),
    PINCTRL_PIN(153, (char *)"vCNV_MFUART1_CTS_B"),
    PINCTRL_PIN(154, (char *)"vCNV_MFUART1_RTS_B"),
    PINCTRL_PIN(155, (char *)"vCNV_GNSS_UART_TXD"),
    PINCTRL_PIN(156, (char *)"vCNV_GNSS_UART_RXD"),
    PINCTRL_PIN(157, (char *)"vCNV_GNSS_UART_CTS_B"),
    PINCTRL_PIN(158, (char *)"vCNV_GNSS_UART_RTS_B"),
    PINCTRL_PIN(159, (char *)"vUART0_TXD"),
    PINCTRL_PIN(160, (char *)"vUART0_RXD"),
    PINCTRL_PIN(161, (char *)"vUART0_CTS_B"),
    PINCTRL_PIN(162, (char *)"vUART0_RTS_B"),
    PINCTRL_PIN(163, (char *)"vISH_UART0_TXD"),
    PINCTRL_PIN(164, (char *)"vISH_UART0_RXD"),
    PINCTRL_PIN(165, (char *)"vISH_UART0_CTS_B"),
    PINCTRL_PIN(166, (char *)"vISH_UART0_RTS_B"),
    PINCTRL_PIN(167, (char *)"vISH_UART1_TXD"),
    PINCTRL_PIN(168, (char *)"vISH_UART1_RXD"),
    PINCTRL_PIN(169, (char *)"vISH_UART1_CTS_B"),
    PINCTRL_PIN(170, (char *)"vISH_UART1_RTS_B"),
    PINCTRL_PIN(171, (char *)"vCNV_BT_I2S_BCLK"),
    PINCTRL_PIN(172, (char *)"vCNV_BT_I2S_WS_SYNC"),
    PINCTRL_PIN(173, (char *)"vCNV_BT_I2S_SDO"),
    PINCTRL_PIN(174, (char *)"vCNV_BT_I2S_SDI"),
    PINCTRL_PIN(175, (char *)"vSSP2_SCLK"),
    PINCTRL_PIN(176, (char *)"vSSP2_SFRM"),
    PINCTRL_PIN(177, (char *)"vSSP2_TXD"),
    PINCTRL_PIN(178, (char *)"vSSP2_RXD"),
    PINCTRL_PIN(179, (char *)"vCNV_GNSS_HOST_WAKEB"),
    PINCTRL_PIN(180, (char *)"vSD3_CD_B"),
    /* GPP_C */
    PINCTRL_PIN(181, (char *)"SMBCLK"),
    PINCTRL_PIN(182, (char *)"SMBDATA"),
    PINCTRL_PIN(183, (char *)"SMBALERTB"),
    PINCTRL_PIN(184, (char *)"SML0CLK"),
    PINCTRL_PIN(185, (char *)"SML0DATA"),
    PINCTRL_PIN(186, (char *)"SML0ALERTB"),
    PINCTRL_PIN(187, (char *)"SML1CLK"),
    PINCTRL_PIN(188, (char *)"SML1DATA"),
    PINCTRL_PIN(189, (char *)"UART0_RXD"),
    PINCTRL_PIN(190, (char *)"UART0_TXD"),
    PINCTRL_PIN(191, (char *)"UART0_RTSB"),
    PINCTRL_PIN(192, (char *)"UART0_CTSB"),
    PINCTRL_PIN(193, (char *)"UART1_RXD"),
    PINCTRL_PIN(194, (char *)"UART1_TXD"),
    PINCTRL_PIN(195, (char *)"UART1_RTSB"),
    PINCTRL_PIN(196, (char *)"UART1_CTSB"),
    PINCTRL_PIN(197, (char *)"I2C0_SDA"),
    PINCTRL_PIN(198, (char *)"I2C0_SCL"),
    PINCTRL_PIN(199, (char *)"I2C1_SDA"),
    PINCTRL_PIN(200, (char *)"I2C1_SCL"),
    PINCTRL_PIN(201, (char *)"UART2_RXD"),
    PINCTRL_PIN(202, (char *)"UART2_TXD"),
    PINCTRL_PIN(203, (char *)"UART2_RTSB"),
    PINCTRL_PIN(204, (char *)"UART2_CTSB"),
    /* GPP_E */
    PINCTRL_PIN(205, (char *)"SATAXPCIE_0"),
    PINCTRL_PIN(206, (char *)"SATAXPCIE_1"),
    PINCTRL_PIN(207, (char *)"SATAXPCIE_2"),
    PINCTRL_PIN(208, (char *)"CPU_GP_0"),
    PINCTRL_PIN(209, (char *)"SATA_DEVSLP_0"),
    PINCTRL_PIN(210, (char *)"SATA_DEVSLP_1"),
    PINCTRL_PIN(211, (char *)"SATA_DEVSLP_2"),
    PINCTRL_PIN(212, (char *)"CPU_GP_1"),
    PINCTRL_PIN(213, (char *)"SATA_LEDB"),
    PINCTRL_PIN(214, (char *)"USB2_OCB_0"),
    PINCTRL_PIN(215, (char *)"USB2_OCB_1"),
    PINCTRL_PIN(216, (char *)"USB2_OCB_2"),
    PINCTRL_PIN(217, (char *)"USB2_OCB_3"),
    PINCTRL_PIN(218, (char *)"DDSP_HPD_0"),
    PINCTRL_PIN(219, (char *)"DDSP_HPD_1"),
    PINCTRL_PIN(220, (char *)"DDSP_HPD_2"),
    PINCTRL_PIN(221, (char *)"DDSP_HPD_3"),
    PINCTRL_PIN(222, (char *)"EDP_HPD"),
    PINCTRL_PIN(223, (char *)"DDPB_CTRLCLK"),
    PINCTRL_PIN(224, (char *)"DDPB_CTRLDATA"),
    PINCTRL_PIN(225, (char *)"DDPC_CTRLCLK"),
    PINCTRL_PIN(226, (char *)"DDPC_CTRLDATA"),
    PINCTRL_PIN(227, (char *)"DDPD_CTRLCLK"),
    PINCTRL_PIN(228, (char *)"DDPD_CTRLDATA"),
    /* JTAG */
    PINCTRL_PIN(229, (char *)"JTAG_TDO"),
    PINCTRL_PIN(230, (char *)"JTAGX"),
    PINCTRL_PIN(231, (char *)"PRDYB"),
    PINCTRL_PIN(232, (char *)"PREQB"),
    PINCTRL_PIN(233, (char *)"CPU_TRSTB"),
    PINCTRL_PIN(234, (char *)"JTAG_TDI"),
    PINCTRL_PIN(235, (char *)"JTAG_TMS"),
    PINCTRL_PIN(236, (char *)"JTAG_TCK"),
    PINCTRL_PIN(237, (char *)"ITP_PMODE"),
    /* HVCMOS */
    PINCTRL_PIN(238, (char *)"L_BKLTEN"),
    PINCTRL_PIN(239, (char *)"L_BKLTCTL"),
    PINCTRL_PIN(240, (char *)"L_VDDEN"),
    PINCTRL_PIN(241, (char *)"SYS_PWROK"),
    PINCTRL_PIN(242, (char *)"SYS_RESETB"),
    PINCTRL_PIN(243, (char *)"MLK_RSTB"),
};

static unsigned int cnllp_spi0_pins[] = { 40, 41, 42, 43, 7 };
static unsigned int cnllp_spi0_modes[] = { 1, 1, 1, 1, 2 };
static unsigned int cnllp_spi1_pins[] = { 44, 45, 46, 47, 11 };
static unsigned int cnllp_spi1_modes[] = { 1, 1, 1, 1, 2 };
static unsigned int cnllp_spi2_pins[] = { 77, 78, 79, 80, 83 };
static unsigned int cnllp_spi2_modes[] = { 3, 3, 3, 3, 2 };

static unsigned int cnllp_i2c0_pins[] = { 197, 198 };
static unsigned int cnllp_i2c1_pins[] = { 199, 200 };
static unsigned int cnllp_i2c2_pins[] = { 121, 122 };
static unsigned int cnllp_i2c3_pins[] = { 123, 124 };
static unsigned int cnllp_i2c4_pins[] = { 125, 126 };
static unsigned int cnllp_i2c5_pins[] = { 127, 128 };

static unsigned int cnllp_uart0_pins[] = { 189, 190, 191, 192 };
static unsigned int cnllp_uart1_pins[] = { 193, 194, 195, 196 };
static unsigned int cnllp_uart2_pins[] = { 201, 202, 203, 204 };

static struct intel_pingroup cnllp_groups[] = {
    PIN_GROUP((char *)"spi0_grp", cnllp_spi0_pins, cnllp_spi0_modes),
    PIN_GROUP((char *)"spi1_grp", cnllp_spi1_pins, cnllp_spi1_modes),
    PIN_GROUP((char *)"spi2_grp", cnllp_spi2_pins, cnllp_spi2_modes),
    PIN_GROUP((char *)"i2c0_grp", cnllp_i2c0_pins, 1),
    PIN_GROUP((char *)"i2c1_grp", cnllp_i2c1_pins, 1),
    PIN_GROUP((char *)"i2c2_grp", cnllp_i2c2_pins, 1),
    PIN_GROUP((char *)"i2c3_grp", cnllp_i2c3_pins, 1),
    PIN_GROUP((char *)"i2c4_grp", cnllp_i2c4_pins, 1),
    PIN_GROUP((char *)"i2c5_grp", cnllp_i2c5_pins, 1),
    PIN_GROUP((char *)"uart0_grp", cnllp_uart0_pins, 1),
    PIN_GROUP((char *)"uart1_grp", cnllp_uart1_pins, 1),
    PIN_GROUP((char *)"uart2_grp", cnllp_uart2_pins, 1),
};

static char * const cnllp_spi0_groups[] = { (char *)"spi0_grp" };
static char * const cnllp_spi1_groups[] = { (char *)"spi1_grp" };
static char * const cnllp_spi2_groups[] = { (char *)"spi2_grp" };
static char * const cnllp_i2c0_groups[] = { (char *)"i2c0_grp" };
static char * const cnllp_i2c1_groups[] = { (char *)"i2c1_grp" };
static char * const cnllp_i2c2_groups[] = { (char *)"i2c2_grp" };
static char * const cnllp_i2c3_groups[] = { (char *)"i2c3_grp" };
static char * const cnllp_i2c4_groups[] = { (char *)"i2c4_grp" };
static char * const cnllp_i2c5_groups[] = { (char *)"i2c5_grp" };
static char * const cnllp_uart0_groups[] = { (char *)"uart0_grp" };
static char * const cnllp_uart1_groups[] = { (char *)"uart1_grp" };
static char * const cnllp_uart2_groups[] = { (char *)"uart2_grp" };

static struct intel_function cnllp_functions[] = {
    FUNCTION((char *)"spi0", cnllp_spi0_groups),
    FUNCTION((char *)"spi1", cnllp_spi1_groups),
    FUNCTION((char *)"spi2", cnllp_spi2_groups),
    FUNCTION((char *)"i2c0", cnllp_i2c0_groups),
    FUNCTION((char *)"i2c1", cnllp_i2c1_groups),
    FUNCTION((char *)"i2c2", cnllp_i2c2_groups),
    FUNCTION((char *)"i2c3", cnllp_i2c3_groups),
    FUNCTION((char *)"i2c4", cnllp_i2c4_groups),
    FUNCTION((char *)"i2c5", cnllp_i2c5_groups),
    FUNCTION((char *)"uart0", cnllp_uart0_groups),
    FUNCTION((char *)"uart1", cnllp_uart1_groups),
    FUNCTION((char *)"uart2", cnllp_uart2_groups),
};

static struct intel_padgroup cnllp_community0_gpps[] = {
    CNL_GPP(0, 0, 24, 0),               /* GPP_A */
    CNL_GPP(1, 25, 50, 32),             /* GPP_B */
    CNL_GPP(2, 51, 58, 64),             /* GPP_G */
    CNL_GPP(3, 59, 67, CNL_NO_GPIO),    /* SPI */
};

static struct intel_padgroup cnllp_community1_gpps[] = {
    CNL_GPP(0, 68, 92, 96),             /* GPP_D */
    CNL_GPP(1, 93, 116, 128),           /* GPP_F */
    CNL_GPP(2, 117, 140, 160),          /* GPP_H */
    CNL_GPP(3, 141, 172, 192),          /* vGPIO */
    CNL_GPP(4, 173, 180, 224),          /* vGPIO */
};

static struct intel_padgroup cnllp_community4_gpps[] = {
    CNL_GPP(0, 181, 204, 256),          /* GPP_C */
    CNL_GPP(1, 205, 228, 288),          /* GPP_E */
    CNL_GPP(2, 229, 237, CNL_NO_GPIO),  /* JTAG */
    CNL_GPP(3, 238, 243, CNL_NO_GPIO),  /* HVCMOS */
};

static struct intel_community cnllp_communities[] = {
    CNLLP_COMMUNITY(0, 0, 67, cnllp_community0_gpps),
    CNLLP_COMMUNITY(1, 68, 180, cnllp_community1_gpps),
    CNLLP_COMMUNITY(2, 181, 243, cnllp_community4_gpps),
};

class VoodooGPIOCannonLakeLP : public VoodooGPIO {
    OSDeclareDefaultStructors(VoodooGPIOCannonLakeLP);

    bool start(IOService *provider) override;
};

#endif /* VoodooGPIOCannonLakeLP_h */
