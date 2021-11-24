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



// float2hex 123.456001

int main(int argc, char *argv[])
{
	float x;
	int y = 0;
	int res;

	regex_t preg;
	char pattern[128];
	int cflags = REG_EXTENDED;
	int err;
	size_t nmatch = 3;
	regmatch_t pmatch[3];
	int len;
	char buf[128];

	setbuf(stdout, 0);
	if (argc != 2)												printf("Usage: float2hex 123.456001\n\n"), exit(1);
	memset(&preg, 0, sizeof(regex_t));
	strcpy(pattern, "^([\\+\\-]{0,}[0-9]{1,}\\.[0-9]{1,})$");
	if(0 != (err = regcomp(&preg, pattern, cflags)))					printf("REGXCOMP ERROR !\n"), exit(1);
	if (0 != (err = regexec(&preg, argv[1], nmatch, pmatch, 0)))		printf("REGEXEC ERROR !\n"), regfree(&preg), exit(1);
	len = pmatch[1].rm_eo - pmatch[1].rm_so;  
	strncpy(buf, argv[1] + pmatch[1].rm_so, len);
	res = sscanf(buf, "%f", &x);
	if (-1 == res)												printf("0XFFFFFFFF\n"), exit(1);
	regfree(&preg);

	memcpy(&y, &x, sizeof(x));
	printf("0X%08X\n", y);
	return 0;
}

