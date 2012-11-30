#include <time.h>

#include "wait/platform.h"

void setnonblock(int file)
{
	u_long mode = 1;
	ioctlsocket(file, FIONBIO, &mode);
	return;
}

