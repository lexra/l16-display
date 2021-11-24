
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <sys/ioctl.h>
#include <fcntl.h>

//#include "list.h"


/////////////////////////////////////////////////////////////////////////////////////////
//

#define PSIO							'P'

#define PS_IOCTL_GETDATA			_IOR(PSIO, 0, int)
#define PS_IOCTL_SETDATA			_IOW(PSIO, 1, int)
#define PS_IOCTL_GETPOS			_IOR(PSIO, 2, int)
#define PS_IOCTL_SETPOS				_IOW(PSIO, 3, int)


/////////////////////////////////////////////////////////////////////////////////////////
//

#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((unsigned int)((unsigned int)((Byte0) & 0x00FF) + ((unsigned int)((Byte1) & 0x00FF) << 8) \
			+ ((unsigned int)((Byte2) & 0x00FF) << 16) + ((unsigned int)((Byte3) & 0x00FF) << 24)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)



/////////////////////////////////////////////////////////////////////////////////////////
//

struct ps_data_axis
{
	short x;
	short y;
	short z;
};

void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (i = 0; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Example Usage: %s -x 100 -y 100 -z 100\n", file), exit(1);
}

int main(int argc, char *argv[])
{
	struct ps_data_axis data;
	int fd = -1;
	int c = 0;
	int x = 0, y = 0, z = 0;

	setbuf(stdout, 0);
	while ((c = getopt(argc, argv, "x:y:z:X:Y:Z:")) != -1)
	{
		switch (c)
		{
		default:
			break;

		case '?':
			PrintUsage();
			break;

		case 'X':
		case 'x':
			if (strlen(optarg) > 0)
			{
				x = atoi(optarg);
				break;
			}
			PrintUsage();
			break;

		case 'Y':
		case 'y':
			if (strlen(optarg) > 0)
			{
				y = atoi(optarg);
				break;
			}
			PrintUsage();
			break;

		case 'Z':
		case 'z':
			if (strlen(optarg) > 0)
			{
				z = atoi(optarg);
				break;
			}
			PrintUsage();
			break;
		}
	}

	if ((fd = open("/dev/psensor", O_RDWR)) < 0)
	{
		printf("(%s %d) OPEN /dev/psensor FAIL, EXIT !\n", __FILE__, __LINE__), exit(1);
		return 1;
	}

	data.x = x; data.y = y; data.z = z;
	if(ioctl(fd, PS_IOCTL_SETDATA, &data) < 0)
	{
		printf("(%s %d) PS_IOCTL_SETDATA FAIL, EXIT !\n", __FILE__, __LINE__), close(fd), exit(2);
		return 1;
	}
	data.x = -101; data.y = -101; data.z = -101;
	if(ioctl(fd, PS_IOCTL_GETDATA, &data) < 0)
	{
		printf("(%s %d) PS_IOCTL_GETDATA FAIL, EXIT !\n", __FILE__, __LINE__), close(fd), exit(2);
		return 1;
	}
	printf("PSENSOR_GETDATA, X=%d Y=%d Z=%d\n", data.x, data.y, data.z);
	close(fd);
	return 0;
}

