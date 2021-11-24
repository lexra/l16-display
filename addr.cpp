
//#include "StdAfx.h"
#include "addr.h"

addr_t::addr_t(void)//:present(0)
{
	psa = 0;
	sa = 0;
	ia = 0;
	present = 0;
}

addr_t::addr_t(const addr_t& copy)
{
	if (this == &copy)
		return;

	present = copy.present;
	psa = copy.psa;
	sa = copy.sa;
	ia = copy.ia;
}

addr_t& addr_t::operator=(const addr_t& obj)
{
	sa = obj.sa;
	ia = obj.ia;
	present = obj.present;
	psa = obj.psa;

	return (*this);
}

bool addr_t::operator== (const addr_t& obj)
{
	if (sa != obj.sa)
		return 0;

	if (ia != obj.ia)
		return 0;

	return 1;
}

bool addr_t::operator!= (const addr_t& obj)
{
	if (sa != obj.sa)
		return 1;

	if (ia != obj.ia)
		return 1;

	return 0;
}

