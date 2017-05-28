
// Function Prototypes
//{{{

int lnxWindowInit();
void lnxWindowDestroy();
void lnxWindowSwapBuffers();

// Calling this is the same hitting Alt-F4 or the X button in the GUI window.
//  Returns 1 if successful, 0 if there was an error
int lnxWindowQuit();

void lnxWindowGetDimensions(int* width, int* height);
int  lnxWindowIsFullscreen();
void lnxWindowEnterFullscreen();
void lnxWindowExitFullscreen();
void lnxWindowSetSize(int width, int height);
void lnxWindowBufferClear(float red, float green, float blue);

//}}}


