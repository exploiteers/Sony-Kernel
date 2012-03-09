/*
 *  arch/x86/include/asm/intel_ce_btv.h
 *      This file contains reboot code for BTV.
 *
 *  Copyright 2010 Sony Corporation.
 *
 *
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


#ifndef __INTEL_CE_BTV_H
#define __INTEL_CE_BTV_H

#include <linux/net.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/byteorder/generic.h>
#include <linux/console.h>

#define REBOOT_CMD_INTERVAL_MSEC   (500)
#define ETHER_CMD_INTERVAL         (3000)
#define COMMAND_LINE_SIZE  (512)

#define UART_CMD_SIZE (9)
extern const unsigned char uartCmdSystemHalt[];
extern const unsigned char uartCmdSystemReboot[];

#define ETH_CMD_SIZE (12)
extern const unsigned char ethCmdSystemHalt[];
extern const unsigned char ethCmdSystemReboot[];

#define PWRCTL_TYPE_SIZE (16)

void btv_send_msg_thru_eth(const unsigned char *buf, int size);
int  is_pwrctl_thru_eth(void);





#endif /* __INTEL_CE_BTV_H */
