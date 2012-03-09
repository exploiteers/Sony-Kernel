/*
 *  GPL LICENSE SUMMARY
 *
 *  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *  The full GNU General Public License is included in this distribution
 *  in the file called LICENSE.GPL.
 *
 *  Contact Information:
 *    Intel Corporation
 *    2200 Mission College Blvd.
 *    Santa Clara, CA  97052
 *
 * Portions (C) Copyright 2010 Google, Inc
 *
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>

#include <asm/pci_x86.h>

#undef DBG
#undef DEBUG_PCI_SIM

#ifdef DEBUG_PCI_SIM
#define DBG(a...) printk(a)
#else
#define DBG(a...) do {} while (0)
#endif

typedef struct {
	u32 value;
	u32 mask;
} sim_reg_t;

typedef struct {
	int dev;
	int func;
	int reg;
	sim_reg_t sim_reg;
} sim_dev_reg_t;

#define MB (1024 * 1024)
#define KB (1024)
#define SIZE_TO_MASK(size) (~(size - 1))

static sim_dev_reg_t av_dev_reg_fixups[] = {
	{  2, 0, 0x10, { 0, SIZE_TO_MASK(16*MB) } },
	{  2, 0, 0x14, { 0, SIZE_TO_MASK(256) } },
	{  2, 1, 0x10, { 0, SIZE_TO_MASK(64*KB) } },
	{  3, 0, 0x10, { 0, SIZE_TO_MASK(64*KB) } },
	{  4, 0, 0x10, { 0, SIZE_TO_MASK(128*KB) } },
	{  4, 1, 0x10, { 0, SIZE_TO_MASK(128*KB) } },
	{  6, 0, 0x10, { 0, SIZE_TO_MASK(512*KB) } },
	{  6, 1, 0x10, { 0, SIZE_TO_MASK(512*KB) } },
	{  6, 2, 0x10, { 0, SIZE_TO_MASK(64*KB) } },

	{  8, 0, 0x10, { 0, SIZE_TO_MASK(1*MB) } },
	{  8, 1, 0x10, { 0, SIZE_TO_MASK(64*KB) } },
	{  8, 2, 0x10, { 0, SIZE_TO_MASK(64*KB) } },

	{  9, 0, 0x10, { 0, SIZE_TO_MASK(1*MB) } },
	{  9, 0, 0x14, { 0, SIZE_TO_MASK(64*KB) } },
	{ 10, 0, 0x10, { 0, SIZE_TO_MASK(256) } },
	{ 10, 0, 0x14, { 0, SIZE_TO_MASK(256*MB) } },
	{ 11, 0, 0x10, { 0, SIZE_TO_MASK(256) } },
	{ 11, 0, 0x14, { 0, SIZE_TO_MASK(256) } },

	{ 11, 1, 0x10, { 0, SIZE_TO_MASK(256) } },
	{ 11, 2, 0x10, { 0, SIZE_TO_MASK(256) } },
	{ 11, 2, 0x14, { 0, SIZE_TO_MASK(256) } },
	{ 11, 2, 0x18, { 0, SIZE_TO_MASK(256) } },

	{ 11, 3, 0x10, { 0, SIZE_TO_MASK(256) } },
	{ 11, 3, 0x14, { 0, SIZE_TO_MASK(256) } },

	{ 11, 4, 0x10, { 0, SIZE_TO_MASK(256) } },
	{ 11, 5, 0x10, { 0, SIZE_TO_MASK(64*KB) } },
	{ 11, 6, 0x10, { 0, SIZE_TO_MASK(256) } },
	{ 11, 7, 0x10, { 0, SIZE_TO_MASK(64*KB) } },

	{ 12, 0, 0x10, { 0, SIZE_TO_MASK(128*KB) } },
	{ 12, 0, 0x14, { 0, SIZE_TO_MASK(256) } },
	{ 12, 1, 0x10, { 0, SIZE_TO_MASK(1024) } },
	{ 13, 0, 0x10, { 0, SIZE_TO_MASK(32*KB) } },
	{ 13, 1, 0x10, { 0, SIZE_TO_MASK(32*KB) } },

	{ 14, 0,    8, { 0x01060100, 0 } },

	{ 14, 0, 0x10, { 0, 0 } },
	{ 14, 0, 0x14, { 0, 0 } },
	{ 14, 0, 0x18, { 0, 0 } },
	{ 14, 0, 0x1C, { 0, 0 } },
	{ 14, 0, 0x20, { 0, 0 } },
	{ 14, 0, 0x24, { 0, SIZE_TO_MASK(0x200) } },

	{ 15, 0, 0x10, { 0, SIZE_TO_MASK(64*KB) } },
	{ 15, 0, 0x14, { 0, SIZE_TO_MASK(64*KB) } },

	{ 16, 0, 0x10, { 0, SIZE_TO_MASK(64*KB) } },
	{ 16, 0, 0x14, { 0, SIZE_TO_MASK(64*MB) } },
	{ 16, 0, 0x18, { 0, SIZE_TO_MASK(64*MB) } },

	{ 17, 0, 0x10, { 0, SIZE_TO_MASK(128*KB) } },
	{ 18, 0, 0x10, { 0, SIZE_TO_MASK(1*KB) } }
};

static void __init init_sim_regs(void)
{
	u32 sata_cfg_phys_addr = 0;
	int i;

	raw_pci_ops->read(0, 1, PCI_DEVFN(14, 0), 0x10, 4, &sata_cfg_phys_addr);
	for (i = 0; i < ARRAY_SIZE(av_dev_reg_fixups); i++) {
		if (av_dev_reg_fixups[i].dev == 14) {
			if (av_dev_reg_fixups[i].reg == 0x24) {
                                /* SATA AHCI base address has an offset 0x400
                                 * from the SATA base physical address.
                                 */
				av_dev_reg_fixups[i].sim_reg.value =
					sata_cfg_phys_addr + 0x400;
			}
		} else {
			raw_pci_ops->read(0, 1, PCI_DEVFN(av_dev_reg_fixups[i].dev,
					  av_dev_reg_fixups[i].func),
					  av_dev_reg_fixups[i].reg, 4,
					  &av_dev_reg_fixups[i].sim_reg.value);
		}
	}
}

sim_reg_t *get_sim_reg(int bus, int devfn, int reg, int len) {
	int dev = PCI_SLOT(devfn);
	int func = PCI_FUNC(devfn);
	int i;

	/* A/V bridge devices are on bus 1. */
	if (bus == 1) {
		for (i = 0; i < ARRAY_SIZE(av_dev_reg_fixups); i++) {
			if ((reg & ~3) == av_dev_reg_fixups[i].reg &&
			    dev == av_dev_reg_fixups[i].dev &&
			    func == av_dev_reg_fixups[i].func)
			{
				return &av_dev_reg_fixups[i].sim_reg;
			}
		}
	}

	return NULL;
}

static inline void extract_bytes(u32 *value, int reg, int len) {
	uint32_t mask;

	*value >>= ((reg & 3) * 8);
	mask = 0xFFFFFFFF >> ((4 - len) * 8);
	*value &= mask;
}

static int gen3_conf_read(struct pci_bus *bus, unsigned int devfn, int reg,
			  int len, u32 *value)
{
	unsigned long flags;
	unsigned int dev;
	unsigned int func;
	u32 av_bridge_base;
	u32 av_bridge_limit;
	int retval;
	sim_reg_t *sim_reg = NULL;
	int simulated_read;

	dev = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);

	simulated_read = true;
	retval = 0;

	sim_reg = get_sim_reg(bus->number, devfn, reg, len);

	if (sim_reg != NULL) {
		raw_spin_lock_irqsave(&pci_config_lock, flags);
		*value = sim_reg->value;
		raw_spin_unlock_irqrestore(&pci_config_lock, flags);

                /* EHCI registers has 0x100 offset. */
		if (bus->number == 1 && dev == 13 && reg == 0x10 && func < 2) {
			if (*value != sim_reg->mask) {
				*value |= 0x100;
			}
		}
		extract_bytes(value, reg, len);
	/* Emulate TDI USB controllers. */
	} else if (bus->number == 1 && dev == 13 && (func == 0 || func == 1) && ((reg & ~3) == PCI_VENDOR_ID)) {
		*value = 0x0101192E;
		extract_bytes(value, reg, len);
        /* b0:d1:f0 is A/V bridge. */
	} else if (bus->number == 0 && dev == 1 && func == 0) {
		switch (reg) {

                        /* Make BARs appear to not request any memory. */
			case PCI_BASE_ADDRESS_0:
			case PCI_BASE_ADDRESS_0 + 1:
			case PCI_BASE_ADDRESS_0 + 2:
			case PCI_BASE_ADDRESS_0 + 3:
				*value = 0;
				break;

                        /* Since subordinate bus number register is hardwired
                         * to zero and read only, so do the simulation.
                         */
			case PCI_PRIMARY_BUS:
				if (len == 4) {
					*value = 0x00010100;
				} else {
					simulated_read = false;
				}
				break;

			case PCI_SUBORDINATE_BUS:
				*value = 1;
				break;

			case PCI_MEMORY_BASE:
			case PCI_MEMORY_LIMIT:
                                /* Get the A/V bridge base address. */
				raw_pci_ops->read(0, 0, PCI_DEVFN(1, 0), PCI_BASE_ADDRESS_0, 4,
					&av_bridge_base);

				av_bridge_limit = av_bridge_base + (512*1024*1024 - 1);
				av_bridge_limit >>= 16;
				av_bridge_limit &= 0xFFF0;

				av_bridge_base >>= 16;
				av_bridge_base &= 0xFFF0;

				if (reg == PCI_MEMORY_LIMIT) {
					*value = av_bridge_limit;
				} else if (len == 2) {
					*value = av_bridge_base;
				} else {
					*value = (av_bridge_limit << 16) | av_bridge_base;
				}
				break;
                        /* Make prefetchable memory limit smaller than prefetchable
                         * memory base, so not claim prefetchable memory space.
                         */
			case PCI_PREF_MEMORY_BASE:
				*value = 0xFFF0;
				break;
			case PCI_PREF_MEMORY_LIMIT:
				*value = 0x0;
				break;
                        /* Make IO limit smaller than IO base, so not claim IO space. */
			case PCI_IO_BASE:
				*value = 0xF0;
				break;
			case PCI_IO_LIMIT:
				*value = 0;
				break;
			default:
				simulated_read = false;
				break;
		}
	} else {
		simulated_read = false;
	}

	if (!simulated_read) {
		retval = raw_pci_ops->read(pci_domain_nr(bus), bus->number, devfn, reg, len, value);
	}

	DBG("gen3_conf_read: %2x:%2x.%d[%2X(%d)] = %X\n", bus->number, dev, func, reg, len, *value);
	return retval;
}

static int gen3_conf_write(struct pci_bus *bus, unsigned int devfn, int where,
			   int size, u32 value)
{
	int dev = PCI_SLOT(devfn);
	int func = PCI_FUNC(devfn);
	sim_reg_t *sim_reg;
	int retval = 0;

	DBG("gen3_conf_write: %2x:%2x.%d[%2X(%d)] <- %X\n", bus->number, dev,
	    func, where, size, value);

	sim_reg = get_sim_reg(bus->number, devfn, where, size);
	if (sim_reg) {
		unsigned long flags;

		raw_spin_lock_irqsave(&pci_config_lock, flags);
		sim_reg->value &= ~sim_reg->mask;
		sim_reg->value |= value & sim_reg->mask;
		raw_spin_unlock_irqrestore(&pci_config_lock, flags);

	} else if (!bus->number && dev == 1 && !func &&
		   (where & ~3) == PCI_BASE_ADDRESS_0) {
                /* Discard writes to A/V bridge BAR. */
	} else {
		retval = raw_pci_ops->write(pci_domain_nr(bus), bus->number,
					    devfn, where, size, value);
	}

	return retval;
}

int __init pci_intel_ce_init(void)
{
	init_sim_regs();
	pci_root_ops.read = gen3_conf_read;
	pci_root_ops.write = gen3_conf_write;
	pci_probe |= PCI_ROOT_NO_CRS;
	return 1;
}
