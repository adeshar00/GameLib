
// Includes
//{{{

#include <SDL2/SDL.h>
#include <GLES2/gl2.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "utils.h"

//}}}

// Global Variables
//{{{

static SDL_Window* lvarWindow;
static SDL_GLContext lvarGlContext;

#ifdef __EMSCRIPTEN__
static int lvarEmFullscreenStatus;
#endif

//}}}


// External Functions
//{{{

 // Init
//{{{
int lnxWindowInit()
{

	// Init SDL
	if(SDL_Init(0) != 0)
	{
		lnxLogError("Unable to initialize SDL: %s\n",SDL_GetError());
		return 1;
	}

	// Init SDL video
	if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
	{
		lnxLogError("Unable to initialize video with SDL: %s\n",SDL_GetError());
		return 1;
	}

	// Create Window
	lvarWindow = SDL_CreateWindow( "Test",
	                               SDL_WINDOWPOS_CENTERED,
	                               SDL_WINDOWPOS_CENTERED,
	                               480,
	                               360,
	                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	                              );

	// Set SDL to use OpenGL ES 2.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create and bind GL context
	lvarGlContext = SDL_GL_CreateContext(lvarWindow);
	if(!lvarGlContext)
	{
		lnxLogError("GL Context creation failed.\n");
		return 1;
	}
	SDL_GL_MakeCurrent(lvarWindow, lvarGlContext);

	// Set swap interval TODO figure out if this can be set in init, do if so
	// 0 swaps whenever, 1 does vsync, -1 swaps anyway if later than vsync
	//  apparently triple buffering is special? SDL doesn't do, so no need to worry about it
	// TODO Put this into it's own function? Enable/Disable Vsync? use -1 or 0 if disabling?
	SDL_GL_SetSwapInterval(1);
	/*
	if(SDL_GL_SetSwapInterval(-1)==-1)
	{
		SDL_GL_SetSwapInterval(1);
	}
	*/

#ifdef __EMSCRIPTEN__

	// TODO see if can use below line so that keyboard only read when canvas in focus??
	//SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT,"#canvas");

	lvarEmFullscreenStatus = 0;

#endif

	// Enable Scissor for lnxWindowViewport calls
	glEnable(GL_SCISSOR_TEST);

	// TODO make this it's own function in graphics.c
	glEnable(GL_CULL_FACE);
	
	// TODO test XXX
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	return 0;
}
//}}}

 // Destroy
//{{{
void lnxWindowDestroy()
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_DestroyWindow(lvarWindow);
	SDL_GL_DeleteContext(lvarGlContext);
	SDL_Quit();
}
//}}}

 // Quit
//{{{
int lnxWindowQuit()
{
	// TODO move this to input.c?

	SDL_Event event;
	event.type = SDL_QUIT;
	event.quit.timestamp = SDL_GetTicks();

	if(SDL_PushEvent(&event)<0)
	{
		lnxLogError("lnxWindowQuit error: %s",SDL_GetError());
		return 0;
	}

	return 1;

}
//}}}

 // Swap Buffers
//{{{
void lnxWindowSwapBuffers()
{
	SDL_GL_SwapWindow(lvarWindow);
}
//}}}

 // Get Dimensions
//{{{
void lnxWindowGetDimensions(int* width, int* height)
{
#ifdef __EMSCRIPTEN__
	emscripten_get_canvas_size(width, height, 0);
#else
	SDL_GetWindowSize(lvarWindow, width, height);
#endif
}
//}}}

 // Is Fullscreen
//{{{
int lnxWindowIsFullscreen()
{

#ifdef __EMSCRIPTEN__

	return lvarEmFullscreenStatus;

#else

	return (SDL_GetWindowFlags(lvarWindow) & SDL_WINDOW_FULLSCREEN);

#endif

}
//}}}

 // Enter Fullscreen
//{{{
void lnxWindowEnterFullscreen() // FIXME cut resfactor, produce list 
{

#ifdef __EMSCRIPTEN__

	// Set fullscreen strategy
	EmscriptenFullscreenStrategy strategy;
	strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
	strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
	strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
	strategy.canvasResizedCallback = 0;
	strategy.canvasResizedCallbackUserData = 0;

	// Request fullscreen
	if(emscripten_enter_soft_fullscreen(0, &strategy) < 0)
	{
		lnxLogError("Error entering fullscreen\n");
		return;
	}
	lvarEmFullscreenStatus = 1;

#else // Native

	// TODO wrap these functions so can generate list of potential fullscreen modes
	// See https://wiki.libsdl.org/SDL_DisplayMode
	static int display_in_use = 0; /* Only using first display */
	int i, display_mode_count;
	SDL_DisplayMode mode;
	SDL_DisplayMode wackymode;
	//Uint32 f;
	SDL_Log("SDL_GetNumVideoDisplays(): %i", SDL_GetNumVideoDisplays());
	display_mode_count = SDL_GetNumDisplayModes(display_in_use);
	if (display_mode_count < 1) {
		SDL_Log("SDL_GetNumDisplayModes failed: %s", SDL_GetError());
		return;
	}
	SDL_Log("SDL_GetNumDisplayModes: %i", display_mode_count);
	for (i = 0; i < display_mode_count; ++i) {
		if (SDL_GetDisplayMode(display_in_use, i, &mode) != 0) {
			SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
			return;
		}
		//f = mode.format;
		if(i==0) wackymode = mode;
		//SDL_Log("Mode %i\tbpp %i\t%s\t%i x %i", i,
		//SDL_BITSPERPIXEL(f), SDL_GetPixelFormatName(f), mode.w, mode.h);
	}


	SDL_SetWindowFullscreen(lvarWindow, SDL_WINDOW_FULLSCREEN);
	SDL_SetWindowDisplayMode(lvarWindow, &wackymode);

#endif

}
//}}}

 // Exit Fullscreen
//{{{
void lnxWindowExitFullscreen() // FIXME cut resfactor, produce list 
{

#ifdef __EMSCRIPTEN__

	if( emscripten_exit_soft_fullscreen() < 0 )
	{
		lnxLogError("Error exiting fullscreen.\n");
		return;
	}
	lvarEmFullscreenStatus = 0;

#else // Native

	SDL_SetWindowFullscreen(lvarWindow, 0);

#endif

}
//}}}

 // Set Size
//{{{
void lnxWindowSetSize(int width, int height)
{
	SDL_SetWindowSize(lvarWindow, width, height);
}
//}}}

 // Buffer Clear
//{{{
void lnxWindowBufferClear(float red, float green, float blue)
{
	// Clears entirety of both buffers, instead of just the scissored viewport of the current
	//  back buffer like lnxWindowClear does

	glDisable(GL_SCISSOR_TEST);
	glClearColor( red, green, blue, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(lvarWindow);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);
}
//}}}

//}}}


