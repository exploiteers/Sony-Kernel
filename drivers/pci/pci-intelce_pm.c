/*
 *  GPL LICENSE SUMMARY
 *
 *  Copyright(c) 2010 Intel Corporation. All rights reserved.
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
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/pci-intelce_pm.h>

#include "pci.h"

static intel_pm_pci_ops_t external_pm_ops;
static DEFINE_MUTEX(icepm_lock);

#define CALL_EXTERNAL(rc, x, args...)		\
	mutex_lock(&icepm_lock);		\
	if (external_pm_ops.x) 			\
		rc = external_pm_ops.x(args);	\
	mutex_unlock(&icepm_lock)

static pci_power_t intel_pm_pci_choose_state(struct pci_dev * dev)
{
	pci_power_t rc = 0;
	CALL_EXTERNAL(rc, choose_state, dev);
	return rc;
}

static bool intel_pm_pci_power_manageable(struct pci_dev *dev)
{
	bool rc = true;
	CALL_EXTERNAL(rc, is_manageable, dev);
	return rc;
}

static int intel_pm_pci_set_power_state(struct pci_dev *dev, pci_power_t state)
{
	int rc = 0;
	CALL_EXTERNAL(rc, set_state, dev, state);
	return rc;
}

static bool intel_pm_pci_can_wakeup(struct pci_dev *dev)
{
	bool rc = true;
	CALL_EXTERNAL(rc, can_wakeup, dev);
	return rc;
}

static int intel_pm_pci_sleep_wake(struct pci_dev *dev, bool enable)
{
	int rc = 0;
	CALL_EXTERNAL(rc, sleep_wake, dev, enable);
	return rc;
}

void intel_pm_register_callback(intel_pm_pci_ops_t *ops)
{
	mutex_lock(&icepm_lock);
	if (!ops)
		memset(&external_pm_ops, 0x0, sizeof(external_pm_ops));
	else
		memcpy(&external_pm_ops, ops, sizeof(external_pm_ops));
	mutex_unlock(&icepm_lock);
}

EXPORT_SYMBOL(intel_pm_register_callback);

/* Unimplemented stubs */
static int intel_pm_pci_run_wake(struct pci_dev *dev, bool enable)
{
	return -ENODEV;
}

static struct pci_platform_pm_ops intel_pci_platform_pm = {
	.is_manageable = intel_pm_pci_power_manageable,
	.set_state = intel_pm_pci_set_power_state,
	.choose_state = intel_pm_pci_choose_state,
	.can_wakeup = intel_pm_pci_can_wakeup,
	.sleep_wake = intel_pm_pci_sleep_wake,
	.run_wake = intel_pm_pci_run_wake,
};

static int __init intel_pm_pci_init(void)
{
	pci_set_platform_pm(&intel_pci_platform_pm);
	return 0;
}

arch_initcall(intel_pm_pci_init);
