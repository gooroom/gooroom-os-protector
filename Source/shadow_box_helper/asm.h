/*
 *                       Shadow-Box Helper
 *                       -----------------
 *      Lightweight Hypervisor-Based Kernel Protector
 *
 *               Copyright (C) 2017 Seunghun Han
 *     at National Security Research Institute of South Korea
 */

/*
 * This software has GPL v2 license. See the GPL_LICENSE file.
 */
#ifndef __ASM_H__
#define __ASM_H__

/*
 * Functions.
 */
extern void* sbh_vm_call(u64 svr_num, void* arg);

#endif
