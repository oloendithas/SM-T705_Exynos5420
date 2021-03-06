/* linux/arch/arm/mach-xxxx/board-xmm6360-modems.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* Modem configuraiton for Exynos5260 + XMM7160 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/ehci_def.h>
#include <linux/pm_qos.h>

#include <linux/platform_data/modem_v2.h>
#include <linux/io.h>
#include <mach/map.h>
#include <mach/regs-pmu.h>
#include <mach/regs-usb-phy.h>

#define EHCI_REG_DUMP

extern unsigned int system_rev;

/* umts target platform data */
static struct modem_io_t umts_io_devices[] = {
	{
		.name = "umts_ipc0",
		.id = SIPC5_CH_ID_FMT_0,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_RX_FRAGMENT),
	},
#ifdef CONFIG_IPC_DS_DN
	{
		.name = "umts_ipc1",
		.id = 0xEC,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_RX_FRAGMENT),
	},
#endif
	{
		.name = "umts_rfs0",
		.id = SIPC5_CH_ID_RFS_0,
		.format = IPC_RFS,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_RX_FRAGMENT)
			| IODEV_ATTR(ATTR_LEGACY_RFS),
	},
	{
		.name = "umts_boot0",
		.id = 0x0,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
	},
	{
		.name = "multipdp",
		.id = 0x0,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_RX_FRAGMENT),
	},
	{
		.name = "umts_router",
		.id = SIPC_CH_ID_BT_DUN,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_RX_FRAGMENT),
	},
	{
		.name = "umts_csd",
		.id = SIPC_CH_ID_CS_VT_DATA,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_RX_FRAGMENT),
	},
	{
		.name = "umts_dm0",
		.id = SIPC_CH_ID_CPLOG1,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
	},
	{ /* To use IPC_Logger */
		.name = "umts_log",
		.id = SIPC_CH_ID_PDP_18,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
	},
	{
		.name = "ipc_loopback0",
		.id = SIPC5_CH_ID_FMT_9,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_RX_FRAGMENT),
	},
#ifndef CONFIG_USB_NET_CDC_NCM
	{
		.name = "rmnet0",
		.id = SIPC_CH_ID_PDP_0,
		.format = IPC_RAW_NCM,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_CDC_NCM),
	},
	{
		.name = "rmnet1",
		.id = SIPC_CH_ID_PDP_1,
		.format = IPC_RAW_NCM,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_CDC_NCM),
	},
	{
		.name = "rmnet2",
		.id = SIPC_CH_ID_PDP_2,
		.format = IPC_RAW_NCM,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_CDC_NCM),
	},
	{
		.name = "rmnet3",
		.id = SIPC_CH_ID_PDP_3,
		.format = IPC_RAW_NCM,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_HSIC),
		.attr = IODEV_ATTR(ATTR_CDC_NCM),
	},
#endif
};

static struct pm_qos_request mif_qos_req;
static struct pm_qos_request int_qos_req;
#define REQ_PM_QOS(req, class_id, arg) \
	do { \
		if (pm_qos_request_active(req)) \
			pm_qos_update_request(req, arg); \
		else \
			pm_qos_add_request(req, class_id, arg); \
	} while (0) \

#define MAX_FREQ_LEVEL 2
static struct {
	unsigned throughput;
	unsigned mif_freq_lock;
	unsigned int_freq_lock;
} freq_table[MAX_FREQ_LEVEL] = {
	{ 100, 266000, 133000 }, /* default */
	{ 150, 667000, 500000 }, /* 100Mbps */
};

static int umts_link_ldo_enble(bool enable)
{
	/* Exynos HSIC V1.2 LDO was controlled by kernel */
	return 0;
}

struct mif_ehci_regs {
	unsigned caps_hc_capbase;
	unsigned caps_hcs_params;
	unsigned caps_hcc_params;
	unsigned reserved0;
	struct ehci_regs regs;
	unsigned port_usb;  /*0x54*/
	unsigned port_hsic0;
	unsigned port_hsic1;
	unsigned reserved[12];
	unsigned insnreg00;	/*0x90*/
	unsigned insnreg01;
	unsigned insnreg02;
	unsigned insnreg03;
	unsigned insnreg04;
	unsigned insnreg05;
	unsigned insnreg06;
	unsigned insnreg07;
};
static struct mif_ehci_regs __iomem *ehci_reg;

#ifdef EHCI_REG_DUMP
#define pr_reg(s, r) printk(KERN_DEBUG "reg(%s):\t 0x%08x\n", s, r)
static void print_ehci_regs(struct mif_ehci_regs *base)
{
	pr_info("------- EHCI reg dump -------\n");
	pr_reg("HCCPBASE", base->caps_hc_capbase);
	pr_reg("HCSPARAMS", base->caps_hcs_params);
	pr_reg("HCCPARAMS", base->caps_hcc_params);
	pr_reg("USBCMD", base->regs.command);
	pr_reg("USBSTS", base->regs.status);
	pr_reg("USBINTR", base->regs.intr_enable);
	pr_reg("FRINDEX", base->regs.frame_index);
	pr_reg("CTRLDSSEGMENT", base->regs.segment);
	pr_reg("PERIODICLISTBASE", base->regs.frame_list);
	pr_reg("ASYNCLISTADDR", base->regs.async_next);
	pr_reg("CONFIGFLAG", base->regs.configured_flag);
	pr_reg("PORT0 Status/Control", base->port_usb);
	pr_reg("PORT1 Status/Control", base->port_hsic0);
	pr_reg("PORT2 Status/Control", base->port_hsic1);
	pr_reg("INSNREG00", base->insnreg00);
	pr_reg("INSNREG01", base->insnreg01);
	pr_reg("INSNREG02", base->insnreg02);
	pr_reg("INSNREG03", base->insnreg03);
	pr_reg("INSNREG04", base->insnreg04);
	pr_reg("INSNREG05", base->insnreg05);
	pr_reg("INSNREG06", base->insnreg06);
	pr_reg("INSNREG07", base->insnreg07);
	pr_info("-----------------------------\n");
}

static void print_phy_regs(void)
{
	pr_info("----- EHCI PHY REG DUMP -----\n");
	pr_reg("USBHOST_PHY_CONTROL", readl(EXYNOS5_USBHOST_PHY_CONTROL));
	pr_reg("HOSTPHYCTRL0", readl(EXYNOS5_PHY_HOST_CTRL0));
	pr_reg("HOSTPHYTUNE0", readl(EXYNOS5_PHY_HOST_TUNE0));
	pr_reg("HSICPHYCTRL1", readl(EXYNOS5_PHY_HSIC_CTRL1));
	pr_reg("HSICPHYTUNE1", readl(EXYNOS5_PHY_HSIC_TUNE1));
/*	pr_reg("HSICPHYCTRL2", readl(EXYNOS5_PHY_HSIC_CTRL2));*/
/*	pr_reg("HSICPHYTUNE2", readl(EXYNOS5_PHY_HSIC_TUNE2));*/
	pr_reg("HOSTEHCICTRL", readl(EXYNOS5_PHY_HOST_EHCICTRL));
	pr_reg("OHCICTRL", readl(EXYNOS5_PHY_HOST_OHCICTRL));
	pr_reg("USBOTG_SYS", readl(EXYNOS5_PHY_OTG_SYS));
	pr_reg("USBOTG_TUNE", readl(EXYNOS5_PHY_OTG_TUNE));
	pr_info("-----------------------------\n");
}

static void debug_ehci_reg_dump(void)
{
	print_phy_regs();
	print_ehci_regs(ehci_reg);
}
#else
#define debug_ehci_reg_dump() do {} while(0);
#endif

#define WAIT_CONNECT_CHECK_CNT 30
static int s5p_ehci_port_reg_init(void)
{
	if (ehci_reg) {
		mif_info("port reg aleady initialized\n");
		return -EBUSY;
	}

	ehci_reg = ioremap((S5P_PA_EHCI), SZ_256);
	if (!ehci_reg) {
		mif_err("fail to get port reg address\n");
		return -EINVAL;
	}
	mif_info("port reg get success (%p)\n", ehci_reg);

	return 0;
}

static void s5p_ehci_wait_cp_resume(int port)
{
	u32 __iomem *portsc;
	int cnt = WAIT_CONNECT_CHECK_CNT;
	u32 val;

	if (!ehci_reg) {
		mif_err("port reg addr invalid\n");
		return;
	}
	portsc = &ehci_reg->port_usb + (port - 1);

	do {
		msleep(20);
		val = readl(portsc);
		mif_debug("port(%d), reg(0x%x)\n", port, val);
	} while (cnt-- && !(val & PORT_CONNECT));
#ifdef EHCI_REG_DUMP
	if (!(val & PORT_CONNECT))
		debug_ehci_reg_dump();
#endif
}

static int xmm_cp_force_crash_exit(void);
static void exynos_frequency_unlock(void);
static void exynos_frequency_lock(unsigned long qosval);
static struct modemlink_pm_data modem_link_pm_data = {
	.name = "link_pm",
	.link_ldo_enable = umts_link_ldo_enble,
	.gpio_link_enable = 0,
	.gpio_link_active = GPIO_ACTIVE_STATE,
	.gpio_link_hostwake = GPIO_IPC_HOST_WAKEUP,
	.gpio_link_slavewake = GPIO_IPC_SLAVE_WAKEUP,
	.cp_force_crash_exit = xmm_cp_force_crash_exit,
	.ehci_reg_dump = debug_ehci_reg_dump,
	.port = 2,
	.wait_cp_resume = s5p_ehci_wait_cp_resume,
	.freqlock = ATOMIC_INIT(0),
	.freq_lock = exynos_frequency_lock,
	.freq_unlock = exynos_frequency_unlock,
};

static struct platform_device modem_linkpm_xmm626x = {
	.name = "linkpm-xmm626x",
	.id = -1,
	.dev = {
		.platform_data = &modem_link_pm_data,
	},
};

static struct modemlink_pm_link_activectl active_ctl;
static void xmm_gpio_revers_bias_clear(void);
static void xmm_gpio_revers_bias_restore(void);
static int xmm_link_reconnect(void);

#define MAX_CDC_ACM_CH 3
#define MAX_CDC_NCM_CH 4
static struct modem_data umts_modem_data = {
	.name = "xmm6262",

	.gpio_cp_on = GPIO_PHONE_ON,
	.gpio_reset_req_n = GPIO_RESET_REQ_N,
	.gpio_cp_reset = GPIO_CP_PMU_RST,
	.gpio_pda_active = GPIO_PDA_ACTIVE,
	.gpio_phone_active = GPIO_PHONE_ACTIVE,
	.gpio_cp_dump_int = GPIO_CP_DUMP_INT,
#ifdef GPIO_AP_DUMP_INT
	.gpio_ap_dump_int = GPIO_AP_DUMP_INT,
#endif
#ifdef GPIO_SIM_DETECT
	.gpio_sim_detect = GPIO_SIM_DETECT,
#endif

	.modem_type = IMC_XMM6262,
	.link_types = LINKTYPE(LINKDEV_HSIC),
	.modem_net = UMTS_NETWORK,
	.use_handover = false,

	.num_iodevs = ARRAY_SIZE(umts_io_devices),
	.iodevs = umts_io_devices,

	.gpio_revers_bias_clear = xmm_gpio_revers_bias_clear,
	.gpio_revers_bias_restore = xmm_gpio_revers_bias_restore,
	.link_reconnect = xmm_link_reconnect,
	.max_link_channel = MAX_CDC_ACM_CH + MAX_CDC_NCM_CH,
	.max_acm_channel = MAX_CDC_ACM_CH,
	.ipc_version = SIPC_VER_50,
};

/* if use more than one modem device, then set id num */
static struct platform_device umts_modem = {
	.name = "mif_sipc5",
	.id = -1,
	.dev = {
		.platform_data = &umts_modem_data,
	},
};

static void exynos_frequency_unlock(void)
{
	if (atomic_read(&modem_link_pm_data.freqlock) != 0) {
		mif_info("unlocking level = %d\n",
			atomic_read(&modem_link_pm_data.freqlock));

		REQ_PM_QOS(&int_qos_req, PM_QOS_DEVICE_THROUGHPUT, 0);
		REQ_PM_QOS(&mif_qos_req, PM_QOS_BUS_THROUGHPUT, 0);
		atomic_set(&modem_link_pm_data.freqlock, 0);
	} else {
		mif_debug("already unlocked, curr_level = %d\n",
			atomic_read(&modem_link_pm_data.freqlock));
	}
}

static void exynos_frequency_lock(unsigned long qosval)
{
	int level;
	unsigned mif_freq, int_freq;

	for (level = 0; level < MAX_FREQ_LEVEL; level++)
		if (qosval < freq_table[level].throughput)
			break;

	level = min(level, MAX_FREQ_LEVEL - 1);
	if (!level && atomic_read(&modem_link_pm_data.freqlock)) {
		mif_debug("locked level = %d, requested level = %d\n",
			atomic_read(&modem_link_pm_data.freqlock), level);
		exynos_frequency_unlock();
		atomic_set(&modem_link_pm_data.freqlock, level);
		return;
	}

	mif_freq = freq_table[level].mif_freq_lock;
	int_freq = freq_table[level].int_freq_lock;

	if (atomic_read(&modem_link_pm_data.freqlock) != level) {
		mif_debug("locked level = %d, requested level = %d\n",
			atomic_read(&modem_link_pm_data.freqlock), level);

		exynos_frequency_unlock();
		mdelay(50);

		REQ_PM_QOS(&mif_qos_req, PM_QOS_BUS_THROUGHPUT, mif_freq);
		REQ_PM_QOS(&int_qos_req, PM_QOS_DEVICE_THROUGHPUT, int_freq);
		atomic_set(&modem_link_pm_data.freqlock, level);

		mif_info("TP=%ld, MIF=%d, INT=%d\n",
				qosval, mif_freq, int_freq);
	} else {
		mif_debug("already locked, curr_level = %d[%d]\n",
			atomic_read(&modem_link_pm_data.freqlock), level);
	}
}

static void umts_modem_cfg_gpio(void)
{
	int ret = 0;

	unsigned gpio_reset_req_n = umts_modem_data.gpio_reset_req_n;
	unsigned gpio_cp_on = umts_modem_data.gpio_cp_on;
	unsigned gpio_cp_rst = umts_modem_data.gpio_cp_reset;
	unsigned gpio_pda_active = umts_modem_data.gpio_pda_active;
	unsigned gpio_phone_active = umts_modem_data.gpio_phone_active;
	unsigned gpio_cp_dump_int = umts_modem_data.gpio_cp_dump_int;
	unsigned gpio_ap_dump_int = umts_modem_data.gpio_ap_dump_int;
	unsigned gpio_flm_uart_sel = umts_modem_data.gpio_flm_uart_sel;
	unsigned gpio_sim_detect = umts_modem_data.gpio_sim_detect;

	if (gpio_reset_req_n) {
		ret = gpio_request(gpio_reset_req_n, "RESET_REQ_N");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "RESET_REQ_N",
				ret);
		gpio_direction_output(gpio_reset_req_n, 0);
		s3c_gpio_setpull(gpio_reset_req_n, S3C_GPIO_PULL_NONE);
	}

	if (gpio_cp_on) {
		ret = gpio_request(gpio_cp_on, "CP_ON");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "CP_ON", ret);
		gpio_direction_output(gpio_cp_on, 0);
	}

	if (gpio_cp_rst) {
		ret = gpio_request(gpio_cp_rst, "CP_RST");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "CP_RST", ret);
		gpio_direction_output(gpio_cp_rst, 0);
		s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_NONE);
	}

	if (gpio_pda_active) {
		ret = gpio_request(gpio_pda_active, "PDA_ACTIVE");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "PDA_ACTIVE",
				ret);
		gpio_direction_output(gpio_pda_active, 0);
	}

	if (gpio_phone_active) {
		ret = gpio_request(gpio_phone_active, "PHONE_ACTIVE");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "PHONE_ACTIVE",
				ret);
		gpio_direction_input(gpio_phone_active);
	}

	if (gpio_sim_detect) {
		ret = gpio_request(gpio_sim_detect, "SIM_DETECT");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "SIM_DETECT",
				ret);

		/* gpio_direction_input(gpio_sim_detect); */
		irq_set_irq_type(gpio_to_irq(gpio_sim_detect),
							IRQ_TYPE_EDGE_BOTH);
	}

	if (gpio_cp_dump_int) {
		ret = gpio_request(gpio_cp_dump_int, "CP_DUMP_INT");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "CP_DUMP_INT",
				ret);
		gpio_direction_input(gpio_cp_dump_int);
	}

	if (gpio_ap_dump_int) {
		ret = gpio_request(gpio_ap_dump_int, "AP_DUMP_INT");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "AP_DUMP_INT",
				ret);
		gpio_direction_output(gpio_ap_dump_int, 0);
	}

	if (gpio_flm_uart_sel) {
		ret = gpio_request(gpio_flm_uart_sel, "GPS_UART_SEL");
		if (ret)
			mif_err("fail to request gpio %s:%d\n", "FLM_SEL",
				ret);
		gpio_direction_output(gpio_reset_req_n, 0);
	}

	if (gpio_phone_active)
		irq_set_irq_type(gpio_to_irq(gpio_phone_active),
							IRQ_TYPE_LEVEL_HIGH);
	mif_info("umts_modem_cfg_gpio done\n");
}

/* HSIC specific function */
void set_slave_wake(void)
{
	if (gpio_get_value(modem_link_pm_data.gpio_link_hostwake)) {
		mif_info("SWK H\n");
		if (gpio_get_value(modem_link_pm_data.gpio_link_slavewake)) {
			mif_info("SWK toggle\n");
			gpio_direction_output(
			modem_link_pm_data.gpio_link_slavewake, 0);
			mdelay(10);
		}
		gpio_direction_output(
			modem_link_pm_data.gpio_link_slavewake, 1);
	}
}

static int xmm_link_reconnect(void)
{
	if (gpio_get_value(umts_modem_data.gpio_phone_active) &&
		gpio_get_value(umts_modem_data.gpio_cp_reset)) {
		mif_info("trying reconnect link\n");
		gpio_set_value(modem_link_pm_data.gpio_link_active, 0);
		mdelay(10);
		set_slave_wake();
		gpio_set_value(modem_link_pm_data.gpio_link_active, 1);
	} else
		return -ENODEV;

	return 0;
}

static void xmm_gpio_revers_bias_clear(void)
{
	gpio_direction_output(umts_modem_data.gpio_pda_active, 0);
	gpio_direction_output(umts_modem_data.gpio_phone_active, 0);
	gpio_direction_output(umts_modem_data.gpio_cp_dump_int, 0);
	gpio_direction_output(modem_link_pm_data.gpio_link_active, 0);
	gpio_direction_output(modem_link_pm_data.gpio_link_hostwake, 0);
	gpio_direction_output(modem_link_pm_data.gpio_link_slavewake, 0);

	if (umts_modem_data.gpio_sim_detect)
		gpio_direction_output(umts_modem_data.gpio_sim_detect, 0);

	msleep(20);
}

static void xmm_gpio_revers_bias_restore(void)
{
	unsigned gpio_sim_detect = umts_modem_data.gpio_sim_detect;

	s3c_gpio_cfgpin(umts_modem_data.gpio_phone_active, S3C_GPIO_SFN(0xF));
	s3c_gpio_cfgpin(modem_link_pm_data.gpio_link_hostwake,
		S3C_GPIO_SFN(0xF));
	gpio_direction_input(umts_modem_data.gpio_cp_dump_int);

	if (gpio_sim_detect) {
		gpio_direction_input(gpio_sim_detect);
		s3c_gpio_cfgpin(gpio_sim_detect, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(gpio_sim_detect, S3C_GPIO_PULL_NONE);
		irq_set_irq_type(gpio_to_irq(gpio_sim_detect),
				IRQ_TYPE_EDGE_BOTH);
		enable_irq_wake(gpio_to_irq(gpio_sim_detect));
	}
}

static int xmm_cp_force_crash_exit(void)
{
	unsigned gpio_ap_dump_int = umts_modem_data.gpio_ap_dump_int;

	if (!gpio_ap_dump_int)
		return -ENXIO;

	if (gpio_get_value(gpio_ap_dump_int)) {
		gpio_set_value(gpio_ap_dump_int, 0);
		msleep(20);
	}
	gpio_set_value(gpio_ap_dump_int, 1);
	msleep(20);
	mif_info("set ap_dump_int(%d) to high=%d\n",
		gpio_ap_dump_int, gpio_get_value(gpio_ap_dump_int));
	gpio_set_value(gpio_ap_dump_int, 0);

	return 0;
}

static void modem_link_pm_config_gpio(void)
{
	int ret = 0;

	unsigned gpio_link_enable = modem_link_pm_data.gpio_link_enable;
	unsigned gpio_link_active = modem_link_pm_data.gpio_link_active;
	unsigned gpio_link_hostwake = modem_link_pm_data.gpio_link_hostwake;
	unsigned gpio_link_slavewake = modem_link_pm_data.gpio_link_slavewake;

	if (gpio_link_enable) {
		ret = gpio_request(gpio_link_enable, "LINK_EN");
		if (ret) {
			mif_err("fail to request gpio %s:%d\n", "LINK_EN",
				ret);
		}
		gpio_direction_output(gpio_link_enable, 0);
	}

	if (gpio_link_active) {
		ret = gpio_request(gpio_link_active, "LINK_ACTIVE");
		if (ret) {
			mif_err("fail to request gpio %s:%d\n", "LINK_ACTIVE",
				ret);
		}
		gpio_direction_output(gpio_link_active, 0);
	}

	if (gpio_link_hostwake) {
		ret = gpio_request(gpio_link_hostwake, "HOSTWAKE");
		if (ret) {
			mif_err("fail to request gpio %s:%d\n", "HOSTWAKE",
				ret);
		}
		gpio_direction_input(gpio_link_hostwake);
	}

	if (gpio_link_slavewake) {
		ret = gpio_request(gpio_link_slavewake, "SLAVEWAKE");
		if (ret) {
			mif_err("fail to request gpio %s:%d\n", "SLAVEWAKE",
				ret);
		}
		gpio_direction_output(gpio_link_slavewake, 0);
	}

	if (gpio_link_hostwake)
		irq_set_irq_type(gpio_to_irq(gpio_link_hostwake),
							IRQ_TYPE_EDGE_BOTH);

	/* set low unused gpios between AP and CP */
	ret = gpio_request(GPIO_SUSPEND_REQUEST, "SUS_REQ");
	if (ret) {
		mif_err("fail to request gpio %s : %d\n", "SUS_REQ", ret);
	} else {
		gpio_direction_output(GPIO_SUSPEND_REQUEST, 0);
		s3c_gpio_setpull(GPIO_SUSPEND_REQUEST, S3C_GPIO_PULL_NONE);
	}

	active_ctl.gpio_initialized = 1;
	mif_info("modem_link_pm_config_gpio done\n");
}

static void board_set_simdetect_polarity(void)
{
	if (umts_modem_data.gpio_sim_detect) {
#if defined(CONFIG_MACH_GC1)
		if (system_rev >= 6) /* GD1 3G real B'd*/
			umts_modem_data.sim_polarity = 1;
		else
			umts_modem_data.sim_polarity = 0;
#else
		umts_modem_data.sim_polarity = 0;
#endif
	}
}
static int __init init_modem(void)
{
	int ret;

	mif_info("init_modem\n");

	/* umts gpios configuration */
	umts_modem_cfg_gpio();
	modem_link_pm_config_gpio();
	board_set_simdetect_polarity();
	s5p_ehci_port_reg_init();

	ret = platform_device_register(&modem_linkpm_xmm626x);
	if (ret < 0)
		mif_err("(%s) register fail with (%d)\n",
				modem_linkpm_xmm626x.name, ret);

	ret = platform_device_register(&umts_modem);
	if (ret < 0)
		mif_err("(%s) register fail with (%d)\n",
				umts_modem.name, ret);

	return ret;
}
late_initcall(init_modem);
