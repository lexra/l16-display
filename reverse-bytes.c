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


void ReverseBytes(unsigned char *pData, unsigned char len)
{
	unsigned char i, j, temp;

	for (i = 0, j = len - 1; len > 1; len -= 2)
	{
		temp = pData[i];
		pData[i++] = pData[j];
		pData[j--] = temp;
	}
}

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
	//unsigned char ch;
	char line[256];
	unsigned char buf[256], *p = buf;

	setbuf(stdout, 0);
	if (argc != 2)												printf("Usage: xorsum 0X000004\n\n"), exit(1);

	if (argv[1][0] != '0')										printf("0X00\n"), exit(1);
	if (argv[1][1] != 'x' && argv[1][1] != 'X')						printf("0X00\n"), exit(1);
	if (strlen(argv[1]) < 4)										printf("0X00\n"), exit(1);
	if (0 != (strlen(argv[1]) % 2))								printf("0X00\n"), exit(1);

	memset(&preg, 0, sizeof(regex_t));
	strcpy(pattern, "^([0-9A-Fa-f]{1,})$");
	if(0 != (err = regcomp(&preg, pattern, cflags)))					printf("REGXCOMP ERROR !\n"), exit(1);
	if (0 != (err = regexec(&preg, argv[1] + 2, nmatch, pmatch, 0)))	printf("REGEXEC ERROR !\n"), regfree(&preg), exit(1);
	regfree(&preg);

	len = strlen(argv[1]);
	n = len / 2;
	for (i = 1; i < n; i++)
	{
		memset(line, 0, sizeof(line));
		strncpy(line, argv[1] + i * 2, 2);
		res = sscanf(line, "%X", (unsigned int *)p);
		if (-1 == res)											printf("0X00\n"), exit(1);
		p++;
	}

	ReverseBytes(buf, p -buf);
	printf("0X");
	for (i = 0; i < p -buf; i++)									printf("%02X", buf[i]);
	printf("\n");
	return 0;
}

