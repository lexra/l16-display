
#if !defined(__ADDR_H__)
#define __ADDR_H__

//#if _MSC_VER > 1000
//#pragma once
//#endif


struct addr_t
{
	unsigned short psa;
	unsigned short sa;
	unsigned long long int ia;
	bool present;

	addr_t& operator=(const addr_t& obj);
	bool operator== (const addr_t& obj);
	bool operator!= (const addr_t& obj);

	addr_t(void);
	addr_t(const addr_t& copy);

	~addr_t(void) {};
};

#endif

