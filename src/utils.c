

// Includes
//{{{

#include <stdarg.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif



//}}}


// External Functions
//{{{

 // Error
//{{{
void lnxLogError(char s[], ...)
{
	va_list arg;

	va_start(arg,s);

	// TODO see if can get this working
	// emscripten_log(EM_LOG_CONSOLE, s, arg);

	// TODO stderr instead of stdout
	//vfprintf(stderr,s,arg);
	vfprintf(stdout,s,arg);

	va_end(arg);

	//fflush(stderr);
	fflush(stdout);
}
//}}}

//}}}
