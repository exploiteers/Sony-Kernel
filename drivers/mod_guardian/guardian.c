/*
 *  guardian.ko
 *    - prohibit remount with rw flag.
 */

#ifndef CONFIG_SECURITY
#error This module depends on CONFIG_SECURITY=y
#endif

#include <linux/init.h>
#include <linux/security.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h> /* copy_from_user */

/* flag to keep protection status */
static int protected = 0;
#define PROC_ENTRY "protected"

/* print out debug messages */
static int debug = 0;

#define DBG(fmt, arg...)                                   \
        do {                                                    \
                if (debug)                                      \
                        printk(KERN_DEBUG "%s: %s: " fmt ,      \
                                "guardian" , __FUNCTION__ ,        \
                                ## arg);                        \
        } while (0)


/*
 *  guardian_sb_mount:
 *      Check permission before an object specified by dev_name is mounted on
 *      the mount point named by path.
 * @dev_name: unused
 * @path: mount point
 * @type: unused
 * @flags: unused
 * @data: unused
 *
 * Return 0 if permission is granted.
 */
static int guardian_sb_mount (char *dev_name,
                          struct path *path,
                          char *type,
                          unsigned long flags,
                          void *data)
{
        DBG("dev=%s flags=%lx\n", dev_name, flags);

	if ( !protected ) {
	  return 0; /* permission is granted */
	}

	/* avoid '-o rw,remount' operation */
	if ( (~flags & MS_RDONLY) && (flags & MS_REMOUNT) ) {
	  DBG("Remount operation not permitted.\n");
	  return -EPERM;
	}

	return 0;
}

/*
 *  Read/Write proc entry
 */
static int guardian_proc_write (struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
        char buf;

        if ( copy_from_user( &buf, buffer, sizeof(char) ) ) {
                printk (KERN_INFO "Failure copy_from_user\n");
                return -EFAULT;
	}

	if ( buf == '1' ) {
	        protected = 1;
	}

	return sizeof(char);
}

static int guardian_proc_read(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
        return sprintf( page, "%d\n", protected );
}

/**
 * Security fooks
 */
static struct security_operations guardian_ops = {
        .name =                         "guardian",
	.sb_mount =                     guardian_sb_mount,
};

static int __init guardian_init (void)
{
    struct proc_dir_entry *entry;

    if (!security_module_enable(&guardian_ops))
        return 0;

    printk(KERN_INFO "Guardian:  Initializing.\n");


    /* proc fs entry */
    entry = create_proc_entry(PROC_ENTRY, 
			      S_IFREG | S_IRUSR | S_IWUSR, NULL);
    if ( !entry ) {
      printk(KERN_INFO "Failure create proc fs entry.\n");
      return -EBUSY;
    }
    entry->read_proc = guardian_proc_read;
    entry->write_proc = guardian_proc_write;

    /* register ourselves with the security framework */
    if (register_security (&guardian_ops))
        panic ("Guardian: Failure registering with the kernel\n");

    return 0;
}

security_initcall (guardian_init);


