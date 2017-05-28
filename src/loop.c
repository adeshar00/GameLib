
// Includes
//{{{

#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

//}}}


// External Functions
//{{{

 // Loop Start
//{{{
void lnxLoopStart(void (*f)())
{

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(f, 0, 1);
#else
	while(1)
	{
		(*f)();
	}
#endif

}
//}}}

 // Get Time
//{{{
int lnxLoopGetTime()
{
	return SDL_GetTicks();
}
//}}}

//}}}


