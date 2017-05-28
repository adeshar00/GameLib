
// Function Prototypes
//{{{

void lnxInputInit();
void lnxInputDestroy();
void lnxInputUpdate();

// Returns 1 if key was held when lnxInputUpdate was last called
int lnxInputIsKeyHeld(int keyScanCode);

// Returns the number of times the key was pressed down between the last two update calls
int lnxInputGetKeyDownCount(int keyScanCode);

// Returns the name of the key (accounts for keyboard layout)
const char* lnxInputGetKeyName(int keyScanCode);

// Mouse lock means mouse can't leave window.  MouseX and Y refer to change in coords since last
//  update if in lock mode.  Mouse cursor is hidden if locked
int lnxInputEnableMouseLock(); // Returns 1 if successful, 0 if error
int lnxInputDisableMouseLock();
int lnxInputGetMouseLock(); // Returns 1 if locked, 0 if unlocked

// Show or Hide OS mouse cursor; returns 1 if sucessful, 0 if error
int lnxInputShowCursor();
int lnxInputHideCursor();

// If user hit alt+f4 or the X button or called lnxWindowQuit
int lnxInputGetQuit();

// FIXME if mouse is on window or not
int lnxInputIsMouseActive();

// Mouse coords
// TODO comment: how is dx/dy if mouselock true; move under lock functions??
int lnxInputGetMouseX();
int lnxInputGetMouseY();

// Mouse wheel (change since last call to lnxInputUpdate)
//
//  Note: on some mice this may increase by more than 1 unit per tick:
//  If doing something like changing weapons where each tick matters,
//  maybe just check if there has been a positive or negative increase
//  since the last update, so the behavior is the same no matter the device.
//
//  Example:
//   Instead of:
//                currentWeapon+= lnxInputGetMouseWheelY();
//   You could do:
//                currentWeapon+= (lnxInputGetMouseWheelY()>0);
//                currentWeapon-= (lnxInputGetMouseWheelY()<0);
int lnxInputGetMouseWheelX();
int lnxInputGetMouseWheelY();

// mouse buttons: downcount is how many clicks between updates, held is if mouse was held at update
int lnxInputGetLeftMouseDownCount();
int lnxInputGetLeftMouseHeld();
int lnxInputGetRightMouseDownCount();
int lnxInputGetRightMouseHeld();
int lnxInputGetMiddleMouseDownCount();
int lnxInputGetMiddleMouseHeld();
int lnxInputGetX1MouseDownCount();
int lnxInputGetX1MouseHeld();
int lnxInputGetX2MouseDownCount();
int lnxInputGetX2MouseHeld();

// how many fingers on the screen
int lnxInputGetTouchCount();

// info for each finger: it's coords, and an ID unique to when that finger first touched screen
int lnxInputGetTouch(int index, int* uid, float* x, float* y);


// On Joysticks:
//  Because joysticks can be added and removed at random, a list of
//  currently active joysticks is made available via the lnxInputGetJoystickCount
//  and lnxInputGetJoystickIndex functions.  Below functions will return -1 if
//  there is an error: call TODO ERRORLOGFUNCTION for details.
// 
// For an example see LINK  TODO make an example of how this works, and put it hyar

// Returns the number of joysticks plugged in
int lnxInputGetJoystickCount();

// Returns the Index of the N'th Joystick (See "On Joysticks")
int lnxInputGetJoystickIndex(int listPosition);

// Returns 1 if joystick is active, 0 if inactive
int lnxInputIsJoystickActive(int joystick);

// Returns number of buttons for joystick
int lnxInputGetJoystickButtonCount(int joystick);

// Returns down count for passed joystick button
int lnxInputGetJoystickButtonDownCount(int joystick, int button);

// Returns 1 if passed joystick button is held
int lnxInputIsJoystickButtonHeld(int joystick, int button);

// Returns number of Axes joystick has
int lnxInputGetAxisCount(int joystick);

// Returns float representing axis value; ranges from -1.0 to 0.0.  Does NOT return a special value for errors (just 0.0).
float lnxInputGetAxis(int joystick, int axis);

// Returns Joystick name, or 0 if error
const char* lnxInputGetJoystickName(int joystick);

//}}}

// Keycode enumeration
//{{{

// NOTE:
// Keys are by position, and ignore the keyboard layout of the operating system.
//  e.x. If the user has a german keyboard with a top row QWERTZ instead of QWERTY,
//  SDL_SCANCODE_Y corresponds to their Z key in the top row, instead of their Y key.
//
// lnxInputGetKeyName WILL however return the name of the key according to layout:
// e.x. If the user has a QWERTZ layout, then lnxInputGetKeyName(LNXKEY_Y) will
//  return "z" instead of "y".
//
// This way, if you want the person to use WASD for movement, and their keyboard layout
//  makes it so the WASD keys aren't where they "should" be, the person can still put
//  their fingers where everyone else does to play the game, and they'll see the names
//  of whatever keys are in that location in their options menu.
//
// Key enumeration is based on usb scancodes (specifically, they're a duplicate
//  of SDL's scancode enum)

#include <SDL2/SDL_scancode.h>

enum
{
LNXKEY_A =                 SDL_SCANCODE_A,
LNXKEY_B =                 SDL_SCANCODE_B,
LNXKEY_C =                 SDL_SCANCODE_C,
LNXKEY_D =                 SDL_SCANCODE_D,
LNXKEY_E =                 SDL_SCANCODE_E,
LNXKEY_F =                 SDL_SCANCODE_F,
LNXKEY_G =                 SDL_SCANCODE_G,
LNXKEY_H =                 SDL_SCANCODE_H,
LNXKEY_I =                 SDL_SCANCODE_I,
LNXKEY_J =                 SDL_SCANCODE_J,
LNXKEY_K =                 SDL_SCANCODE_K,
LNXKEY_L =                 SDL_SCANCODE_L,
LNXKEY_M =                 SDL_SCANCODE_M,
LNXKEY_N =                 SDL_SCANCODE_N,
LNXKEY_O =                 SDL_SCANCODE_O,
LNXKEY_P =                 SDL_SCANCODE_P,
LNXKEY_Q =                 SDL_SCANCODE_Q,
LNXKEY_R =                 SDL_SCANCODE_R,
LNXKEY_S =                 SDL_SCANCODE_S,
LNXKEY_T =                 SDL_SCANCODE_T,
LNXKEY_U =                 SDL_SCANCODE_U,
LNXKEY_V =                 SDL_SCANCODE_V,
LNXKEY_W =                 SDL_SCANCODE_W,
LNXKEY_X =                 SDL_SCANCODE_X,
LNXKEY_Y =                 SDL_SCANCODE_Y,
LNXKEY_Z =                 SDL_SCANCODE_Z,
LNXKEY_1 =                 SDL_SCANCODE_1,
LNXKEY_2 =                 SDL_SCANCODE_2,
LNXKEY_3 =                 SDL_SCANCODE_3,
LNXKEY_4 =                 SDL_SCANCODE_4,
LNXKEY_5 =                 SDL_SCANCODE_5,
LNXKEY_6 =                 SDL_SCANCODE_6,
LNXKEY_7 =                 SDL_SCANCODE_7,
LNXKEY_8 =                 SDL_SCANCODE_8,
LNXKEY_9 =                 SDL_SCANCODE_9,
LNXKEY_0 =                 SDL_SCANCODE_0,
LNXKEY_RETURN =            SDL_SCANCODE_RETURN,
LNXKEY_ESCAPE =            SDL_SCANCODE_ESCAPE,
LNXKEY_BACKSPACE =         SDL_SCANCODE_BACKSPACE,
LNXKEY_TAB =               SDL_SCANCODE_TAB,
LNXKEY_SPACE =             SDL_SCANCODE_SPACE,
LNXKEY_MINUS =             SDL_SCANCODE_MINUS,
LNXKEY_EQUALS =            SDL_SCANCODE_EQUALS,
LNXKEY_LEFTBRACKET =       SDL_SCANCODE_LEFTBRACKET,
LNXKEY_RIGHTBRACKET =      SDL_SCANCODE_RIGHTBRACKET,
LNXKEY_BACKSLASH =         SDL_SCANCODE_BACKSLASH,
LNXKEY_NONUSHASH =         SDL_SCANCODE_NONUSHASH,
LNXKEY_SEMICOLON =         SDL_SCANCODE_SEMICOLON,
LNXKEY_APOSTROPHE =        SDL_SCANCODE_APOSTROPHE,
LNXKEY_GRAVE =             SDL_SCANCODE_GRAVE,
LNXKEY_COMMA =             SDL_SCANCODE_COMMA,
LNXKEY_PERIOD =            SDL_SCANCODE_PERIOD,
LNXKEY_SLASH =             SDL_SCANCODE_SLASH,
LNXKEY_CAPSLOCK =          SDL_SCANCODE_CAPSLOCK,
LNXKEY_F1 =                SDL_SCANCODE_F1,
LNXKEY_F2 =                SDL_SCANCODE_F2,
LNXKEY_F3 =                SDL_SCANCODE_F3,
LNXKEY_F4 =                SDL_SCANCODE_F4,
LNXKEY_F5 =                SDL_SCANCODE_F5,
LNXKEY_F6 =                SDL_SCANCODE_F6,
LNXKEY_F7 =                SDL_SCANCODE_F7,
LNXKEY_F8 =                SDL_SCANCODE_F8,
LNXKEY_F9 =                SDL_SCANCODE_F9,
LNXKEY_F10 =               SDL_SCANCODE_F10,
LNXKEY_F11 =               SDL_SCANCODE_F11,
LNXKEY_F12 =               SDL_SCANCODE_F12,
LNXKEY_PRINTSCREEN =       SDL_SCANCODE_PRINTSCREEN,
LNXKEY_SCROLLLOCK =        SDL_SCANCODE_SCROLLLOCK,
LNXKEY_PAUSE =             SDL_SCANCODE_PAUSE,
LNXKEY_INSERT =            SDL_SCANCODE_INSERT,
LNXKEY_HOME =              SDL_SCANCODE_HOME,
LNXKEY_PAGEUP =            SDL_SCANCODE_PAGEUP,
LNXKEY_DELETE =            SDL_SCANCODE_DELETE,
LNXKEY_END =               SDL_SCANCODE_END,
LNXKEY_PAGEDOWN =          SDL_SCANCODE_PAGEDOWN,
LNXKEY_RIGHT =             SDL_SCANCODE_RIGHT,
LNXKEY_LEFT =              SDL_SCANCODE_LEFT,
LNXKEY_DOWN =              SDL_SCANCODE_DOWN,
LNXKEY_UP =                SDL_SCANCODE_UP,
LNXKEY_NUMLOCKCLEAR =      SDL_SCANCODE_NUMLOCKCLEAR,
LNXKEY_KP_DIVIDE =         SDL_SCANCODE_KP_DIVIDE,
LNXKEY_KP_MULTIPLY =       SDL_SCANCODE_KP_MULTIPLY,
LNXKEY_KP_MINUS =          SDL_SCANCODE_KP_MINUS,
LNXKEY_KP_PLUS =           SDL_SCANCODE_KP_PLUS,
LNXKEY_KP_ENTER =          SDL_SCANCODE_KP_ENTER,
LNXKEY_KP_1 =              SDL_SCANCODE_KP_1,
LNXKEY_KP_2 =              SDL_SCANCODE_KP_2,
LNXKEY_KP_3 =              SDL_SCANCODE_KP_3,
LNXKEY_KP_4 =              SDL_SCANCODE_KP_4,
LNXKEY_KP_5 =              SDL_SCANCODE_KP_5,
LNXKEY_KP_6 =              SDL_SCANCODE_KP_6,
LNXKEY_KP_7 =              SDL_SCANCODE_KP_7,
LNXKEY_KP_8 =              SDL_SCANCODE_KP_8,
LNXKEY_KP_9 =              SDL_SCANCODE_KP_9,
LNXKEY_KP_0 =              SDL_SCANCODE_KP_0,
LNXKEY_KP_PERIOD =         SDL_SCANCODE_KP_PERIOD,
LNXKEY_NONUSBACKSLASH =    SDL_SCANCODE_NONUSBACKSLASH,
LNXKEY_APPLICATION =       SDL_SCANCODE_APPLICATION,
LNXKEY_POWER =             SDL_SCANCODE_POWER,
LNXKEY_KP_EQUALS =         SDL_SCANCODE_KP_EQUALS,
LNXKEY_F13 =               SDL_SCANCODE_F13,
LNXKEY_F14 =               SDL_SCANCODE_F14,
LNXKEY_F15 =               SDL_SCANCODE_F15,
LNXKEY_F16 =               SDL_SCANCODE_F16,
LNXKEY_F17 =               SDL_SCANCODE_F17,
LNXKEY_F18 =               SDL_SCANCODE_F18,
LNXKEY_F19 =               SDL_SCANCODE_F19,
LNXKEY_F20 =               SDL_SCANCODE_F20,
LNXKEY_F21 =               SDL_SCANCODE_F21,
LNXKEY_F22 =               SDL_SCANCODE_F22,
LNXKEY_F23 =               SDL_SCANCODE_F23,
LNXKEY_F24 =               SDL_SCANCODE_F24,
LNXKEY_EXECUTE =           SDL_SCANCODE_EXECUTE,
LNXKEY_HELP =              SDL_SCANCODE_HELP,
LNXKEY_MENU =              SDL_SCANCODE_MENU,
LNXKEY_SELECT =            SDL_SCANCODE_SELECT,
LNXKEY_STOP =              SDL_SCANCODE_STOP,
LNXKEY_AGAIN =             SDL_SCANCODE_AGAIN,
LNXKEY_UNDO =              SDL_SCANCODE_UNDO,
LNXKEY_CUT =               SDL_SCANCODE_CUT,
LNXKEY_COPY =              SDL_SCANCODE_COPY,
LNXKEY_PASTE =             SDL_SCANCODE_PASTE,
LNXKEY_FIND =              SDL_SCANCODE_FIND,
LNXKEY_MUTE =              SDL_SCANCODE_MUTE,
LNXKEY_VOLUMEUP =          SDL_SCANCODE_VOLUMEUP,
LNXKEY_VOLUMEDOWN =        SDL_SCANCODE_VOLUMEDOWN,
//LNXKEY_LOCKINGCAPSLOCK =   SDL_SCANCODE_LOCKINGCAPSLOCK,
//LNXKEY_LOCKINGNUMLOCK =    SDL_SCANCODE_LOCKINGNUMLOCK,
//LNXKEY_LOCKINGSCROLLLOCK = SDL_SCANCODE_LOCKINGSCROLLLOCK,
LNXKEY_KP_COMMA =          SDL_SCANCODE_KP_COMMA,
LNXKEY_KP_EQUALSAS400 =    SDL_SCANCODE_KP_EQUALSAS400,
LNXKEY_INTERNATIONAL1 =    SDL_SCANCODE_INTERNATIONAL1,
LNXKEY_INTERNATIONAL2 =    SDL_SCANCODE_INTERNATIONAL2,
LNXKEY_INTERNATIONAL3 =    SDL_SCANCODE_INTERNATIONAL3,
LNXKEY_INTERNATIONAL4 =    SDL_SCANCODE_INTERNATIONAL4,
LNXKEY_INTERNATIONAL5 =    SDL_SCANCODE_INTERNATIONAL5,
LNXKEY_INTERNATIONAL6 =    SDL_SCANCODE_INTERNATIONAL6,
LNXKEY_INTERNATIONAL7 =    SDL_SCANCODE_INTERNATIONAL7,
LNXKEY_INTERNATIONAL8 =    SDL_SCANCODE_INTERNATIONAL8,
LNXKEY_INTERNATIONAL9 =    SDL_SCANCODE_INTERNATIONAL9,
LNXKEY_LANG1 =             SDL_SCANCODE_LANG1,
LNXKEY_LANG2 =             SDL_SCANCODE_LANG2,
LNXKEY_LANG3 =             SDL_SCANCODE_LANG3,
LNXKEY_LANG4 =             SDL_SCANCODE_LANG4,
LNXKEY_LANG5 =             SDL_SCANCODE_LANG5,
LNXKEY_LANG6 =             SDL_SCANCODE_LANG6,
LNXKEY_LANG7 =             SDL_SCANCODE_LANG7,
LNXKEY_LANG8 =             SDL_SCANCODE_LANG8,
LNXKEY_LANG9 =             SDL_SCANCODE_LANG9,
LNXKEY_ALTERASE =          SDL_SCANCODE_ALTERASE,
LNXKEY_SYSREQ =            SDL_SCANCODE_SYSREQ,
LNXKEY_CANCEL =            SDL_SCANCODE_CANCEL,
LNXKEY_CLEAR =             SDL_SCANCODE_CLEAR,
LNXKEY_PRIOR =             SDL_SCANCODE_PRIOR,
LNXKEY_RETURN2 =           SDL_SCANCODE_RETURN2,
LNXKEY_SEPARATOR =         SDL_SCANCODE_SEPARATOR,
LNXKEY_OUT =               SDL_SCANCODE_OUT,
LNXKEY_OPER =              SDL_SCANCODE_OPER,
LNXKEY_CLEARAGAIN =        SDL_SCANCODE_CLEARAGAIN,
LNXKEY_CRSEL =             SDL_SCANCODE_CRSEL,
LNXKEY_EXSEL =             SDL_SCANCODE_EXSEL,
LNXKEY_KP_00 =             SDL_SCANCODE_KP_00,
LNXKEY_KP_000 =            SDL_SCANCODE_KP_000,
LNXKEY_THOUSANDSSEPARATOR= SDL_SCANCODE_THOUSANDSSEPARATOR,
LNXKEY_DECIMALSEPARATOR =  SDL_SCANCODE_DECIMALSEPARATOR,
LNXKEY_CURRENCYUNIT =      SDL_SCANCODE_CURRENCYUNIT,
LNXKEY_CURRENCYSUBUNIT =   SDL_SCANCODE_CURRENCYSUBUNIT,
LNXKEY_KP_LEFTPAREN =      SDL_SCANCODE_KP_LEFTPAREN,
LNXKEY_KP_RIGHTPAREN =     SDL_SCANCODE_KP_RIGHTPAREN,
LNXKEY_KP_LEFTBRACE =      SDL_SCANCODE_KP_LEFTBRACE,
LNXKEY_KP_RIGHTBRACE =     SDL_SCANCODE_KP_RIGHTBRACE,
LNXKEY_KP_TAB =            SDL_SCANCODE_KP_TAB,
LNXKEY_KP_BACKSPACE =      SDL_SCANCODE_KP_BACKSPACE,
LNXKEY_KP_A =              SDL_SCANCODE_KP_A,
LNXKEY_KP_B =              SDL_SCANCODE_KP_B,
LNXKEY_KP_C =              SDL_SCANCODE_KP_C,
LNXKEY_KP_D =              SDL_SCANCODE_KP_D,
LNXKEY_KP_E =              SDL_SCANCODE_KP_E,
LNXKEY_KP_F =              SDL_SCANCODE_KP_F,
LNXKEY_KP_XOR =            SDL_SCANCODE_KP_XOR,
LNXKEY_KP_POWER =          SDL_SCANCODE_KP_POWER,
LNXKEY_KP_PERCENT =        SDL_SCANCODE_KP_PERCENT,
LNXKEY_KP_LESS =           SDL_SCANCODE_KP_LESS,
LNXKEY_KP_GREATER =        SDL_SCANCODE_KP_GREATER,
LNXKEY_KP_AMPERSAND =      SDL_SCANCODE_KP_AMPERSAND,
LNXKEY_KP_DBLAMPERSAND =   SDL_SCANCODE_KP_DBLAMPERSAND,
LNXKEY_KP_VERTICALBAR =    SDL_SCANCODE_KP_VERTICALBAR,
LNXKEY_KP_DBLVERTICALBAR = SDL_SCANCODE_KP_DBLVERTICALBAR,
LNXKEY_KP_COLON =          SDL_SCANCODE_KP_COLON,
LNXKEY_KP_HASH =           SDL_SCANCODE_KP_HASH,
LNXKEY_KP_SPACE =          SDL_SCANCODE_KP_SPACE,
LNXKEY_KP_AT =             SDL_SCANCODE_KP_AT,
LNXKEY_KP_EXCLAM =         SDL_SCANCODE_KP_EXCLAM,
LNXKEY_KP_MEMSTORE =       SDL_SCANCODE_KP_MEMSTORE,
LNXKEY_KP_MEMRECALL =      SDL_SCANCODE_KP_MEMRECALL,
LNXKEY_KP_MEMCLEAR =       SDL_SCANCODE_KP_MEMCLEAR,
LNXKEY_KP_MEMADD =         SDL_SCANCODE_KP_MEMADD,
LNXKEY_KP_MEMSUBTRACT =    SDL_SCANCODE_KP_MEMSUBTRACT,
LNXKEY_KP_MEMMULTIPLY =    SDL_SCANCODE_KP_MEMMULTIPLY,
LNXKEY_KP_MEMDIVIDE =      SDL_SCANCODE_KP_MEMDIVIDE,
LNXKEY_KP_PLUSMINUS =      SDL_SCANCODE_KP_PLUSMINUS,
LNXKEY_KP_CLEAR =          SDL_SCANCODE_KP_CLEAR,
LNXKEY_KP_CLEARENTRY =     SDL_SCANCODE_KP_CLEARENTRY,
LNXKEY_KP_BINARY =         SDL_SCANCODE_KP_BINARY,
LNXKEY_KP_OCTAL =          SDL_SCANCODE_KP_OCTAL,
LNXKEY_KP_DECIMAL =        SDL_SCANCODE_KP_DECIMAL,
LNXKEY_KP_HEXADECIMAL =    SDL_SCANCODE_KP_HEXADECIMAL,
LNXKEY_LCTRL =             SDL_SCANCODE_LCTRL,
LNXKEY_LSHIFT =            SDL_SCANCODE_LSHIFT,
LNXKEY_LALT =              SDL_SCANCODE_LALT,
LNXKEY_LGUI =              SDL_SCANCODE_LGUI,
LNXKEY_RCTRL =             SDL_SCANCODE_RCTRL,
LNXKEY_RSHIFT =            SDL_SCANCODE_RSHIFT,
LNXKEY_RALT =              SDL_SCANCODE_RALT,
LNXKEY_RGUI =              SDL_SCANCODE_RGUI,
LNXKEY_MODE =              SDL_SCANCODE_MODE,
LNXKEY_AUDIONEXT =         SDL_SCANCODE_AUDIONEXT,
LNXKEY_AUDIOPREV =         SDL_SCANCODE_AUDIOPREV,
LNXKEY_AUDIOSTOP =         SDL_SCANCODE_AUDIOSTOP,
LNXKEY_AUDIOPLAY =         SDL_SCANCODE_AUDIOPLAY,
LNXKEY_AUDIOMUTE =         SDL_SCANCODE_AUDIOMUTE,
LNXKEY_MEDIASELECT =       SDL_SCANCODE_MEDIASELECT,
LNXKEY_WWW =               SDL_SCANCODE_WWW,
LNXKEY_MAIL =              SDL_SCANCODE_MAIL,
LNXKEY_CALCULATOR =        SDL_SCANCODE_CALCULATOR,
LNXKEY_COMPUTER =          SDL_SCANCODE_COMPUTER,
LNXKEY_AC_SEARCH =         SDL_SCANCODE_AC_SEARCH,
LNXKEY_AC_HOME =           SDL_SCANCODE_AC_HOME,
LNXKEY_AC_BACK =           SDL_SCANCODE_AC_BACK,
LNXKEY_AC_FORWARD =        SDL_SCANCODE_AC_FORWARD,
LNXKEY_AC_STOP =           SDL_SCANCODE_AC_STOP,
LNXKEY_AC_REFRESH =        SDL_SCANCODE_AC_REFRESH,
LNXKEY_AC_BOOKMARKS =      SDL_SCANCODE_AC_BOOKMARKS,
LNXKEY_BRIGHTNESSDOWN =    SDL_SCANCODE_BRIGHTNESSDOWN,
LNXKEY_BRIGHTNESSUP =      SDL_SCANCODE_BRIGHTNESSUP,
LNXKEY_DISPLAYSWITCH =     SDL_SCANCODE_DISPLAYSWITCH,
LNXKEY_KBDILLUMTOGGLE =    SDL_SCANCODE_KBDILLUMTOGGLE,
LNXKEY_KBDILLUMDOWN =      SDL_SCANCODE_KBDILLUMDOWN,
LNXKEY_KBDILLUMUP =        SDL_SCANCODE_KBDILLUMUP,
LNXKEY_EJECT =             SDL_SCANCODE_EJECT,
LNXKEY_SLEEP =             SDL_SCANCODE_SLEEP,
LNXKEY_APP1 =              SDL_SCANCODE_APP1,
LNXKEY_APP2 =              SDL_SCANCODE_APP2
};
//}}}

