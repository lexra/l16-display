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
#include <regex.h>


int main(int argc, char *argv[])
{
	int res;

	regex_t preg;
	char pattern[128];
	int cflags = REG_EXTENDED;
	int err;
	size_t nmatch = 256;
	regmatch_t pmatch[256];
	int len, n, i;
	unsigned char sum = 0;
	unsigned char ch;
	char buf[256];

	setbuf(stdout, 0);
	if (argc != 2)												printf("Usage: xorsum 0X000004\n\n"), exit(1);

	if (argv[1][0] != '0')										printf("0XFF\n"), exit(1);
	if (argv[1][1] != 'x' && argv[1][1] != 'X')						printf("0XFF\n"), exit(1);
	if (strlen(argv[1]) < 4)										printf("0XFF\n"), exit(1);
	if (0 != (strlen(argv[1]) % 2))								printf("0XFF\n"), exit(1);

	memset(&preg, 0, sizeof(regex_t));
	strcpy(pattern, "^([0-9A-Fa-f]{1,})$");
	if(0 != (err = regcomp(&preg, pattern, cflags)))					printf("REGXCOMP ERROR !\n"), exit(1);
	if (0 != (err = regexec(&preg, argv[1] + 2, nmatch, pmatch, 0)))	printf("REGEXEC ERROR !\n"), regfree(&preg), exit(1);
	regfree(&preg);

	len = strlen(argv[1]);
	n = len / 2;
	for (i = 1; i < n; i++)
	{
		memset(buf, 0, sizeof(buf));
		strncpy(buf, argv[1] + i * 2, 2);
		res = sscanf(buf, "%X", (unsigned int *)&ch);
		if (-1 == res)											printf("0XFF\n"), exit(1);
		sum ^= ch;
	}

	printf("0X%02X\n", sum);
	return 0;
}

