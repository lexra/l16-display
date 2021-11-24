
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

#include <math.h>
#include <regex.h>

#include "cgic.h"



int cgiMain()
{
	int res;
	char temp[256];
	int nwk = 0, gate = 0, fwd = 0, usb;
	char ip[32], h0[32], h1[32];
	char mask[32];
	char dgw[32];
	int baud;
	int listen, p0, p1;

	FILE *pf;//, *pf1, *pf2;
	//char *fn, *fn1, *fn2;
	//FILE *pf, *pf1, *pf2;
	//char *fn, *fn1, *fn2;
	char FN[256];

	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("nwk", temp, 3);
	if (cgiFormNotFound == res)
		cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);
	nwk = atoi(temp) ? 1 : 0;


	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("gate", temp, 3);
	if (cgiFormNotFound == res)
		gate = 0;
	else
	{
		gate = (1 == atoi(temp)) ? 1 : 0;
	}


	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("fwd", temp, 3);
	if (cgiFormNotFound == res)
		fwd = 0;
	else
	{
		fwd = (1 == atoi(temp)) ? 1 : 0;
	}
	//cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) gate=%d fwd=%d !!\n", __FILE__, __LINE__, gate, fwd), exit(1);



	if (1 == nwk)
	{
		memset(ip, 0, sizeof(ip));
		res = cgiFormStringNoNewlines("ip", ip, 16);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);

		memset(mask, 0, sizeof(mask));
		res = cgiFormStringNoNewlines("mask", mask, 16);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);

		memset(dgw, 0, sizeof(dgw));
		res = cgiFormStringNoNewlines("dgw", dgw, 16);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);
	}


/*
	http://192.168.2.137:9100/cgi-bin/bost.cgi?nwk=1&ip=192.168.2.99&mask=255.255.255.0&dgw=192.168.2.254&gate=1&usb=2&baud=38400&listen=34000&fwd=1&h0=127.0.0.1&p0=34000&h1=211.79.161.6&p1=14000
*/

	if (1 == gate)
	{
		memset(temp, 0, sizeof(temp));
		res = cgiFormStringNoNewlines("usb", temp, 3);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);
		usb = atoi(temp);
		if (usb > 4 || usb < 0)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) usb > 4 || usb < 0 !!\n", __FILE__, __LINE__), exit(1);

		memset(temp, 0, sizeof(temp));
		res = cgiFormStringNoNewlines("baud", temp, 9);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);
		baud = atoi(temp);
		if (baud != 115200 && baud != 57600 && baud != 38400 && baud != 19200 && baud != 9600)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) baud error !!\n", __FILE__, __LINE__), exit(1);

		memset(temp, 0, sizeof(temp));
		res = cgiFormStringNoNewlines("listen", temp, 9);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);
		listen = atoi(temp);
		if (listen < 9000)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) listen < 9000 !!\n", __FILE__, __LINE__), exit(1);
	}

	if (1 == fwd)
	{
		memset(h0, 0, sizeof(h0));
		if (cgiFormNotFound == (res = cgiFormStringNoNewlines("h0", h0, 16)))
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);

		memset(h1, 0, sizeof(h1));
		if (cgiFormNotFound == (res = cgiFormStringNoNewlines("h1", h1, 16)))
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);

		memset(temp, 0, sizeof(temp));
		res = cgiFormStringNoNewlines("p0", temp, 9);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);
		if ((p0 = atoi(temp)) < 9000)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) p0 < 9000 !!\n", __FILE__, __LINE__), exit(1);

		memset(temp, 0, sizeof(temp));
		res = cgiFormStringNoNewlines("p1", temp, 9);
		if (cgiFormNotFound == res)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) cgiFormNotFound !!\n", __FILE__, __LINE__), exit(1);
		if ((p1 = atoi(temp)) < 9000)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) p1 < 9000 !!\n", __FILE__, __LINE__), exit(1);
	}

	srand(time(NULL));
	sprintf(FN, "/tmp/bost-%d.tmp", rand());

	pf = fopen(FN, "w+");
	if (NULL == pf)
		cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) fopen %s fail !!\n", __FILE__, __LINE__, FN), exit(1);
	fseek(pf, 0, SEEK_SET);
	fputs("#!/bin/bash\n\n", pf);
	if (1 == nwk)
	{
		fputs("/usr/bin/killall dhclient\n", pf);
		fputs("/sbin/ifconfig lo 127.0.0.1\n", pf);
		sprintf(temp, "/sbin/ifconfig eth0 %s netmask %s\n", ip, mask);
		fputs(temp, pf);
		fputs("GW=`/sbin/route|/bin/grep default|/usr/bin/awk '{ print $2 }'`\n", pf);
		fputs("/sbin/route del default gw $GW\n", pf);
		sprintf(temp, "/sbin/route add default gw %s\n\n", dgw);
		fputs(temp, pf);
	}
	if (1 == gate)
		fputs("[ -x /home/bin/gate.sh ] && /home/bin/gate.sh &\n", pf);
	if (1 == fwd)
		fputs("[ -x /home/bin/fwd.sh ] && /home/bin/fwd.sh &\n", pf);
	fputs("/sbin/ifconfig eth0:0 10.1.2.240 netmask 255.255.255.0\n", pf);
	fputs("[ -x /usr/sbin/thttpd ] && /usr/sbin/thttpd -u root -l /dev/null -p 9100 -d /var/thttpd -c /cgi-bin/* &\n\n", pf);
	fclose(pf);
	sprintf(temp, "/bin/cp -f %s /home/bin/bost.sh", FN);
	system(temp);
	system("/bin/chmod ugo+x /home/bin/bost.sh");
	system("/bin/rm -rf /tmp/bost-*.tmp");


	if (1 == gate)
	{
		sprintf(FN, "/tmp/gate-%d.tmp", rand());
		pf = fopen(FN, "w+");
		if (NULL == pf)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) fopen %s fail !!\n", __FILE__, __LINE__, FN), exit(1);
		fseek(pf, 0, SEEK_SET);
		fputs("#!/bin/bash\n\n", pf);
		fputs("while [ 1 -eq 1 ]\ndo\n", pf);
		sprintf(temp, "\tCOM=`/bin/dmesg | /bin/grep 1-2.%d | /bin/grep pl2303 | /bin/grep attach | /usr/bin/awk -F \"to\" '{print $2}' | /usr/bin/awk '{print $1}' | /usr/bin/tail -1`\n", usb + 2);
		fputs(temp, pf);
		sprintf(temp, "\t/home/bin/gate /dev/$COM B%d %d > /dev/null\n", baud, listen);
		fputs(temp, pf);
		fputs("\t/bin/sleep 3\ndone\n\n", pf);
		fclose(pf);
		sprintf(temp, "/bin/cp -f %s /home/bin/gate.sh", FN);
		system(temp);
		system("/bin/chmod ugo+x /home/bin/gate.sh");
		system("/bin/rm /tmp/gate-*.tmp");
	}


	if (1 == fwd)
	{
		sprintf(FN, "/tmp/fwd-%d.tmp", rand());
		pf = fopen(FN, "w+");
		if (NULL == pf)
			cgiHeaderContentType("text/plain"), fprintf(cgiOut, "(%s %d) fopen %s fail !!\n", __FILE__, __LINE__, FN), exit(1);
		fseek(pf, 0, SEEK_SET);
		fputs("#!/bin/bash\n\n", pf);
		fputs("while [ 1 -eq 1 ]\ndo\n\t/bin/sleep 3\n", pf);
		sprintf(temp, "\t/home/bin/fwd %s %d %s %d > /dev/null\n", h0, p0, h1, p1);
		fputs(temp, pf);
		fputs("done\n\n", pf);
		fclose(pf);
		sprintf(temp, "/bin/cp -f %s /home/bin/fwd.sh", FN);
		system(temp);
		system("/bin/chmod ugo+x /home/bin/fwd.sh");
		system("/bin/rm /tmp/fwd-*.tmp");
	}


	cgiHeaderContentType("text/html");

	fprintf(cgiOut, "<html>\n");
	fprintf(cgiOut, "<head>\n");

	fprintf(cgiOut, "<title>Beagle_xM Optional Setting</title>\n");
	fprintf(cgiOut, "<meta http-equiv=Content-Type content=\"text/html; charset=UTF-8\">\n");
	fprintf(cgiOut, "<meta http-equiv=pragma content=no-cache>\n");
	fprintf(cgiOut, "<meta http-equiv=cache-control content=no-cache>\n");

	fprintf(cgiOut, "<script language=javascript>\n<!--\n");
	//fprintf(cgiOut, "function sleep(n)\n{\n\tvar start = new Date().getTime();\n\twhile(true) if(new Date().getTime() - start > n) break;\n}\n");

	fprintf(cgiOut, "-->\n</script>\n\n");

	fprintf(cgiOut, "</head>\n");


	fprintf(cgiOut, "<body>\n");


	fprintf(cgiOut, "<h1>Beagle_xM Optional Setting</h1>\n");

	fprintf(cgiOut, "SUCCESS !!<p>\n");

	fprintf(cgiOut, "Please Reboot Beagle xM !!\n");


	fprintf(cgiOut, "</body>\n");
	fprintf(cgiOut, "</html>\n");

	//fprintf(cgiOut, "<script language=javascript>\n<!--\n");
	//	fprintf(cgiOut, "\tsetTimeout(3000);history.back();\n");
	//fprintf(cgiOut, "-->\n</script>\n\n");

	return 0;
}

