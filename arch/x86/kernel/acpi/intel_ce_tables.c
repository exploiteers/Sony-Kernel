/*
 * Intel CE 41xx fallback ACPI tables for pre-ACPI CEFDK.
 *
 * Copyright (c) 2011 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation;
 */

#include <acpi/acpi.h>
#include <acpi/actbl1.h>
#include <acpi/actbl2.h>

#include <asm/apicdef.h>
#include <asm/io.h>
#include <asm/pci-direct.h>

#include <linux/init.h>
#include <linux/pci_ids.h>

#include "../../../../drivers/acpi/acpica/acconfig.h"
#include "../../../../drivers/acpi/acpica/actables.h"

#define OEM_ID		"INTEL "

#define ACPI_TABLE_HEADER(sign, len, rev)	\
	{					\
		.signature = sign,		\
		.length = len,			\
		.revision = rev,		\
		.oem_id = OEM_ID,		\
		.oem_table_id = "        ",	\
		.asl_compiler_id = "    ",	\
	}

static struct acpi_table_rsdp rsdp __initdata = {
	.signature = ACPI_SIG_RSDP,
	.oem_id = OEM_ID,
	.revision = 2,
	.length = sizeof(rsdp),
};

static struct __acpi_table_rsdt {
	struct acpi_table_header header;
	u32 table_offset_entry[3];
} rsdt __initdata = {
	.header = ACPI_TABLE_HEADER(ACPI_SIG_RSDT, sizeof(rsdt), 1),
};

#define PM1A_BASE	0x1000
#define PM1A_CTRL	(PM1A_BASE + 4)
#define PM_TIMER	(PM1A_BASE + 8)
#define GPE0_BASE	0x10c0

static struct acpi_table_fadt fadt __initdata = {
	.header = ACPI_TABLE_HEADER(ACPI_SIG_FADT, sizeof(fadt), 3),
	.sci_interrupt = 9,
	.smi_command = 0xb2,
	.acpi_enable = 0xe1,
	.acpi_disable = 0x1e,
	.pm1a_event_block = PM1A_BASE,
	.pm1a_control_block = PM1A_CTRL,
	.pm_timer_block = PM_TIMER,
	.gpe0_block = GPE0_BASE,
	.pm1_event_length = 4,
	.pm1_control_length = 2,
	.pm_timer_length = 4,
	.gpe0_block_length = 8,
	.C2latency = 101,
	.C3latency = 1001,
	.duty_offset = 1,
	.duty_width = 3,
	.day_alarm = 0x0d,
	.century = 0x50,
	.boot_flags = ACPI_FADT_LEGACY_DEVICES | ACPI_FADT_8042,
	.flags = ACPI_FADT_WBINVD | ACPI_FADT_C1_SUPPORTED |
		 ACPI_FADT_RESET_REGISTER | ACPI_FADT_SLEEP_TYPE,
	.reset_register = {
		.space_id = ACPI_ADR_SPACE_SYSTEM_IO,
		.bit_width = 8,
		.address = 0xcf9,
	},
	.reset_value = 0x06,
	.xpm1a_event_block = {
		.space_id = ACPI_ADR_SPACE_SYSTEM_IO,
		.bit_width = 4,
		.address = PM1A_BASE,
	},
	.xpm1a_control_block = {
		.space_id = ACPI_ADR_SPACE_SYSTEM_IO,
		.bit_width = 2,
		.address = PM1A_CTRL,
	},
	.xpm_timer_block = {
		.space_id = ACPI_ADR_SPACE_SYSTEM_IO,
		.bit_width = 4,
		.address = PM_TIMER,
	},
	.xgpe0_block = {
		.space_id = ACPI_ADR_SPACE_SYSTEM_IO,
		.bit_width = 8,
		.address = GPE0_BASE,
	},
};

static struct acpi_table_facs facs __initdata = {
	.signature = ACPI_SIG_FACS,
	.length = sizeof(facs),
	.version = 1,
};

static u8 dsdt[] __initdata = {
#include "intel_ce_DSDT.hex"
};

static struct __acpi_table_apic {
	struct acpi_table_madt madt;
	struct acpi_madt_local_apic local_apic;
	struct acpi_madt_io_apic io_apic0;
	struct acpi_madt_io_apic io_apic1;
	struct acpi_madt_interrupt_override override;
} madt __initdata = {
	.madt = {
		.header = ACPI_TABLE_HEADER(ACPI_SIG_MADT, sizeof(madt), 1),
		.address = APIC_DEFAULT_PHYS_BASE,
		.flags = ACPI_MADT_PCAT_COMPAT,
	},
	.local_apic = {
		.header = {
			.type = ACPI_MADT_TYPE_LOCAL_APIC,
			.length = sizeof(madt.local_apic),
		},
		.processor_id = 0,
		.id = 0,
		.lapic_flags = ACPI_MADT_ENABLED,
	},
	.io_apic0 = {
		.header = {
			.type = ACPI_MADT_TYPE_IO_APIC,
			.length = sizeof(madt.io_apic0),
		},
		.id = 0,
		.address = 0xfec00000,
		.global_irq_base = 0,
	},
	.io_apic1 = {
		.header = {
			.type = ACPI_MADT_TYPE_IO_APIC,
			.length = sizeof(madt.io_apic1),
		},
		.id = 1,
		.address = 0xbffff000,
		.global_irq_base = 24,
	},
	.override = {
		.header = {
			.type = ACPI_MADT_TYPE_INTERRUPT_OVERRIDE,
			.length = sizeof(madt.override),
		},
		.source_irq = 9,
		.global_irq = 9,
		.inti_flags = ACPI_MADT_POLARITY_ACTIVE_HIGH |
			      ACPI_MADT_TRIGGER_LEVEL,
	},
};

static struct acpi_table_hpet hpet __initdata = {
	.header = ACPI_TABLE_HEADER(ACPI_SIG_HPET, sizeof(hpet), 1),
	.id = (PCI_VENDOR_ID_INTEL << 16) | 0xa201,
	.address = {
		.space_id = ACPI_ADR_SPACE_SYSTEM_MEMORY,
		.address = 0xfed00000,
	},
	.minimum_tick = 128,
	.flags = ACPI_HPET_NO_PAGE_PROTECT,
};

static inline u8 acpi_checksum(const void *buf, size_t len)
{
	return -acpi_tb_checksum((u8*)buf, len);
}

void __init x86_intel_ce_fill_acpi_tables(unsigned long base,
					  unsigned long size)
{
	acpi_physical_address pa;

	/* We don't need no APCI IRQ routing */
	if (acpi_noirq)
		return;

	/* Don't do anything if firmware supports ACPI */
	if (ACPI_SUCCESS(acpi_find_root_pointer(&pa)))
		return;

	/* Enable local APIC if needed */
	if (!cpu_has_apic) {
		u32 l, h;

		rdmsr(MSR_IA32_APICBASE, l, h);
		if (!(l & MSR_IA32_APICBASE_ENABLE)) {
			l &= ~MSR_IA32_APICBASE_BASE;
			l |= MSR_IA32_APICBASE_ENABLE | APIC_DEFAULT_PHYS_BASE;
			wrmsr(MSR_IA32_APICBASE, l, h);
		}

		/* The APIC feature bit should now be enabled in `cpuid' */
		if (!(cpuid_edx(1) & (1 << X86_FEATURE_APIC))) {
			pr_warning("intel_ce: could not enable APIC!\n");
			return;
		} else {
			pr_info("intel_ce: enabled local APIC\n");
		}

		set_cpu_cap(&boot_cpu_data, X86_FEATURE_APIC);
	}

	pr_info("intel_ce: using fallback UP ACPI tables\n");

	/* FACS */
	fadt.facs = base;
	memcpy(__va(base), &facs, sizeof(facs));
	base += ALIGN(sizeof(facs), 64);

	/* DSDT */
	fadt.Xdsdt = fadt.dsdt = base;
	memcpy(__va(base), &dsdt, sizeof(dsdt));
	base += ALIGN(sizeof(dsdt), 64);

	/* FADT */
	rsdt.table_offset_entry[0] = base;
	fadt.header.checksum = acpi_checksum(&fadt, sizeof(fadt));
	memcpy(__va(base), &fadt, sizeof(fadt));
	base += ALIGN(sizeof(fadt), 64);

	/* APIC */
	rsdt.table_offset_entry[1] = base;
	madt.madt.header.checksum = acpi_checksum(&madt, sizeof(madt));
	memcpy(__va(base), &madt, sizeof(madt));
	base += ALIGN(sizeof(madt), 64);

	/* HPET */
	rsdt.table_offset_entry[2] = base;
	hpet.header.checksum = acpi_checksum(&hpet, sizeof(hpet));
	memcpy(__va(base), &hpet, sizeof(hpet));
	base += ALIGN(sizeof(hpet), 64);

	/* RSDT */
	rsdp.rsdt_physical_address = base;
	rsdt.header.checksum = acpi_checksum(&rsdt, sizeof(rsdt));
	memcpy(__va(base), &rsdt, sizeof(rsdt));

	/* RSD PTR */
	rsdp.checksum = acpi_checksum(&rsdp, ACPI_RSDP_CHECKSUM_LENGTH);
	rsdp.extended_checksum = acpi_checksum(&rsdp,
					       ACPI_RSDP_XCHECKSUM_LENGTH);
	memcpy(__va(ACPI_HI_RSDP_WINDOW_BASE), &rsdp, sizeof(rsdp));

	/* Set base addresses for power management I/O */
	write_pci_config(0, 0x1f, 0, 0x48, PM1A_BASE | 0x80000000);
	write_pci_config(0, 0x1f, 0, 0x4c, GPE0_BASE | 0x80000000);

	/* Enable SCI */
	write_pci_config(0, 0x1f, 0, 0x58, 0);
	outb(1, PM1A_CTRL);
}

