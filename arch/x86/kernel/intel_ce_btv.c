/*
 *  arch/x86/kernel/intel_ce_btv.c
 *      This file contains reboot code for BTV.
 *
 *  Copyright 2010 Sony Corporation.
 *
 *  This file is based on the arch/x86/kernel/intel_ce_btv.c
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#define ONE_MB                 (1024 * 1024)

#define CE_UART_COMMON(__base)			\
	.iobase = __base,				\
	.uartclk = 14745600,				\
	.iotype = UPIO_PORT,				\
	.flags = UPF_SKIP_TEST | UPF_BOOT_AUTOCONF |	\
		 UPF_FIXED_TYPE | UPF_FIXED_PORT,	\
	.type = PORT_XSCALE

#define CE_UART0_COMMON		CE_UART_COMMON(0x3f8)
#define CE_UART1_COMMON		CE_UART_COMMON(0x2f8)

/* Where CEFDK puts ACPI tables */
#define CEFDK_ACPI_BASE     0x10000
#define CEFDK_ACPI_SIZE     0x8000

#define SOCK_FOR_SYNC  (0)
#define SOCK_FOR_ASYNC (1)
#define NUM_OF_SOCKS   (2)

#include <asm/intel_ce_btv.h>
const unsigned char uartCmdSystemHalt[] =
{ 0xA8, 0x57, 0x00, 0x03, 0x0F, 0x00, 0x00, 0x59, 0x9A };
const unsigned char uartCmdSystemReboot[] =
{ 0xA8, 0x57, 0x00, 0x03, 0x0F, 0x01, 0x00, 0x6A, 0xAB };

const unsigned char ethCmdSystemHalt[] =
{0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00};
const unsigned char ethCmdSystemReboot[] =
{0x00, 0x0f, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00};

char pwrctl_type[PWRCTL_TYPE_SIZE] = {0};

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

static int __init parse_pwrctl_type(char *param, char *val)
{
    /* Capture board_name */
    if (!strcmp(param, "pwrctl_type") && val) {
        strlcpy(pwrctl_type, val, sizeof(pwrctl_type));
    }
    return 0;
}

int is_pwrctl_thru_eth(void){
    return (!strncmp(pwrctl_type, "ether", PWRCTL_TYPE_SIZE));
}

void
btv_send_msg_thru_eth(const unsigned char *buf, int size)
{
    struct socket *sock[NUM_OF_SOCKS];
    struct sockaddr ksockaddr[NUM_OF_SOCKS] = {0};
    struct msghdr msg;
    struct kvec   iov;

    printk("Send Message Thru Eth\n");

    for (;;) {

    /* To communicate TVSOC, we have to prepare two sockets for sync and async ports */

    sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock[SOCK_FOR_SYNC]);
    sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock[SOCK_FOR_ASYNC]);

    /* initialize socket for sync command */
    ksockaddr[SOCK_FOR_SYNC].sa_family      = PF_INET;
    /* sockaddr.sin_addr.s_addr = inet_addr((const char*)("198.18.186.199")); */
    //*((unsigned short *)&(ksockaddr.sa_data[0])) = 0x409c; /* 0x9c40:40000 */
    *((unsigned short *)&(ksockaddr[SOCK_FOR_SYNC].sa_data[0])) = htons(40000); /* 0x9c40:40000 */
    //*((unsigned int   *)&(ksockaddr.sa_data[2])) = 0x0b9c082b; /*  0x2b:43 0x08:08 0x9c:156 0x0b:11 */
    *((unsigned int   *)&(ksockaddr[SOCK_FOR_SYNC].sa_data[2])) = 0xc7ba12c6; /*  0xc6:198 0x12:18 0xba:186 0xc7:199 */

    /* initialize socket for async command */
    ksockaddr[SOCK_FOR_ASYNC].sa_family      = PF_INET;
    *((unsigned short *)&(ksockaddr[SOCK_FOR_ASYNC].sa_data[0])) = htons(40001); /* 0x9d40:40001 */
    //*((unsigned int   *)&(ksockaddr.sa_data[2])) = 0x0b9c082b; /*  0x2b:43 0x08:08 0x9c:156 0x0b:11 */
    *((unsigned int   *)&(ksockaddr[SOCK_FOR_ASYNC].sa_data[2])) = 0xc7ba12c6; /*  0xc6:198 0x12:18 0xba:186 0xc7:199 */



    while(kernel_connect(sock[SOCK_FOR_SYNC], (struct sockaddr *)&ksockaddr[SOCK_FOR_SYNC], sizeof(struct sockaddr), 0) < 0) {
        printk("Cannot connect server sync port, Retry \n");
        mdelay(REBOOT_CMD_INTERVAL_MSEC);
	}
    while(kernel_connect(sock[SOCK_FOR_ASYNC], (struct sockaddr *)&ksockaddr[SOCK_FOR_ASYNC], sizeof(struct sockaddr), 0) < 0) {
        printk("Cannot connect server async port, Retry \n");
        mdelay(REBOOT_CMD_INTERVAL_MSEC);
    }

    iov.iov_base = (void *)buf;
    iov.iov_len = ETH_CMD_SIZE;

    msg.msg_name = (struct sockaddr *)&ksockaddr[SOCK_FOR_SYNC];
    msg.msg_namelen = sizeof(struct sockaddr);
    msg.msg_iov = (struct iovec *) &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;

    kernel_sendmsg(sock[SOCK_FOR_SYNC], &msg, &iov, 1, ETH_CMD_SIZE);

	sock_release(sock[SOCK_FOR_SYNC]);
	sock_release(sock[SOCK_FOR_ASYNC]);

    mdelay(ETHER_CMD_INTERVAL);
	}
    /* NOREACHED */
}

static void
btv_send_msg_thru_uart(const unsigned char *buf, int size)
{
    struct console co, *con;

    printk("Send Message Thru Uart\n");

    for (con = console_drivers; con; con = con->next) {
        if (!strcmp(con->name, "ttyS"))
            break;
    }

    if (!con || !(con->write)) {
        printk("Poweroff/Reboot failed -- System halted\n");
        while (1);
    }

    co.index = 1; /* ttyS1 */
    con->setup(&co, "115200n8"); /* set baudrate */

    for (;;) {
        con->write(&co, buf, size);

        mdelay(REBOOT_CMD_INTERVAL_MSEC);
    }
    /* NOREACHED */
}

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
    e820_update_range(0x10000, 0x8000, E820_RAM, E820_ACPI);
#endif
    /* 8051 microcode can be loaded at 0x30000 or 0x40000 */
    e820_update_range(0x30000, 0x20000, E820_RAM, E820_RESERVED);

    /* e1000 "NVRAM" */
    e820_update_range(0x60000, 0x1000, E820_RAM, E820_RESERVED);

    linux_top = x86_intel_ce_machine_top_of_ram(memory_top);
    if (linux_top < memory_top)
        e820_update_range(linux_top, memory_top - linux_top, E820_RAM,
                  E820_RESERVED);

    /* check board name for reboot and halt sequence*/
    {
    static __initdata char tmp_cmdline[COMMAND_LINE_SIZE];
    strlcpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
    parse_args("beyond_param", tmp_cmdline, NULL, 0, parse_pwrctl_type);
    }

    return "BIOS-e801";
}

void __init __attribute__((weak))
x86_intel_ce_fill_acpi_tables(unsigned long base, unsigned long size) {}

static void __init x86_intel_ce_probe_roms(void)
{
    x86_intel_ce_fill_acpi_tables(CEFDK_ACPI_BASE, CEFDK_ACPI_SIZE);
}

static void intel_ce_poweroff(void)
{
    if(!is_pwrctl_thru_eth()){
	    btv_send_msg_thru_uart(uartCmdSystemHalt, UART_CMD_SIZE);
    } 
}

static void intel_ce_emergency_restart(void)
{
    if(!is_pwrctl_thru_eth()){
        btv_send_msg_thru_uart(uartCmdSystemReboot, UART_CMD_SIZE);
    }
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
