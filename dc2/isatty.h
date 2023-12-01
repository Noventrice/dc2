#ifndef ISATTY_H
	#define ISATTY_H
	#ifdef __WIN64__
		#include <io.h>
		#define isatty _isatty
	#else
		#include <unistd.h>
	#endif
#endif
