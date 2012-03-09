/*
 * Intel CE 41xx fallback ASL.
 *
 * Copyright (c) 2011 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation;
 */

#define IRQ_LINK(__name, __uid, __irq)						\
	Device (__name) {							\
		Name (_HID, EisaId ("PNP0C0F"))					\
		Name (_UID, __uid)						\
										\
		Name (INTR, ResourceTemplate () {				\
			Interrupt (, Level, ActiveHigh, Shared) { __irq }	\
		})								\
		Method (_CRS) { return (INTR) }					\
		Method (_PRS) { return (INTR) }					\
		Method (_SRS, 1) {}						\
	}

DefinitionBlock ("intel_ce_DSDT.aml", "DSDT", 0x01, "Intel ", "CE4100  ", 0x00000001) {
	Scope (_SB) {
		Processor (CPU0, 0x0, 0x00000000, 0) {}

		Device (PCI0) {
			Name (_HID, EisaId ("PNP0A03"))
			Name (_UID, 0)
			Name (_ADR, 0x00000000)

			Name (_CRS, ResourceTemplate () {
				WordBusNumber (ResourceProducer, MinFixed, MaxFixed,
					       PosDecode, 0, 0, 0xfe, 0, 0xff)
			})

			Device (PCI1) {
				Name (_UID, 1)
				Name (_ADR, 0x00010000)

				IRQ_LINK(LGFC, 0x10, 24)
				IRQ_LINK(LSPC, 0x11, 25)
				IRQ_LINK(LMFD, 0x12, 26)
				IRQ_LINK(LCRS, 0x13, 27)
				IRQ_LINK(LTSP, 0x14, 28)
				IRQ_LINK(LTSD, 0x15, 29)
				IRQ_LINK(LDP0, 0x16, 30)
				IRQ_LINK(LDP1, 0x17, 31)
				IRQ_LINK(LAIF, 0x18, 32)
				IRQ_LINK(LVDC, 0x19, 33)
				IRQ_LINK(LDPE, 0x1a, 34)
				IRQ_LINK(LHDT, 0x1b, 35)
				IRQ_LINK(LSEC, 0x1c, 36)
				IRQ_LINK(LUAR, 0x1d, 38)
				IRQ_LINK(LNAN, 0x1e, 37)
				IRQ_LINK(LIDL, 0x1f, 39)
				IRQ_LINK(LI2C, 0x20, 40)
				IRQ_LINK(LAVC, 0x21, 44)
				IRQ_LINK(LGBE, 0x22, 45)
				IRQ_LINK(LUSB, 0x23, 46)
				IRQ_LINK(LSAT, 0x24, 47)

				Name (_PRT, Package () {
					Package () { 0x0002FFFF, 0, LGFC, 0 },
					Package () { 0x0003FFFF, 0, LMFD, 0 },
					Package () { 0x0004FFFF, 0, LTSP, 0 },
					Package () { 0x0004FFFF, 1, LTSD, 0 },
					Package () { 0x0006FFFF, 0, LDP0, 0 },
					Package () { 0x0006FFFF, 1, LDP1, 0 },
					Package () { 0x0006FFFF, 2, LAIF, 0 },
					Package () { 0x0008FFFF, 0, LVDC, 0 },
					Package () { 0x0008FFFF, 1, LDPE, 0 },
					Package () { 0x0008FFFF, 2, LHDT, 0 },
					Package () { 0x0009FFFF, 0, LSEC, 0 },
					Package () { 0x000BFFFF, 0, LUAR, 0 },
					Package () { 0x000BFFFF, 1, LIDL, 0 },
					Package () { 0x000BFFFF, 2, LI2C, 0 },
					Package () { 0x000BFFFF, 3, LIDL, 0 },
					Package () { 0x000BFFFF, 4, LIDL, 0 },
					Package () { 0x000CFFFF, 0, LGBE, 0 },
					Package () { 0x000CFFFF, 1, LCRS, 0 },
					Package () { 0x000DFFFF, 0, LUSB, 0 },
					Package () { 0x000EFFFF, 0, LSAT, 0 },
					Package () { 0x000FFFFF, 0, LNAN, 0 },
					Package () { 0x0011FFFF, 0, LSPC, 0 },
					Package () { 0x0012FFFF, 0, LAVC, 0 },
				})
			}
		}
	}
}

