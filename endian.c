#include <stdint.h>

unsigned long long int lltob(unsigned long long int a) {
	unsigned long long int ret;
	int i;
	for(i=0;i<8;++i) {
		ret <<= 8;
		ret |= a&0xff;
		a>>=8;
	}
	return ret;
}

uint32_t ltob(uint32_t a) {
	uint32_t ret=0;
	int i;
	for(i=0;i<4;++i) {
		ret <<= 8;
		ret |= a&0xff;
		a>>=8;
	}
	return ret;
}

unsigned short stob(unsigned short a) {
	unsigned short ret;
	int i;
	for(i=0;i<4;++i) {
		ret <<= 8;
		ret |= a&0xff;
		a>>=8;
	}
	return ret;
}
