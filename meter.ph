#!/usr/bin/php
<?php

define("LENGTH", 32);
define("SIGN_BIT", 31);
define("EXPONENT_MSB", 30);
define("EXPONENT_LSB", 23);
define("MANTISSA_MSB", 22);
define("MANTISSA_LSB", 0);
	
function hex2float($strHex)
{
	$dec = hexdec($strHex);
	$sign = ($dec & (1 << SIGN_BIT)) != 0;
	$exp = (($dec & ((2 << EXPONENT_MSB) - (1 << EXPONENT_LSB))) >>
	EXPONENT_LSB)
	- (1 << (EXPONENT_MSB - EXPONENT_LSB))
	- (MANTISSA_MSB - MANTISSA_LSB);
	$man = (($dec & ((2 << MANTISSA_MSB) - (1 << MANTISSA_LSB))) >>
	MANTISSA_LSB)
	+ (2 << (MANTISSA_MSB - MANTISSA_LSB));
	$float = floatval($man * pow(2, $exp) * ($sign ? -1 : 1));
	return $float;
}




////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

// V1 A1 1000A1 W1 1000W1 P1 100P1 1000P1 VA1 1000VA1 SA UPTIME S1 1000S1 ZERO
// PROG 0X00124B000001796F I1 100P1 UPTIME STR
// /usr/bin/uptime | /usr/bin/awk '{print $3 " " $4 " " $5}' | /bin/sed s/,$//



	if ($argc != 6 && $argc != 3)
	{
		//echo "HELLO2\n";
		exit;
	}
	$pat = '/^0[xX]+[0-9A-Fa-f]{16}$/';
	if (1 != preg_match($pat, $argv[1], $regs))
	{
		//echo "HELLO3\n";
		exit;
	}

	$querySA = 0;
	if ($argc == 3)		$querySA = 1;
	if ($argc == 3 && 0 != strcmp($argv[2], 'SA'))
	{
		//echo "HELLO\n";
		exit;
	}

	if ($argc == 6)
	for ($i = 0; $i < 3; $i++)
	{
		if (
			(0 != strcmp($argv[$i + 2], 'A1')) &&
			(0 != strcmp($argv[$i + 2], '1000A1')) &&
			(0 != strcmp($argv[$i + 2], 'W1')) &&
			(0 != strcmp($argv[$i + 2], '1000W1')) &&
			(0 != strcmp($argv[$i + 2], 'PF1')) &&
			(0 != strcmp($argv[$i + 2], '1000PF1')) &&
			(0 != strcmp($argv[$i + 2], '100PF1')) &&
			(0 != strcmp($argv[$i + 2], 'V1')) &&
			(0 != strcmp($argv[$i + 2], 'VA1')) &&
			(0 != strcmp($argv[$i + 2], '1000VA1')) &&
			(0 != strcmp($argv[$i + 2], 'SA')) &&
			(0 != strcmp($argv[$i + 2], 'UPTIME')) &&
			(0 != strcmp($argv[$i + 2], 'S1')) &&
			(0 != strcmp($argv[$i + 2], 'ZERO')) &&
			(0 != strcmp($argv[$i + 2], '1000S1'))
		)
		{
			//echo "HELLO1\n";
			exit;
		}
	}


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////


	$stamp = 1;
	$V1 = (float)0.000001;
	$A1 = (float)0.000001;
	$W1 = (float)0.000001;
	$VA1 = (float)0.000001;
	$S1 = (float)0.000001;
	$SA = "0X0000";

	$pat0 = '/^.+ ([0-9]{10})\] [A-Z]+_RSP.+\[SA\]=(0X[0-9A-F]+).+\[IA\]=';
	$pat0 .= $argv[1];
	$pat0 .= '.+$/';

	$pat1 = '/^.+ ([0-9]{10})\] [A-Z]+_RSP.+\[SA\]=(0X[0-9A-F]+).+\[IA\]=';
	$pat1 .= $argv[1];
	$pat1 .= '.+\[R-0X009C\]=(0X[0-9A-F]+).+\[R-0X009D\]=(0X[0-9A-F]+).+$/';

	$pat2 = '/^.+ ([0-9]{10})\] [A-Z]+_RSP.+\[SA\]=(0X[0-9A-F]+).+\[IA\]=0X[0-9A-F]+.+\[R\-0X00A4\]=(0X[0-9A-F]+).+\[R\-0X00A5\]=(0X[0-9A-F]+).+$/';
	$pat3 = '/^.+ ([0-9]{10})\] [A-Z]+_RSP.+\[SA\]=(0X[0-9A-F]+).+\[IA\]=0X[0-9A-F]+.+\[R\-0X00AC\]=(0X[0-9A-F]+).+\[R\-0X00AD\]=(0X[0-9A-F]+).+$/';
	$pat4 = '/^.+ ([0-9]{10})\] [A-Z]+_RSP.+\[SA\]=(0X[0-9A-F]+).+\[IA\]=0X[0-9A-F]+.+\[R\-0X00B4\]=(0X[0-9A-F]+).+\[R\-0X00B5\]=(0X[0-9A-F]+).+$/';
	$pat5 = '/^.+ ([0-9]{10})\] [A-Z]+_RSP.+\[SA\]=(0X[0-9A-F]+).+\[IA\]=0X[0-9A-F]+.+\[R\-0X00C6\]=(0X[0-9A-F]+).+\[R\-0X00C7\]=(0X[0-9A-F]+).+$/';

	$h = popen("/usr/bin/tail -n 500 /tmp/zlog.* | /usr/bin/sort -u","r");
	if (!$h)
	{
		//echo "HELLO5\n";
		exit;
	}
	while (!feof($h))
	{
		$line = fgets($h, 1200);
		$line = trim($line, "\n");

		if (1 == $querySA)
		{
			if (1 != preg_match($pat0, $line, $regs))	continue;
			if ($stamp <= $regs[1])
			{
				$stamp = $regs[1];
				//printf("%s\n", $regs[2]);
				$SA = $regs[2];
			}
			continue;
		}

		if(strlen($line) < 1024)			continue;

		if (1 != preg_match($pat1, $line, $regs))	continue;
		if ($stamp <= $regs[1])
		{
			$stamp = $regs[1];
			$V1 = hex2float(substr($regs[3], -4) . substr($regs[4], -4));
			$SA = $regs[2];
		}
		if (1 != preg_match($pat2, $line, $regs))	continue;
		$A1 = hex2float(substr($regs[3], -4) . substr($regs[4], -4));

		if (1 != preg_match($pat3, $line, $regs))	continue;
		$W1 = hex2float(substr($regs[3], -4) . substr($regs[4], -4));

		if (1 != preg_match($pat4, $line, $regs))	continue;
		$VA1 = hex2float(substr($regs[3], -4) . substr($regs[4], -4));

		if (1 != preg_match($pat5, $line, $regs))	continue;
		$S1 = hex2float(substr($regs[3], -4) . substr($regs[4], -4));
	}
	pclose($h);


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

	if (1 == $querySA)
	{
		printf("%s\n", $SA);
		exit;
	}

	$PF1 = (float)2.000000;
	//if ($VA1 > 0)	$PF1 = round($W1 / $VA1, 6);
	if ($VA1 > 0)	$PF1 = $W1 / $VA1;


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

	for ($i = 0; $i < 3; $i++)
	{
		if (0 == strcmp($argv[$i + 2], 'A1'))		printf("%.06f\n", $A1 * 1.00, 6);
		if (0 == strcmp($argv[$i + 2], '1000A1'))	printf("%.06f\n", $A1 * 1000.00, 6);
		if (0 == strcmp($argv[$i + 2], 'W1'))		printf("%.06f\n", $W1 * 1.00);
		if (0 == strcmp($argv[$i + 2], '1000W1'))	printf("%.06f\n", $W1 * 1000.00);
		if (0 == strcmp($argv[$i + 2], 'V1'))		printf("%.06f\n", $V1 * 1.00);
		if (0 == strcmp($argv[$i + 2], 'P1'))		printf("%.06f\n", $PF1 * 1.00);
		if (0 == strcmp($argv[$i + 2], '1000PF1'))	printf("%.06f\n", $PF1 * 1000.00);
		if (0 == strcmp($argv[$i + 2], '100PF1'))	printf("%.06f\n", $PF1 * 100.00);
		if (0 == strcmp($argv[$i + 2], 'VA1'))		printf("%.06f\n", $VA1 * 1.00);
		if (0 == strcmp($argv[$i + 2], '1000VA1'))	printf("%.06f\n", $VA1 * 1000.00);
		if (0 == strcmp($argv[$i + 2], 'S1'))		printf("%.06f\n", $S1 * 1.00);
		if (0 == strcmp($argv[$i + 2], '1000S1'))	printf("%.06f\n", $S1 * 1000.00);
		if (0 == strcmp($argv[$i + 2], 'ZERO'))		printf("0.000000\n");

		if (0 == strcmp($argv[$i + 2], 'SA'))		printf("%s\n", $SA);

		if (0 == strcmp($argv[$i + 2], 'UPTIME'))
		{
			$UT = "";
			$f = popen("/usr/bin/uptime | /usr/bin/awk '{print $3 \" \" $4 \" \" $5}' | /bin/sed s/,$//","r");
			while (!feof($f))		$UT .= fgets($f, 1200);
			pclose($f);
			$UT = trim($UT, "\n");
			printf("%s\n", $UT);
		}
	}
	printf("$argv[5]\n");
?>
