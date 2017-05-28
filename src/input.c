
// Includes
//{{{

#include <SDL2/SDL.h>
#include <stdlib.h>

#include "utils.h"

//}}}

// Definitions
//{{{

// Number of possible keyboard keys to be tracked
#define KEYCODECOUNT 512

// Max number of fingers tackable on touchpad
#define TOUCHCOUNT 32

// Max number of joysticks
#define MAXJOYSTICKS 16

// Mouse Button Template
#define INPUTMICE \
MOUSETEMPLATE(Left, SDL_BUTTON_LEFT) \
MOUSETEMPLATE(Right, SDL_BUTTON_RIGHT) \
MOUSETEMPLATE(Middle, SDL_BUTTON_MIDDLE) \
MOUSETEMPLATE(X1, SDL_BUTTON_X1) \
MOUSETEMPLATE(X2, SDL_BUTTON_X2)

//}}}

// Structs
//{{{

struct button
{
	int held;
	int downCount;
};

struct lnxInputTouch // TODO cut lnxInput prefix on this and Joystick below??
{
	int active;
	int fingerId;
	float x;
	float y;
};

struct lnxInputJoystick
{
	SDL_JoystickID id; // This is -1 if the joystick struct doesn't represent a joysick
	int axisCount;
	int buttonCount;
	int hatStart;
	struct button* buttons;
	// SDL Hats are mapped to button structs, 4 button structs per hat.  Buttons representing
	//  hat movement are at the end of the "button" array; hatStart is the index of
	//  the first hat button.
};

//}}}

// Variables
//{{{

// All file-scope variables are prefixed with "lvar"

static int lvarInitialized = 0;

// 1 if SDL_Quit event has happened since last call to lnxInputUpdate, 0 otherwise
static int lvarQuit;

// Keyboard state
static struct button lvarKeys[KEYCODECOUNT];

// Touchpad state
static struct lnxInputTouch lvarTouches[TOUCHCOUNT];
static int lvarTouchList[TOUCHCOUNT];
static int lvarTouchCount;

// Joystick state
static struct lnxInputJoystick lvarJoysticks[MAXJOYSTICKS];
static int lvarJoystickList[MAXJOYSTICKS];
static int lvarJoystickCount;

// Mouse state
static int lvarMouseActive;
static int lvarMouseX;
static int lvarMouseY;
static int lvarMouseWheelX;
static int lvarMouseWheelY;
#define MOUSETEMPLATE(NAME, BUTTON) static struct button lvar ## NAME ## Mouse;
INPUTMICE
#undef MOUSETEMPLATE

//}}}


// Internal Functions
//{{{

 // Reset
//{{{
static void lnxInputReset()
{

	// Reset keyboard downcounts
	{
		int i;
		for(i=0;i<KEYCODECOUNT;i++)
		{
			lvarKeys[i].downCount = 0;
		}
	}

	// Reset joystick button downcounts
	{
		int jl;
		for(jl=0;jl<lvarJoystickCount;jl++)
		{
			int j = lvarJoystickList[jl];
			int bc = lvarJoysticks[j].buttonCount;
			int b;
			for(b=0;b<bc;b++)
			{
				lvarJoysticks[j].buttons[b].downCount = 0;
			}
		}
	}
	
	// Reset mouse coords if in relative mode
	if(SDL_GetRelativeMouseMode()==SDL_TRUE)
	{
		lvarMouseX = 0;
		lvarMouseY = 0;
	}
	lvarMouseWheelX = 0;
	lvarMouseWheelY = 0;

	// Reset mouse button downcounts
#define MOUSETEMPLATE(NAME, BUTTON) lvar ## NAME ## Mouse.downCount = 0;
INPUTMICE
#undef MOUSETEMPLATE

	// Reset other input variables
	lvarQuit = 0;

}
//}}}

 // Clear "held" fields for buttons and touches
//{{{
static void lnxInputClearHelds()
{

	// Clear keyboard helds
	{
		int k;
		for(k=0;k<KEYCODECOUNT;k++)
		{
			lvarKeys[k].held = 0;
		}
	}

	// Clear joystick button helds
	{
		int jl;
		for(jl=0;jl<lvarJoystickCount;jl++)
		{
			int j = lvarJoystickList[jl];
			int bc = lvarJoysticks[j].buttonCount;
			int b;
			for(b=0;b<bc;b++)
			{
				lvarJoysticks[j].buttons[b].held = 0;
			}
		}
	}

	// Clear touches
	lvarTouchCount = 0;
	{
		int t;
		for(t=0;t<TOUCHCOUNT;t++)
		{
			lvarTouches[t].active = 0;
		}
	}
	
	// Clear mouse variables
	lvarMouseActive = 0;
	lvarMouseX = 0;
	lvarMouseY = 0;
#define MOUSETEMPLATE(NAME, BUTTON) lvar ## NAME ## Mouse.held = 0;
INPUTMICE
#undef MOUSETEMPLATE

}
//}}}

 // Instance ID to joystick index
//{{{
static int lnxInputIdToIndex(SDL_JoystickID instance)
{
	int jl;
	for(jl=0;jl<lvarJoystickCount;jl++)
	{
		int j = lvarJoystickList[jl];
		if(lvarJoysticks[j].id == instance)
		{
			return j;
		}
	}

	// Instance ID not found: return error
	return -1;
}
//}}}

 // Free Joystick
//{{{
static void lnxInputFreeJoystick(int index)
{

	// Free memory
	SDL_JoystickClose(SDL_JoystickFromInstanceID(lvarJoysticks[index].id));
	lvarJoysticks[index].id = -1;
	free(lvarJoysticks[index].buttons);

	// Remove joystick ID from joystick list
	int jl;
	for(jl=lvarJoystickCount-1;jl>=0;jl--)
	{
		if(lvarJoystickList[jl]==index)
		{
			lvarJoystickList[jl] = lvarJoystickList[lvarJoystickCount-1];
			lvarJoystickCount-= 1;
		}
	}

}
//}}}

//}}}

// External Functions
//{{{

 // Init
//{{{
void lnxInputInit()
{

	if(lvarInitialized) return;

	lnxInputReset();
	lnxInputClearHelds();

	lvarJoystickCount = 0;
	{
		int j;
		for(j=0;j<MAXJOYSTICKS;j++)
		{
			lvarJoysticks[j].id = -1;
		}
	}

	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0)
	{
		lnxLogError("Unable to initialize joystick system with SDL: %s\n",SDL_GetError());
	}

	lvarInitialized = 1;

}
//}}}

 // Destroy
//{{{
void lnxInputDestroy()
{

	if(!lvarInitialized) return;

	// Free joystick state
	//  (SDL_JOYSTICKADDED events are requeued for each plugged in joystick
	//  if lnxInputInit is called after this)
	int jl;
	for(jl=0;jl<lvarJoystickCount;jl++)
	{
		lnxInputFreeJoystick(lvarJoystickList[jl]);
	}
	lvarJoystickCount = 0;

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

	lvarInitialized = 0;

}
//}}}

 // Update
//{{{
void lnxInputUpdate()
{

	if(!lvarInitialized)
	{
		lnxLogError("Update called when input not initialized.\n");
		return;
	}

	lnxInputReset();

	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{

			// Quit
			//{{{
			case SDL_QUIT:
			{
				lvarQuit = 1;
			}break;
			//}}}

			// Window
			//{{{
			case SDL_WINDOWEVENT:
			{
				switch(event.window.event)
				{
					case SDL_WINDOWEVENT_FOCUS_LOST:
					{
						lnxInputClearHelds();
					}break;

					case SDL_WINDOWEVENT_ENTER:
					{
						lvarMouseActive = 1;
					}break;

					case SDL_WINDOWEVENT_LEAVE:
					{
						lvarMouseActive = 0;
					}break;
				}
			}break;
			//}}}

			// Keys
			//{{{
			case SDL_KEYDOWN:
			{
				if(!event.key.repeat)
				{
					int code = event.key.keysym.scancode;
					if(code>=0 && code<KEYCODECOUNT)
					{
						lvarKeys[code].downCount+= 1;
						lvarKeys[code].held = 1;
					}
				}
			}break;

			case SDL_KEYUP:
			{
				int code = event.key.keysym.scancode;
				if(code>=0 && code<KEYCODECOUNT)
				{
					lvarKeys[code].held = 0;
				}
			}break;
			//}}}

			// Mouse
			//{{{
			// NOTE: mouse enter/leave events are handled in SDL_WINDOWEVENT case above
			case SDL_MOUSEBUTTONDOWN:
			{
				if(event.button.which==SDL_TOUCH_MOUSEID) break;
				switch(event.button.button)
				{
#define MOUSETEMPLATE(NAME, BUTTON) \
					case BUTTON:    \
					{               \
						lvar ## NAME ## Mouse.downCount+= 1; \
						lvar ## NAME ## Mouse.held = 1; \
					}break;
INPUTMICE
#undef MOUSETEMPLATE
				}

			}break;

			case SDL_MOUSEBUTTONUP:
			{
				if(event.button.which==SDL_TOUCH_MOUSEID) break;
				switch(event.button.button)
				{
#define MOUSETEMPLATE(NAME, BUTTON) \
					case BUTTON:    \
					{               \
						lvar ## NAME ## Mouse.held = 0; \
					}break;
INPUTMICE
#undef MOUSETEMPLATE
				}
			}break;

			case SDL_MOUSEMOTION:
			{
				if(event.button.which==SDL_TOUCH_MOUSEID) break;
				if(SDL_GetRelativeMouseMode() == SDL_FALSE)
				{
					lvarMouseX = event.motion.x;
					lvarMouseY = event.motion.y;
				}
				else
				{
					lvarMouseX+= event.motion.xrel;
					lvarMouseY+= event.motion.yrel;
				}

			}break;

			case SDL_MOUSEWHEEL:
			{
				if(event.button.which==SDL_TOUCH_MOUSEID) break;
				int flipper = 2*(event.wheel.direction==SDL_MOUSEWHEEL_NORMAL)-1;
				lvarMouseWheelX+= event.wheel.x*flipper;
				lvarMouseWheelY+= event.wheel.y*flipper;
			}break;
			//}}}

			// Touch
			//{{{
			case SDL_FINGERDOWN:
			{
				int t;
				for(t=0;t<TOUCHCOUNT;t++)
				{
					if(lvarTouches[t].active==0)
					{
						lvarTouches[t].active = 1;
						lvarTouches[t].fingerId = event.tfinger.fingerId;
						lvarTouches[t].x = event.tfinger.x;
						lvarTouches[t].y = event.tfinger.y;
						break;
					}
				}
			}break;

			case SDL_FINGERUP:
			{
				int t;
				for(t=0;t<TOUCHCOUNT;t++)
				{
					if(lvarTouches[t].fingerId==event.tfinger.fingerId)
					{
						lvarTouches[t].active = 0;
					}
				}
			}break;

			case SDL_FINGERMOTION:
			{
				int t;
				for(t=0;t<TOUCHCOUNT;t++)
				{
					if(lvarTouches[t].fingerId==event.tfinger.fingerId)
					{
						lvarTouches[t].x = event.tfinger.x;
						lvarTouches[t].y = event.tfinger.y;
					}
				}
			}break;
			//}}}

			// Joystick
			//{{{
			case SDL_JOYDEVICEADDED:
			{

				// Index of lvarJoystick array to use for new joystick
				int index = 0;

				// Set index to lowest open struct in lvarJoystick array
				while(index<MAXJOYSTICKS && lvarJoysticks[index].id!=-1) index++;

				// Check that MAXJOYSTICKS not exceeded
				if(index>=MAXJOYSTICKS)
				{
					lnxLogError("Cannot add joystick, maximum joystick count exceeded.\n");
					break;
				}

				// Create SDL_Joystick object
				SDL_Joystick* j = SDL_JoystickOpen(event.jdevice.which);
				if(!j)
				{
					lnxLogError("Unable to open joystick: %s\n",SDL_GetError());
					break;
				}

				// Get joystick button & axis count
				int joybuttons = SDL_JoystickNumButtons(j);
				if(joybuttons<0)
				{
					lnxLogError("Joystick add error on numbuttons fetch: %s", SDL_GetError());
					break;
				}
				int hats = SDL_JoystickNumHats(j);
				if(hats<0)
				{
					lnxLogError("Joystick add error on numhats fetch: %s", SDL_GetError());
					break;
				}
				lvarJoysticks[index].hatStart = joybuttons;
				int buttons = joybuttons + 4*hats;
				lvarJoysticks[index].buttonCount = buttons;
				int axes = SDL_JoystickNumAxes(j);
				if(axes<0)
				{
					lnxLogError("Joystick add error on numaxes fetch: %s", SDL_GetError());
					break;
				}
				lvarJoysticks[index].axisCount = axes;

				// Create joystick button state
				struct button* blist;
				EMALLOC(blist, struct button, buttons)
				{
					int b;
					for(b=0;b<buttons;b++)
					{
						blist[b].downCount = 0;
						blist[b].held = 0;
					}
				}
				lvarJoysticks[index].buttons = blist;

				// 
				if(lvarJoystickCount>=MAXJOYSTICKS)
				{
					lnxLogError("Joystick add joycount error.\n");
					break;
				}
				lvarJoystickList[lvarJoystickCount] = index;
				lvarJoystickCount+= 1;

				lvarJoysticks[index].id = SDL_JoystickInstanceID(j);

			}break;

			case SDL_JOYDEVICEREMOVED:
			{

				int index = lnxInputIdToIndex(event.jbutton.which);
				if(index==-1)
				{
					lnxLogError("Removal of untracked joystick.\n");
					break;
				}
				lnxInputFreeJoystick(index);

			}break;

			case SDL_JOYBUTTONDOWN:
			{

				int index = lnxInputIdToIndex(event.jbutton.which);
				int bid = event.jbutton.button;

				// Check that device identifier valid
				if(index<0 || index>=MAXJOYSTICKS)
				{
					lnxLogError("Joystick with invalid device index on button down. (Index: %d)\n",index);
					break;
				}

				// Check that button is valid
				if(bid<0 || bid>=lvarJoysticks[index].hatStart)
				{
					lnxLogError("Invalid joystick button on button down (Button: %d)\n",bid);
					break;
				}

				// Check that joystick struct is active
				if(lvarJoysticks[index].id==-1) break;

				lvarJoysticks[index].buttons[bid].downCount+= 1;
				lvarJoysticks[index].buttons[bid].held = 1;

			}break;

			case SDL_JOYBUTTONUP:
			{

				int index = lnxInputIdToIndex(event.jbutton.which);
				int bid = event.jbutton.button;

				// Check that device identifier valid
				if(index<0 || index>=MAXJOYSTICKS)
				{
					lnxLogError("Joystick with invalid device index on button up. (Index: %d)\n",index);
					break;
				}

				// Check that button is valid
				if(bid<0 || bid>=lvarJoysticks[index].hatStart)
				{
					lnxLogError("Invalid joystick button on button up (Button: %d)\n",bid);
					break;
				}

				// Check that joystick struct is active
				if(lvarJoysticks[index].id==-1) break;

				lvarJoysticks[index].buttons[bid].held = 0;

			}break;

			case SDL_JOYHATMOTION:
			{

				int index = lnxInputIdToIndex(event.jbutton.which);

				// Check that device identifier valid
				if(index<0 || index>=MAXJOYSTICKS)
				{
					lnxLogError("Joystick with invalid device index on hat move. (Index: %d)\n",index);
					break;
				}

				int hid = event.jhat.hat*4+lvarJoysticks[index].hatStart;

				// Check that hat buttons are valid
				if(hid<0 || hid+3>=lvarJoysticks[index].buttonCount)
				{
					lnxLogError("Invalid joystick button on hat move.\n");
					break;
				}

				// Check that joystick struct is active
				if(lvarJoysticks[index].id==-1) break;
				
				// Set hat button variables
				#define up 0
				#define down 1
				#define left 2
				#define right 3
				struct button* bar = &(lvarJoysticks[index].buttons[hid]);
				switch(event.jhat.value)
				{
					case SDL_HAT_CENTERED:
						bar[down].held = 0;
						bar[left].held = 0;
						bar[up].held = 0;
						bar[right].held = 0;
						break;
					case SDL_HAT_LEFTUP:
						bar[up].downCount+= bar[up].held==0;
						bar[left].downCount+= bar[left].held==0;
						bar[up].held = 1;
						bar[left].held = 1;
						bar[down].held = 0;
						bar[right].held = 0;
						break;
					case SDL_HAT_LEFTDOWN:
						bar[down].downCount+= bar[down].held==0;
						bar[left].downCount+= bar[left].held==0;
						bar[down].held = 1;
						bar[left].held = 1;
						bar[up].held = 0;
						bar[right].held = 0;
						break;
					case SDL_HAT_RIGHTUP:
						bar[up].downCount+= bar[up].held==0;
						bar[right].downCount+= bar[right].held==0;
						bar[up].held = 1;
						bar[right].held = 1;
						bar[down].held = 0;
						bar[left].held = 0;
						break;
					case SDL_HAT_RIGHTDOWN:
						bar[down].downCount+= bar[down].held==0;
						bar[right].downCount+= bar[right].held==0;
						bar[down].held = 1;
						bar[right].held = 1;
						bar[up].held = 0;
						bar[left].held = 0;
						break;
					case SDL_HAT_UP:
						bar[up].downCount+= bar[up].held==0;
						bar[up].held = 1;
						bar[down].held = 0;
						bar[left].held = 0;
						bar[right].held = 0;
						break;
					case SDL_HAT_DOWN:
						bar[down].downCount+= bar[down].held==0;
						bar[up].held = 0;
						bar[down].held = 1;
						bar[left].held = 0;
						bar[right].held = 0;
						break;
					case SDL_HAT_LEFT:
						bar[left].downCount+= bar[left].held==0;
						bar[up].held = 0;
						bar[down].held = 0;
						bar[left].held = 1;
						bar[right].held = 0;
						break;
					case SDL_HAT_RIGHT:
						bar[right].downCount+= bar[right].held==0;
						bar[up].held = 0;
						bar[down].held = 0;
						bar[left].held = 0;
						bar[right].held = 1;
						break;
				}
				#undef up
				#undef down
				#undef left
				#undef right

			}break;
			//}}}

		}
	}

	// Update touch list
	lvarTouchCount = 0;
	{
		int t = 0;
		for(t=0;t<TOUCHCOUNT;t++)
		{
			if(lvarTouches[t].active)
			{
				lvarTouchList[lvarTouchCount] = t;
				lvarTouchCount++;
			}
		}
	}

}
//}}}

 // Mouse Lock Functions
//{{{
int lnxInputEnableMouseLock()
{

	if(SDL_SetRelativeMouseMode(SDL_TRUE) == 0)
	{
		return 1;
	}
	else
	{
		lnxLogError("EnableMouseLock failure: %s\n", SDL_GetError());
		return 0;
	}

}

int lnxInputDisableMouseLock()
{

	if(SDL_SetRelativeMouseMode(SDL_FALSE) == 0)
	{
		return 1;
	}
	else
	{
		lnxLogError("EnableMouseLock failure: %s\n", SDL_GetError());
		return 0;
	}

}
//}}}

 // Mouse Visibility
//{{{

int lnxInputShowCursor()
{
	if(SDL_ShowCursor(SDL_ENABLE) >= 0)
	{
		return 1;
	}
	else
	{
		lnxLogError("Show Cursor failure: %s\n", SDL_GetError());
		return 0;
	}
}

int lnxInputHideCursor()
{
	if(SDL_ShowCursor(SDL_DISABLE) >= 0)
	{
		return 1;
	}
	else
	{
		lnxLogError("Hide Cursor failure: %s\n", SDL_GetError());
		return 0;
	}
}
//}}}

 // Get Functions
//{{{

  // Keyboard
//{{{

int lnxInputIsKeyHeld(int keyScanCode)
{
	if(keyScanCode<0 || keyScanCode>=KEYCODECOUNT)
	{
		lnxLogError("Invalid keyscan code (%d) passed to lnxInputIsKeyHeld.\n", keyScanCode);
		return 0;
	}

	return lvarKeys[keyScanCode].held;
}

int lnxInputGetKeyDownCount(int keyScanCode)
{
	if(keyScanCode<0 || keyScanCode>=KEYCODECOUNT)
	{
		lnxLogError("Invalid keyscan code (%d) passed to lnxInputGetKeyDownCount.\n", keyScanCode);
		return 0;
	}

	return lvarKeys[keyScanCode].downCount;
}

const char* lnxInputGetKeyName(int keyScanCode)
{
	return SDL_GetKeyName(SDL_GetKeyFromScancode(keyScanCode));
}

//}}}

  // Mouse
//{{{

int lnxInputGetMouseLock()
{
	return SDL_GetRelativeMouseMode()==SDL_TRUE;
}

int lnxInputIsMouseActive()
{
	return lvarMouseActive;
}

int lnxInputGetMouseX()
{
	return lvarMouseX;
}

int lnxInputGetMouseY()
{
	return lvarMouseY;
}

int lnxInputGetMouseWheelX()
{
	return lvarMouseWheelX;
}

int lnxInputGetMouseWheelY()
{
	return lvarMouseWheelY;
}

#define MOUSETEMPLATE(NAME, BUTTON)                \
int lnxInputGet ## NAME ## MouseDownCount()        \
{                                                  \
	return lvar ## NAME ## Mouse.downCount; \
}                                                  \
                                                   \
int lnxInputGet ## NAME ## MouseHeld()             \
{                                                  \
	return lvar ## NAME ## Mouse.held;      \
}
INPUTMICE
#undef MOUSETEMPLATE

//}}}

  // Touch
//{{{

int lnxInputGetTouchCount()
{
	return lvarTouchCount;
}

int lnxInputGetTouch(int index, int* uid, float* x, float* y)
{
	if(index<0 || index>=lvarTouchCount)
	{
		return 0;
	}

	int ri = lvarTouchList[index]; // real index

	*uid = lvarTouches[ri].fingerId; 
	//*uid = ri; // uncomment for more obvious ipad bug TODO
	*x = lvarTouches[ri].x;
	*y = lvarTouches[ri].y;

	return 1;
}

//}}}

  // Joystick
//{{{

int lnxInputGetJoystickCount()
{
	return lvarJoystickCount;
}

int lnxInputGetJoystickIndex(int listPosition)
{
	if(listPosition<0 || listPosition>=lvarJoystickCount)
	{
		lnxLogError("Invalid list position passed to lnxInputGetJoystickIndex.\n");
		return -1;
	}

	return lvarJoystickList[listPosition];
}

int lnxInputIsJoystickActive(int joystick)
{
	if(joystick<0 || joystick>=MAXJOYSTICKS)
	{
		lnxLogError("Invalid joystick index passed to lnxInputIsJoystickActive.\n");
		return -1;
	}

	return !(lvarJoysticks[joystick].id==-1);

}

int lnxInputGetJoystickButtonCount(int joystick)
{
	if(joystick<0 || joystick>=MAXJOYSTICKS)
	{
		lnxLogError("Invalid joystick index passed to lnxInputGetJoystickButtonCount.\n");
		return -1;
	}

	if(lvarJoysticks[joystick].id==-1)
	{
		lnxLogError("Joystick index for closed joystick passed to lnxInputGetJoystickButtonCount.\n");
		return -1;
	}

	return lvarJoysticks[joystick].buttonCount;

}

int lnxInputGetJoystickButtonDownCount(int joystick, int button)
{
	if(joystick<0 || joystick>=MAXJOYSTICKS)
	{
		lnxLogError("Invalid joystick index passed to lnxInputGetJoystickButtonDownCount.\n");
		return -1;
	}

	if(lvarJoysticks[joystick].id==-1)
	{
		lnxLogError("Joystick index for closed joystick passed to lnxInputGetJoystickButtonDownCount.\n");
		return -1;
	}

	int bc = lvarJoysticks[joystick].buttonCount;

	if(button<0 || button>=bc)
	{
		lnxLogError("Invalid button index passed to lnxInputGetJoystickButtonDownCount.\n");
		return -1;
	}

	return lvarJoysticks[joystick].buttons[button].downCount;

}

int lnxInputIsJoystickButtonHeld(int joystick, int button)
{
	if(joystick<0 || joystick>=MAXJOYSTICKS)
	{
		lnxLogError("Invalid joystick index passed to lnxInputIsJoystickButtonHeld.\n");
		return -1;
	}

	if(lvarJoysticks[joystick].id==-1)
	{
		lnxLogError("Joystick index for closed joystick passed to lnxInputIsJoystickButtonHeld.\n");
		return -1;
	}

	int bc = lvarJoysticks[joystick].buttonCount;

	if(button<0 || button>=bc)
	{
		lnxLogError("Invalid button index passed to lnxInputIsJoystickButtonHeld.\n");
		return -1;
	}

	return lvarJoysticks[joystick].buttons[button].held;

}

int lnxInputGetAxisCount(int joystick)
{
	if(joystick<0 || joystick>=MAXJOYSTICKS)
	{
		lnxLogError("Invalid joystick index passed to lnxInputGetAxisCount.\n");
		return -1;
	}

	if(lvarJoysticks[joystick].id==-1)
	{
		lnxLogError("Joystick index for closed joystick passed to lnxInputGetAxisCount.\n");
		return -1;
	}

	SDL_Joystick* j = SDL_JoystickFromInstanceID(lvarJoysticks[joystick].id);

	if(!j)
	{
		lnxLogError("JoystickFromId failure in lnxInputGetAxisCount: %s",SDL_GetError());
		return -1;
	}

	int num = SDL_JoystickNumAxes(j);

	if(num<0)
	{
		lnxLogError("JoystickNumAxes fetch error in lnxInputGetAxisCount: %s",SDL_GetError());
		return -1;
	}

	return num;
}

float lnxInputGetAxis(int joystick, int axis)
{
	if(joystick<0 || joystick>=MAXJOYSTICKS)
	{
		lnxLogError("Invalid joystick index passed to lnxInputGetAxis.\n");
		return 0;
	}

	if(lvarJoysticks[joystick].id==-1)
	{
		lnxLogError("Joystick index for closed joystick passed to lnxInputGetAxis.\n");
		return 0;
	}

	SDL_Joystick* j = SDL_JoystickFromInstanceID(lvarJoysticks[joystick].id);

	if(!j)
	{
		lnxLogError("JoystickFromId failure in lnxInputGetAxis: %s",SDL_GetError());
		return 0;
	}

	int num = SDL_JoystickNumAxes(j);

	if(num<0)
	{
		lnxLogError("JoystickNumAxes fetch error in lnxInputGetAxis: %s",SDL_GetError());
		return 0;
	}

	if(axis<0 || axis>=num)
	{
		lnxLogError("Invalid axis index passed to lnxInputGetAxis.\n");
		return 0;
	}

	int raw = SDL_JoystickGetAxis(j, axis);

	if(0)//if(raw==0) // docs say JoystickGetAxis returns 0 on failure, but I'm not seeing that...
	{
		lnxLogError("JoystickAxis fetch error in lnxInputGetAxis: %s",SDL_GetError());
		return 0;
	}

	if(raw<2 && raw>-2) raw = 0;
	if(raw==-32768) raw = -32767;
	return raw/32767.0f;

}

const char* lnxInputGetJoystickName(int joystick)
{
	if(joystick<0 || joystick>=MAXJOYSTICKS)
	{
		lnxLogError("Invalid joystick index passed to lnxInputGetJoystickName.\n");
		return 0;
	}

	if(lvarJoysticks[joystick].id==-1)
	{
		lnxLogError("Joystick index for closed joystick passed to lnxInputGetName.\n");
		return 0;
	}

	SDL_Joystick* j = SDL_JoystickFromInstanceID(lvarJoysticks[joystick].id);

	if(!j)
	{
		lnxLogError("JoystickFromId failure in lnxInputGetName: %s",SDL_GetError());
		return 0;
	}

	const char* name = SDL_JoystickName(j);

	if(!name)
	{
		lnxLogError("JoystickName failure in lnxInputGetName: %s",SDL_GetError());
		return 0;
	}

	return name;

}

//}}}

  // Quit
//{{{
int lnxInputGetQuit()                  
{                                           
	return lvarQuit;             
}
//}}}

//}}}

//}}}


