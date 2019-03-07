/*
 * inteltool - dump all registers on an Intel CPU + chipset based system.
 *
 * Copyright (C) 2017 secunet Security Networks AG
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include "inteltool.h"
#include "pcr.h"

#define SBBAR_SIZE	(16 * MiB)
#define PCR_PORT_SIZE	(64 * KiB)

struct gpio_group {
	const char *display;
	size_t pad_count;
	size_t func_count;
	const char *const *pad_names; /* indexed by 'pad * func_count + func' */
};

struct gpio_community {
	const char *name;
	uint8_t pcr_port_id;
	size_t group_count;
	const struct gpio_group *const *groups;
};

/*
 * Names prefixed with an *asterisk are the default.
 * (if it's the first column, GPIO is the default, no matter the name)
 */

static const char *const apl_group_north_names[] = {
	"*GPIO_0",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_1",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_2",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_3",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_4",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_5",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_6",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_7",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_8",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_9",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_10",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_11",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_12",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_13",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_14",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_15",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_16",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_17",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_18",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_19",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_20",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_21",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_22",	"n/a",		"n/a",	"n/a",	"n/a",	"SATA_GP0",
	"*GPIO_23",	"n/a",		"n/a",	"n/a",	"n/a",	"SATA_GP1",
	"*GPIO_24",	"n/a",		"n/a",	"n/a",	"n/a",	"SATA_DEVSLP0",
	"*GPIO_25",	"n/a",		"n/a",	"n/a",	"n/a",	"SATA_DEVSLP1",
	"*GPIO_26",	"n/a",		"n/a",	"n/a",	"n/a",	"SATA_LEDN",
	"*GPIO_27",	"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_28",	"n/a",	"ISH_GPIO_10",	"n/a",	"n/a",	"n/a",
	"*GPIO_29",	"n/a",	"ISH_GPIO_11",	"n/a",	"n/a",	"n/a",
	"*GPIO_30",	"ISH_GPIO_12",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_31",	"ISH_GPIO_13",	"n/a",	"n/a",	"n/a",	"SUSCLK1",
	"*GPIO_32",	"ISH_GPIO_14",	"n/a",	"n/a",	"n/a",	"SUSCLK2",
	"*GPIO_33",	"ISH_GPIO_15",	"n/a",	"n/a",	"n/a",	"SUSCLK3",
	"*GPIO_34",	"PWM0",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_35",	"PWM1",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_36",	"PWM2",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_37",	"PWM3",		"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_38", "LPSS_UART0_RXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_39", "LPSS_UART0_TXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_40", "LPSS_UART0_RTS_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_41", "LPSS_UART0_CTS_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_42", "LPSS_UART1_RXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_43", "LPSS_UART1_TXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_44", "LPSS_UART1_RTS_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_45", "LPSS_UART1_CTS_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_46", "LPSS_UART2_RXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_47", "LPSS_UART2_TXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_48", "LPSS_UART2_RTS_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_49", "LPSS_UART2_CTS_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_62",	"GP_CAMERASB0",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_63",	"GP_CAMERASB1",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_64",	"GP_CAMERASB2",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_65",	"GP_CAMERASB3",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_66",	"GP_CAMERASB4",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_67",	"GP_CAMERASB5",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_68",	"GP_CAMERASB6",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_69",	"GP_CAMERASB7",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_70",	"GP_CAMERASB8",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_71",	"GP_CAMERASB9",	"n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_72",	"GP_CAMERASB10","n/a",	"n/a",	"n/a",	"n/a",
	"*GPIO_73",	"GP_CAMERASB11","n/a",	"n/a",	"n/a",	"n/a",
	"TCK",		"*JTAG_TCK",	"n/a",	"n/a",	"n/a",	"n/a",
	"TRST_B",	"*JTAG_TRST_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"TMS",		"*JTAG_TMS",	"n/a",	"n/a",	"n/a",	"n/a",
	"TDI",		"*JTAG_TDI",	"n/a",	"n/a",	"n/a",	"n/a",
	"CX_PMODE",	"*JTAG_PMODE",	"n/a",	"n/a",	"n/a",	"n/a",
	"CX_PREQ_B",	"*JTAG_PREQ_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"JTAGX"	,	"*JTAGX",	"n/a",	"n/a",	"n/a",	"n/a",
	"CX_PRDY_B",	"*JTAG_PRDY_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"TDO",		"*JTAG_TDO",	"n/a",	"n/a",	"n/a",	"n/a",
	"CNV_BRI_DT",	"*GPIO_216",	"n/a",	"n/a",	"n/a",	"n/a",
	"CNV_BRI_RSP",	"*GPIO_217",	"n/a",	"n/a",	"n/a",	"n/a",
	"CNV_RGI_DT",	"*GPIO_218",	"n/a",	"n/a",	"n/a",	"n/a",
	"CNV_RGI_RSP",	"*n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"SVID0_ALERT_B","*SVID0_ALERT_N","n/a",	"n/a",	"n/a",	"n/a",
	"SVID0_DATA",	"*SVID0_DATA",	"n/a",	"n/a",	"n/a",	"n/a",
	"SVID0_CLK",	"*SVID0_CLK",	"n/a",	"n/a",	"n/a",	"n/a",
};

static const struct gpio_group apl_group_north = {
	.display	= "------- GPIO Group North -------",
	.pad_count	= ARRAY_SIZE(apl_group_north_names) / 6,
	.func_count	= 6,
	.pad_names	= apl_group_north_names,
};

static const struct gpio_group *const apl_community_north_groups[] = {
	&apl_group_north,
};

static const struct gpio_community apl_community_north = {
	.name		= "----- GPIO Community North -----",
	.pcr_port_id	= 0xc5,
	.group_count	= ARRAY_SIZE(apl_community_north_groups),
	.groups		= apl_community_north_groups,
};

static const char *const apl_group_northwest_names[] = {
	"GPIO_187",	 "*DDI0_DDC_SDA",	"n/a",		"n/a",
	"GPIO_188",	 "*DDI0_DDC_SCL",	"n/a",		"n/a",
	"GPIO_189",	 "*DDI1_DDC_SDA",	"n/a",		"n/a",
	"GPIO_190",	 "*DDI1_DDC_SCL",	"n/a",		"n/a",
	"GPIO_191",	 "*MIPI_I2C_SDA",	"n/a",		"n/a",
	"GPIO_192",	 "*MIPI_I2C_SCL",	"n/a",		"n/a",
	"GPIO_193",	 "*PNL0_VDDEN",		"n/a",		"n/a",
	"GPIO_194",	 "*PNL0_BKLTEN",	"n/a",		"n/a",
	"GPIO_195",	 "*PNL0_BKLTCTL",	"n/a",		"n/a",
	"GPIO_196",	 "*PNL1_VDDEN",		"n/a",		"n/a",
	"GPIO_197",	 "*PNL1_BKLTEN",	"n/a",		"n/a",
	"GPIO_198",	 "*PNL1_BKLTCTL",	"n/a",		"n/a",
	"GPIO_199",	 "*GPIO_199",		"DDI1_HPD",	"n/a",
	"GPIO_200",	 "*GPIO_200",		"DDI0_HPD",	"n/a",
	"GPIO_201",	 "*MDSI_A_TE",		"n/a",		"n/a",
	"GPIO_202",	 "*MDSI_C_TE",		"n/a",		"n/a",
	"GPIO_203",	 "*USB_OC0_N",		"n/a",		"n/a",
	"GPIO_204",	 "*USB_OC1_N",		"n/a",		"n/a",
	"PMC_SPI_FS0",	 "*PMC_SPI_FS0",	"n/a",		"n/a",
	"PMC_SPI_FS1",	 "*PMC_SPI_FS1",	"DDI2_HPD",	"n/a",
	"PMC_SPI_FS2",	 "*PMC_SPI_FS2",	"FST_SPI_CS2_N","n/a",
	"PMC_SPI_RXD",	 "*PMC_SPI_RXD",	"n/a",		"n/a",
	"PMC_SPI_TXD",	 "*PMC_SPI_TXD",	"n/a",		"n/a",
	"PMC_SPI_CLK",	 "*PMC_SPI_CLK",	"n/a",		"n/a",
	"PMIC_PWRGOOD",	 "*n/a",		"n/a",		"n/a",
	"PMIC_RESET_B",	 "*GPIO_223",		"n/a",		"n/a",
	"GPIO_213",	 "*GPIO_213",		"n/a",		"n/a",
	"GPIO_214",	 "*GPIO_214",		"n/a",		"n/a",
	"GPIO_215",	 "*GPIO_215",		"n/a",		"n/a",
	"PMIC_THERMTRIP_B", "*THERMTRIP_N",	"n/a",		"n/a",
	"PMIC_STDBY",	 "*GPIO_224",		"n/a",		"n/a",
	"PROCHOT_B",	 "*PROCHOT_N",		"n/a",		"n/a",
	"PMIC_I2C_SCL",	 "*PMIC_I2C_SCL",	"n/a",		"n/a",
	"PMIC_I2C_SDA",	 "*PMIC_I2C_SDA",	"n/a",		"n/a",
	"*GPIO_74",	 "AVS_I2S1_MCLK" ,	"n/a",		"n/a",
	"*GPIO_75",	 "AVS_I2S1_BCLK",	"n/a",		"n/a",
	"*GPIO_76",	 "AVS_I2S1_WS_SYNC",	"n/a",		"n/a",
	"*GPIO_77",	 "AVS_I2S1_SDI",	"n/a",		"n/a",
	"*GPIO_78",	 "AVS_I2S1_SDO",	"n/a",		"n/a",
	"*GPIO_79",	 "AVS_DMIC_CLK_A1",	"AVS_I2S4_BCLK","n/a",
	"*GPIO_80",	 "AVS_DMIC_CLK_B1",	"AVS_I2S4_WS_SYNC","n/a",
	"*GPIO_81",	 "AVS_DMIC_DATA_1",	"AVS_I2C4_SDI",	"n/a",
	"*GPIO_82",	 "AVS_DMIC_CLK_AB2",	"AVS_I2S4_SDO",	"n/a",
	"*GPIO_83",	 "AVS_DMIC_DATA_2",	"n/a",		"n/a",
	"*GPIO_84",	 "AVS_I2S2_MCLK",	"AVS_HDA_RST_N","n/a",
	"*GPIO_85",	 "AVS_I2S2_BCLK",	"n/a",		"n/a",
	"*GPIO_86",	 "AVS_I2S2_WS_SYNC",	"n/a",		"n/a",
	"*GPIO_87",	 "AVS_I2S2_SDI",	"n/a",		"n/a",
	"*GPIO_88",	 "AVS_I2S2_SDO",	"n/a",		"n/a",
	"*GPIO_89",	 "AVS_I2S3_BCLK",	"n/a",		"n/a",
	"*GPIO_90",	 "AVS_I2S3_WS_SYNC",	"n/a",		"n/a",
	"*GPIO_91",	 "AVS_I2S3_SDI",	"n/a",		"n/a",
	"*GPIO_92",	 "AVS_I2S3_SDO",	"n/a",		"n/a",
	"GPIO_97",	 "*FST_SPI_CS0_N",	"n/a",		"n/a",
	"GPIO_98",	 "*FST_SPI_CS1_N",	"n/a",		"n/a",
	"GPIO_99",	 "*FST_SPI_MOSI_IO0",	"n/a",		"n/a",
	"GPIO_100",	 "*FST_SPI_MISO_IO1",	"n/a",		"n/a",
	"GPIO_101",	 "*FST_SPI_IO2",	"n/a",		"n/a",
	"GPIO_102",	 "*FST_SPI_IO3",	"n/a",		"n/a",
	"GPIO_103",	 "*FST_SPI_CLK",	"n/a",		"n/a",
	"FST_SPI_CLK_FB", "*n/a",		"n/a",		"n/a",
	"*GPIO_104",	 "SIO_SPI_0_CLK",	"n/a",		"n/a",
	"*GPIO_105",	 "SIO_SPI_0_FS0",	"n/a",		"n/a",
	"*GPIO_106",	 "SIO_SPI_0_FS1",	"n/a",		"FST_SPI_CS2_N",
	"*GPIO_109",	 "SIO_SPI_0_RXD",	"n/a",		"n/a",
	"*GPIO_110",	 "SIO_SPI_0_TXD",	"n/a",		"n/a",
	"*GPIO_111",	 "SIO_SPI_1_CLK",	"n/a",		"n/a",
	"*GPIO_112",	 "SIO_SPI_1_FS0",	"n/a",		"n/a",
	"*GPIO_113",	 "SIO_SPI_1_FS1",	"n/a",		"n/a",
	"*GPIO_116",	 "SIO_SPI_1_RXD",	"n/a",		"n/a",
	"*GPIO_117",	 "SIO_SPI_1_TXD",	"n/a",		"n/a",
	"*GPIO_118",	 "SIO_SPI_2_CLK",	"n/a",		"n/a",
	"*GPIO_119",	 "SIO_SPI_2_FS0",	"n/a",		"n/a",
	"*GPIO_120",	 "SIO_SPI_2_FS1",	"n/a",		"n/a",
	"*GPIO_121",	 "SIO_SPI_2_FS2",	"n/a",		"n/a",
	"*GPIO_122",	 "SIO_SPI_2_RXD",	"n/a",		"n/a",
	"*GPIO_123",	 "SIO_SPI_2_TXD",	"n/a",		"n/a",
};

static const struct gpio_group apl_group_northwest = {
	.display	= "----- GPIO Group NorthWest -----",
	.pad_count	= ARRAY_SIZE(apl_group_northwest_names) / 4,
	.func_count	= 4,
	.pad_names	= apl_group_northwest_names,
};

static const struct gpio_group *const apl_community_northwest_groups[] = {
	&apl_group_northwest,
};

static const struct gpio_community apl_community_northwest = {
	.name		= "--- GPIO Community NorthWest ---",
	.pcr_port_id	= 0xc4,
	.group_count	= ARRAY_SIZE(apl_community_northwest_groups),
	.groups		= apl_community_northwest_groups,
};


static const char *const apl_group_west_names[] = {
	"*GPIO_124",	"LPSS_I2C0_SDA",	"n/a",		"n/a",
	"*GPIO_125",	"LPSS_I2C0_SCL",	"n/a",		"n/a",
	"*GPIO_126",	"LPSS_I2C1_SDA",	"n/a",		"n/a",
	"*GPIO_127",	"LPSS_I2C1_SCL",	"n/a",		"n/a",
	"*GPIO_128",	"LPSS_I2C2_SDA",	"n/a",		"n/a",
	"*GPIO_129",	"LPSS_I2C2_SCL",	"n/a",		"n/a",
	"*GPIO_130",	"LPSS_I2C3_SDA",	"n/a",		"n/a",
	"*GPIO_131",	"LPSS_I2C3_SCL",	"n/a",		"n/a",
	"*GPIO_132",	"LPSS_I2C4_SDA",	"n/a",		"n/a",
	"*GPIO_133",	"LPSS_I2C4_SCL",	"n/a",		"n/a",
	"*GPIO_134",	"LPSS_I2C5_SDA","ISH_I2C0_SDA",		"n/a",
	"*GPIO_135",	"LPSS_I2C5_SCL","ISH_I2C0_SCL",		"n/a",
	"*GPIO_136",	"LPSS_I2C6_SDA","ISH_I2C1_SDA",		"n/a",
	"*GPIO_137",	"LPSS_I2C6_SCL","ISH_I2C1_SCL",		"n/a",
	"*GPIO_138",	"LPSS_I2C7_SDA","ISH_I2C2_SDA",		"n/a",
	"*GPIO_139",	"LPSS_I2C7_SCL","ISH_I2C2_SCL",		"n/a",
	"*GPIO_146",	"ISH_GPIO_0",	"AVS_I2S6_BCLK",	"AVS_HDA_BCLK",
	"*GPIO_147",	"ISH_GPIO_1",	"AVS_I2S6_WS_SYNC",	"AVS_HDA_WS_SYNC",
	"*GPIO_148",	"ISH_GPIO_2",	"AVS_I2S6_SDI",		"AVS_HDA_SDI",
	"*GPIO_149",	"ISH_GPIO_3",	"AVS_I2S6_SDO",		"AVS_HDA_SDO",
	"*GPIO_150",	"ISH_GPIO_4",	"AVS_I2S5_BCLK",	"LPSS_UART2_RXD",
	"*GPIO_151",	"ISH_GPIO_5",	"AVS_I2S5_WS_SYNC",	"LPSS_UART2_TXD",
	"*GPIO_152",	"ISH_GPIO_6",	"AVS_I2S5_SDI",		"LPSS_UART2_RTS_B",
	"*GPIO_153",	"ISH_GPIO_7",	"AVS_I2S5_SDO",		"LPSS_UART2_CTS_B",
	"*GPIO_154",	"ISH_GPIO_8",		"n/a",		"n/a",
	"*GPIO_155",	"ISH_GPIO_9",		"SPKR",		"n/a",
	"GPIO_209",	"*PCIE_CLKREQ0_N",	"MODEM_CLKREQ", "n/a",
	"GPIO_210",	"*PCIE_CLKREQ1_N",	"n/a",		"n/a",
	"GPIO_211",	"*PCIE_CLKREQ2_N",	"n/a",		"n/a",
	"GPIO_212",	"*PCIE_CLKREQ3_N",	"n/a",		"n/a",
	"OSC_CLK_OUT_0","*OSC_CLK_OUT_0",	"n/a",		"n/a",
	"OSC_CLK_OUT_1","*OSC_CLK_OUT_1",	"n/a",		"n/a",
	"OSC_CLK_OUT_2","*OSC_CLK_OUT_2",	"n/a",		"n/a",
	"OSC_CLK_OUT_3","*OSC_CLK_OUT_3",	"n/a",		"n/a",
	"OSC_CLK_OUT_4","*OSC_CLK_OUT_4",	"n/a",		"n/a",
	"*PMU_AC_PRESENT","PMU_AC_PRESENT",	"n/a",		"n/a",
	"PMU_BATLOW_B",	"*PMU_BATLOW_N",	"n/a",		"n/a",
	"PMU_PLTRST_B",	"*PMU_PLTRST_N",	"n/a",		"n/a",
	"PMU_PWRBTN_B",	"*PMU_PWRBTN_N",	"n/a",		"n/a",
	"PMU_RESETBUTTON_B",	"*PMU_RSTBTN_N",	"n/a",	"n/a",
	"PMU_SLP_S0_B",	"*PMU_SLP_S0_N",	"n/a",		"n/a",
	"PMU_SLP_S3_B",	"*PMU_SLP_S3_N",	"n/a",		"n/a",
	"PMU_SLP_S4_B",	"*PMU_SLP_S4_N",	"n/a",		"n/a",
	"PMU_SUSCLK",	"*PMU_SUSCLK",		"n/a",		"n/a",
	"*PMU_WAKE_B",	"PMU_WAKE_B/EMMC_PWR_EN_N","n/a",	"n/a",
	"SUS_STAT_B",	"*SUS_STAT_B",		"n/a",		"n/a",
	"SUSPWRDNACK",	"*SUSPWRDNACK",		"n/a",		"n/a",
};

static const struct gpio_group apl_group_west = {
	.display	= "-------- GPIO Group West -------",
	.pad_count	= ARRAY_SIZE(apl_group_west_names) / 4,
	.func_count	= 4,
	.pad_names	= apl_group_west_names,
};

static const struct gpio_group *const apl_community_west_groups[] = {
	&apl_group_west,
};

static const struct gpio_community apl_community_west = {
	.name		= "------ GPIO Community West -----",
	.pcr_port_id	= 0xc7,
	.group_count	= ARRAY_SIZE(apl_community_west_groups),
	.groups		= apl_community_west_groups,
};

static const char *const apl_group_southwest_names[] = {
	"*GPIO_205",		"PCIE_WAKE0_N",		"n/a",
	"*GPIO_206",		"PCIE_WAKE1_N",		"n/a",
	"*GPIO_207",		"PCIE_WAKE2_N",		"n/a",
	"*GPIO_208",		"PCIE_WAKE3_N",		"n/a",
	"GPIO_156",		"*EMMC_CLK",		"n/a",
	"GPIO_157",		"*EMMC_D0",		"n/a",
	"GPIO_158",		"*EMMC_D1",		"n/a",
	"GPIO_159",		"*EMMC_D2",		"n/a",
	"GPIO_160",		"*EMMC_D3",		"n/a",
	"GPIO_161",		"*EMMC_D4",		"n/a",
	"GPIO_162",		"*EMMC_D5",		"n/a",
	"GPIO_163",		"*EMMC_D6",		"n/a",
	"GPIO_164",		"*EMMC_D7",		"n/a",
	"GPIO_165",		"*EMMC_CMD",		"n/a",
	"*GPIO_166",		"GPIO_166",		"n/a",
	"*GPIO_167",		"GPIO_167",		"n/a",
	"*GPIO_168",		"GPIO_168",		"n/a",
	"*GPIO_169",		"GPIO_169",		"n/a",
	"*GPIO_170",		"GPIO_170",		"n/a",
	"*GPIO_171",		"GPIO_171",		"n/a",
	"*GPIO_172",		"SDCARD_CLK",		"n/a",
	"*GPIO_179",		"n/a",			"n/a",
	"*GPIO_173",		"SDCARD_D0",		"n/a",
	"*GPIO_174",		"SDCARD_D1",		"n/a",
	"*GPIO_175",		"SDCARD_D2",		"n/a",
	"*GPIO_176",		"SDCARD_D3",		"n/a",
	"*GPIO_177",		"SDCARD_CD_B",		"n/a",
	"*GPIO_178",		"SDCARD_CMD",		"n/a",
	"*GPIO_186",		"SDCARD_LVL_WP",	"n/a",
	"GPIO_182",		"*EMMC_RCLK",		"n/a",
	"GPIO_183",		"GPIO_183",		"n/a",
	"*SMB_ALERTB",		"SMB_ALERT_N",		"n/a",
	"*SMB_CLK",		"SMB_CLK",		"LPSS_I2C7_SCL",
	"*SMB_DATA",		"SMB_DATA",		"LPSS_I2C7_SDA",
	"*LPC_ILB_SERIRQ",	"LPC_ILB_SERIRQ",	"n/a",
	"*LPC_CLKOUT0",		"LPC_CLKOUT0",		"n/a",
	"*LPC_CLKOUT1",		"LPC_CLKOUT1",		"n/a",
	"*LPC_AD0",		"LPC_AD0",		"n/a",
	"*LPC_AD1",		"LPC_AD1",		"n/a",
	"*LPC_AD2",		"LPC_AD2",		"n/a",
	"*LPC_AD3",		"LPC_AD3",		"n/a",
	"*LPC_CLKRUNB",		"LPC_CLKRUNB",		"n/a",
	"*LPC_FRAMEB",		"LPC_FRAMEB",		"n/a",
};

static const struct gpio_group apl_group_southwest = {
	.display	= "----- GPIO Group SouthWest -----",
	.pad_count	= ARRAY_SIZE(apl_group_southwest_names) / 3,
	.func_count	= 3,
	.pad_names	= apl_group_southwest_names,
};

static const struct gpio_group *const apl_community_southwest_groups[] = {
	&apl_group_southwest,
};

static const struct gpio_community apl_community_southwest = {
	.name		= "--- GPIO Community SouthWest ---",
	.pcr_port_id	= 0xc0,
	.group_count	= ARRAY_SIZE(apl_community_southwest_groups),
	.groups		= apl_community_southwest_groups,
};

static const struct gpio_community *const apl_communities[] = {
	&apl_community_north, &apl_community_northwest,
	&apl_community_west, &apl_community_southwest,
};

static const char *const sunrise_group_a_names[] = {
	"GPP_A0",	"RCIN#",		"n/a",		"ESPI_ALERT1#",
	"GPP_A1",	"LAD0",			"n/a",		"ESPI_IO0",
	"GPP_A2",	"LAD1",			"n/a",		"ESPI_IO1",
	"GPP_A3",	"LAD2",			"n/a",		"ESPI_IO2",
	"GPP_A4",	"LAD3",			"n/a",		"ESPI_IO3",
	"GPP_A5",	"LFRAME#",		"n/a",		"ESPI_CS#",
	"GPP_A6",	"SERIRQ",		"n/a",		"ESPI_CS1#",
	"GPP_A7",	"PIRQA#",		"n/a",		"ESPI_ALERT0#",
	"GPP_A8",	"CLKRUN#",		"n/a",		"n/a",
	"GPP_A9",	"CLKOUT_LPC0",		"n/a",		"ESPI_CLK",
	"GPP_A10",	"CLKOUT_LPC1",		"n/a",		"n/a",
	"GPP_A11",	"PME#",			"n/a",		"n/a",
	"GPP_A12",	"BM_BUSY#",		"ISH_GP6",	"SX_EXIT_HOLDOFF#",
	"GPP_A13",	"SUSWARN#/SUSPWRDNACK",	"n/a",		"n/a",
	"GPP_A14",	"SUS_STAT#",		"n/a",		"ESPI_RESET#",
	"GPP_A15",	"SUS_ACK#",		"n/a",		"n/a",
	"GPP_A16",	"CLKOUT_48",		"n/a",		"n/a",
	"GPP_A17",	"ISH_GP7",		"n/a",		"n/a",
	"GPP_A18",	"ISH_GP0",		"n/a",		"n/a",
	"GPP_A19",	"ISH_GP1",		"n/a",		"n/a",
	"GPP_A20",	"ISH_GP2",		"n/a",		"n/a",
	"GPP_A21",	"ISH_GP3",		"n/a",		"n/a",
	"GPP_A22",	"ISH_GP4",		"n/a",		"n/a",
	"GPP_A23",	"ISH_GP5",		"n/a",		"n/a",
};

static const struct gpio_group sunrise_group_a = {
	.display	= "------- GPIO Group GPP_A -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_a_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_a_names,
};

static const char *const sunrise_lp_group_a_names[] = {
	"GPP_A0",	"RCIN#",		"n/a",		"n/a",
	"GPP_A1",	"LAD0",			"n/a",		"ESPI_IO0",
	"GPP_A2",	"LAD1",			"n/a",		"ESPI_IO1",
	"GPP_A3",	"LAD2",			"n/a",		"ESPI_IO2",
	"GPP_A4",	"LAD3",			"n/a",		"ESPI_IO3",
	"GPP_A5",	"LFRAME#",		"n/a",		"ESPI_CS#",
	"GPP_A6",	"SERIRQ",		"n/a",		"n/a",
	"GPP_A7",	"PIRQA#",		"n/a",		"n/a",
	"GPP_A8",	"CLKRUN#",		"n/a",		"n/a",
	"GPP_A9",	"CLKOUT_LPC0",		"n/a",		"ESPI_CLK",
	"GPP_A10",	"CLKOUT_LPC1",		"n/a",		"n/a",
	"GPP_A11",	"PME#",			"n/a",		"n/a",
	"GPP_A12",	"BM_BUSY#",		"ISH_GP6",	"SX_EXIT_HOLDOFF#",
	"GPP_A13",	"SUSWARN#/SUSPWRDNACK",	"n/a",		"n/a",
	"GPP_A14",	"SUS_STAT#",		"n/a",		"ESPI_RESET#",
	"GPP_A15",	"SUS_ACK#",		"n/a",		"n/a",
	"GPP_A16",	"SD_1P8_SEL",		"n/a",		"n/a",
	"GPP_A17",	"SD_PWR_EN#",		"ISH_GP7",	"n/a",
	"GPP_A18",	"ISH_GP0",		"n/a",		"n/a",
	"GPP_A19",	"ISH_GP1",		"n/a",		"n/a",
	"GPP_A20",	"ISH_GP2",		"n/a",		"n/a",
	"GPP_A21",	"ISH_GP3",		"n/a",		"n/a",
	"GPP_A22",	"ISH_GP4",		"n/a",		"n/a",
	"GPP_A23",	"ISH_GP5",		"n/a",		"n/a",
};

static const struct gpio_group sunrise_lp_group_a = {
	.display	= "------- GPIO group GPP_A -------",
	.pad_count	= ARRAY_SIZE(sunrise_lp_group_a_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_lp_group_a_names,
};

static const char *const sunrise_group_b_names[] = {
	"GPP_B0",	"n/a",		"n/a",		"n/a",
	"GPP_B1",	"n/a",		"n/a",		"n/a",
	"GPP_B2",	"VRALERT#",	"n/a",		"n/a",
	"GPP_B3",	"CPU_GP2",	"n/a",		"n/a",
	"GPP_B4",	"CPU_GP3",	"n/a",		"n/a",
	"GPP_B5",	"SRCCLKREQ0#",	"n/a",		"n/a",
	"GPP_B6",	"SRCCLKREQ1#",	"n/a",		"n/a",
	"GPP_B7",	"SRCCLKREQ2#",	"n/a",		"n/a",
	"GPP_B8",	"SRCCLKREQ3#",	"n/a",		"n/a",
	"GPP_B9",	"SRCCLKREQ4#",	"n/a",		"n/a",
	"GPP_B10",	"SRCCLKREQ5#",	"n/a",		"n/a",
	"GPP_B11",	"n/a",		"n/a",		"n/a",
	"GPP_B12",	"SLP_S0#",	"n/a",		"n/a",
	"GPP_B13",	"PLTRST#",	"n/a",		"n/a",
	"GPP_B14",	"SPKR",		"n/a",		"n/a",
	"GPP_B15",	"GSPIO_CS#",	"n/a",		"n/a",
	"GPP_B16",	"GSPIO_CLK",	"n/a",		"n/a",
	"GPP_B17",	"GSPIO_MISO",	"n/a",		"n/a",
	"GPP_B18",	"GSPIO_MOSI",	"n/a",		"n/a",
	"GPP_B19",	"GSPI1_CS#",	"n/a",		"n/a",
	"GPP_B20",	"GSPI1_CLK",	"n/a",		"n/a",
	"GPP_B21",	"GSPI1_MISO",	"n/a",		"n/a",
	"GPP_B22",	"GSPI1_MOSI",	"n/a",		"n/a",
	"GPP_B23",	"SML1ALERT#",	"PCHHOT#",	"n/a",
};

static const struct gpio_group sunrise_group_b = {
	.display	= "------- GPIO Group GPP_B -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_b_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_b_names,
};

static const char *const sunrise_lp_group_b_names[] = {
	"GPP_B0",	"CORE_VID0",		"n/a",		"n/a",
	"GPP_B1",	"CORE_VID1",		"n/a",		"n/a",
	"GPP_B2",	"VRALERT#",		"n/a",		"n/a",
	"GPP_B3",	"CPU_GP2",		"n/a",		"n/a",
	"GPP_B4",	"CPU_GP3",		"n/a",		"n/a",
	"GPP_B5",	"SRCCLKREQ0#",		"n/a",		"n/a",
	"GPP_B6",	"SRCCLKREQ1#",		"n/a",		"n/a",
	"GPP_B7",	"SRCCLKREQ2#",		"n/a",		"n/a",
	"GPP_B8",	"SRCCLKREQ3#",		"n/a",		"n/a",
	"GPP_B9",	"SRCCLKREQ4#",		"n/a",		"n/a",
	"GPP_B10",	"SRCCLKREQ5#",		"n/a",		"n/a",
	"GPP_B11",	"EXT_PWR_GATE#",	"n/a",		"n/a",
	"GPP_B12",	"SLP_S0#",		"n/a",		"n/a",
	"GPP_B13",	"PLTRST#",		"n/a",		"n/a",
	"GPP_B14",	"SPKR",			"n/a",		"n/a",
	"GPP_B15",	"GSPI0_CS#",		"n/a",		"n/a",
	"GPP_B16",	"GSPI0_CLK",		"n/a",		"n/a",
	"GPP_B17",	"GSPI0_MISO",		"n/a",		"n/a",
	"GPP_B18",	"GSPI0_MOSI",		"n/a",		"n/a",
	"GPP_B19",	"GSPI1_CS#",		"n/a",		"n/a",
	"GPP_B20",	"GSPI1_CLK",		"n/a",		"n/a",
	"GPP_B21",	"GSPI1_MISO",		"n/a",		"n/a",
	"GPP_B22",	"GSPI1_MOSI",		"n/a",		"n/a",
	"GPP_B23",	"SML1ALERT#",		"PCHHOT#",	"n/a",
};

static const struct gpio_group sunrise_lp_group_b = {
	.display	= "------- GPIO Group GPP_B -------",
	.pad_count	= ARRAY_SIZE(sunrise_lp_group_b_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_lp_group_b_names,
};

static const struct gpio_group *const sunrise_community_ab_groups[] = {
	&sunrise_group_a, &sunrise_group_b,
};

static const struct gpio_group *const sunrise_lp_community_ab_groups[] = {
	&sunrise_lp_group_a, &sunrise_lp_group_b,
};

static const struct gpio_community sunrise_community_ab = {
	.name		= "------- GPIO Community 0 -------",
	.pcr_port_id	= 0xaf,
	.group_count	= ARRAY_SIZE(sunrise_community_ab_groups),
	.groups		= sunrise_community_ab_groups,
};

static const struct gpio_community sunrise_lp_community_ab = {
	.name		= "------- GPIO Community 0 -------",
	.pcr_port_id	= 0xaf,
	.group_count	= ARRAY_SIZE(sunrise_lp_community_ab_groups),
	.groups		= sunrise_lp_community_ab_groups,
};

static const char *const sunrise_group_c_names[] = {
	"GPP_C0",	"SMBCLK",	"n/a",			"n/a",
	"GPP_C1",	"SMBDATA",	"n/a",			"n/a",
	"GPP_C2",	"SMBALERT#",	"n/a",			"n/a",
	"GPP_C3",	"SML0CLK",	"n/a",			"n/a",
	"GPP_C4",	"SML0DATA",	"n/a",			"n/a",
	"GPP_C5",	"SML0ALERT#",	"n/a",			"n/a",
	"GPP_C6",	"SML1CLK",	"n/a",			"n/a",
	"GPP_C7",	"SML1DATA",	"n/a",			"n/a",
	"GPP_C8",	"UART0_RXD",	"n/a",			"n/a",
	"GPP_C9",	"UART0_TXD",	"n/a",			"n/a",
	"GPP_C10",	"UART0_RTS#",	"n/a",			"n/a",
	"GPP_C11",	"UART0_CTS#",	"n/a",			"n/a",
	"GPP_C12",	"UART1_RXD",	"ISH_UART1_RXD",	"n/a",
	"GPP_C13",	"UART1_TXD",	"ISH_UART1_TXD",	"n/a",
	"GPP_C14",	"UART1_RTS#",	"ISH_UART1_RTS#",	"n/a",
	"GPP_C15",	"UART1_CTS#",	"ISH_UART1_CTS#",	"n/a",
	"GPP_C16",	"I2C0_SDA",	"n/a",			"n/a",
	"GPP_C17",	"I2C0_SCL",	"n/a",			"n/a",
	"GPP_C18",	"I2C1_SDA",	"n/a",			"n/a",
	"GPP_C19",	"I2C1_SCL",	"n/a",			"n/a",
	"GPP_C20",	"UART2_RXD",	"n/a",			"n/a",
	"GPP_C21",	"UART2_TXD",	"n/a",			"n/a",
	"GPP_C22",	"UART2_RTS#",	"n/a",			"n/a",
	"GPP_C23",	"UART2_CTS#",	"n/a",			"n/a",
};

static const struct gpio_group sunrise_group_c = {
	.display	= "------- GPIO Group GPP_C -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_c_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_c_names,
};

static const char *const sunrise_group_d_names[] = {
	"GPP_D0",	"n/a",			"n/a",		"n/a",
	"GPP_D1",	"n/a",			"n/a",		"n/a",
	"GPP_D2",	"n/a",			"n/a",		"n/a",
	"GPP_D3",	"n/a",			"n/a",		"n/a",
	"GPP_D4",	"ISH_I2C2_SDA",		"I2C3_SDA",	"n/a",
	"GPP_D5",	"I2S_SFRM",		"n/a",		"n/a",
	"GPP_D6",	"I2S_TXD",		"n/a",		"n/a",
	"GPP_D7",	"I2S_RXD",		"n/a",		"n/a",
	"GPP_D8",	"I2S_SCLK",		"n/a",		"n/a",
	"GPP_D9",	"n/a",			"n/a",		"n/a",
	"GPP_D10",	"n/a",			"n/a",		"n/a",
	"GPP_D11",	"n/a",			"n/a",		"n/a",
	"GPP_D12",	"n/a",			"n/a",		"n/a",
	"GPP_D13",	"ISH_UART0_RXD",	"n/a",		"I2C2_SDA",
	"GPP_D14",	"ISH_UART0_TXD",	"n/a",		"I2C2_SCL",
	"GPP_D15",	"ISH_UART0_RTS#",	"n/a",		"n/a",
	"GPP_D16",	"ISH_UART0_CTS#",	"n/a",		"n/a",
	"GPP_D17",	"DMIC_CLK1",		"n/a",		"n/a",
	"GPP_D18",	"DMIC_DATA1",		"n/a",		"n/a",
	"GPP_D19",	"DMIC_CLK0",		"n/a",		"n/a",
	"GPP_D20",	"DMIC_DATA0",		"n/a",		"n/a",
	"GPP_D21",	"n/a",			"n/a",		"n/a",
	"GPP_D22",	"n/a",			"n/a",		"n/a",
	"GPP_D23",	"ISH_I2C2_SCL",		"I2C3_SCL",	"n/a",
};

static const struct gpio_group sunrise_group_d = {
	.display	= "------- GPIO Group GPP_D -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_d_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_d_names,
};

static const char *const sunrise_lp_group_d_names[] = {
	"GPP_D0",	"SPI1_CS#",		"n/a",		"n/a",
	"GPP_D1",	"SPI1_CLK",		"n/a",		"n/a",
	"GPP_D2",	"SPI1_MISO",		"n/a",		"n/a",
	"GPP_D3",	"SPI1_MOSI",		"n/a",		"n/a",
	"GPP_D4",	"FLASHTRIG",		"n/a",		"n/a",
	"GPP_D5",	"ISH_I2C0_SDA",		"n/a",		"n/a",
	"GPP_D6",	"ISH_I2C0_SCL",		"n/a",		"n/a",
	"GPP_D7",	"ISH_I2C1_SDA",		"n/a",		"n/a",
	"GPP_D8",	"ISH_I2C1_SCL",		"n/a",		"n/a",
	"GPP_D9",	"n/a",			"n/a",		"n/a",
	"GPP_D10",	"n/a",			"n/a",		"n/a",
	"GPP_D11",	"n/a",			"n/a",		"n/a",
	"GPP_D12",	"n/a",			"n/a",		"n/a",
	"GPP_D13",	"ISH_UART0_RXD",	"n/a",		"n/a",
	"GPP_D14",	"ISH_UART0_TXD",	"n/a",		"n/a",
	"GPP_D15",	"ISH_UART0_RTS#",	"n/a",		"n/a",
	"GPP_D16",	"ISH_UART0_CTS#",	"n/a",		"n/a",
	"GPP_D17",	"DMIC_CLK1",		"n/a",		"n/a",
	"GPP_D18",	"DMIC_DATA1",		"n/a",		"n/a",
	"GPP_D19",	"DMIC_CLK0",		"n/a",		"n/a",
	"GPP_D20",	"DMIC_DATA0",		"n/a",		"n/a",
	"GPP_D21",	"SPI1_IO2",		"n/a",		"n/a",
	"GPP_D22",	"SPI1_IO3",		"n/a",		"n/a",
	"GPP_D23",	"I2S_MCLK",		"n/a",		"n/a",
};

static const struct gpio_group sunrise_lp_group_d = {
	.display	= "------- GPIO Group GPP_D -------",
	.pad_count	= ARRAY_SIZE(sunrise_lp_group_d_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_lp_group_d_names,
};

static const char *const sunrise_group_e_names[] = {
	"GPP_E0",	"SATAXPCIE0",	"SATAGP0",	"n/a",
	"GPP_E1",	"SATAXPCIE1",	"SATAGP1",	"n/a",
	"GPP_E2",	"SATAXPCIE2",	"SATAGP2",	"n/a",
	"GPP_E3",	"CPU_GP0",	"n/a",		"n/a",
	"GPP_E4",	"SATA_DEVSLP0",	"n/a",		"n/a",
	"GPP_E5",	"SATA_DEVSLP1",	"n/a",		"n/a",
	"GPP_E6",	"SATA_DEVSLP2",	"n/a",		"n/a",
	"GPP_E7",	"CPU_GP1",	"n/a",		"n/a",
	"GPP_E8",	"SATA_LED#",	"n/a",		"n/a",
	"GPP_E9",	"USB_OC0#",	"n/a",		"n/a",
	"GPP_E10",	"USB_OC1#",	"n/a",		"n/a",
	"GPP_E11",	"USB_OC2#",	"n/a",		"n/a",
	"GPP_E12",	"USB_OC3#",	"n/a",		"n/a",
};

static const struct gpio_group sunrise_group_e = {
	.display	= "------- GPIO Group GPP_E -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_e_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_e_names,
};

static const char *const sunrise_lp_group_e_names[] = {
	"GPP_E0",	"SATAXPCIE0",		"SATAGP0",	"n/a",
	"GPP_E1",	"SATAXPCIE1",		"SATAGP1",	"n/a",
	"GPP_E2",	"SATAXPCIE2",		"SATAGP2",	"n/a",
	"GPP_E3",	"CPU_GP0",		"n/a",		"n/a",
	"GPP_E4",	"SATA_DEVSLP0",		"n/a",		"n/a",
	"GPP_E5",	"SATA_DEVSLP1",		"n/a",		"n/a",
	"GPP_E6",	"SATA_DEVSLP2",		"n/a",		"n/a",
	"GPP_E7",	"CPU_GP1",		"n/a",		"n/a",
	"GPP_E8",	"SATALED#",		"n/a",		"n/a",
	"GPP_E9",	"USB2_OC0#",		"n/a",		"n/a",
	"GPP_E10",	"USB2_OC1#",		"n/a",		"n/a",
	"GPP_E11",	"USB2_OC2#",		"n/a",		"n/a",
	"GPP_E12",	"USB2_OC3#",		"n/a",		"n/a",
	"GPP_E13",	"DDPB_HPD0",		"n/a",		"n/a",
	"GPP_E14",	"DDPC_HPD1",		"n/a",		"n/a",
	"GPP_E15",	"DDPD_HPD2",		"n/a",		"n/a",
	"GPP_E16",	"DDPE_HPD3",		"n/a",		"n/a",
	"GPP_E17",	"EDP_HPD",		"n/a",		"n/a",
	"GPP_E18",	"DDPB_CTRLCLK",		"n/a",		"n/a",
	"GPP_E19",	"DDPB_CTRLDATA",	"n/a",		"n/a",
	"GPP_E20",	"DDPC_CTRLCLK",		"n/a",		"n/a",
	"GPP_E21",	"DDPC_CTRLDATA",	"n/a",		"n/a",
	"GPP_E22",	"n/a",			"n/a",		"n/a",
	"GPP_E23",	"n/a",			"n/a",		"n/a",
};

static const struct gpio_group sunrise_lp_group_e = {
	.display	= "------- GPIO Group GPP_E -------",
	.pad_count	= ARRAY_SIZE(sunrise_lp_group_e_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_lp_group_e_names,
};

static const char *const sunrise_group_f_names[] = {
	"GPP_F0",	"SATAXPCIE3",		"SATAGP3",	"n/a",
	"GPP_F1",	"SATAXPCIE4",		"SATAGP4",	"n/a",
	"GPP_F2",	"SATAXPCIE5",		"SATAGP5",	"n/a",
	"GPP_F3",	"SATAXPCIE6",		"SATAGP6",	"n/a",
	"GPP_F4",	"SATAXPCIE7",		"SATAGP7",	"n/a",
	"GPP_F5",	"SATA_DEVSLP3",		"n/a",		"n/a",
	"GPP_F6",	"SATA_DEVSLP4",		"n/a",		"n/a",
	"GPP_F7",	"SATA_DEVSLP5",		"n/a",		"n/a",
	"GPP_F8",	"SATA_DEVSLP6",		"n/a",		"n/a",
	"GPP_F9",	"SATA_DEVSLP7",		"n/a",		"n/a",
	"GPP_F10",	"SATA_SCLOCK",		"n/a",		"n/a",
	"GPP_F11",	"SATA_SLOAD",		"n/a",		"n/a",
	"GPP_F12",	"SATA_SDATAOUT1",	"n/a",		"n/a",
	"GPP_F13",	"SATA_SDATAOUT2",	"n/a",		"n/a",
	"GPP_F14",	"n/a",			"n/a",		"n/a",
	"GPP_F15",	"USB_OC4#",		"n/a",		"n/a",
	"GPP_F16",	"USB_OC5#",		"n/a",		"n/a",
	"GPP_F17",	"USB_OC6#",		"n/a",		"n/a",
	"GPP_F18",	"USB_OC7#",		"n/a",		"n/a",
	"GPP_F19",	"eDP_VDDEN",		"n/a",		"n/a",
	"GPP_F20",	"eDP_BKLTEN",		"n/a",		"n/a",
	"GPP_F21",	"eDP_BKLTCTL",		"n/a",		"n/a",
	"GPP_F22",	"n/a",			"n/a",		"n/a",
	"GPP_F23",	"n/a",			"n/a",		"n/a",
};

static const struct gpio_group sunrise_group_f = {
	.display	= "------- GPIO Group GPP_F -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_f_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_f_names,
};

static const char *const sunrise_lp_group_f_names[] = {
	"GPP_F0",	"I2S2_SCLK",		"n/a",		"n/a",
	"GPP_F1",	"I2S2_SFRM",		"n/a",		"n/a",
	"GPP_F2",	"I2S2_TXD",		"n/a",		"n/a",
	"GPP_F3",	"I2S2_RXD",		"n/a",		"n/a",
	"GPP_F4",	"I2C2_SDA",		"n/a",		"n/a",
	"GPP_F5",	"I2C2_SCL",		"n/a",		"n/a",
	"GPP_F6",	"I2C3_SDA",		"n/a",		"n/a",
	"GPP_F7",	"I2C3_SCL",		"n/a",		"n/a",
	"GPP_F8",	"I2C4_SDA",		"n/a",		"n/a",
	"GPP_F9",	"I2C4_SCL",		"n/a",		"n/a",
	"GPP_F10",	"I2C5_SDA",		"ISH_I2C2_SDA",	"n/a",
	"GPP_F11",	"I2C5_SCL",		"ISH_I2C2_SCL",	"n/a",
	"GPP_F12",	"EMMC_CMD",		"n/a",		"n/a",
	"GPP_F13",	"EMMC_DATA0",		"n/a",		"n/a",
	"GPP_F14",	"EMMC_DATA1",		"n/a",		"n/a",
	"GPP_F15",	"EMMC_DATA2",		"n/a",		"n/a",
	"GPP_F16",	"EMMC_DATA3",		"n/a",		"n/a",
	"GPP_F17",	"EMMC_DATA4",		"n/a",		"n/a",
	"GPP_F18",	"EMMC_DATA5",		"n/a",		"n/a",
	"GPP_F19",	"EMMC_DATA6",		"n/a",		"n/a",
	"GPP_F20",	"EMMC_DATA7",		"n/a",		"n/a",
	"GPP_F21",	"EMMC_RCLK",		"n/a",		"n/a",
	"GPP_F22",	"EMMC_CLK",		"n/a",		"n/a",
	"GPP_F23",	"n/a",			"n/a",		"n/a",
};

static const struct gpio_group sunrise_lp_group_f = {
	.display	= "------- GPIO Group GPP_F -------",
	.pad_count	= ARRAY_SIZE(sunrise_lp_group_f_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_lp_group_f_names,
};

static const char *const sunrise_group_g_names[] = {
	"GPP_G0",	"FAN_TACH_0",	"n/a",	"n/a",
	"GPP_G1",	"FAN_TACH_1",	"n/a",	"n/a",
	"GPP_G2",	"FAN_TACH_2",	"n/a",	"n/a",
	"GPP_G3",	"FAN_TACH_3",	"n/a",	"n/a",
	"GPP_G4",	"FAN_TACH_4",	"n/a",	"n/a",
	"GPP_G5",	"FAN_TACH_5",	"n/a",	"n/a",
	"GPP_G6",	"FAN_TACH_6",	"n/a",	"n/a",
	"GPP_G7",	"FAN_TACH_7",	"n/a",	"n/a",
	"GPP_G8",	"FAN_PWM_0",	"n/a",	"n/a",
	"GPP_G9",	"FAN_PWM_1",	"n/a",	"n/a",
	"GPP_G10",	"FAN_PWM_2",	"n/a",	"n/a",
	"GPP_G11",	"FAN_PWM_3",	"n/a",	"n/a",
	"GPP_G12",	"GSXDOUT",	"n/a",	"n/a",
	"GPP_G13",	"GSXSLOAD",	"n/a",	"n/a",
	"GPP_G14",	"GSXDIN",	"n/a",	"n/a",
	"GPP_G15",	"GSXRESET#",	"n/a",	"n/a",
	"GPP_G16",	"GSXCLK",	"n/a",	"n/a",
	"GPP_G17",	"ADR_COMPLETE",	"n/a",	"n/a",
	"GPP_G18",	"NMI#",		"n/a",	"n/a",
	"GPP_G19",	"SMI#",		"n/a",	"n/a",
	"GPP_G20",	"n/a",		"n/a",	"n/a",
	"GPP_G21",	"n/a",		"n/a",	"n/a",
	"GPP_G22",	"n/a",		"n/a",	"n/a",
	"GPP_G23",	"n/a",		"n/a",	"n/a",
};

static const struct gpio_group sunrise_group_g = {
	.display	= "------- GPIO Group GPP_G -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_g_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_g_names,
};

static const char *const sunrise_lp_group_g_names[] = {
	"GPP_G0",	"SD_CMD",	"n/a",	"n/a",
	"GPP_G1",	"SD_DATA0",	"n/a",	"n/a",
	"GPP_G2",	"SD_DATA1",	"n/a",	"n/a",
	"GPP_G3",	"SD_DATA2",	"n/a",	"n/a",
	"GPP_G4",	"SD_DATA3",	"n/a",	"n/a",
	"GPP_G5",	"SD_CD#",	"n/a",	"n/a",
	"GPP_G6",	"SD_CLK",	"n/a",	"n/a",
	"GPP_G7",	"SD_WP",	"n/a",	"n/a",
};

static const struct gpio_group sunrise_lp_group_g = {
	.display	= "------- GPIO Group GPP_G -------",
	.pad_count	= ARRAY_SIZE(sunrise_lp_group_g_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_lp_group_g_names,
};

static const char *const sunrise_group_h_names[] = {
	"GPP_H0",	"SRCCLKREQ6#",	"n/a",	"n/a",
	"GPP_H1",	"SRCCLKREQ7#",	"n/a",	"n/a",
	"GPP_H2",	"SRCCLKREQ8#",	"n/a",	"n/a",
	"GPP_H3",	"SRCCLKREQ9#",	"n/a",	"n/a",
	"GPP_H4",	"SRCCLKREQ10#",	"n/a",	"n/a",
	"GPP_H5",	"SRCCLKREQ11#",	"n/a",	"n/a",
	"GPP_H6",	"SRCCLKREQ12#",	"n/a",	"n/a",
	"GPP_H7",	"SRCCLKREQ13#",	"n/a",	"n/a",
	"GPP_H8",	"SRCCLKREQ14#",	"n/a",	"n/a",
	"GPP_H9",	"SRCCLKREQ15#",	"n/a",	"n/a",
	"GPP_H10",	"SML2CLK",	"n/a",	"n/a",
	"GPP_H11",	"SML2DATA",	"n/a",	"n/a",
	"GPP_H12",	"SML2ALERT#",	"n/a",	"n/a",
	"GPP_H13",	"SML3CLK",	"n/a",	"n/a",
	"GPP_H14",	"SML3DATA",	"n/a",	"n/a",
	"GPP_H15",	"SML3ALERT#",	"n/a",	"n/a",
	"GPP_H16",	"SML4CLK",	"n/a",	"n/a",
	"GPP_H17",	"SML4DATA",	"n/a",	"n/a",
	"GPP_H18",	"SML4ALERT#",	"n/a",	"n/a",
	"GPP_H19",	"ISH_I2C0_SDA",	"n/a",	"n/a",
	"GPP_H20",	"ISH_I2C0_SCL",	"n/a",	"n/a",
	"GPP_H21",	"ISH_I2C1_SDA",	"n/a",	"n/a",
	"GPP_H22",	"ISH_I2C1_SCL",	"n/a",	"n/a",
	"GPP_H23",	"n/a",		"n/a",	"n/a",
};

static const struct gpio_group sunrise_group_h = {
	.display	= "------- GPIO Group GPP_H -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_h_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_h_names,
};

static const struct gpio_group *const sunrise_community_cdefgh_groups[] = {
	&sunrise_group_c, &sunrise_group_d, &sunrise_group_e,
	&sunrise_group_f, &sunrise_group_g, &sunrise_group_h,
};

static const struct gpio_group *const sunrise_lp_community_cde_groups[] = {
	&sunrise_group_c, &sunrise_lp_group_d, &sunrise_lp_group_e,
};

static const struct gpio_community sunrise_community_cdefgh = {
	.name		= "------- GPIO Community 1 -------",
	.pcr_port_id	= 0xae,
	.group_count	= ARRAY_SIZE(sunrise_community_cdefgh_groups),
	.groups		= sunrise_community_cdefgh_groups,
};

static const struct gpio_community sunrise_lp_community_cde = {
	.name		= "------- GPIO Community 1 -------",
	.pcr_port_id	= 0xae,
	.group_count	= ARRAY_SIZE(sunrise_lp_community_cde_groups),
	.groups		= sunrise_lp_community_cde_groups,
};

static const char *const sunrise_group_gpd_names[] = {
	"GPD0",		"BATLOW#",	"n/a",	"n/a",
	"GPD1",		"ACPRESENT",	"n/a",	"n/a",
	"GPD2",		"LAN_WAKE#",	"n/a",	"n/a",
	"GPD3",		"PWRBTN#",	"n/a",	"n/a",
	"GPD4",		"SLP_S3#",	"n/a",	"n/a",
	"GPD5",		"SLP_S4#",	"n/a",	"n/a",
	"GPD6",		"SLP_A#",	"n/a",	"n/a",
	"GPD7",		"RESERVED",	"n/a",	"n/a",
	"GPD8",		"SUSCLK",	"n/a",	"n/a",
	"GPD9",		"SLP_WLAN#",	"n/a",	"n/a",
	"GPD10",	"SLP_S5#",	"n/a",	"n/a",
	"GPD11",	"LANPHYPC",	"n/a",	"n/a",
};

static const struct gpio_group sunrise_group_gpd = {
	.display	= "-------- GPIO Group GPD --------",
	.pad_count	= ARRAY_SIZE(sunrise_group_gpd_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_gpd_names,
};

static const struct gpio_group *const sunrise_community_gpd_groups[] = {
	&sunrise_group_gpd,
};

static const struct gpio_community sunrise_community_gpd = {
	.name		= "------- GPIO Community 2 -------",
	.pcr_port_id	= 0xad,
	.group_count	= ARRAY_SIZE(sunrise_community_gpd_groups),
	.groups		= sunrise_community_gpd_groups,
};

static const char *const sunrise_group_i_names[] = {
	"GPP_I0",	"DDPB_HPD0",		"n/a",	"n/a",
	"GPP_I1",	"DDPC_HPD1",		"n/a",	"n/a",
	"GPP_I2",	"DDPD_HPD2",		"n/a",	"n/a",
	"GPP_I3",	"DDPE_HPD3",		"n/a",	"n/a",
	"GPP_I4",	"EDP_HPD",		"n/a",	"n/a",
	"GPP_I5",	"DDPB_CTRLCLK",		"n/a",	"n/a",
	"GPP_I6",	"DDPB_CTRLDATA",	"n/a",	"n/a",
	"GPP_I7",	"DDPC_CTRLCLK",		"n/a",	"n/a",
	"GPP_I8",	"DDPC_CTRLDATA",	"n/a",	"n/a",
	"GPP_I9",	"DDPD_CTRLCLK",		"n/a",	"n/a",
	"GPP_I10",	"DDPD_CTRLDATA",	"n/a",	"n/a",
};

static const struct gpio_group sunrise_group_i = {
	.display	= "------- GPIO Group GPP_I -------",
	.pad_count	= ARRAY_SIZE(sunrise_group_i_names) / 4,
	.func_count	= 4,
	.pad_names	= sunrise_group_i_names,
};

static const struct gpio_group *const sunrise_community_i_groups[] = {
	&sunrise_group_i,
};

static const struct gpio_group *const sunrise_lp_community_fg_groups[] = {
	&sunrise_lp_group_f, &sunrise_lp_group_g,
};

static const struct gpio_community sunrise_community_i = {
	.name		= "------- GPIO Community 3 -------",
	.pcr_port_id	= 0xac,
	.group_count	= ARRAY_SIZE(sunrise_community_i_groups),
	.groups		= sunrise_community_i_groups,
};

static const struct gpio_community sunrise_lp_community_fg = {
	.name		= "------- GPIO Community 3 -------",
	.pcr_port_id	= 0xac,
	.group_count	= ARRAY_SIZE(sunrise_lp_community_fg_groups),
	.groups		= sunrise_lp_community_fg_groups,
};

static const struct gpio_community *const sunrise_communities[] = {
	&sunrise_community_ab, &sunrise_community_cdefgh,
	&sunrise_community_gpd, &sunrise_community_i,
};

static const struct gpio_community *const sunrise_lp_communities[] = {
	&sunrise_lp_community_ab, &sunrise_lp_community_cde,
	&sunrise_community_gpd, &sunrise_lp_community_fg,
};

static const char *const denverton_group_north_all_names[] = {
	"NORTH_ALL_GBE0_SDP0",
	"NORTH_ALL_GBE1_SDP0",
	"NORTH_ALL_GBE0_SDP1",
	"NORTH_ALL_GBE1_SDP1",
	"NORTH_ALL_GBE0_SDP2",
	"NORTH_ALL_GBE1_SDP2",
	"NORTH_ALL_GBE0_SDP3",
	"NORTH_ALL_GBE1_SDP3",
	"NORTH_ALL_GBE2_LED0",
	"NORTH_ALL_GBE2_LED1",
	"NORTH_ALL_GBE0_I2C_CLK",
	"NORTH_ALL_GBE0_I2C_DATA",
	"NORTH_ALL_GBE1_I2C_CLK",
	"NORTH_ALL_GBE1_I2C_DATA",
	"NORTH_ALL_NCSI_RXD0",
	"NORTH_ALL_NCSI_CLK_IN",
	"NORTH_ALL_NCSI_RXD1",
	"NORTH_ALL_NCSI_CRS_DV",
	"NORTH_ALL_NCSI_ARB_IN",
	"NORTH_ALL_NCSI_TX_EN",
	"NORTH_ALL_NCSI_TXD0",
	"NORTH_ALL_NCSI_TXD1",
	"NORTH_ALL_NCSI_ARB_OUT",
	"NORTH_ALL_GBE0_LED0",
	"NORTH_ALL_GBE0_LED1",
	"NORTH_ALL_GBE1_LED0",
	"NORTH_ALL_GBE1_LED1",
	"NORTH_ALL_GPIO_0",
	"NORTH_ALL_PCIE_CLKREQ0_N",
	"NORTH_ALL_PCIE_CLKREQ1_N",
	"NORTH_ALL_PCIE_CLKREQ2_N",
	"NORTH_ALL_PCIE_CLKREQ3_N",
	"NORTH_ALL_PCIE_CLKREQ4_N",
	"NORTH_ALL_GPIO_1",
	"NORTH_ALL_GPIO_2",
	"NORTH_ALL_SVID_ALERT_N",
	"NORTH_ALL_SVID_DATA",
	"NORTH_ALL_SVID_CLK",
	"NORTH_ALL_THERMTRIP_N",
	"NORTH_ALL_PROCHOT_N",
	"NORTH_ALL_MEMHOT_N",
};

static const struct gpio_group denverton_group_north_all = {
	.display	= "------- GPIO Group North All -------",
	.pad_count	= ARRAY_SIZE(denverton_group_north_all_names) / 1,
	.func_count	= 1,
	.pad_names	= denverton_group_north_all_names,
};

static const struct gpio_group *const denverton_community_north_groups[] = {
	&denverton_group_north_all,
};

static const struct gpio_community denverton_community_north = {
	.name		= "------- GPIO Community 0 -------",
	.pcr_port_id	= 0xc2,
	.group_count	= ARRAY_SIZE(denverton_community_north_groups),
	.groups		= denverton_community_north_groups,
};

static const char *const denverton_group_south_dfx_names[] = {
	"SOUTH_DFX_DFX_PORT_CLK0",
	"SOUTH_DFX_DFX_PORT_CLK1",
	"SOUTH_DFX_DFX_PORT0",
	"SOUTH_DFX_DFX_PORT1",
	"SOUTH_DFX_DFX_PORT2",
	"SOUTH_DFX_DFX_PORT3",
	"SOUTH_DFX_DFX_PORT4",
	"SOUTH_DFX_DFX_PORT5",
	"SOUTH_DFX_DFX_PORT6",
	"SOUTH_DFX_DFX_PORT7",
	"SOUTH_DFX_DFX_PORT8",
	"SOUTH_DFX_DFX_PORT9",
	"SOUTH_DFX_DFX_PORT10",
	"SOUTH_DFX_DFX_PORT11",
	"SOUTH_DFX_DFX_PORT12",
	"SOUTH_DFX_DFX_PORT13",
	"SOUTH_DFX_DFX_PORT14",
	"SOUTH_DFX_DFX_PORT15",
};

static const struct gpio_group denverton_group_south_dfx = {
	.display	= "------- GPIO Group South DFX -------",
	.pad_count	= ARRAY_SIZE(denverton_group_south_dfx_names) / 1,
	.func_count	= 1,
	.pad_names	= denverton_group_south_dfx_names,
};

static const char *const denverton_group_south_group0_names[] = {
	"SOUTH_GROUP0_GPIO_12",
	"SOUTH_GROUP0_SMB5_GBE_ALRT_N",
	"SOUTH_GROUP0_PCIE_CLKREQ5_N",
	"SOUTH_GROUP0_PCIE_CLKREQ6_N",
	"SOUTH_GROUP0_PCIE_CLKREQ7_N",
	"SOUTH_GROUP0_UART0_RXD",
	"SOUTH_GROUP0_UART0_TXD",
	"SOUTH_GROUP0_SMB5_GBE_CLK",
	"SOUTH_GROUP0_SMB5_GBE_DATA",
	"SOUTH_GROUP0_ERROR2_N",
	"SOUTH_GROUP0_ERROR1_N",
	"SOUTH_GROUP0_ERROR0_N",
	"SOUTH_GROUP0_IERR_N",
	"SOUTH_GROUP0_MCERR_N",
	"SOUTH_GROUP0_SMB0_LEG_CLK",
	"SOUTH_GROUP0_SMB0_LEG_DATA",
	"SOUTH_GROUP0_SMB0_LEG_ALRT_N",
	"SOUTH_GROUP0_SMB1_HOST_DATA",
	"SOUTH_GROUP0_SMB1_HOST_CLK",
	"SOUTH_GROUP0_SMB2_PECI_DATA",
	"SOUTH_GROUP0_SMB2_PECI_CLK",
	"SOUTH_GROUP0_SMB4_CSME0_DATA",
	"SOUTH_GROUP0_SMB4_CSME0_CLK",
	"SOUTH_GROUP0_SMB4_CSME0_ALRT_N",
	"SOUTH_GROUP0_USB_OC0_N",
	"SOUTH_GROUP0_FLEX_CLK_SE0",
	"SOUTH_GROUP0_FLEX_CLK_SE1",
	"SOUTH_GROUP0_GPIO_4",
	"SOUTH_GROUP0_GPIO_5",
	"SOUTH_GROUP0_GPIO_6",
	"SOUTH_GROUP0_GPIO_7",
	"SOUTH_GROUP0_SATA0_LED_N",
	"SOUTH_GROUP0_SATA1_LED_N",
	"SOUTH_GROUP0_SATA_PDETECT0",
	"SOUTH_GROUP0_SATA_PDETECT1",
	"SOUTH_GROUP0_SATA0_SDOUT",
	"SOUTH_GROUP0_SATA1_SDOUT",
	"SOUTH_GROUP0_UART1_RXD",
	"SOUTH_GROUP0_UART1_TXD",
	"SOUTH_GROUP0_GPIO_8",
	"SOUTH_GROUP0_GPIO_9",
	"SOUTH_GROUP0_TCK",
	"SOUTH_GROUP0_TRST_N",
	"SOUTH_GROUP0_TMS",
	"SOUTH_GROUP0_TDI",
	"SOUTH_GROUP0_TDO",
	"SOUTH_GROUP0_CX_PRDY_N",
	"SOUTH_GROUP0_CX_PREQ_N",
	"SOUTH_GROUP0_CTBTRIGINOUT",
	"SOUTH_GROUP0_CTBTRIGOUT",
	"SOUTH_GROUP0_DFX_SPARE2",
	"SOUTH_GROUP0_DFX_SPARE3",
	"SOUTH_GROUP0_DFX_SPARE4",
};

static const struct gpio_group denverton_group_south_group0 = {
	.display	= "------- GPIO Group South Group0 -------",
	.pad_count	= ARRAY_SIZE(denverton_group_south_group0_names) / 1,
	.func_count	= 1,
	.pad_names	= denverton_group_south_group0_names,
};

static const char *const denverton_group_south_group1_names[] = {
	"SOUTH_GROUP1_SUSPWRDNACK",
	"SOUTH_GROUP1_PMU_SUSCLK",
	"SOUTH_GROUP1_ADR_TRIGGER",
	"SOUTH_GROUP1_PMU_SLP_S45_N",
	"SOUTH_GROUP1_PMU_SLP_S3_N",
	"SOUTH_GROUP1_PMU_WAKE_N",
	"SOUTH_GROUP1_PMU_PWRBTN_N",
	"SOUTH_GROUP1_PMU_RESETBUTTON_N",
	"SOUTH_GROUP1_PMU_PLTRST_N",
	"SOUTH_GROUP1_SUS_STAT_N",
	"SOUTH_GROUP1_SLP_S0IX_N",
	"SOUTH_GROUP1_SPI_CS0_N",
	"SOUTH_GROUP1_SPI_CS1_N",
	"SOUTH_GROUP1_SPI_MOSI_IO0",
	"SOUTH_GROUP1_SPI_MISO_IO1",
	"SOUTH_GROUP1_SPI_IO2",
	"SOUTH_GROUP1_SPI_IO3",
	"SOUTH_GROUP1_SPI_CLK",
	"SOUTH_GROUP1_SPI_CLK_LOOPBK",
	"SOUTH_GROUP1_ESPI_IO0",
	"SOUTH_GROUP1_ESPI_IO1",
	"SOUTH_GROUP1_ESPI_IO2",
	"SOUTH_GROUP1_ESPI_IO3",
	"SOUTH_GROUP1_ESPI_CS0_N",
	"SOUTH_GROUP1_ESPI_CLK",
	"SOUTH_GROUP1_ESPI_RST_N",
	"SOUTH_GROUP1_ESPI_ALRT0_N",
	"SOUTH_GROUP1_GPIO_10",
	"SOUTH_GROUP1_GPIO_11",
	"SOUTH_GROUP1_ESPI_CLK_LOOPBK",
	"SOUTH_GROUP1_EMMC_CMD",
	"SOUTH_GROUP1_EMMC_STROBE",
	"SOUTH_GROUP1_EMMC_CLK",
	"SOUTH_GROUP1_EMMC_D0",
	"SOUTH_GROUP1_EMMC_D1",
	"SOUTH_GROUP1_EMMC_D2",
	"SOUTH_GROUP1_EMMC_D3",
	"SOUTH_GROUP1_EMMC_D4",
	"SOUTH_GROUP1_EMMC_D5",
	"SOUTH_GROUP1_EMMC_D6",
	"SOUTH_GROUP1_EMMC_D7",
	"SOUTH_GROUP1_GPIO_3",
};

static const struct gpio_group denverton_group_south_group1 = {
	.display	= "------- GPIO Group South Group1 -------",
	.pad_count	= ARRAY_SIZE(denverton_group_south_group1_names) / 1,
	.func_count	= 1,
	.pad_names	= denverton_group_south_group1_names,
};

static const struct gpio_group *const denverton_community_south_groups[] = {
	&denverton_group_south_dfx,
	&denverton_group_south_group0,
	&denverton_group_south_group1,
};

static const struct gpio_community denverton_community_south = {
	.name		= "------- GPIO Community 1 -------",
	.pcr_port_id	= 0xc5,
	.group_count	= ARRAY_SIZE(denverton_community_south_groups),
	.groups		= denverton_community_south_groups,
};

static const struct gpio_community *const denverton_communities[] = {
	&denverton_community_north, &denverton_community_south,
};

static const char *decode_pad_mode(const struct gpio_group *const group,
				   const size_t pad, const uint32_t dw0)
{
	const size_t pad_mode = dw0 >> 10 & 7;
	const char *const pad_name =
		group->pad_names[pad * group->func_count + pad_mode];

	if (!pad_mode)
		return pad_name[0] == '*' ? "*GPIO" : "GPIO";
	else if (pad_mode < group->func_count)
		return group->pad_names[pad * group->func_count + pad_mode];
	else
		return "RESERVED";
}

static void print_gpio_group(const uint8_t pid, size_t pad_cfg,
			     const struct gpio_group *const group)
{
	size_t p;

	printf("%s\n", group->display);

	for (p = 0; p < group->pad_count; ++p, pad_cfg += 8) {
		const uint32_t dw0 = read_pcr32(pid, pad_cfg);
		const uint32_t dw1 = read_pcr32(pid, pad_cfg + 4);
		const char *const pad_name =
			group->pad_names[p * group->func_count];

		printf("0x%04zx: 0x%016"PRIx64" %-12s %-20s\n", pad_cfg,
		       (uint64_t)dw1 << 32 | dw0,
		       pad_name[0] == '*' ? &pad_name[1] : pad_name,
		       decode_pad_mode(group, p, dw0));
	}
}

static void print_gpio_community(const struct gpio_community *const community)
{
	size_t group, pad_count;
	size_t pad_cfg; /* offset in bytes under this communities PCR port */

	printf("%s\n\nPCR Port ID: 0x%06zx\n\n",
	       community->name, (size_t)community->pcr_port_id << 16);

	for (group = 0, pad_count = 0; group < community->group_count; ++group)
		pad_count += community->groups[group]->pad_count;
	assert(pad_count * 8 <= PCR_PORT_SIZE - 0x10);

	pad_cfg = read_pcr32(community->pcr_port_id, 0x0c);
	if (pad_cfg + pad_count * 8 > PCR_PORT_SIZE) {
		fprintf(stderr, "Bad Pad Base Address: 0x%08zx\n", pad_cfg);
		return;
	}

	for (group = 0; group < community->group_count; ++group) {
		print_gpio_group(community->pcr_port_id,
				 pad_cfg, community->groups[group]);
		pad_cfg += community->groups[group]->pad_count * 8;
	}
}

void print_gpio_groups(struct pci_dev *const sb)
{
	size_t community_count;
	const struct gpio_community *const *communities;

	switch (sb->device_id) {
	case PCI_DEVICE_ID_INTEL_B150:
	case PCI_DEVICE_ID_INTEL_CM236:
		community_count = ARRAY_SIZE(sunrise_communities);
		communities = sunrise_communities;
		pcr_init(sb);
		break;
	case PCI_DEVICE_ID_INTEL_SUNRISEPOINT_LP_U_BASE:
	case PCI_DEVICE_ID_INTEL_SUNRISEPOINT_LP_U_PREM:
	case PCI_DEVICE_ID_INTEL_SUNRISEPOINT_LP_Y_PREM:
	case PCI_DEVICE_ID_INTEL_SUNRISEPOINT_LP_U_IHDCP_BASE:
	case PCI_DEVICE_ID_INTEL_SUNRISEPOINT_LP_U_IHDCP_PREM:
	case PCI_DEVICE_ID_INTEL_SUNRISEPOINT_LP_Y_IHDCP_PREM:
		community_count = ARRAY_SIZE(sunrise_lp_communities);
		communities = sunrise_lp_communities;
		pcr_init(sb);
		break;
	case PCI_DEVICE_ID_INTEL_DNV_LPC:
		community_count = ARRAY_SIZE(denverton_communities);
		communities = denverton_communities;
		pcr_init(sb);
		break;
	case PCI_DEVICE_ID_INTEL_APL_LPC:
		community_count = ARRAY_SIZE(apl_communities);
		communities = apl_communities;
		pcr_init(sb);
		break;
	default:
		return;
	}

	printf("\n============= GPIOS =============\n\n");

	for (; community_count; --community_count)
		print_gpio_community(*communities++);
}
