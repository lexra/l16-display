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



// hex2float 0X42F6E979

int main(int argc, char *argv[])
{
	int x = 0XFFFFFFFF;
	float y;
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
	if (argc != 2)												printf("Usage: hex2float 0X42F6E979\n\n"), exit(1);

	if (argv[1][0] != '0')										printf("-0.000001\n"), exit(1);
	if (argv[1][1] != 'x' && argv[1][1] != 'X')						printf("-0.000001\n"), exit(1);
	if (strlen(argv[1]) > 10)										printf("-0.000001\n"), exit(1);
	if (0 != (strlen(argv[1]) % 2))								printf("-0.000001\n"), exit(1);

	memset(&preg, 0, sizeof(regex_t));
	strcpy(pattern, "^(0[xX]{1}[0-9A-Fa-f]{2,})$");
	if(0 != (err = regcomp(&preg, pattern, cflags)))					printf("REGXCOMP ERROR !\n"), exit(1);
	if (0 != (err = regexec(&preg, argv[1], nmatch, pmatch, 0)))		printf("REGEXEC ERROR !\n"), regfree(&preg), exit(1);
	len = pmatch[1].rm_eo - pmatch[1].rm_so;  
	strncpy(buf, argv[1] + pmatch[1].rm_so, len);
	regfree(&preg);

	if (buf[1] == 'X')											res = sscanf(buf, "%X", &x);
	else														res = sscanf(buf, "%x", &x);
	if (-1 == res)												printf("-0.000001\n"), exit(1);

	memcpy(&y, &x, sizeof(x));
	printf("%f\n", y);
	return 0;
}

