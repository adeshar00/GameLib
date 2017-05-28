
// TODO scratch Re var code... rethink how to handle mech lag/quickness...

// for bible? or here? put more concise "how-to" notes into sound.h (though explain refshift)
//{{{
/*

   Since SDL is a state machine, sound.c acts similarly (maintaining some state via
    global variables)

   split init into init and openaudio or startaudio (similarly deinit and closeaudio).
   call init denit and app start/close, call start/stopaudio when changing sound 
    mode (mono -> stereo and vice versa)

   Sound effects being played are tracked via the array lvarSoundQueue.
   Each node in the array tracks the sound being played, the timing of the sound, and the volume.
   Timing tracked with sampleoffset from end of sound and latest sample consumed by callback.
   Sounds are added via the addSound function, and removed in the callback function when
    they're finished playing.
   Scratch this explanation? It kind of points out the obvious??

   in upper level module, make it so channel volumes are determined by 3D vector?

   lvarTimeZero
   TODO describe time as a line, how callback is like a shark in negative region, moving
    forward and chomping on strings of sound: every so often, he'll take a chomp, and move forward a bit.  The "PlaySound" function places a new string into the water for the shark to chomp at.  "lvarTimeZero" tracks the distance between the sharks mouth and time zero.  The "ShiftTimeReference" function effective shifts the camera to the right, so that from our frame of reference the position of the sharks and strings have all shifted the the left (since string times are stored as the distance between the sharks mouth and the end of the string, by altering lvarTimeZero, the shark and therefore the strings are all shifted)
	Distance is softly clamped between 0.5*lvarTimeLag and 1.5*lvarTimeLag.

*/
//}}}

// Includes
//{{{

#include <SDL2/SDL.h>

#include "utils.h"

//}}}

// Definitions
//{{{

#define MAXCHANNELS (6)

#define QSIZE (128)

//}}}

// Structs
//{{{

struct lnxSfxRaw
{
	float* stream;
	int sampcount;
};
typedef struct lnxSfxRaw* lnxSfx;

struct qlnode
{
	lnxSfx sound;            // Sound effect to be played
	int endtime;             // How many samples left until the sound is finiished
	float vols[MAXCHANNELS]; // Volume of each channels
};

//}}}

// Variables
//{{{

static int lvarDeviceId = 0;
static int lvarSampsPerSecond = 0;
static int lvarChannels = 0;

static struct qlnode lvarSoundQueue[QSIZE]; // TODO rename? not a queue really...
static int lvarSoundQueueSize = 0;

// TODO note how used for adding new sounds
static int lvarTimeZero = 0;
//static int lvarTimeLag = 0; // TODO delete?

// TODO for any lvar referenced in callback, make sure to call SDL_Lock audio before referencing
//  it in any other function!!!!

static int* ReArray = 0; // rename ring??
static int ReArraySize = 0;
static int ReArraySpot = 0;
static int ReArrayTotal = 0;
static int ReTarget = 0;
//xx

//}}}


// Internal Functions
//{{{

 // Callback
//{{{
static void callback(void* userdata, Uint8* stream, int len)
{
	int sampcount = len/4/lvarChannels;
	float samps[sampcount*lvarChannels];

	// Initialize samples
	{
		int s;
		for(s=0;s<sampcount;s++)
		{
			int c;
			for(c=0;c<lvarChannels;c++)
			{
				samps[lvarChannels*s+c] = 0.0f;
			}
		}
	}


	// Populate samples
	{
		int q;
		for(q=0;q<lvarSoundQueueSize;q++)
		{
			lnxSfx sound;  // Sound being copied
			int endtime;   // End time of sound
			float* vols;   // Channel volumes
			int start;     // Point in stream where sound copying should start
			int end;       // Point in stream where sound copying should end
			int os;        // Sample offset (where callback is in relation to sound start)

			sound   = lvarSoundQueue[q].sound;
			vols    = lvarSoundQueue[q].vols;
			endtime = lvarSoundQueue[q].endtime;

			os      = sound->sampcount - endtime;
			if(os<0)   start = -os;
			else       start = 0;
			if(endtime<sampcount)  end = endtime;
			else                   end = sampcount;

			// Copy Samples
			int s;
			for(s=start;s<end;s++)
			{
				float val = sound->stream[os+s];

				int c;
				for(c=0;c<lvarChannels;c++)
				{
					samps[lvarChannels*s+c]+= val*(vols[c]);
				}
			}

			// Update endtime
			lvarSoundQueue[q].endtime-= sampcount;
		}
	}

	// Copy samples to buffer
	{
		int s;
		for(s=0;s<sampcount;s++)
		{
			int c;
			for(c=0;c<lvarChannels;c++)
			{
				int si = lvarChannels*s+c;               // samps index
				float* sp = (float*)(&(stream[4*(si)])); // stream pointer
				*sp = samps[si];
			}
		}
	}

	// Destroy unused sounds in queue
	{
		int q;
		for(q=lvarSoundQueueSize-1;q>=0;q--)
		{
			if(lvarSoundQueue[q].endtime<=0)
			{
				lvarSoundQueue[q] = lvarSoundQueue[lvarSoundQueueSize-1];
				lvarSoundQueueSize--;
			}
		}
	}

	// Adjust Time Zero
	lvarTimeZero-= sampcount;
	//xx
	/*
	if(0) // TODO XXX this should be in shiftref (and zero check in play)
	{
		float spring = 0.1f; // TODO set this as 1/TPS or something? set via public function?
		float windowstart = 0.0f;
		int rz = lvarTimeZero - sampcount;

		if(rz<lvarTimeLag*windowstart)
		{
			float uf = windowstart*lvarTimeLag - rz;
			rz+= uf*spring;
		}

		if(rz>lvarTimeLag*(windowstart+1.0f))
		{
			float uf = rz - (windowstart+1.0f)*lvarTimeLag;
			rz-= uf*spring;
		}

		lvarTimeZero = rz;
		// TODO track when shaving happens, and then take the average and subtract
		// it here (or when shifttime called)?  Maybe keep an average here, and have the
		// average reset when shift is called (which keeps it's own average?)

	}
	*/

}
//}}}

//}}}

// External Functions
//{{{


 // Play Sfx
//{{{
void lnxSoundPlaySfx(lnxSfx sound, float starttime, float vols[MAXCHANNELS])
{
	// sound:      Sound effect to be played
	// starttime:  Offset from time zero, in seconds, when sound should start playing
	// vols:       Volume for each channel


	SDL_LockAudioDevice(lvarDeviceId);

	// Check that queue isn't full
	if(lvarSoundQueueSize>=QSIZE)
	{
		SDL_UnlockAudioDevice(lvarDeviceId);
		return;
	}

	// Add sound to queue with argument data
	struct qlnode* n = &(lvarSoundQueue[lvarSoundQueueSize]);
	lvarSoundQueueSize++;
	n->sound = sound;
	int start = lvarTimeZero + (int)(starttime*lvarSampsPerSecond);
	if(start<0)
	{
		start = 0;
		// TODO put some sort of marker here
	}
	n->endtime = start + sound->sampcount;
	int c;
	for(c=0;c<lvarChannels;c++)
	{
		n->vols[c] = vols[c];
	}

	// Return
	SDL_UnlockAudioDevice(lvarDeviceId);
	return;

}
//}}}

 // Shift Time Reference
//{{{
void lnxSoundShiftTimeReference(float time)
{
	// TODO lock


	lvarTimeZero+= time*lvarSampsPerSecond;
	//lvarTimeZero = time*lvarSampsPerSecond;
	//xx

	/*
	int dt = ReTarget - lvarTimeZero;

	ReArrayTotal-= ReArray[ReArraySpot];
	ReArrayTotal+= dt;
	ReArray[ReArraySpot] = dt;

	ReArraySpot = (ReArraySpot+1)%ReArraySize;

	//lvarTimeZero+= time*lvarSampsPerSecond + ReArrayTotal/ReArraySize;
	lvarTimeZero+= ReArrayTotal/ReArraySize; // TODO
	*/


	// TODO unlock

}
//}}}


 // Create Sfx
//{{{
lnxSfx lnxSoundCreateSfx(float stream[], int samplecount)
{
	lnxSfx nsfx;
	EMALLOC(nsfx, struct lnxSfxRaw, 1)

	nsfx->sampcount = samplecount;

	EMALLOC(nsfx->stream, float, samplecount)
	{
		int s;
		for(s=0;s<samplecount;s++)
		{
			nsfx->stream[s] = stream[s];
		}
	}

	return nsfx;
}
//}}}

 // Destroy Sfx
//{{{
void lnxSoundDestroySfx(lnxSfx sfx)
{
	free(sfx->stream);
	free(sfx);
}
//}}}


 // Init
//{{{
void lnxSoundInit(int dsampps, int* osampps)//TODO take channels as argument
{
	// TODO
	// dsampps:  desired samples per second // SCRATHC THIS??? keep sampsPerSecond internal
	// osampps:  obtained samples per second // SCRATCH THIS?

	// TODO make VAF visible to this, instead of upper level?  question is whether sampps is considered "private" or not...

	ReArraySize = 32; // TODO base on arg? like how many seconds shit should be spread over?
	{
		EMALLOC(ReArray, int, ReArraySize);
		int i;
		for(i=0;i<ReArraySize;i++)
		{
			ReArray[i] = 0;
		}
	}
	ReArraySpot = 0;
	ReArrayTotal = 0;
	ReTarget = 250; // TODO base on arg?

	int channels = 2;

	if(channels!=1 && channels!=2 && channels!=4 && channels!=6)
	{
		lnxLogError("Sound Init failure: invalid number of channels passed (passed %d; must be 1, 2, 4, or 6).\n", channels);
		return;
	}

	// Init sound  TODO break init/deinit into their own functions? have majority of this function called "start audio" or something??
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		lnxLogError("Unable to initialize audio system with SDL: %s\n",SDL_GetError());
		return;
	}

	// Open Audio Device
	{
		SDL_AudioSpec desired;
		SDL_AudioSpec obtained;

		desired.freq = dsampps;
		desired.format = AUDIO_F32SYS; // U16 glitches out on chrome...
		desired.channels = channels;
		//desired.silence = 0;
		desired.samples = 2048; // TODO make not arbitrary
		//desired.size = 0;
		desired.callback = callback;
		desired.userdata = 0;

		// TODO if more errors in the future, try fiddling with allowed changes

		lvarDeviceId = SDL_OpenAudioDevice(0, 0, &desired, &obtained,
		                                   SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

		if(lvarDeviceId == 0)
		{
			lnxLogError("sound.c:  Failed to open audio: %s\n", SDL_GetError());
		}

		// TODO scratch?
		if(desired.freq!=obtained.freq) printf("NF: %d\n",obtained.freq);
		if(desired.channels!=obtained.channels) printf("Nc: %d\n",obtained.channels);
		if(desired.samples!=obtained.samples) printf("Ns: %d\n",obtained.samples);
		if(desired.format!=obtained.format) printf("NFor: %d\n",obtained.format);

		if(osampps) *osampps = obtained.freq;

		if(desired.channels!=obtained.channels) printf("TODO\n");
		lvarSampsPerSecond = obtained.freq;
		lvarChannels = obtained.channels;
	}

	SDL_PauseAudioDevice(lvarDeviceId,0);// TODO put in start function?

}
//}}}

 // Deinit
//{{{
void lnxSoundDeinit()
{
	free(ReArray); // TODO make a var to track if init is active, and check before calls to init, so redundant init calls won't cause memory leak.  (redundant deinit probably fine, but check anyway)
	SDL_CloseAudioDevice(lvarDeviceId);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
//}}}


//}}}


