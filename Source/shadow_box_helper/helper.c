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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include "asm.h"
#include "helper.h"

/*
 * Functions.
 */
static int sbh_file_open(struct inode* i, struct file* f);
static int sbh_file_close(struct inode* i, struct file* f);
static long sbh_file_ioctl(struct file* i, unsigned int cmd, unsigned long arg);
void sbh_printf(int level, char* format, ...);
void sbh_error_log(int error_code);
static int sbh_log_thread(void* argument);

/*
 * Variables.
 */
struct task_struct *g_log_thread_id = NULL;
static struct kfifo* g_log_info = NULL;
static int g_dev_major = -1;
static struct class* g_dev_class = NULL;
static struct device* g_dev_device = NULL;

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = sbh_file_open,
	.release = sbh_file_close,
	.unlocked_ioctl = sbh_file_ioctl,
};

/*
 * Handle file open.
 */
static int sbh_file_open(struct inode *i, struct file *f)
{
	sbh_printf(LOG_LEVEL_DEBUG, LOG_INFO "open: task=%s open file", 
		current->comm); 
	return 0;
}

/*
 * Handle file close.
 */
static int sbh_file_close(struct inode* i, struct file* f)
{
	sbh_printf(LOG_LEVEL_DEBUG, LOG_INFO "close: task=%s close file", 
		current->comm); 
	return 0;
}

/*
 * Handle file close.
 */
static long sbh_file_ioctl(struct file* f, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd)
	{
	/* Start printing Shadow-box log. */
	case IOCTL_START_LOGGING:
		sbh_printf(LOG_LEVEL_DEBUG, LOG_INFO "ioctl: task=%s IOCTL_START_LOGGING "
			"call start\n", current->comm); 
		
		if (g_log_thread_id != NULL)
		{
			kthread_stop(g_log_thread_id);
			msleep(1000);
		}
		
		g_log_info = sbh_vm_call(VM_SERVICE_GET_LOGINFO, NULL);
		if (g_log_info == NULL)
		{
			sbh_error_log(ERROR_LOGGING_FAIL);
			ret = -EINVAL;
		}
		
		g_log_thread_id = kthread_run(sbh_log_thread, (void*) g_log_info, "logger");
		if (g_log_thread_id == NULL)
		{
			sbh_error_log(ERROR_LOGGING_FAIL);
			ret = -EINVAL;
		}

		sbh_printf(LOG_LEVEL_DEBUG, LOG_INFO "ioctl: task=%s IOCTL_START_LOGGING "
			"call end, g_log_info %lX\n", current->comm, g_log_info); 

		break;

	/* Get work status. */
	case IOCTL_GET_STATUS:
		sbh_printf(LOG_LEVEL_DEBUG, LOG_INFO "ioctl: task=%s IOCTL_GET_STATUS "
			"call start\n", current->comm);

		if (g_log_info != NULL)
		{
			sbh_printf(LOG_LEVEL_DEBUG, LOG_INFO "Logging is activated, return 0\n");
			ret = 0;
		}
		else
		{
			sbh_printf(LOG_LEVEL_DEBUG, LOG_INFO "Logging is activated, return -1\n");
			ret = -EINVAL;
		}

		break;
	
	default:
		sbh_printf(LOG_LEVEL_NONE, LOG_INFO "ioctl: task=%s Unkown IOCTL call, "
			"cmd=%d\n", current->comm, cmd); 
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*
 * Print Shadow-box log.
 */
void sbh_printf(int level, char* format, ...)
{
	va_list arg_list;

	if (level <= LOG_LEVEL)
	{
		va_start(arg_list, format);
		vprintk(format, arg_list);
		va_end(arg_list);
	}
}

/*
 * Print Shadow-box error.
 */
void sbh_error_log(int error_code)
{
	sbh_printf(LOG_LEVEL_NONE, LOG_ERROR "errorcode=%d\n", error_code);
}

/*
 * Thread for log print.
 */
static int sbh_log_thread(void* argument)
{
	char buffer[MAX_LOG_LINE];
	int ret;
	int index;
	int remain;
	int end;
	int copy_bytes;
	int i;
	struct kfifo* fifo;

	fifo = (struct kfifo*) argument;
	index = 0;
	memset(buffer, 0, sizeof(buffer));

	while (!kthread_should_stop())
	{	
		remain = sizeof(buffer) - index;
		ret = kfifo_out(fifo, buffer + index, remain);
		end = index + ret;
		
		for (i = 0 ; i < end ; i++)
		{
			/* All flags are set by caller. */
			if (buffer[i] == '\n')
			{
				buffer[i] = '\0';
				printk("%s", buffer);

				/* Calculate new index. */
				copy_bytes = end - (i + 1);
				memcpy(buffer, buffer + i + 1, copy_bytes);
				index = copy_bytes;

				break;
			}
		}

		/* Buffer is full and no newline in buffer, then flush it. */
		if (i == sizeof(buffer))
		{
			buffer[sizeof(buffer) - 1] = '\0';
			printk("%s", buffer);

			index = 0;
		}

		/* if no data from fifo, sleep. */
		if ((remain > 0) && (ret == 0))
		{
			msleep(1);
		}
	}

	return 0;
}

/*
 * Clean up device information.
 */
static void sbh_clean_up_device(void)
{
	if (g_dev_device != NULL)
	{
		device_destroy(g_dev_class, MKDEV(g_dev_major, 0));
	}

	if (g_dev_class != NULL)
	{
		class_unregister(g_dev_class);
	}

	if (g_dev_major != -1)
	{
		unregister_chrdev(g_dev_major, DEVICE_NAME);
	}
}

/*
 * Start function of Shadow-box helper module
 */
static int __init shadow_box_helper_init(void)
{
	g_dev_major = register_chrdev(0, DEVICE_NAME, &fops);
	if (g_dev_major < 0)
	{
		goto ERROR;
	}

	g_dev_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
	if (g_dev_class == NULL)
	{
		goto ERROR;
	}

	g_dev_device = device_create(g_dev_class, NULL, MKDEV(g_dev_major, 0), NULL, DEVICE_NAME);
	if (g_dev_device == NULL)
	{
		goto ERROR;
	}

	sbh_printf(LOG_LEVEL_NORMAL, LOG_INFO "Start Shadow-Box Helper\n");
	sbh_error_log(ERROR_SUCCESS);

	/* Prevent module unloald. */
	try_module_get(THIS_MODULE);

	return 0;

ERROR:
	sbh_clean_up_device();
	sbh_error_log(ERROR_START_FAIL);
	return -1;
}

/*
 * End function of Shadow-box helper module.
 */
static void __exit shadow_box_helper_exit(void)
{
	sbh_printf(LOG_LEVEL_NORMAL, LOG_INFO "Stop Shadow-Box Helper\n");
	if (g_log_thread_id != NULL)
	{
		kthread_stop(g_log_thread_id);
	}

	sbh_clean_up_device();
}


module_init(shadow_box_helper_init);
module_exit(shadow_box_helper_exit);

MODULE_AUTHOR("Seunghun Han");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("Shadow-box-helper: Lightweight Hypervisor-based Kernel "
	"Protector.");
