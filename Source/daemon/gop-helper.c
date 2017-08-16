#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "gop-helper.h"

/*
 *	버퍼를 관리하는 구조체
 */
typedef struct buffer_struct
{
	char buffer[4096];
	int index;
} BUFFER;

/*
 *	정보를 출력하는 함수
 */
void print_id(const char *comment)
{
	fprintf(stderr, "sid: %5d, pgid: %5d, pid: %5d, ppid: %5d   # %s\n",
			(int)getsid(0), (int)getpgid(0), (int)getpid(), (int)getppid(), comment);
}

/*
 *	버퍼를 비우는 함수
 */
void clear_buffer(BUFFER* buffer)
{
	buffer->index = 0;
}

/*
 *	버퍼에 문자열을 추가하는 함수
 */
int add_to_buffer(BUFFER* buffer, char c)
{
	/* 개행이거나 버퍼가 꽉 찾으면 처리 필요. */
	if ((c == '\n') || (buffer->index >= sizeof(buffer->buffer)))
	{
		buffer->buffer[buffer->index] = '\0';
		return 1;
	}

	buffer->buffer[buffer->index] = c;
	buffer->index++;

	return 0;
}

/*
 *	버퍼에서 에러코드 찾기
 * 	6,1722,55121010,-;shadow-box: errorcode=1
 */
int find_error_code_in_buffer(BUFFER* buffer)
{
	int i;
	int error_code = -1;
	char* signature = "shadow-box: errorcode=";

	/* ; 문자까지 찾은 후 : 문자를 찾음. */
	for (i = 0 ; i < buffer->index ; i++)
	{
		if (buffer->buffer[i] == ';')
		{
			if (strncmp(buffer->buffer + i + 1, signature, strlen(signature)) == 0)
			{
				error_code = atoi(buffer->buffer + i + 1 + strlen(signature));
				break;
			}
		}
	}

	return error_code;
}

/*
 *	로그 메시지를 파일에 쓰는 함수
 */
void write_msg_to_file(int error_code)
{
	FILE* fp;
	char msg_unknown_buffer[1024] = {0,};
	char* msg_buffer[] =
	{
		"서비스가 정상적으로 실행되었습니다",
		"서비스가 시작되지 않았습니다",
		"지원되지 않는 하드웨어입니다",
		"서비스가 정상적으로 실행되지 않았습니다",
		"악의적인 커널 변조가 감지되었습니다",
		"지원되지 않는 운영체제입니다. OS Protector 업데이트가 필요합니다.",
		"악의적인 시스템 종료 또는 리부팅이 감지되었습니다",
		"메모리 할당에 실패했습니다. 시스템 재시작이 필요합니다",
		"최대 작업 수가 초과했습니다. 시스템 재시작이 필요합니다.",
		"최대 모듈 수가 초과했습니다. 시스템 재시작이 필요합니다."
	};

	mkdir("/tmp/gooroom_message", S_IRWXU | S_IRGRP  | S_IXGRP | S_IROTH | S_IXOTH);
	fp = fopen("/tmp/gooroom_message/message.txt", "w");
	if (fp == NULL)
	{
		return ;
	}

	/* 에러코드의 범위 검사. */
	if (error_code >= (sizeof(msg_buffer) / sizeof(char*)))
	{
		sprintf(msg_unknown_buffer, "알 수 없는 오류가 발생했습니다. 에러코드:%d", error_code);
		fwrite(msg_unknown_buffer, 1, strlen(msg_unknown_buffer), fp);
		fprintf(stderr, "message: %s\n", msg_unknown_buffer);
	}
	else
	{
		fwrite(msg_buffer[error_code], 1, strlen(msg_buffer[error_code]), fp);
		fprintf(stderr, "message: %s\n", msg_buffer[error_code]);
	}
	fclose(fp);
}

/*
 *	메인 함수
 */
int main(void)
{
	pid_t pid;
	int fd;
	FILE* fp_kmsg;
	char buffer[2];
	int read_size;
	int error;
	int error_code;
	BUFFER line_buffer = {"", 0};
	char insmod_buffer[200];
	int i;
	long argument;

	/* Daemon 만드는 코드. */
	if (daemon(1, 1) == -1)
	{
		return -1;
	}

	if ((pid = fork()) == -1)
	{
		return -1;
	}
	else if (pid > 0)
	{
		/* 부모는 종료. */
		_exit(0);
	}

	/* 자식은 damon으로 실행되었음. */
	signal(SIGHUP, SIG_IGN);
	chdir("/");

	if (setsid() == -1)
	{
		fprintf(stderr, "setsid() failed: %s\n", strerror(errno));
		return -1;
	}

	/* 시작되었다는 메시지를 표시. */
	write_msg_to_file(ERROR_NOT_START);

	/* Shadow-box 모듈과 Shadow-box helper 모듈을 로딩. */
	sprintf(insmod_buffer, "/sbin/insmod /usr/share/gooroom/security/os-protector/shadow_box_helper.ko");
	error = system(insmod_buffer);
	if ((error == 127) || (error == -1))
	{
		fprintf(stderr, "Shadow-box-helper module insmod fail\n", strerror(errno));
		return -1;
	}
	else if (WEXITSTATUS(error) != 0)
	{
		fprintf(stderr, "Shadow-box-helper module load fail\n", strerror(errno));
		return -1;
	}

	sprintf(insmod_buffer, "/sbin/insmod /usr/share/gooroom/security/os-protector/shadow_box.ko");
	error = system(insmod_buffer);
	if ((error == 127) || (error == -1))
	{
		fprintf(stderr, "Shadow-box module insmod fail\n", strerror(errno));
		return -1;
	}
	else if (WEXITSTATUS(error) != 0)
	{
		fprintf(stderr, "Shadow-box module load fail\n", strerror(errno));
		return -1;
	}

	/* IOCTL로 모듈 로딩을 시작. */
	fd = open("/dev/sb-helper", O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "Shadow-box-helper module start fail\n", strerror(errno));
		system("rmmod shadow_box_helper");
		return -1;
	}

	if (ioctl(fd, IOCTL_START_LOGGING, &argument) != 0)
	{
		fprintf(stderr, "Shadow-box-helper module start fail\n", strerror(errno));
		system("rmmod shadow_box_helper");
		return -1;
	}

	close(fd);

	clear_buffer(&line_buffer);

	/* /proc/kmsg를 열지 못하면 실패. */
	fp_kmsg = fopen("/dev/kmsg", "r");
	if (fp_kmsg == NULL)
	{
		fprintf(stderr, "/proc/kmsg open fail\n");
		return -1;
	}

	print_id("os_protector start success.");

	while(1)
	{
		read_size = fread(buffer, 1, sizeof(buffer) - 1, fp_kmsg);	
		buffer[read_size] = '\0';

		if (add_to_buffer(&line_buffer, buffer[0]) == 1)
		{
			error_code = find_error_code_in_buffer(&line_buffer);
			if (error_code != -1)
			{
				fprintf(stderr, "errorcode: %d\n", error_code);
				write_msg_to_file(error_code);
			}
			clear_buffer(&line_buffer);
		}

		if (read_size < (sizeof (buffer) - 1))
		{
			sleep(1);
		}
	}

	stdin = freopen("/dev/null", "r", stdin);
	stdout = freopen("/dev/null", "w", stdout);
	stderr = freopen("/dev/null", "w", stderr);

	return 0;
}
