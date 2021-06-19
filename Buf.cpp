#include "Buf.h"
#include <iostream>
using namespace std;

Buf::Buf() {
	b_flags = 0;
	b_forw = NULL;
	b_back = NULL;
	b_wcount = 0;
	b_addr = NULL;
	b_blkno = -1;
	b_error = -1;
	b_resid = 0;
}

Buf::~Buf()
{

}