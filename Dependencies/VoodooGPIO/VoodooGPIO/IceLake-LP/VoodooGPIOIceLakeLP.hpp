//
//  VoodooGPIOIceLakeLP.h
//  VoodooGPIO
//
//  Created by linyuzheng on 2020/8/11.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "../VoodooGPIO.hpp"

#ifndef VoodooGPIOIceLakeLP_h
#define VoodooGPIOIceLakeLP_h

#define ICL_PAD_OWN         0x020
#define ICL_PADCFGLOCK      0x080
#define ICL_HOSTSW_OWN      0x0b0
#define ICL_GPI_IS          0x100
#define ICL_GPI_IE          0x110

#define ICL_GPP(r, s, e, g)         \
    {                               \
        .reg_num = (r),             \
        .base = (s),                \
        .size = ((e) - (s) + 1),    \
        .gpio_base = (g),           \
    }
#define ICL_NO_GPIO     -1

#define ICL_COMMUNITY(b, s, e, g)               \
    {                                           \
        .barno = (b),                           \
        .padown_offset = ICL_PAD_OWN,           \
        .padcfglock_offset = ICL_PADCFGLOCK,    \
        .hostown_offset = ICL_HOSTSW_OWN,       \
        .ie_offset = ICL_GPI_IE,                \
        .pin_base = (s),                        \
        .npins = ((e) - (s) + 1),               \
        .gpps = (g),                            \
        .ngpps = ARRAY_SIZE(g),                 \
    }

/* Ice Lake-LP */
static struct pinctrl_pin_desc icllp_pins[] = {
    /* GPP_G */
    PINCTRL_PIN(0, (char *)"SD3_CMD"),
    PINCTRL_PIN(1, (char *)"SD3_D0"),
    PINCTRL_PIN(2, (char *)"SD3_D1"),
    PINCTRL_PIN(3, (char *)"SD3_D2"),
    PINCTRL_PIN(4, (char *)"SD3_D3"),
    PINCTRL_PIN(5, (char *)"SD3_CDB"),
    PINCTRL_PIN(6, (char *)"SD3_CLK"),
    PINCTRL_PIN(7, (char *)"SD3_WP"),
    /* GPP_B */
    PINCTRL_PIN(8, (char *)"CORE_VID_0"),
    PINCTRL_PIN(9, (char *)"CORE_VID_1"),
    PINCTRL_PIN(10, (char *)"VRALERTB"),
    PINCTRL_PIN(11, (char *)"CPU_GP_2"),
    PINCTRL_PIN(12, (char *)"CPU_GP_3"),
    PINCTRL_PIN(13, (char *)"ISH_I2C0_SDA"),
    PINCTRL_PIN(14, (char *)"ISH_I2C0_SCL"),
    PINCTRL_PIN(15, (char *)"ISH_I2C1_SDA"),
    PINCTRL_PIN(16, (char *)"ISH_I2C1_SCL"),
    PINCTRL_PIN(17, (char *)"I2C5_SDA"),
    PINCTRL_PIN(18, (char *)"I2C5_SCL"),
    PINCTRL_PIN(19, (char *)"PMCALERTB"),
    PINCTRL_PIN(20, (char *)"SLP_S0B"),
    PINCTRL_PIN(21, (char *)"PLTRSTB"),
    PINCTRL_PIN(22, (char *)"SPKR"),
    PINCTRL_PIN(23, (char *)"GSPI0_CS0B"),
    PINCTRL_PIN(24, (char *)"GSPI0_CLK"),
    PINCTRL_PIN(25, (char *)"GSPI0_MISO"),
    PINCTRL_PIN(26, (char *)"GSPI0_MOSI"),
    PINCTRL_PIN(27, (char *)"GSPI1_CS0B"),
    PINCTRL_PIN(28, (char *)"GSPI1_CLK"),
    PINCTRL_PIN(29, (char *)"GSPI1_MISO"),
    PINCTRL_PIN(30, (char *)"GSPI1_MOSI"),
    PINCTRL_PIN(31, (char *)"SML1ALERTB"),
    PINCTRL_PIN(32, (char *)"GSPI0_CLK_LOOPBK"),
    PINCTRL_PIN(33, (char *)"GSPI1_CLK_LOOPBK"),
    /* GPP_A */
    PINCTRL_PIN(34, (char *)"ESPI_IO_0"),
    PINCTRL_PIN(35, (char *)"ESPI_IO_1"),
    PINCTRL_PIN(36, (char *)"ESPI_IO_2"),
    PINCTRL_PIN(37, (char *)"ESPI_IO_3"),
    PINCTRL_PIN(38, (char *)"ESPI_CSB"),
    PINCTRL_PIN(39, (char *)"ESPI_CLK"),
    PINCTRL_PIN(40, (char *)"ESPI_RESETB"),
    PINCTRL_PIN(41, (char *)"I2S2_SCLK"),
    PINCTRL_PIN(42, (char *)"I2S2_SFRM"),
    PINCTRL_PIN(43, (char *)"I2S2_TXD"),
    PINCTRL_PIN(44, (char *)"I2S2_RXD"),
    PINCTRL_PIN(45, (char *)"SATA_DEVSLP_2"),
    PINCTRL_PIN(46, (char *)"SATAXPCIE_1"),
    PINCTRL_PIN(47, (char *)"SATAXPCIE_2"),
    PINCTRL_PIN(48, (char *)"USB2_OCB_1"),
    PINCTRL_PIN(49, (char *)"USB2_OCB_2"),
    PINCTRL_PIN(50, (char *)"USB2_OCB_3"),
    PINCTRL_PIN(51, (char *)"DDSP_HPD_C"),
    PINCTRL_PIN(52, (char *)"DDSP_HPD_B"),
    PINCTRL_PIN(53, (char *)"DDSP_HPD_1"),
    PINCTRL_PIN(54, (char *)"DDSP_HPD_2"),
    PINCTRL_PIN(55, (char *)"I2S5_TXD"),
    PINCTRL_PIN(56, (char *)"I2S5_RXD"),
    PINCTRL_PIN(57, (char *)"I2S1_SCLK"),
    PINCTRL_PIN(58, (char *)"ESPI_CLK_LOOPBK"),
    /* GPP_H */
    PINCTRL_PIN(59, (char *)"SD_1P8_SEL"),
    PINCTRL_PIN(60, (char *)"SD_PWR_EN_B"),
    PINCTRL_PIN(61, (char *)"GPPC_H_2"),
    PINCTRL_PIN(62, (char *)"SX_EXIT_HOLDOFFB"),
    PINCTRL_PIN(63, (char *)"I2C2_SDA"),
    PINCTRL_PIN(64, (char *)"I2C2_SCL"),
    PINCTRL_PIN(65, (char *)"I2C3_SDA"),
    PINCTRL_PIN(66, (char *)"I2C3_SCL"),
    PINCTRL_PIN(67, (char *)"I2C4_SDA"),
    PINCTRL_PIN(68, (char *)"I2C4_SCL"),
    PINCTRL_PIN(69, (char *)"SRCCLKREQB_4"),
    PINCTRL_PIN(70, (char *)"SRCCLKREQB_5"),
    PINCTRL_PIN(71, (char *)"M2_SKT2_CFG_0"),
    PINCTRL_PIN(72, (char *)"M2_SKT2_CFG_1"),
    PINCTRL_PIN(73, (char *)"M2_SKT2_CFG_2"),
    PINCTRL_PIN(74, (char *)"M2_SKT2_CFG_3"),
    PINCTRL_PIN(75, (char *)"DDPB_CTRLCLK"),
    PINCTRL_PIN(76, (char *)"DDPB_CTRLDATA"),
    PINCTRL_PIN(77, (char *)"CPU_VCCIO_PWR_GATEB"),
    PINCTRL_PIN(78, (char *)"TIME_SYNC_0"),
    PINCTRL_PIN(79, (char *)"IMGCLKOUT_1"),
    PINCTRL_PIN(80, (char *)"IMGCLKOUT_2"),
    PINCTRL_PIN(81, (char *)"IMGCLKOUT_3"),
    PINCTRL_PIN(82, (char *)"IMGCLKOUT_4"),
    /* GPP_D */
    PINCTRL_PIN(83, (char *)"ISH_GP_0"),
    PINCTRL_PIN(84, (char *)"ISH_GP_1"),
    PINCTRL_PIN(85, (char *)"ISH_GP_2"),
    PINCTRL_PIN(86, (char *)"ISH_GP_3"),
    PINCTRL_PIN(87, (char *)"IMGCLKOUT_0"),
    PINCTRL_PIN(88, (char *)"SRCCLKREQB_0"),
    PINCTRL_PIN(89, (char *)"SRCCLKREQB_1"),
    PINCTRL_PIN(90, (char *)"SRCCLKREQB_2"),
    PINCTRL_PIN(91, (char *)"SRCCLKREQB_3"),
    PINCTRL_PIN(92, (char *)"ISH_SPI_CSB"),
    PINCTRL_PIN(93, (char *)"ISH_SPI_CLK"),
    PINCTRL_PIN(94, (char *)"ISH_SPI_MISO"),
    PINCTRL_PIN(95, (char *)"ISH_SPI_MOSI"),
    PINCTRL_PIN(96, (char *)"ISH_UART0_RXD"),
    PINCTRL_PIN(97, (char *)"ISH_UART0_TXD"),
    PINCTRL_PIN(98, (char *)"ISH_UART0_RTSB"),
    PINCTRL_PIN(99, (char *)"ISH_UART0_CTSB"),
    PINCTRL_PIN(100, (char *)"ISH_GP_4"),
    PINCTRL_PIN(101, (char *)"ISH_GP_5"),
    PINCTRL_PIN(102, (char *)"I2S_MCLK"),
    PINCTRL_PIN(103, (char *)"GSPI2_CLK_LOOPBK"),
    /* GPP_F */
    PINCTRL_PIN(104, (char *)"CNV_BRI_DT"),
    PINCTRL_PIN(105, (char *)"CNV_BRI_RSP"),
    PINCTRL_PIN(106, (char *)"CNV_RGI_DT"),
    PINCTRL_PIN(107, (char *)"CNV_RGI_RSP"),
    PINCTRL_PIN(108, (char *)"CNV_RF_RESET_B"),
    PINCTRL_PIN(109, (char *)"EMMC_HIP_MON"),
    PINCTRL_PIN(110, (char *)"CNV_PA_BLANKING"),
    PINCTRL_PIN(111, (char *)"EMMC_CMD"),
    PINCTRL_PIN(112, (char *)"EMMC_DATA0"),
    PINCTRL_PIN(113, (char *)"EMMC_DATA1"),
    PINCTRL_PIN(114, (char *)"EMMC_DATA2"),
    PINCTRL_PIN(115, (char *)"EMMC_DATA3"),
    PINCTRL_PIN(116, (char *)"EMMC_DATA4"),
    PINCTRL_PIN(117, (char *)"EMMC_DATA5"),
    PINCTRL_PIN(118, (char *)"EMMC_DATA6"),
    PINCTRL_PIN(119, (char *)"EMMC_DATA7"),
    PINCTRL_PIN(120, (char *)"EMMC_RCLK"),
    PINCTRL_PIN(121, (char *)"EMMC_CLK"),
    PINCTRL_PIN(122, (char *)"EMMC_RESETB"),
    PINCTRL_PIN(123, (char *)"A4WP_PRESENT"),
    /* vGPIO */
    PINCTRL_PIN(124, (char *)"CNV_BTEN"),
    PINCTRL_PIN(125, (char *)"CNV_WCEN"),
    PINCTRL_PIN(126, (char *)"CNV_BT_HOST_WAKEB"),
    PINCTRL_PIN(127, (char *)"CNV_BT_IF_SELECT"),
    PINCTRL_PIN(128, (char *)"vCNV_BT_UART_TXD"),
    PINCTRL_PIN(129, (char *)"vCNV_BT_UART_RXD"),
    PINCTRL_PIN(130, (char *)"vCNV_BT_UART_CTS_B"),
    PINCTRL_PIN(131, (char *)"vCNV_BT_UART_RTS_B"),
    PINCTRL_PIN(132, (char *)"vCNV_MFUART1_TXD"),
    PINCTRL_PIN(133, (char *)"vCNV_MFUART1_RXD"),
    PINCTRL_PIN(134, (char *)"vCNV_MFUART1_CTS_B"),
    PINCTRL_PIN(135, (char *)"vCNV_MFUART1_RTS_B"),
    PINCTRL_PIN(136, (char *)"vUART0_TXD"),
    PINCTRL_PIN(137, (char *)"vUART0_RXD"),
    PINCTRL_PIN(138, (char *)"vUART0_CTS_B"),
    PINCTRL_PIN(139, (char *)"vUART0_RTS_B"),
    PINCTRL_PIN(140, (char *)"vISH_UART0_TXD"),
    PINCTRL_PIN(141, (char *)"vISH_UART0_RXD"),
    PINCTRL_PIN(142, (char *)"vISH_UART0_CTS_B"),
    PINCTRL_PIN(143, (char *)"vISH_UART0_RTS_B"),
    PINCTRL_PIN(144, (char *)"vCNV_BT_I2S_BCLK"),
    PINCTRL_PIN(145, (char *)"vCNV_BT_I2S_WS_SYNC"),
    PINCTRL_PIN(146, (char *)"vCNV_BT_I2S_SDO"),
    PINCTRL_PIN(147, (char *)"vCNV_BT_I2S_SDI"),
    PINCTRL_PIN(148, (char *)"vI2S2_SCLK"),
    PINCTRL_PIN(149, (char *)"vI2S2_SFRM"),
    PINCTRL_PIN(150, (char *)"vI2S2_TXD"),
    PINCTRL_PIN(151, (char *)"vI2S2_RXD"),
    PINCTRL_PIN(152, (char *)"vSD3_CD_B"),
    /* GPP_C */
    PINCTRL_PIN(153, (char *)"SMBCLK"),
    PINCTRL_PIN(154, (char *)"SMBDATA"),
    PINCTRL_PIN(155, (char *)"SMBALERTB"),
    PINCTRL_PIN(156, (char *)"SML0CLK"),
    PINCTRL_PIN(157, (char *)"SML0DATA"),
    PINCTRL_PIN(158, (char *)"SML0ALERTB"),
    PINCTRL_PIN(159, (char *)"SML1CLK"),
    PINCTRL_PIN(160, (char *)"SML1DATA"),
    PINCTRL_PIN(161, (char *)"UART0_RXD"),
    PINCTRL_PIN(162, (char *)"UART0_TXD"),
    PINCTRL_PIN(163, (char *)"UART0_RTSB"),
    PINCTRL_PIN(164, (char *)"UART0_CTSB"),
    PINCTRL_PIN(165, (char *)"UART1_RXD"),
    PINCTRL_PIN(166, (char *)"UART1_TXD"),
    PINCTRL_PIN(167, (char *)"UART1_RTSB"),
    PINCTRL_PIN(168, (char *)"UART1_CTSB"),
    PINCTRL_PIN(169, (char *)"I2C0_SDA"),
    PINCTRL_PIN(170, (char *)"I2C0_SCL"),
    PINCTRL_PIN(171, (char *)"I2C1_SDA"),
    PINCTRL_PIN(172, (char *)"I2C1_SCL"),
    PINCTRL_PIN(173, (char *)"UART2_RXD"),
    PINCTRL_PIN(174, (char *)"UART2_TXD"),
    PINCTRL_PIN(175, (char *)"UART2_RTSB"),
    PINCTRL_PIN(176, (char *)"UART2_CTSB"),
    /* HVCMOS */
    PINCTRL_PIN(177, (char *)"L_BKLTEN"),
    PINCTRL_PIN(178, (char *)"L_BKLTCTL"),
    PINCTRL_PIN(179, (char *)"L_VDDEN"),
    PINCTRL_PIN(180, (char *)"SYS_PWROK"),
    PINCTRL_PIN(181, (char *)"SYS_RESETB"),
    PINCTRL_PIN(182, (char *)"MLK_RSTB"),
    /* GPP_E */
    PINCTRL_PIN(183, (char *)"SATAXPCIE_0"),
    PINCTRL_PIN(184, (char *)"SPI1_IO_2"),
    PINCTRL_PIN(185, (char *)"SPI1_IO_3"),
    PINCTRL_PIN(186, (char *)"CPU_GP_0"),
    PINCTRL_PIN(187, (char *)"SATA_DEVSLP_0"),
    PINCTRL_PIN(188, (char *)"SATA_DEVSLP_1"),
    PINCTRL_PIN(189, (char *)"GPPC_E_6"),
    PINCTRL_PIN(190, (char *)"CPU_GP_1"),
    PINCTRL_PIN(191, (char *)"SATA_LEDB"),
    PINCTRL_PIN(192, (char *)"USB2_OCB_0"),
    PINCTRL_PIN(193, (char *)"SPI1_CSB"),
    PINCTRL_PIN(194, (char *)"SPI1_CLK"),
    PINCTRL_PIN(195, (char *)"SPI1_MISO_IO_1"),
    PINCTRL_PIN(196, (char *)"SPI1_MOSI_IO_0"),
    PINCTRL_PIN(197, (char *)"DDSP_HPD_A"),
    PINCTRL_PIN(198, (char *)"ISH_GP_6"),
    PINCTRL_PIN(199, (char *)"ISH_GP_7"),
    PINCTRL_PIN(200, (char *)"DISP_MISC_4"),
    PINCTRL_PIN(201, (char *)"DDP1_CTRLCLK"),
    PINCTRL_PIN(202, (char *)"DDP1_CTRLDATA"),
    PINCTRL_PIN(203, (char *)"DDP2_CTRLCLK"),
    PINCTRL_PIN(204, (char *)"DDP2_CTRLDATA"),
    PINCTRL_PIN(205, (char *)"DDPA_CTRLCLK"),
    PINCTRL_PIN(206, (char *)"DDPA_CTRLDATA"),
    /* JTAG */
    PINCTRL_PIN(207, (char *)"JTAG_TDO"),
    PINCTRL_PIN(208, (char *)"JTAGX"),
    PINCTRL_PIN(209, (char *)"PRDYB"),
    PINCTRL_PIN(210, (char *)"PREQB"),
    PINCTRL_PIN(211, (char *)"CPU_TRSTB"),
    PINCTRL_PIN(212, (char *)"JTAG_TDI"),
    PINCTRL_PIN(213, (char *)"JTAG_TMS"),
    PINCTRL_PIN(214, (char *)"JTAG_TCK"),
    PINCTRL_PIN(215, (char *)"ITP_PMODE"),
    /* GPP_R */
    PINCTRL_PIN(216, (char *)"HDA_BCLK"),
    PINCTRL_PIN(217, (char *)"HDA_SYNC"),
    PINCTRL_PIN(218, (char *)"HDA_SDO"),
    PINCTRL_PIN(219, (char *)"HDA_SDI_0"),
    PINCTRL_PIN(220, (char *)"HDA_RSTB"),
    PINCTRL_PIN(221, (char *)"HDA_SDI_1"),
    PINCTRL_PIN(222, (char *)"I2S1_TXD"),
    PINCTRL_PIN(223, (char *)"I2S1_RXD"),
    /* GPP_S */
    PINCTRL_PIN(224, (char *)"SNDW1_CLK"),
    PINCTRL_PIN(225, (char *)"SNDW1_DATA"),
    PINCTRL_PIN(226, (char *)"SNDW2_CLK"),
    PINCTRL_PIN(227, (char *)"SNDW2_DATA"),
    PINCTRL_PIN(228, (char *)"SNDW3_CLK"),
    PINCTRL_PIN(229, (char *)"SNDW3_DATA"),
    PINCTRL_PIN(230, (char *)"SNDW4_CLK"),
    PINCTRL_PIN(231, (char *)"SNDW4_DATA"),
    /* SPI */
    PINCTRL_PIN(232, (char *)"SPI0_IO_2"),
    PINCTRL_PIN(233, (char *)"SPI0_IO_3"),
    PINCTRL_PIN(234, (char *)"SPI0_MOSI_IO_0"),
    PINCTRL_PIN(235, (char *)"SPI0_MISO_IO_1"),
    PINCTRL_PIN(236, (char *)"SPI0_TPM_CSB"),
    PINCTRL_PIN(237, (char *)"SPI0_FLASH_0_CSB"),
    PINCTRL_PIN(238, (char *)"SPI0_FLASH_1_CSB"),
    PINCTRL_PIN(239, (char *)"SPI0_CLK"),
    PINCTRL_PIN(240, (char *)"SPI0_CLK_LOOPBK"),
};

static unsigned int icllp_spi0_pins[] = { 22, 23, 24, 25, 26 };
static unsigned int icllp_spi0_modes[] = { 3, 1, 1, 1, 1 };
static unsigned int icllp_spi1_pins[] = { 27, 28, 29, 30, 31 };
static unsigned int icllp_spi1_modes[] = { 1, 1, 1, 1, 3 };
static unsigned int icllp_spi2_pins[] = { 92, 93, 94, 95, 98 };
static unsigned int icllp_spi2_modes[] = { 3, 3, 3, 3, 2 };

static unsigned int icllp_i2c0_pins[] = { 169, 170 };
static unsigned int icllp_i2c1_pins[] = { 171, 172 };
static unsigned int icllp_i2c2_pins[] = { 63, 64 };
static unsigned int icllp_i2c3_pins[] = { 65, 66 };
static unsigned int icllp_i2c4_pins[] = { 67, 68 };

static unsigned int icllp_uart0_pins[] = { 161, 162, 163, 164 };
static unsigned int icllp_uart1_pins[] = { 165, 166, 167, 168 };
static unsigned int icllp_uart2_pins[] = { 173, 174, 175, 176 };

static struct intel_pingroup icllp_groups[] = {
    PIN_GROUP((char *)"spi0_grp", icllp_spi0_pins, icllp_spi0_modes),
    PIN_GROUP((char *)"spi1_grp", icllp_spi1_pins, icllp_spi1_modes),
    PIN_GROUP((char *)"spi2_grp", icllp_spi2_pins, icllp_spi2_modes),
    PIN_GROUP((char *)"i2c0_grp", icllp_i2c0_pins, 1),
    PIN_GROUP((char *)"i2c1_grp", icllp_i2c1_pins, 1),
    PIN_GROUP((char *)"i2c2_grp", icllp_i2c2_pins, 1),
    PIN_GROUP((char *)"i2c3_grp", icllp_i2c3_pins, 1),
    PIN_GROUP((char *)"i2c4_grp", icllp_i2c4_pins, 1),
    PIN_GROUP((char *)"uart0_grp", icllp_uart0_pins, 1),
    PIN_GROUP((char *)"uart1_grp", icllp_uart1_pins, 1),
    PIN_GROUP((char *)"uart2_grp", icllp_uart2_pins, 1),
};

static char * const icllp_spi0_groups[] = { (char *)"spi0_grp" };
static char * const icllp_spi1_groups[] = { (char *)"spi1_grp" };
static char * const icllp_spi2_groups[] = { (char *)"spi2_grp" };
static char * const icllp_i2c0_groups[] = { (char *)"i2c0_grp" };
static char * const icllp_i2c1_groups[] = { (char *)"i2c1_grp" };
static char * const icllp_i2c2_groups[] = { (char *)"i2c2_grp" };
static char * const icllp_i2c3_groups[] = { (char *)"i2c3_grp" };
static char * const icllp_i2c4_groups[] = { (char *)"i2c4_grp" };
static char * const icllp_uart0_groups[] = { (char *)"uart0_grp" };
static char * const icllp_uart1_groups[] = { (char *)"uart1_grp" };
static char * const icllp_uart2_groups[] = { (char *)"uart2_grp" };

static struct intel_function icllp_functions[] = {
    FUNCTION((char *)"spi0", icllp_spi0_groups),
    FUNCTION((char *)"spi1", icllp_spi1_groups),
    FUNCTION((char *)"spi2", icllp_spi2_groups),
    FUNCTION((char *)"i2c0", icllp_i2c0_groups),
    FUNCTION((char *)"i2c1", icllp_i2c1_groups),
    FUNCTION((char *)"i2c2", icllp_i2c2_groups),
    FUNCTION((char *)"i2c3", icllp_i2c3_groups),
    FUNCTION((char *)"i2c4", icllp_i2c4_groups),
    FUNCTION((char *)"uart0", icllp_uart0_groups),
    FUNCTION((char *)"uart1", icllp_uart1_groups),
    FUNCTION((char *)"uart2", icllp_uart2_groups),
};

static struct intel_padgroup icllp_community0_gpps[] = {
    ICL_GPP(0, 0, 7, 0),                /* GPP_G */
    ICL_GPP(1, 8, 33, 32),                /* GPP_B */
    ICL_GPP(2, 34, 58, 64),                /* GPP_A */
};
static struct intel_padgroup icllp_community1_gpps[] = {
    ICL_GPP(0, 59, 82, 96),                /* GPP_H */
    ICL_GPP(1, 83, 103, 128),            /* GPP_D */
    ICL_GPP(2, 104, 123, 160),            /* GPP_F */
    ICL_GPP(3, 124, 152, 192),            /* vGPIO */
};

static struct intel_padgroup icllp_community4_gpps[] = {
    ICL_GPP(0, 153, 176, 224),            /* GPP_C */
    ICL_GPP(1, 177, 182, ICL_NO_GPIO),    /* HVCMOS */
    ICL_GPP(2, 183, 206, 256),            /* GPP_E */
    ICL_GPP(3, 207, 215, ICL_NO_GPIO),    /* JTAG */
};

static struct intel_padgroup icllp_community5_gpps[] = {
    ICL_GPP(0, 216, 223, 288),            /* GPP_R */
    ICL_GPP(1, 224, 231, 320),            /* GPP_S */
    ICL_GPP(2, 232, 240, ICL_NO_GPIO),    /* SPI */
};

static struct intel_community icllp_communities[] = {
    ICL_COMMUNITY(0, 0, 58, icllp_community0_gpps),
    ICL_COMMUNITY(1, 59, 152, icllp_community1_gpps),
    ICL_COMMUNITY(2, 153, 215, icllp_community4_gpps),
    ICL_COMMUNITY(3, 216, 240, icllp_community5_gpps),
};

class VoodooGPIOIceLakeLP : public VoodooGPIO {
    OSDeclareDefaultStructors(VoodooGPIOIceLakeLP);

    bool start(IOService *provider) override;
};

#endif /* VoodooGPIOIceLakeLP_h */
