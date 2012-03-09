/*
 * Simple priority mutex.
 * Android cgroups are used for task prioritization
 *
 * Copyright (C) 2010, 2011 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 */
#ifndef _GROSS_PRIO_MUTEX_H_
#define _GROSS_PRIO_MUTEX_H_

#ifdef CONFIG_YAFFS_CGROUP_PRIO_MUTEX

#include <linux/list.h>
#include <linux/spinlock.h>

struct gross_prio_mutex {
	spinlock_t lock;
	struct task_struct *owner;
	struct list_head fg_waiters;
	struct list_head bg_waiters;
	int fg_quota;
};

void gross_prio_mutex_init(struct gross_prio_mutex *m);

void __gross_prio_mutex_lock(struct gross_prio_mutex *m);
static inline void gross_prio_mutex_lock(struct gross_prio_mutex *m)
{
	spin_lock(&m->lock);
	if (likely(!m->owner))
		m->owner = current;
	else
		__gross_prio_mutex_lock(m);
	spin_unlock(&m->lock);
}

void __gross_prio_mutex_release(struct gross_prio_mutex *m);
static inline void gross_prio_mutex_release(struct gross_prio_mutex *m)
{
	spin_lock(&m->lock);
	if (likely(list_empty(&m->fg_waiters) && list_empty(&m->bg_waiters)))
		m->owner = NULL;
	else
		__gross_prio_mutex_release(m);
	spin_unlock(&m->lock);
}

#else /* !CONFIG_YAFFS_CGROUP_PRIO_MUTEX */

#define gross_prio_mutex		semaphore
#define gross_prio_mutex_init		init_MUTEX
#define gross_prio_mutex_lock		down
#define gross_prio_mutex_release	up

#endif /* !CONFIG_YAFFS_CGROUP_PRIO_MUTEX */
#endif /* _GROSS_PRIO_MUTEX_H_ */
