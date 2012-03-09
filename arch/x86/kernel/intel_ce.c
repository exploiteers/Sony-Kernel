/*
 * Intel CE 3100/4100 SoC platform specific setup code
 *
 * (C) Copyright 2010 Google, Inc
 * Author: Eugene Surovegin <es@google.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>

#include <asm/acpi.h>
#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/setup.h>
#include <asm/x86_init.h>

#define ONE_MB			(1024 * 1024)

#define CE_UART_COMMON(__base)				\
	.iobase = __base,				\
	.uartclk = 14745600,				\
	.iotype = UPIO_PORT,				\
	.flags = UPF_SKIP_TEST | UPF_BOOT_AUTOCONF |	\
		 UPF_FIXED_TYPE | UPF_FIXED_PORT,	\
	.type = PORT_XSCALE

#define CE_UART0_COMMON		CE_UART_COMMON(0x3f8)
#define CE_UART1_COMMON		CE_UART_COMMON(0x2f8)

/* Where CEFDK puts ACPI tables */
#define CEFDK_ACPI_BASE		0x10000
#define CEFDK_ACPI_SIZE		0x8000

static inline int __uart_irq(void)
{
#ifdef CONFIG_ACPI
	if (!acpi_noirq)
		return 38;
#endif
	return 4;
}

#ifdef CONFIG_SERIAL_8250_CONSOLE
static struct __initdata uart_port early_uarts[] = {
	{
		.line = 0,
		CE_UART0_COMMON
	},
	{
		.line = 1,
		CE_UART1_COMMON
	}
};

static int __init early_serial_dev_init(void)
{
	int i, res;
	for (i = res = 0; i < ARRAY_SIZE(early_uarts) && !res; ++i) {
		struct uart_port *port = early_uarts + i;
		port->irq = __uart_irq();
		res = early_serial_setup(port);
	}
	return res;
}
console_initcall(early_serial_dev_init);
#endif  /* CONFIG_SERIAL_8250_CONSOLE */

static struct platform_device serial_device = {
	.name	= "serial8250",
	.id	= PLAT8250_DEV_PLATFORM,
	.dev	= {
		.platform_data = (struct plat_serial8250_port[]) {
			{ CE_UART0_COMMON },
			{ CE_UART1_COMMON },
			{}	/* sentinel */
		}
	},
};

static int __init serial_dev_init(void)
{
	struct plat_serial8250_port *port =
		(struct plat_serial8250_port *)serial_device.dev.platform_data;

	while (port->iobase) {
		port->irq = __uart_irq();
		++port;
	}
	return platform_device_register(&serial_device);
}
device_initcall(serial_dev_init);

/* By default Linux gets all RAM */
unsigned long __init __attribute__((weak))
x86_intel_ce_machine_top_of_ram(unsigned long top)
{
	return top;
}

static char * __init x86_intel_ce_memory_setup(void)
{
	unsigned long memory_top, linux_top;

	/* Our only memory config option is e801,
	 * firmware doesn't seem to bother with e820 map.
	 */
	memory_top = ONE_MB + boot_params.alt_mem_k * 1024;

	/* Ancient bootloaders pass garbage here, do a basic sanity check */
	if (memory_top < 4 * ONE_MB) {
		printk(KERN_WARNING
			"Incorrect e801 memory size 0x%08lx, using 768 MB\n",
			memory_top);
		memory_top = 768 * ONE_MB;
	}

	e820_add_region(0, LOWMEMSIZE(), E820_RAM);
	e820_add_region(HIGH_MEMORY, memory_top - HIGH_MEMORY, E820_RAM);

	/* CEFDK uses some regions in the first 1MB for various purposes.
	 * We hardcode superset of those regions to support different
	 * CEFDK releases.
	 */
#ifdef CONFIG_ACPI
	e820_update_range(CEFDK_ACPI_BASE, CEFDK_ACPI_SIZE, E820_RAM, E820_ACPI);
#endif
	/* 8051 microcode can be loaded at 0x30000 or 0x40000 */
	e820_update_range(0x30000, 0x20000, E820_RAM, E820_RESERVED);

	/* e1000 "NVRAM" */
	e820_update_range(0x60000, 0x1000, E820_RAM, E820_RESERVED);

	linux_top = x86_intel_ce_machine_top_of_ram(memory_top);
	if (linux_top < memory_top)
		e820_update_range(linux_top, memory_top - linux_top, E820_RAM,
				  E820_RESERVED);

	return "BIOS-e801";
}

void __init __attribute__((weak))
x86_intel_ce_fill_acpi_tables(unsigned long base, unsigned long size) {}

static void __init x86_intel_ce_probe_roms(void)
{
	x86_intel_ce_fill_acpi_tables(CEFDK_ACPI_BASE, CEFDK_ACPI_SIZE);
}

static void do_cf9(int cmd)
{
	for (;;) {
		outb(cmd, 0xcf9);
		udelay(50);
	}
}

static void intel_ce_poweroff(void)
{
	do_cf9(0x4);
}

static void intel_ce_emergency_restart(void)
{
	do_cf9(0x2);
}

extern int pci_intel_ce_init(void) __init;

void __init x86_intel_ce_early_setup(void)
{
	x86_init.resources.memory_setup = x86_intel_ce_memory_setup;
	x86_init.resources.probe_roms = x86_intel_ce_probe_roms;
	x86_init.resources.reserve_resources = x86_init_noop;
	x86_init.pci.arch_init = pci_intel_ce_init;

	pm_power_off = intel_ce_poweroff;
	machine_ops.emergency_restart = intel_ce_emergency_restart;
}
