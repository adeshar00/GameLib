

#include<stdio.h>
#include<stdlib.h> // for exit TODO delete
#include<math.h> // for trig in camera rotation

#include "mlnx.h"

// VARIABLES
//{{{

int fontwidth = 6;
int fontheight = 8;

lnxProgram p =0;
lnxProgram p2 =0;
lnxProgram p3 =0;
lnxProgram pdrawchar =0;
lnxProgram p5 =0;
lnxModel sm =0;
lnxModel sm2 =0;
lnxModel sm3 =0;
lnxModel smquad =0;
lnxModel sm5 =0;
lnxTexture t1 =0;
lnxTexture t2 =0;
lnxTexture t3 =0;
lnxTexture t4 =0;

lnxSfx sfx1;
lnxSfx sfx2;
lnxSfx sfx3;
//xx

int lastTime;
int timeDebt; // Milliseconds since last time mechanics routine was run
int debtCap;  // Max milliseconds allowed in timedebt

// Ticks per second
#define TPS 32

int col = 0; // TODO temp delete
int FSCOOLDOWN = 0;// TODO fix for fullscreen glitch in SDL, make this fire a full second
// after FS switch
//  maybe fire once every few seconds??

// TODO only works on first call- if having a main loop, put update in there so this works if
//  called mutliple times within the same loop
int lastWidth = 0;
int lastHeight = 0;

float maxViewportRatio = 1.2;
float minViewportRatio = 0.8;
float currentViewportRatio = 1.0; // TODO cut this? can get by dividing viewportwidth by height...
int offsetx; // pixel offset for mouse translation (if 3D, offset x is offset of left viewport)
int offsety;
int viewportwidth; // pixel scale for mouse translation RENAME viewport width/height??
int viewportheight;
int screenwidth; // width of window

//cam vars
float camTheta = 3.14f/6.0f;
float camPhi = -3.14f/6.0f;
float camX = -6.0f;
float camY = -5.0f;
float camZ = 5.0f;

int mode3d = 1;
float cameraTan = 0.5f; // screenheight/screendist/2
//float cameraTan = 0.5f; // screenheight/screendist/2
float hudDist = 1.0f; // distance from HUD to screen
float eyedist = 0.1f; // how far eye is from bridge of nose
//}}}

// FUNCTIONS
//{{{

//  resizecheck
//{{{
int resizeCheck()
{

	int width;
	int height;

	lnxWindowGetDimensions(&width, &height);
	// TODO do fullscreen check, have lastwindowedwidth vars, or can piggy back existing??

	if((width!=lastWidth) || (height!=lastHeight))
	{
		lastWidth = width;
		lastHeight = height;
		return 1;
	}

	return 0;
}
//}}}

// set viewport
//{{{
void setViewport(int width, int height) // width and height of screen, in pixels
{
	// TODO have option so width angle locked instead of height
	//  If doing, use pixelsToGlUnit instead of viewportheight for mouse coord calc

	if(mode3d)
	{
		// have border size between eye screens?
		float ratio = 1.0f*width/height;

		if(ratio > 2.0f*maxViewportRatio) // TODO make these non global
		{
			// Too wide
			currentViewportRatio = maxViewportRatio;
			viewportwidth = maxViewportRatio*height;
			viewportheight = height;
			offsetx = (width-2.0f*viewportwidth)/2;
			offsety = 0;
		}
		else if(ratio < 2.0f*minViewportRatio)
		{
			// Too high
			currentViewportRatio = minViewportRatio;
			viewportwidth = width/2;
			viewportheight = width/2/minViewportRatio;
			offsetx = 0;
			offsety = (height-viewportheight)/2;
		}
		else
		{
			// Just right!
			currentViewportRatio = ratio/2;
			viewportwidth = width/2;
			viewportheight = height;
			offsetx = 0;
			offsety = 0;
		}
		// viewports are set during render
	}
	else
	{
		float ratio = 1.0*width/height;

		if(ratio > maxViewportRatio) // TODO make these non global
		{
			// Too wide
			currentViewportRatio = maxViewportRatio;
			int nwidth = maxViewportRatio*height;
			viewportwidth = nwidth;
			viewportheight = height;
			offsetx = (width-nwidth)/2;
			offsety = 0;
		}
		else if(ratio < minViewportRatio)
		{
			// Too high
			currentViewportRatio = minViewportRatio;
			int nheight = width/minViewportRatio;
			viewportwidth = width;
			viewportheight = width/minViewportRatio;
			offsetx = 0;
			offsety = (height-nheight)/2;
		}
		else
		{
			// Just right!
			currentViewportRatio = ratio;
			viewportwidth = width;
			viewportheight = height;
			offsetx = 0;
			offsety = 0;
		}
		lnxGraphicsSetSViewport(offsetx, offsety, viewportwidth, viewportheight);
	}
}
//}}}

// kill
//{{{
static void kill()
{
	lnxGraphicsDestroyModel(sm);
	lnxGraphicsDestroyProgram(p);
	lnxSoundDestroySfx(sfx1);
	lnxWindowDestroy();
	exit(0);
}
//}}}

// graphics
//{{{
void graphics(float time) // TODO pass time as 0.0<=t<=1.0?
{


	int e;
	int eyes = 1+(mode3d!=0);

	// Draw Things with GoLdEn background
	// TODO eventually for 3D, have this do two draw calls for each draw, instead of
	//  resending every single ogl command!
	for(e=0;e<eyes;e++)
	{
		float ed = eyedist;
		float eyedist; // TODO this is hacky!!! eyedist is also a global
		float tcx = camX;
		float tcy = camY;
		float camX;
		float camY;
		float em; // mode for which eye is draw: mult times some factor for
		  // crosseye stereoscopy or straight on helmet
		if(mode3d) 
		{
			float c = cos(camTheta);
			float s = sin(camTheta);
			em = (1.0f-2.0f*e);
			eyedist = ed*em;
			camX = tcx+s*eyedist;
			camY = tcy-c*eyedist;
			int ox = e*screenwidth/2 + (1-e)*offsetx;
			lnxGraphicsSetSViewport(ox, offsety, viewportwidth, viewportheight); 
			//lnxLogError("%d %d  %d %d\n",offsetx, offsety, viewportwidth, viewportheight);  XXX
		}
		else
		{
			camX = tcx;
			camY = tcy;
			em = 0.0f;
			eyedist = 0.0f;
		}

		lnxGraphicsClear(1.0f*(col+time)/TPS, 1.0f*(col+time)/TPS, 0.0, 0.0);
		float mat[16];
		float tmat[16];
		lnxMatrixProjection(mat, -1.0, -100.0, currentViewportRatio*cameraTan, cameraTan);

		lnxMatrixRotateX(mat, cos(-camPhi), sin(-camPhi));
		lnxMatrixRotateY(mat, cos(-camTheta), sin(-camTheta));
		lnxMatrixRotateY(mat, 0.0f, 1.0f);
		lnxMatrixRotateX(mat, 0.0f, -1.0f);
		lnxMatrixTranslate(mat, -camX, -camY, -camZ);

		/*
		{
			lnxMatrixIdentity(tmat);
			lnxMatrixTranslate(tmat, 0,1,0);
			lnxMatrixMultiply(tmat,mat,tmat);
			void** values = malloc(sizeof(void*)*1);
			values[0] = tmat;
			//lnxGraphics(sm2, p, values);
		}
		{
			lnxMatrixIdentity(tmat);
			lnxMatrixTranslate(tmat, 0,2,2);
			lnxMatrixMultiply(tmat,mat,tmat);
			void** values = malloc(sizeof(void*)*1);
			values[0] = tmat;
			//lnxGraphics(sm2, p, values);
		}
		{
			lnxMatrixIdentity(tmat);
			lnxMatrixTranslate(tmat, 0,1,3);
			//lnxMatrixScale(tmat, 0.3,0.3,0.3);
			lnxMatrixMultiply(tmat,mat,tmat);
			void** values = malloc(sizeof(void*)*7);
			values[0] = tmat;
			float col1[] = {0.0, 0.0, 1.0};
			values[1] = col1;
			float x = camX+col;
			float y = -camY;
			float z = camZ-3.0;
			float dt = sqrt(x*x+y*y+z*z);
			if(dt<=0) dt = 0.00001;
			float light[] = {x/dt,y/dt,z/dt};
			values[2] = light;
			float col2[] = {0.0, 0.2*(col+time)/TPS, 0.7*(col+time)/TPS};
			values[3] = col2;
			float off = 1;
			values[4] = &off;
			values[5] = t1;
			values[6] = t2;
			int d;
			for(d=0;d<2;d++)
			{
				off = 2.0f*d;
				//lnxGraphics(sm2, p2, values);
			}
		}
		*/
		{
			lnxTexture ts[2] = {t1,t2};
			lnxMatrixIdentity(tmat);
			lnxMatrixTranslate(tmat, 0,-2,2);
			lnxMatrixMultiply(tmat,mat,tmat);

			lnxGraphicsSetProgram(p3);
			lnxGraphicsSetUniformTextures(1, 2, ts);
			lnxGraphicsSetUniformMatrix4fv(0, 1, tmat);
			lnxGraphicsSetModel(sm3);
			lnxGraphicsDraw();

			lnxMatrixTranslate(tmat, 0,-2,0);
			lnxGraphicsSetProgram(p3);
			lnxGraphicsSetUniformTextures(1, 2, ts);
			lnxGraphicsSetUniformMatrix4fv(0, 1, tmat);
			lnxGraphicsSetModel(sm2);
			lnxGraphicsDraw();

			lnxGraphicsSetProgram(p);
			lnxGraphicsSetModel(sm2);
			lnxMatrixTranslate(tmat, 0,-2,0);
			lnxGraphicsSetUniformMatrix4fv(0, 1, mat);
			//lnxGraphicsSetUniform1fv(1, 3, cols);
			//lnxGraphicsSetUniformTextures(1, 2, ts);
			lnxGraphicsSetUniformMatrix4fv(0, 1, tmat);
			//lnxGraphicsSetUniformMatrix4fv(0, 1, tmat);
			lnxGraphicsDraw();
			//lnxGraphicsDestroyTexture(t);

			float umat[16];
			/*
			lnxGraphicsSetProgram(pdrawchar);
			lnxGraphicsSetModel(sm4);
			int scale = 4;
			// TODO disable depthtest
			//lnxGraphicsSetUniformTexture(1, t3);
			ts[0] = t3;
			lnxGraphicsSetUniformTextures(1, 1, ts);
			//int chars[] = {7,4,11,11,14,22,14,17,11,3};
			unsigned char chars[] = "Testing testing 1 2 3";
			int strlen = sizeof(chars);
			int charis[strlen];
			{int i;for(i=0;i<strlen;i++) charis[i] = chars[i];}
			lnxGraphicsSetUniform1i(3, strlen);
			lnxGraphicsSetUniform1iv(2, strlen, charis);
			lnxMatrixIdentity(umat);
			lnxMatrixTranslate(umat,-1.0f,-1.0f,-0.9f);
			lnxMatrixScale(umat,scale*fontwidth*strlen*1.0f/viewportwidth,scale*fontheight*1.0f/viewportheight,1.0f);
			lnxGraphicsSetUniformMatrix4fv(0, 1, umat);
			lnxGraphicsDraw();
			*/

			int s = 4;
			int strlen = 22;
			lnxMatrixIdentity(umat);
			lnxMatrixTranslate(umat,-1.0f,-0.8f,0.0f);
			lnxMatrixScale(umat,s*fontwidth*strlen*1.0f/viewportwidth,s*fontheight*1.0f/viewportheight,1.0f);
			lnxGraphicsSetProgram(p5);
			lnxGraphicsSetModel(smquad);
			lnxGraphicsSetUniformMatrix4fv(0, 1, umat);
			lnxGraphicsSetUniformTexture(1,t4);
			lnxGraphicsDraw();

		}


		// draw mouse cursor
		// TODO this doesn't draw the arrow in the right place if fullscreen
		//  with emscripten; figure out why
		lnxMatrixIdentity(mat);
		lnxMatrixScale(mat, 1/currentViewportRatio, 1.0f, 1.0f);

		float mousemat[16];
		float finmat[16];
		if(lnxInputIsMouseActive())
		{
			float mx = 2*((float)lnxInputGetMouseX()-offsetx-(viewportwidth-viewportheight)/2)/viewportheight-1;
			float my = -2*((float)lnxInputGetMouseY()-offsety)/viewportheight+1;
			if(lnxInputGetMouseLock())
			{
				mx+=1.0f+(viewportwidth-viewportheight)/2/viewportheight;
				my-=1.0f;
			}

			// TODO check if is in viewport: if not, showcursor??
			lnxMatrixIdentity(mousemat);
			lnxMatrixTranslate(mousemat, mx, my, 0.0f);
			lnxMatrixTranslate(mousemat, -eyedist/cameraTan/hudDist, 0.0f, 0.0f);
			lnxMatrixRotateZ(mousemat, 0.75f, 0.75f);
			lnxMatrixScale(mousemat, 0.05f, 0.1f, 1.0f);
			lnxMatrixTranslate(mousemat, 0.0f, -1.0f, -0.99f);

			lnxMatrixMultiply(finmat,mat,mousemat);
			{
				//void* values[1];
				//values[0] = finmat;
				//if(eyedist>=0)
				{
					//lnxGraphics(sm, p, values);
				}
			}
		}

		// ARROWS AT EDGE OF SCREEN
		//{{{
		lnxMatrixIdentity(mousemat);
		lnxMatrixTranslate(mousemat, 0.0f, 1.0f, 0.0f);
		lnxMatrixScale(mousemat, 0.05f, 0.1f, 1.0f);
		lnxMatrixTranslate(mousemat, 0.0f, -1.0f, -0.99f);
		lnxMatrixMultiply(finmat,mat,mousemat);
		{
			//void* values[1];
			//values[0] = finmat;
			//lnxGraphics(sm, p, values);
		}

		lnxMatrixIdentity(mousemat);
		lnxMatrixTranslate(mousemat, 0.0f, -1.0f, 0.0f);
		lnxMatrixRotateZ(mousemat, -1.0f, 0.0f);
		lnxMatrixScale(mousemat, 0.05f, 0.1f, 1.0f);
		lnxMatrixTranslate(mousemat, 0.0f, -1.0f, -0.99f);
		lnxMatrixMultiply(finmat,mat,mousemat);
		{
			//void* values[1];
			//values[0] = finmat;
			//lnxGraphics(sm, p, values);
		}

		lnxMatrixIdentity(mousemat);
		lnxMatrixTranslate(mousemat, -1.2f, 0.0f, 0.0f);
		lnxMatrixRotateZ(mousemat, 0.0f, 1.0f);
		lnxMatrixScale(mousemat, 0.05f, 0.1f, 1.0f);
		lnxMatrixTranslate(mousemat, 0.0f, -1.0f, -0.99f);
		lnxMatrixMultiply(finmat,mat,mousemat);
		{
			//void* values[1];
			//values[0] = finmat;
			//lnxGraphics(sm, p, values);
		}

		lnxMatrixIdentity(mousemat);
		lnxMatrixTranslate(mousemat, 0.8f, 0.0f, 0.0f);
		lnxMatrixRotateZ(mousemat, 0.0f, -1.0f);
		lnxMatrixScale(mousemat, 0.05f, 0.1f, 1.0f);
		lnxMatrixTranslate(mousemat, 0.0f, -1.0f, -0.99f);
		lnxMatrixMultiply(finmat,mat,mousemat);
		{
			//void* values[1];
			//values[0] = finmat;
			//lnxGraphics(sm, p, values);
		}
		//}}}

		// draw touches, with different angles
		{
			int tc = lnxInputGetTouchCount();
			int i;
			int uid;
			float x;
			float y;
			float tx;
			float ty;

			for(i=0;i<tc;i++)
			{
				int width;
				int height;
				lnxWindowGetDimensions(&width, &height);

				lnxInputGetTouch(i, &uid, &x, &y);
				//lnxLogError("ID: %d  X: %f  Y: %f\n", uid, x, y);
				tx = 2*(x*width-offsetx-(viewportwidth-viewportheight)/2)/viewportheight-1;
				ty = -2*(y*height-offsety)/viewportheight+1;
				//lnxLogError("ID: %d  X: %f  Y: %f\n", uid, x, y);

				float mousemat[16];
				lnxMatrixIdentity(mousemat);
				lnxMatrixTranslate(mousemat, tx, ty, 0.0f);
				float cos = (uid&1)*((uid&2)-1);
				float sin = (!(uid&1))*((uid&2)-1);
				lnxMatrixRotateZ(mousemat, 0.707, 0.707);
				lnxMatrixRotateZ(mousemat, cos, sin);
				lnxMatrixScale(mousemat, 0.05f, 0.1f, 1.0f);
				lnxMatrixTranslate(mousemat, 0.0f, -3.0f, 0.0f);

				lnxMatrixMultiply(finmat,mat,mousemat);
				{
					//void* values[1];
					//values[0] = finmat;
					//lnxGraphics(sm, p, values);
				}
			}
		}
	}

	lnxWindowSwapBuffers();
}
//}}}

// mechanics
//{{{
void mechanics()
{
	col = (col+1)%TPS;

	if(FSCOOLDOWN>0) FSCOOLDOWN--;

	lnxInputUpdate();

	// Mouse lock test
	if(lnxInputGetKeyDownCount(LNXKEY_J))
	{
		if(lnxInputGetMouseLock())
			lnxInputDisableMouseLock();
		else
			lnxInputEnableMouseLock();

	}
	if(lnxInputGetKeyDownCount(LNXKEY_K))
	{
		lnxInputDisableMouseLock();
	}

	// Move Camera
	{
		float cost = cos(camTheta);
		float sint = sin(camTheta);
		float cosp = cos(camPhi);
		float sinp = sin(camPhi);

		// Move
		//{{{
		float speed = 3.0;
		if(lnxInputIsKeyHeld(LNXKEY_W))
		{
			camX+= speed/TPS * cost*cosp;
			camY+= speed/TPS * sint*cosp;
			camZ+= speed/TPS * sinp;
		}
		if(lnxInputIsKeyHeld(LNXKEY_S))
		{
			camX-= speed/TPS * cost*cosp;
			camY-= speed/TPS * sint*cosp;
			camZ-= speed/TPS * sinp;
		}
		if(lnxInputIsKeyHeld(LNXKEY_D))
		{
			camX+= speed/TPS * sint;
			camY-= speed/TPS * cost;
		}
		if(lnxInputIsKeyHeld(LNXKEY_A))
		{
			camX-= speed/TPS * sint;
			camY+= speed/TPS * cost;
		}
		if(lnxInputIsKeyHeld(LNXKEY_SPACE))
		{
			camZ+= speed/TPS;
		}
		if(lnxInputIsKeyHeld(LNXKEY_C))
		{
			camZ-= speed/TPS;
		}
		//}}}

		if(lnxInputGetRightMouseHeld())
		{
			if(lnxInputGetMouseLock()==0)
			{
				lnxInputEnableMouseLock();
			}
			else
			{
				camTheta-= 1.0f*lnxInputGetMouseX()/viewportwidth;
				camPhi-= 1.0f*lnxInputGetMouseY()/viewportheight;
			}
		}
		else
		{
			if(lnxInputGetMouseLock()==1) lnxInputDisableMouseLock();
		}
	}

	// Quit
	if(lnxInputGetQuit())
	{
		kill();
	}

	// init test
	if(lnxInputGetKeyDownCount(LNXKEY_P))
	{
		lnxInputDestroy();
		lnxInputInit();
	}

	// Toggle fullscreen
	{
		//int altenter = lnxInputIsKeyHeld(LNXKEY_LALT) || lnxInputIsKeyHeld(LNXKEY_RALT);
		int altenter = 1;
		altenter &= lnxInputGetKeyDownCount(LNXKEY_RETURN)>0;

		if(altenter & (FSCOOLDOWN==0))
		{
			FSCOOLDOWN = 5;
			if(lnxWindowIsFullscreen())
			{
				lnxWindowExitFullscreen();
			}
			else
			{
				lnxWindowEnterFullscreen();
			}
		}
	}

	// Hide/show cursor
	if(lnxInputGetKeyDownCount(LNXKEY_U))
	{
		lnxInputHideCursor();
	}
	if(lnxInputGetKeyDownCount(LNXKEY_I))
	{
		lnxInputShowCursor();
	}

	// Change eyedist
	if(lnxInputIsKeyHeld(LNXKEY_M))
	{
		eyedist+= 0.01f;
	}
	if(lnxInputIsKeyHeld(LNXKEY_N))
	{
		eyedist-= 0.01f;
	}

	if(lnxInputGetLeftMouseDownCount())
		lnxLogError("LM! %d\n",lnxInputGetLeftMouseDownCount());

	if(lnxInputGetMiddleMouseHeld())
		lnxLogError("MHELD!!!!!!\n");

	if(lnxInputGetMouseWheelX()!=0)
		lnxLogError("Wheel DX: %d\n",lnxInputGetMouseWheelX());

	if(lnxInputGetMouseWheelY()!=0)
		lnxLogError("Wheel DY: %d\n",lnxInputGetMouseWheelY());

	int resized = resizeCheck();
	// Handle window resize (should this be in graphics??)
	if(resized | FSCOOLDOWN)
	//if(lnxInputResize() | FSCOOLDOWN) XXX
	{
		// TODO Do this on any window event?
		// Set both buffers to black
		// TODO not red in emscripten, see if why (scissor always enabled maybe??)
		lnxWindowBufferClear(1, 0, 0); // TODO make this black

	}
	if(resized)
	//if(lnxInputResize()) XXX
	{
		// Adjust viewport dimensions
		//int width = lnxInputResizeWidth(); XXX
		//int height = lnxInputResizeHeight(); XXX
		int width;
		int height;
		lnxWindowGetDimensions(&width, &height);

		screenwidth = width;
		//lnxLogError("Resize:  w: %d  h: %d\n", width, height); // XXX
		setViewport(width, height);
	}

	// TODO TEST getlastkey with name!!!! have example here for that

	if(lnxInputGetKeyDownCount(LNXKEY_F))
	{
		lnxLogError("FS?: %d\n", lnxWindowIsFullscreen());
	}

	if(lnxInputGetKeyDownCount(LNXKEY_Y))
	{
		lnxLogError("%s\n",lnxInputGetKeyName(LNXKEY_Y));
	}

	if(lnxInputIsKeyHeld(LNXKEY_V))
	{
		lnxWindowBufferClear(0, 0, 1);
	}

	// JOYSTIIIICKS
	if(lnxInputIsKeyHeld(LNXKEY_B))
	{
		// TODO when making controller options, make so can swap controller layouts
		// TODO for control setting, maybe look for changes instead of values: some axes
		//  are -1 by default... maybe some buttons on joysticks work the same way??
		int jc = lnxInputGetJoystickCount();
		int jl;
		for(jl=0;jl<jc;jl++)
		{
			int j = lnxInputGetJoystickIndex(jl);
			printf("J%d:  %s\n",j,lnxInputGetJoystickName(j));
			int ac = lnxInputGetAxisCount(j);
			int bc = lnxInputGetJoystickButtonCount(j);
			int b;
			for(b=0;b<bc;b++)
			{
				printf("B%d:%d,%d ",b,
						lnxInputGetJoystickButtonDownCount(j,b),
						lnxInputIsJoystickButtonHeld(j,b));
			}
			printf("\n");
			int a;
			for(a=0;a<ac;a++)
			{
				printf("A%d:%f ",a,
						lnxInputGetAxis(j,a));
			}
			printf("\n");
		}
		printf("\n");
	}

	// SOOUUUNNNDDDDDDDD DD DD DD D
	if(lnxInputIsKeyHeld(LNXKEY_G))
	{
		static int TEST = 1;
		float vols[2];
		vols[0] = (TEST&1)*0.3;
		vols[1] = (TEST&2)*0.3;
		TEST%= 3;
		TEST++;

		lnxSoundPlaySfx(sfx2, 0.0f, vols);
	}
	if(lnxInputGetKeyDownCount(LNXKEY_H))
	{
		static int TEST = 1;
		float vols[2];
		vols[0] = (TEST&1)*0.3;
		vols[1] = (TEST&2)*0.3;
		TEST%= 3;
		TEST++;

		lnxSoundPlaySfx(sfx1, 0.0f, vols);
		lnxSoundPlaySfx(sfx1, 0.99f/TPS, vols);
	}

}
//}}}

//  mainloop
//{{{

void woo()
{
	int period = 1000/TPS;

	int currTime = lnxLoopGetTime();
	int dt = currTime - lastTime;
	if(sizeof(int)>4) dt&= 0xffffffff;

	timeDebt+= dt;
	if(timeDebt>debtCap) timeDebt = debtCap;

	while(timeDebt>period)
	{
		timeDebt-= period;
		lnxSoundShiftTimeReference(1.0f/TPS);
		mechanics();
	}

	// TODO document that video lag is at least 1/TPS seconds
	graphics(1.0f*timeDebt/period);

	lastTime = currTime;
}
//}}}

// Load staticmodel function TEMP TODO put this in layer1 module (maybe make it's own)
//{{{
lnxModel loadModel(char* filename)
{
	int indisize = 0;
	unsigned int* indices = 0;
	int vertCount = 0;
	float* verts = 0;
	int attributeCount = 1;
	const char* names[] = {"vert"};
	int sizes[] = {3};
	float* pointers[attributeCount];

	unsigned char* ba;	// byte array
	int baLength;	// length of ba

	// Copy file to byte array
	{
		FILE* file;
		file = fopen(filename,"rb");
		if(file==0)
		{
			lnxLogError("Unable to open file \"%s\"\n", filename);
			return 0;
		}
		fseek(file, 0, SEEK_END);
		baLength = ftell(file);
		rewind(file);

		EMALLOC(ba, unsigned char, baLength)
		int r = fread(ba, baLength, 1, file);
		//if(r!=baLength) lnxLogError("READ ERROR\n"); // TODO
		fclose(file);
	}

	// Build staticmodel data (cpuside) based on data in byte array
	//{{{
	{

		// Bytes in file header
		const int FILEHEADLEN = 2;
		// Bytes in header of each object in model
		const int OBHEADLEN = 22;
		// Bytes per vertex
		const int BPVERT = 6;
		// Bytes per tri
		const int BPTRI = 6;

		int obCount;
		obCount = 256*ba[1] + ba[0];
		if(obCount) obCount = 1; // "staticmodels" are only single objects atm FLAG
		int pi = FILEHEADLEN;  // Parse Index

		int ob;
		for(ob=0; ob<obCount; ob++)
		{
			vertCount = 256*ba[pi+1] + ba[pi+0];
			int triCount = 256*ba[pi+3] + ba[pi+2];
			// load trans/rot/scale data

			if(pi+OBHEADLEN+vertCount*BPVERT+triCount*BPTRI > baLength)
			{
				printf("deerp"); // FLAG
				return 0; // FLAG populate an empty staticmodel and give "invalid file" error
			}

			// Generate Verts
			{

				verts = (float*)malloc(sizeof(float)*vertCount*3); // TODO EMALLOC, do below too
				float* vd = verts; // vert data pointer
				int v;
				int vstart = pi+OBHEADLEN;
				for(v=0; v<vertCount; v++)
				{
					int bi = vstart + v*BPVERT; // byte array index
					// X component
					vd[v*3+0] = (float)ba[bi+1] - 128.0f + ((float)ba[bi+0]/256.0f);
					// Y component
					vd[v*3+1] = (float)ba[bi+3] - 128.0f + ((float)ba[bi+2]/256.0f);
					// z component
					vd[v*3+2] = (float)ba[bi+5] - 128.0f + ((float)ba[bi+4]/256.0f);
				}
			}

			// Generate Tris
			{
				indisize = triCount*3;
				indices = (unsigned int*)malloc(sizeof(unsigned int)*triCount*3);
				unsigned int* td = indices; // tri data pointer
				int t;
				int tstart = pi+OBHEADLEN+vertCount*BPVERT;
				for(t=0; t<triCount; t++)
				{
					int bi = tstart + t*BPTRI;
					// Index 1
					td[3*t+0] = 256*ba[bi+1] + ba[bi+0];
					// Index 2
					td[3*t+1] = 256*ba[bi+3] + ba[bi+2];
					// Index 3
					td[3*t+2] = 256*ba[bi+5] + ba[bi+4];
				}
			}

			// Update parse index
			pi += OBHEADLEN+vertCount*BPVERT+triCount*BPTRI;

		}
		
		pointers[0] = verts;
		free(ba);

	}
	//}}}

	{
		int i;
		for(i=0;i<vertCount;i++)
		{
			pointers[0][3*i+2]-= 1.0f;
		}
	}

	lnxModel m = lnxGraphicsCreateModel(
			indisize,
			indices,
			vertCount,
			attributeCount,
			names,
			sizes,
			pointers);

	free(verts);
	free(indices);

	return m;


}
//}}}

// file to bytes
//{{{
// TODO put in utils?
void lnxTODOFileToBytes(char filename[], unsigned char** rbytes, int* rlength)
{
	unsigned char* bytes;
	int length;

	// Open File
	FILE* file;
	file = fopen(filename,"rb");
	if(file==0)
	{
		lnxLogError("Unable to open file \"%s\"\n", filename);
		if(rbytes) *rbytes = 0;
		if(rlength) *rlength = 0;
		return;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	rewind(file);

	// Read file to byte array
	EMALLOC(bytes, unsigned char, length)
	int tlen = fread(bytes, 1, length, file);
	fclose(file);
	if(tlen!=length)
	{
		lnxLogError("Error reading file \"%s\"\n", filename);
		if(rbytes) *rbytes = 0;
		if(rlength) *rlength = 0;
		return;
	}

	// Return results
	if(rbytes) *rbytes = bytes;
	if(rlength) *rlength = length;

}
//}}}

//  main
//{{{

int main(int argc, char* argv[])
{
	if(argc>0) printf("%s\n",argv[0]);

	if(lnxWindowInit())	return 1;
	lnxGraphicsEnableDepthTest();
	lnxInputInit(); // check input

	// Create programs
	//{{{
	{
		const char* vs[1];
		vs[0] = "#version 100\n\
			precision highp float;\n\
			\n\
			attribute vec2 coords;\n\
			uniform mat4 testmatrix;\n\
			varying vec2 pos;\n\
			\n\
			void main()\n\
			{\n\
				pos = coords;\n\
				gl_Position = testmatrix*vec4(coords, 0.0, 1.0);\n\
			}";
		const char* fs[1];
		fs[0] = "#version 100\n\
			precision highp float;\n\
			uniform sampler2D texture;\n\
			varying vec2 pos;\n\
			\n\
			void main()\n\
			{\n\
				gl_FragColor = texture2D(texture,pos);\n\
			}";
		int size = 2;
		const char* ulist[] = {"testmatrix","texture"};
		p5 =  lnxGraphicsCreateProgram(vs,fs,size,ulist);

	}
	{
		const char* vs[1];
		vs[0] = "#version 100\n\
			precision highp float;\n\
			\n\
			attribute vec2 coords;\n\
			uniform int index;\n\
			uniform int strlen;\n\
			uniform int asciival;\n\
			varying vec2 tcoords;\n\
			\n\
			void main()\n\
			{\n\
				tcoords = vec2(1.0/128.0*(coords.x+float(asciival)), coords.y);\n\
				gl_Position = vec4(2.0/float(strlen)*(coords.x+float(index))-1.0, 2.0*coords.y-1.0, 0.0, 1.0);\n\
			}";

		const char* fs[1];
		fs[0] = "#version 100\n\
			precision highp float;\n\
			uniform sampler2D texture;\n\
			varying vec2 tcoords;\n\
			\n\
			void main()\n\
			{\n\
				gl_FragColor = texture2D(texture,tcoords);\n\
			}";
		int size = 4;
		const char* ulist[] = {"asciival","index","strlen","texture"};
		pdrawchar =  lnxGraphicsCreateProgram(vs,fs,size,ulist);

	}
	{
		const char* vs[1];
		vs[0] = "#version 100\n\
			precision highp float;\n\
			\n\
			attribute vec3 vert;\n\
			uniform mat4 testmatrix;\n\
			varying vec3 pos;\n\
			\n\
			void main()\n\
			{\n\
				pos = vert;\n\
				gl_Position = testmatrix*vec4(vert, 1.0);\n\
			}";
		const char* fs[1];
		fs[0] = "#version 100\n\
			precision highp float;\n\
			uniform int cols[3];\n\
			uniform sampler2D texture[2];\n\
			varying vec3 pos;\n\
			\n\
			void main()\n\
			{\n\
				vec2 tcoords = pos.xy*30.0;\n\
				gl_FragColor = mix(texture2D(texture[1],tcoords),texture2D(texture[0],tcoords),(pos.z+1.0)/2.0);\n\
			}";
		//const char ulist[] = "mat4 testmatrix   int cols 3"; 
		// TODO parse whitespace seperated list of names?
		int size = 2;
		const char* ulist[] = {"testmatrix","texture"};
		p3 =  lnxGraphicsCreateProgram(vs,fs,size,ulist);
		//exit(1);///TODO

	}
	{
		const char* vs[1];
		vs[0] = "#version 100\n\
			precision highp float;\n\
			\n\
			attribute vec3 color;\n\
			attribute vec3 vert;\n\
			uniform mat4 testmatrix;\n\
			varying vec3 c;\n\
			\n\
			void main()\n\
			{\n\
				c = color;\n\
				gl_Position = testmatrix*vec4(vert, 1.0);\n\
			}";
		const char* fs[1];
		fs[0] = "#version 100\n\
			precision highp float;\n\
			varying vec3 c;\n\
			\n\
			void main()\n\
			{\n\
				gl_FragColor = vec4(c, 1.0);\n\
			}";
		int size = 1;
		const char* ulist[] = {"testmatrix"};
		p =  lnxGraphicsCreateProgram(vs,fs,size,ulist);

	}
	{
		const char* vs[1];
		vs[0] = "#version 100\n\
			precision highp float;\n\
			\n\
			attribute vec3 vert;\n\
			uniform mat4 testmatrix;\n\
			varying float z;\n\
			\n\
			void main()\n\
			{\n\
				z = vert.z;\n\
				gl_Position = testmatrix*vec4(vert, 1.0);\n\
			}";
		const char* fs[1];
		fs[0] = "#version 100\n\
			precision highp float;\n\
			varying float z;\n\
			\n\
			void main()\n\
			{\n\
				gl_FragColor = vec4(0.0, mix(0.0,1.0,(z+1.0)/2.0), 0.0, 1.0);\n\
			}";
		int size = 1;
		const char* ulist[] = {"testmatrix"};
		p2 =  lnxGraphicsCreateProgram(vs,fs,size,ulist);

	}
	//}}}

	// Create models
	//{{{
	{
		// TODO make model and prog that take attrib with name longer than 32 chars

		sm3 = loadModel("data/unit.csmf");

		// Quad (-1 -> 1)
		{
			int vertCount = 4;
			int attributeCount = 1;
			const char* names[] = {"vert"};
			int sizes[] = {3};
			float* pointers[attributeCount];

			float verts[] = {-1.0,-1.0, 0.0,
			                  1.0,-1.0, 0.0,
			                  1.0, 1.0, 0.0,
			                 -1.0, 1.0, 0.0};
			pointers[0] = verts;

			int indisize = 3*2;
			unsigned int indices[] = {0,1,2, 0,2,3};

			sm5 = lnxGraphicsCreateModel(
					indisize,
					indices,
					vertCount,
					attributeCount,
					names,
					sizes,
					pointers);
		}
		// Quad
		{
			int vertCount = 4;
			int attributeCount = 1;
			const char* names[] = {"coords"};
			int sizes[] = {2};
			float* pointers[attributeCount];

			float verts[] = { 0.0, 0.0,
			                  1.0, 0.0,
			                  1.0, 1.0,
			                  0.0, 1.0};
			pointers[0] = verts;

			int indisize = 3*2;
			unsigned int indices[] = {0,1,2, 0,2,3};

			smquad = lnxGraphicsCreateModel(
					indisize,
					indices,
					vertCount,
					attributeCount,
					names,
					sizes,
					pointers);
		}

		// Tri
		{
			int vertCount = 3;
			int attributeCount = 2;
			const char* names[] = {"vert","color"};
			int sizes[] = {3,3};
			float* pointers[attributeCount];

			float verts[] = { 1.0,-1.0, 0.0,
			                  0.0, 1.0, 0.0,
			                 -1.0,-1.0, 0.0};
			pointers[0] = verts;
			float colors[] = {  0.0,  1.0,  0.5,
	                            1.0,  0.0,  0.0,
	                            0.0,  0.5,  1.0};
			pointers[1] = colors;

			int indisize = 3;
			unsigned int indices[] = {0,1,2};

			sm = lnxGraphicsCreateModel(
					indisize,
					indices,
					vertCount,
					attributeCount,
					names,
					sizes,
					pointers);
		}

		// Octo
		{
			int vertCount = 6;
			int attributeCount = 2;
			const char* names[] = {"vert","color"};
			int sizes[] = {3,3};
			float* pointers[attributeCount];

			float verts[] = {  0.0,  0.0,  1.0,
							   1.0,  0.0,  0.0,
							   0.0,  1.0,  0.0,
							  -1.0,  0.0,  0.0,
							   0.0, -1.0,  0.0,
							   0.0,  0.0, -1.0};
			pointers[0] = verts;
			float colors[] = {  0.0,  0.0,  1.0,
								1.0,  0.0,  0.0,
								0.0,  1.0,  0.0,
								1.0,  1.0,  0.0,
								0.0,  1.0,  1.0,
								1.0,  0.0,  1.0};
			pointers[1] = colors;

			int indisize = 3*8;
			unsigned int indices[] = { 0,1,2,
									   0,2,3,
									   0,3,4,
									   0,4,1,
									   5,2,1,
									   5,3,2,
									   5,4,3,
									   5,1,4};

			sm2 = lnxGraphicsCreateModel(
					indisize,
					indices,
					vertCount,
					attributeCount,
					names,
					sizes,
					pointers);
		}

	}
	//}}}

	// Create texture
	//{{{
	{
		unsigned char data[16];

			data[0] = 255; data[1] =   0; data[2] =   0; data[3] = 255;
			data[4] =   0; data[5] = 255; data[6] =   0; data[7] = 200;
			data[8] =   0; data[9] =   0; data[10]= 255; data[11]= 255;
			data[12]=   0; data[13]=   0; data[14]=   0; data[15]=  55;
		t1 = lnxGraphicsCreateTexture(2,2,data,1);

			data[0] =   0; data[1] = 255; data[2] = 255; data[3] = 255;
			data[4] = 255; data[5] =   0; data[6] = 255; data[7] = 200;
			data[8] = 255; data[9] = 255; data[10]=   0; data[11]= 255;
			data[12]= 255; data[13]= 255; data[14]= 255; data[15]= 200;
		t2 = lnxGraphicsCreateTexture(2,2,data,1);

		{
			int width = fontwidth*26;
			int height = fontheight;
			unsigned char t[] = 
			// A           B             C             D             E             F             G             H             I             J             K             L             M             N             O             P             Q             R             S             T             U             V             W             X             Y             Z           
			{0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0,
                                                                                                                                                                                                                                                                                                                                                                                       
			 0,0,1,0,0, 0, 1,1,1,1,0, 0, 0,1,1,1,0, 0, 1,1,1,1,0, 0, 1,1,1,1,1, 0, 1,1,1,1,1, 0, 0,1,1,1,0, 0, 1,0,0,0,1, 0, 1,1,1,1,1, 0, 0,1,1,1,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 0,1,1,1,0, 0, 1,1,1,1,0, 0, 0,1,1,1,0, 0, 1,1,1,1,0, 0, 0,1,1,1,1, 0, 1,1,1,1,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,1,1,1,1, 0,
			 0,1,1,1,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 0,0,1,0,0, 0, 0,0,0,1,0, 0, 1,0,0,1,0, 0, 1,0,0,0,0, 0, 1,1,0,1,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 0,0,1,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 0,0,0,0,1, 0,
			 0,1,0,1,0, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,0, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 0,0,1,0,0, 0, 0,0,0,1,0, 0, 1,0,1,0,0, 0, 1,0,0,0,0, 0, 1,0,1,0,1, 0, 1,1,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 0,0,1,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 0,1,0,1,0, 0, 0,1,0,1,0, 0, 0,0,0,1,0, 0,
			 1,0,0,0,1, 0, 1,1,1,1,0, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,1,1,1,0, 0, 1,1,1,1,0, 0, 1,0,0,0,0, 0, 1,1,1,1,1, 0, 0,0,1,0,0, 0, 0,0,0,1,0, 0, 1,1,0,0,0, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,1,0,1, 0, 1,0,0,0,1, 0, 1,1,1,1,0, 0, 1,0,0,0,1, 0, 1,1,1,1,0, 0, 0,1,1,1,0, 0, 0,0,1,0,0, 0, 1,0,0,0,1, 0, 0,1,0,1,0, 0, 1,0,1,0,1, 0, 0,0,1,0,0, 0, 0,0,1,0,0, 0, 0,0,1,0,0, 0,
			 1,1,1,1,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,0, 0, 1,0,1,1,1, 0, 1,0,0,0,1, 0, 0,0,1,0,0, 0, 0,0,0,1,0, 0, 1,0,1,0,0, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,0,1,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,1,0,1, 0, 1,0,1,0,0, 0, 0,0,0,0,1, 0, 0,0,1,0,0, 0, 1,0,0,0,1, 0, 0,1,0,1,0, 0, 1,0,1,0,1, 0, 0,1,0,1,0, 0, 0,0,1,0,0, 0, 0,1,0,0,0, 0,
			 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 0,0,1,0,0, 0, 1,0,0,1,0, 0, 1,0,0,1,0, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,1,0, 0, 1,0,0,1,0, 0, 0,0,0,0,1, 0, 0,0,1,0,0, 0, 1,0,0,0,1, 0, 0,0,1,0,0, 0, 1,0,1,0,1, 0, 1,0,0,0,1, 0, 0,0,1,0,0, 0, 1,1,0,0,0, 0,
			 1,0,0,0,1, 0, 1,1,1,1,0, 0, 0,1,1,1,0, 0, 1,1,1,1,0, 0, 1,1,1,1,1, 0, 1,0,0,0,0, 0, 0,1,1,1,0, 0, 1,0,0,0,1, 0, 1,1,1,1,1, 0, 0,1,1,0,0, 0, 1,0,0,0,1, 0, 1,1,1,1,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 0,1,1,1,0, 0, 1,0,0,0,0, 0, 0,1,1,0,1, 0, 1,0,0,0,1, 0, 1,1,1,1,0, 0, 0,0,1,0,0, 0, 0,1,1,1,0, 0, 0,0,1,0,0, 0, 0,1,0,1,0, 0, 1,0,0,0,1, 0, 0,0,1,0,0, 0, 1,1,1,1,1, 0,
                                                                                                                                                                                                                                                                                                                                                                                       
			 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0};
			int nw = fontwidth*10;
			int nh = fontheight;
			unsigned char n[] = 
			// 0           1             2             3             4             5             6             7             8             9                  
			{0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0, 0,0,0,0,0, 0,
                                                                                                                                                       
			 0,1,1,1,0, 0, 0,0,1,0,0, 0, 0,1,1,1,0, 0, 0,1,1,1,0, 0, 1,0,0,0,1, 0, 1,1,1,1,1, 0, 0,1,1,1,0, 0, 1,1,1,1,1, 0, 0,1,1,1,0, 0, 0,1,1,1,0, 0,
			 1,0,0,0,1, 0, 0,1,1,0,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,0, 0, 0,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0,
			 1,0,0,0,1, 0, 1,0,1,0,0, 0, 0,0,0,0,1, 0, 0,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,0, 0, 0,0,0,1,0, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0,
			 1,0,1,0,1, 0, 0,0,1,0,0, 0, 0,0,0,1,0, 0, 0,0,1,1,0, 0, 1,1,1,1,1, 0, 1,1,1,1,0, 0, 1,1,1,1,0, 0, 0,0,1,0,0, 0, 0,1,1,1,0, 0, 0,1,1,1,1, 0,
			 1,0,0,0,1, 0, 0,0,1,0,0, 0, 0,0,1,0,0, 0, 0,0,0,0,1, 0, 0,0,0,0,1, 0, 0,0,0,0,1, 0, 1,0,0,0,1, 0, 0,1,0,0,0, 0, 1,0,0,0,1, 0, 0,0,0,0,1, 0,
			 1,0,0,0,1, 0, 0,0,1,0,0, 0, 0,1,0,0,0, 0, 1,0,0,0,1, 0, 0,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,1, 0, 1,0,0,0,0, 0, 1,0,0,0,1, 0, 0,0,0,0,1, 0,
			 0,1,1,1,0, 0, 1,1,1,1,1, 0, 1,1,1,1,1, 0, 0,1,1,1,0, 0, 0,0,0,0,1, 0, 0,1,1,1,0, 0, 0,1,1,1,0, 0, 1,0,0,0,0, 0, 0,1,1,1,0, 0, 0,1,1,1,0, 0};
			// TODO make similar to above but with numbers, then make data large enough to hold all renderable ascii, and map numbers and letters to data (leaving the rest as zero)
			unsigned char d[128*fontwidth*fontheight*4];
			{int i;for(i=0;i<128*fontwidth*fontheight;i++){d[4*i+0] = 0;d[4*i+1] = 0;d[4*i+2] = 200;d[4*i+3] = 100;}}
			int x,y;
			// Caps chars
			for(y=0;y<height;y++)
			{
				for(x=0;x<width;x++)
				{
					int ti = y*width+x;
					int di = 4*((height-y-1)*128*fontwidth+x+(fontwidth*(65)));
					d[di+0] = 255*t[ti];
					d[di+1] = 255*t[ti];
					d[di+2] = 255*t[ti];
					d[di+3] = 100+155*t[ti];
				}
			}
			// lower chars
			for(y=0;y<height;y++)
			{
				for(x=0;x<width;x++)
				{
					int ti = y*width+x;
					int di = 4*((height-y-1)*128*fontwidth+x+(fontwidth*(97)));
					d[di+0] = 255*t[ti];
					d[di+1] = 255*t[ti];
					d[di+2] = 255*t[ti];
					d[di+3] = 150+105*t[ti];
				}
			}
			// numbers
			for(y=0;y<nh;y++)
			{
				for(x=0;x<nw;x++)
				{
					int ni = y*nw+x;
					int di = 4*((nh-y-1)*128*fontwidth+x+(fontwidth*(48)));
					d[di+0] = 255*n[ni];
					d[di+1] = 255*n[ni];
					d[di+2] = 255*n[ni];
					d[di+3] = 150+105*n[ni];
				}
			}
			t3 = lnxGraphicsCreateTexture(128*fontwidth, fontheight, d, 0);
		}

	}
	//}}}

	lnxWindowSetSize(600, 400);

	lastTime = lnxLoopGetTime();
	timeDebt = 0;
	debtCap = 1000;

	lnxInputHideCursor();

	// Test graphics init
	//{{{
	//if(0)
	{
		unsigned char chars[] = "Testing testing 1 2 3";
		int strlen = sizeof(chars);
		int charis[strlen];
		{int i;for(i=0;i<strlen;i++) charis[i] = chars[i];}

		t4 = lnxGraphicsCreateTexture(fontwidth*strlen, fontheight, 0, 0);
		if(lnxGraphicsSetFramebufferTexture(t4,1)!=1)
				lnxLogError("derp\n");
		lnxGraphicsSetSViewport(0,0,fontwidth*strlen, fontheight);

		/*
		lnxGraphicsSetProgram(pdrawchar);
		lnxGraphicsSetModel(sm4);
		lnxGraphicsSetUniformTexture(1,t3);
		float umat[16];
		lnxGraphicsSetUniform1i(3, strlen);
		lnxTexture t = lnxGraphicsCreateTexture(strlen,1,data);
		//lnxGraphicsSetUniform1iv(2, strlen, charis);
		lnxGraphicsSetUniformTexture(2, t);
		lnxMatrixIdentity(umat);
		lnxMatrixTranslate(umat,-1.0f,-1.0f, 0.0f);
		lnxMatrixScale(umat, 2.0f,2.0,1.0);
		//lnxMatrixScale(umat,scale*fontwidth*strlen*1.0f/viewportwidth,scale*fontheight*1.0f/viewportheight,1.0f);
		lnxGraphicsSetUniformMatrix4fv(0, 1, umat);
		lnxGraphicsDraw();
		*/

		//lnxGraphicsDisableDepthTest();
		lnxGraphicsClear(1,0,0,0.5);

		lnxGraphicsSetProgram(0);
		float umat[16];
		lnxGraphicsSetProgram(pdrawchar);
		lnxGraphicsSetModel(smquad);
		lnxGraphicsSetUniformTexture(3,t3);
		int i;
		for(i=0;i<strlen;i++)
		{
			lnxGraphicsSetUniform1i(0,charis[i]);
			lnxGraphicsSetUniform1i(1,i);
			lnxGraphicsSetUniform1i(2,strlen);
			lnxGraphicsDraw();
		}

		lnxMatrixIdentity(umat);
		lnxMatrixProjection(umat, -1.0, -100.0, 1.0,0.5);
		lnxGraphicsSetProgram(p);
		lnxGraphicsSetModel(sm2);
		lnxMatrixTranslate(umat, 0,0,-3);
		lnxGraphicsSetUniformMatrix4fv(0, 1, umat);
		lnxGraphicsDraw();
		lnxMatrixTranslate(umat, -1.0,0,-1);
		lnxGraphicsSetUniformMatrix4fv(0, 1, umat);
		lnxGraphicsDraw();
		lnxGraphicsEnableDepthTest();
		lnxGraphicsSetProgram(0);

		lnxGraphicsSetFramebufferTexture(0,0);

	}
	//}}}

	// Sound
	//{{{
	{

		// Init
		int sampps;
		lnxSoundInit(48000, &sampps);

		// TODO make a fileToBytes function in utils??
		{
			unsigned char* vaf;
			int streamsize;
			float* stream;

			lnxTODOFileToBytes("data/tick.vecaud", &vaf, 0);
			vaf_vafToStream(vaf, sampps, &streamsize, &stream);
			sfx1 = lnxSoundCreateSfx(stream, streamsize);
			free(vaf);
			free(stream);

			lnxTODOFileToBytes("data/test.vecaud", &vaf, 0);
			vaf_vafToStream(vaf, sampps, &streamsize, &stream);
			sfx2 = lnxSoundCreateSfx(stream, streamsize);
			free(vaf);
			free(stream);
		}

		//xx

	}
	//}}}

	lnxLoopStart(woo);

	return 0;
}
//}}}

//}}}


