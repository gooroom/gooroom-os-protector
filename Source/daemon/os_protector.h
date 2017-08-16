#ifndef __OS_PROTECTOR_H__
#define __OS_PROTECTOR_H__

/*
 *	매크로
 */

/* Error code. */
#define ERROR_SUCCESS							0
#define ERROR_NOT_START						 	1
#define ERROR_HW_NOT_SUPPORT					2
#define ERROR_LAUNCH_FAIL						3
#define ERROR_KERNEL_MODIFICATION				4
#define ERROR_KERNEL_VERSION_MISMATCH			5
#define ERROR_SHUTDOWN_TIME_OUT					6
#define ERROR_MEMORY_ALLOC_FAIL					7
#define ERROR_TASK_OVERFLOW						8
#define ERROR_MODULE_OVERFLOW					9

/* IOCTL number. */
#define IOCTL_START_LOGGING						0
#define IOCTL_GET_STATUS						1

#endif
