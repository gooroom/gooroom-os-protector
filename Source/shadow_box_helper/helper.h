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
#ifndef __SHADOW_BOX_HELPER_H__
#define __SHADOW_BOX_HELPER_H__

/*
 * Macros.
 */
/* Log level. */
#define LOG_LEVEL							(LOG_LEVEL_NORMAL)
#define LOG_LEVEL_NONE						0
#define LOG_LEVEL_NORMAL					1
#define LOG_LEVEL_DEBUG						2
#define LOG_LEVEL_DETAIL					3

/* Log type. */
#define LOG_INFO							KERN_INFO "shadow-box-helper: "
#define LOG_ERROR							KERN_ERR "shadow-box-helper: "
#define LOG_WARNING							KERN_WARNING "shadow-box-helper: "

/* Log buffer size */
#define MAX_LOG_BUFFER_SIZE					(4 * 1024 * 1024)
#define MAX_LOG_LINE						(1024)

/* Shadow-box-helper error codes. */
#define ERROR_SUCCESS						0
#define ERROR_START_FAIL				 	1
#define ERROR_LOGGING_FAIL				 	2

/* Device Name. */
#define DEVICE_NAME							"sb-helper"
#define DEVICE_CLASS_NAME					"sb-helper"

/* VM call service number. */
#define VM_SERVICE_GET_LOGINFO				0
#define VM_SERVICE_SHUTDOWN					10000
#define VM_SERVICE_SHUTDOWN_THIS_CORE		10001

/* IOCTL number. */
#define IOCTL_START_LOGGING					0
#define IOCTL_GET_STATUS					1

#endif
