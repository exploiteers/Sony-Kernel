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
#include <linux/cgroup.h>
#include <linux/sched.h>

#include "gross_prio_mutex.h"

/* how many foreground tasks get ahead of one background */
#define FG_QUOTA	3

/* Name of cpu cgroup used to mark background tasks */
#define BG_CGROUP	"/bg_non_interactive"

void gross_prio_mutex_init(struct gross_prio_mutex *m)
{
	spin_lock_init(&m->lock);
	m->owner = NULL;
	INIT_LIST_HEAD(&m->fg_waiters);
	INIT_LIST_HEAD(&m->bg_waiters);
	m->fg_quota = FG_QUOTA;
}

static int __is_foreground_task(void)
{
	struct cgroup *cg;
	int res = 1;

	rcu_read_lock();
	cg = task_cgroup(current, cpu_cgroup_subsys_id);
	if (cg) {
		char buf[64];
		if (!cgroup_path(cg, buf, sizeof(buf)))
			res = strcmp(buf, BG_CGROUP);
	}
	rcu_read_unlock();
	return res;
}

struct __wait {
	struct task_struct *task;
	struct list_head list;
};

/* m->lock locked */
void __gross_prio_mutex_lock(struct gross_prio_mutex *m)
{
	struct __wait wait = {
		.task = current,
		.list = LIST_HEAD_INIT(wait.list),
	};

	BUG_ON(m->owner == current);

	if (__is_foreground_task())
		list_add_tail(&wait.list, &m->fg_waiters);
	else
		list_add_tail(&wait.list, &m->bg_waiters);

	do {
		set_current_state(TASK_UNINTERRUPTIBLE);
		spin_unlock(&m->lock);
		schedule();
		spin_lock(&m->lock);
	} while (m->owner != current) ;
	BUG_ON(current->state != TASK_RUNNING);
	list_del(&wait.list);
}

/* m->lock locked */
void __gross_prio_mutex_release(struct gross_prio_mutex *m)
{
	struct list_head *l;

	if (!list_empty(&m->fg_waiters) &&
	    (m->fg_quota > 0 || list_empty(&m->bg_waiters))) {
		if (m->fg_quota > 0)
			--m->fg_quota;
		l = &m->fg_waiters;
	} else {
		BUG_ON(list_empty(&m->bg_waiters));
		m->fg_quota = FG_QUOTA;
		l = &m->bg_waiters;
	}
	m->owner = list_entry(l->next, struct __wait, list)->task;
	wake_up_process(m->owner);
}
