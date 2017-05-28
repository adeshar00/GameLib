
// Converts VAF data to an audio stream (float array, at least atm)

// Includes
//{{{

#include <stdio.h>  // TODO delete?
#include <math.h>   // TODO delete?
#include <stdlib.h>

//}}}

// TODO add curve feature (start with just simple "slope" parameter), test it out, see if more robust curving necessary or if slope is good enough

// Internal Functions
//{{{

// IFFT TODO rewrite this
//{{{
static void ifft(float fourierx[], float fouriery[], int bits)
{
	int sampleCount = 1<<bits;

	// Unblend iterations
	{
		int chunksize = sampleCount;
		while(chunksize>=2)
		{
			int chunk;
			for(chunk=0;chunk<sampleCount;chunk+=chunksize)
			{
				int step;
				for(step=0;step<chunksize/2;step++)
				{
					int a = chunk+step;
					int b = chunk+step+chunksize/2;
					float theta = 1.0*step/chunksize*2*M_PI;
					//printf("c %d  s %d  t %f\n",chunk,step,theta); TODO XXX?
					float c = cos(theta);
					float s = sin(theta);
					float ofax = fourierx[a];
					float ofay = fouriery[a];
					float ofbx = fourierx[b];
					float ofby = fouriery[b];
					float ufbx = ofax - ofbx;
					float ufby = ofay - ofby;
					fourierx[a] = ofax + ofbx;
					fouriery[a] = ofay + ofby;
					fourierx[b] = c*ufbx - s*ufby;
					fouriery[b] = s*ufbx + c*ufby;
				}
			}
			chunksize>>=1;
		}
	}

	// Populate samples (in-place: fourierx now represents samples, fouriery is garbage)
	{
		int f;
		for(f=0;f<sampleCount;f++)
		{
			int rev = 0; // Bitwise reverse of f (given a bitwidth of "bits")

			// Determine rev
			{
				int b;
				for(b=0;b<bits;b++)
				{
					rev|= (((1<<b)&f)>>b)<<(bits-b-1);
				}
			}

			// Swap f and rev if hasn't already happened
			if(f<rev)
			{
				float t = fourierx[f];
				fourierx[f] = fourierx[rev];
				fourierx[rev] = t;
			}
		}
	}

}
//}}}

// make spread TODO rewrite this
//{{{
static void makespread(float** samples, int* peak, float spread, int freq, int bitcount)
{
	// TODO figure out behavior if spread>1.0
	// Note: make sure to free memory that return pointer points to

	int sampleCount = 1<<bitcount;
	float* fourierx;
	float* fouriery;
	int peakid;

	if(freq<0 || freq>=sampleCount)
	{
		printf("sound.c:  Error: Invalid frequency or bitcount passed to makespread\n");
		return;
	}

	// Malloc fourier
	fourierx = (float*)malloc(sizeof(float)*sampleCount);
	if(fourierx==0) {printf("sound.c:  Malloc failure in makespread\n");}
	fouriery = (float*)malloc(sizeof(float)*sampleCount);
	if(fouriery==0) {printf("sound.c:  Malloc failure in makespread\n");}

	// Zero out fouriers
	{
		int s;
		for(s=0;s<sampleCount;s++)
		{
			fourierx[s] = 0.0f;
			fouriery[s] = 0.0f;
		}
	}

	// Populate fouriers
	{
		srand(197);
		int idspread = spread*freq; // Number of fourier vectors above/below freq to alter
		float density = 1.0f/(idspread+1); // Max mag of a fourier vector

		// Set fourier vectors around freq
		int f;
		for(f=0;f<idspread;f++)
		{
			float dist = 1.0f*f/idspread;
			float factor = 3.0f*dist*dist+2.0f*dist*dist*dist;
			float mag = factor*density;
			float theta;
			// Set lower
			theta = 2.0f*M_PI*(1.0f*(rand()&0xffff)/0xffff);
			fourierx[freq-idspread+f] = mag*cos(theta);
			fouriery[freq-idspread+f] = mag*sin(theta);
			// Set upper
			theta = 2.0f*M_PI*(1.0f*(rand()&0xffff)/0xffff);
			fourierx[freq+idspread-f] = mag*cos(theta);
			fouriery[freq+idspread-f] = mag*sin(theta);
		}

		// Set freq fourier vector itself
		{
			float mag = density;
			float theta = 2.0f*M_PI*(1.0f*(rand()&0xffff)/0xffff);
			fourierx[freq] = mag*cos(theta);
			fouriery[freq] = mag*sin(theta);
		}
	}

	// Inverse Fast Fourier Transform to turn fourier vectors into samples
	{
		ifft(fourierx, fouriery, bitcount);
	}

	// Find peak, and scale samples so peak amp is 1.0
	{
		float max = 0.0f;
		int pid = 0;
		int s;

		// Find peak sample, store it's amp and id
		for(s=0;s<sampleCount;s++)
		{
			if( fourierx[s]>max){  max =  fourierx[s];  pid = s;  }
			if(-fourierx[s]>max){  max = -fourierx[s];  pid = s;  }
		}

		// Save id of peak
		peakid = pid;

		// Scale all samples so max amp is 1.0
		if(max>0.0f)
		{
			// Determine scale factor
			float factor = 1.0f/max;

			// Ensure scale factor has no rounding errors
			while(factor*max>1.0f) factor*= 1023.0f/1024.0f;

			// Scale samples
			for(s=0;s<sampleCount;s++) fourierx[s]*= factor;
		}
	}

	// Free y component of samples, and set samples and peak
	free(fouriery);
	if(samples) *samples = fourierx;
	if(peak)    *peak = peakid;
	
}
//}}}

// burst TODO rewrite this
//{{{
static void burst(int sampleCount,
		float* samples,
		float* spreadSamples,
		int spreadPeakOffset,
		int spreadBits,
		float stepPerSamp,
		float amplitude,
		int start,
		int headlen,
		int bodylen,
		int taillen)
{
	// spreadBits:        How many bits the spread buffer is (i.e. how many 1s in the roll mask)
	// stepPerSamp:       How many spreadSamples should be traversed for every sample
	// spreadSamples:     Spread data (note: samplerate is different from sampps)
	// spreadPeakOffset:  Offset so that peak sample occurs at end of start+headlen
	// spreadBits:        Log(base 2) of the size of spreadSamples
	// stepPerSamp:       Resolution of spreadSamples / sampps

	int spreadMask = (1<<spreadBits)-1;

	// Ignore negative values
	start = start*(start>=0);
	headlen = headlen*(headlen>=0);
	bodylen = bodylen*(bodylen>=0);
	taillen = taillen*(taillen>=0);

	// Return if burst write would exceed sampleCount
	if(start+headlen+bodylen+taillen>sampleCount)
	{
		printf("sound.c:  buffer size error\n");
		return;
	}

	int s;
	for(s=0;s<headlen;s++)
	{
		int z = start;
		float stepf = (z+s-start)*stepPerSamp+spreadPeakOffset;
		int step = stepf;
		float s0 = spreadSamples[step&spreadMask];
		float s1 = spreadSamples[(step+1)&spreadMask];
		float mix = stepf-step+(stepf<0.0f);
		float samp = s0*(1.0-mix) + s1*mix;
		float d = 1.0f*s/headlen;
		float scale = (3.0f*d*d-2.0f*d*d*d)*amplitude;
		samples[z+s]+= samp*scale;
	}
	for(s=0;s<bodylen;s++)
	{
		int z = start+headlen;
		float stepf = (z+s-start)*stepPerSamp+spreadPeakOffset;
		int step = stepf;
		float s0 = spreadSamples[step&spreadMask];
		float s1 = spreadSamples[(step+1)&spreadMask];
		float mix = stepf-step+(stepf<0.0f);
		float samp = s0*(1.0-mix) + s1*mix;
		float scale = amplitude;
		samples[z+s]+= samp*scale;
	}
	for(s=0;s<taillen;s++)
	{
		int z = start+headlen+bodylen;
		float stepf = (z+s-start)*stepPerSamp+spreadPeakOffset;
		int step = stepf;
		float s0 = spreadSamples[step&spreadMask];
		float s1 = spreadSamples[(step+1)&spreadMask];
		float mix = stepf-step+(stepf<0.0f);
		float samp = s0*(1.0-mix) + s1*mix;
		float d = 1.0f-1.0f*s/taillen;
		float scale = (3.0f*d*d-2.0f*d*d*d)*amplitude;
		samples[z+s]+= samp*scale;
	}
}
//}}}

// temp TODO rewrite this
//{{{
static void temp(float fre, float str, float att, float sus, float dec,
                 float amp, float spr,
                 int sampps, int streamsize, float stream[])
{
	float* spread = 0; // Pointer to spread samples
	int peak = 0;      // Index of peak sample in spread
	int spcPow = 16;   // Samples Per Second Power: Determines resolution of spread
	int stPow = -1;    // Spread Time Power: 2^(-1) TODO base on segment length?
	float spreadTime = pow(2,stPow); // Time of spread, in seconds
									 //  (e.i. time between repeats)
	// Create base spread noise
	makespread(
			&spread,
			&peak,
			spr,
			(int)(fre*spreadTime),
			spcPow+stPow);
	if(spread==0){ printf("Sound.c:  Malloc error in generate\n"); }

	// Add a burst using the spread noise base
	burst(streamsize,
			stream,
			spread,
			peak-(att)*(1<<spcPow),
			spcPow+stPow,
			1.0*(1<<spcPow)/sampps,
			amp,
			str*sampps,
			att*sampps,
			sus*sampps,
			dec*sampps);

	// Free spread noise
	free(spread);

	// TODO scale so max amplitude is "amp"

}
//}}}

 // Add Seg TODO take channel count, and array of channel volumes
//{{{
static void addSeg(float fre, float str, float att, float sus, float dec,
                   float amp, float spr, float tml, float timbre[], 
				   int sampps, int streamsize, float stream[])
{

	// TODO interpolation creates imperfect sin waves: if spread==0, call something else


	if(tml==0)
	{
		temp(fre, str, att, sus, dec, amp, spr, sampps, streamsize, stream);
	}
	else
	{
		int t;
		for(t=0;t<tml;t++)
		{
			temp(fre*(t+1), str, att, sus, dec, amp*timbre[t], spr, sampps, streamsize, stream);
		}
	}

	// TODO scale so max amplitude is "amp"

	// */ TODO XXX
	/*
	{
		int s;
		int top = sampleCount;
		if(top>(1<<spcPow)) top = (1<<(spcPow+stPow));
		for(s=0;s<top;s++)
		{
			samples[s] = spread[s];
		}
	}
	// */

}
//}}}

//}}}

// External Functions
//{{{

 // VAF to Stream
//{{{
void vaf_vafToStream(unsigned char vaf[], int sampps, int* size, float** stream)
{
	// vaf:     Byte array for VAF
	// sampps:  Samples per second
	// size:    Pointer to return the size of the generated stream
	// stream:  Pointer to return the byte array of the generated stream


	// Constants
	//{{{

	// un: unit
	// bc: bytecount
	// os: offset
	const float frequn = 0.01;
	const int   freqbc = 3;
	const float timeun = 0.0001;
	const int   timebc = 3;
	const float ampun = 0.005;
	const int   ampbc = 1;
	const float spreadun = 0.01;
	const int   spreadbc = 1;
	const float timlenun = 1;
	const int   timlenbc = 1;
	const int   timlenos = freqbc+4*timebc+spreadbc+ampbc;

	const int headlen = 4; // Number of bytes in VAF header

	//}}}

	// Variables
	//{{{

	int segcount;     // Number of segments represented in VAF
	int strlen;       // Stream length (number of samples)
	//int segoffs[];  // Starting indices in VAF for each segment (declared below)
	float* strdat;    // Stream data

	//}}}

	// Macros  TODO make static func?
	//{{{
#define BTV(NUMBER, INDEX, UNIT, BYTECOUNT) \
{                                           \
	int T = 0;                              \
	int B;                                  \
	for(B=0;B<BYTECOUNT;B++)                \
	{                                       \
		T+= vaf[INDEX+B]<<(8*B);            \
	}                                       \
	NUMBER = UNIT*T;                        \
}
	//}}}


	// TODO check version number
	// ...
	
	// Determine Segment Count
	segcount = (vaf[3]<<8) + vaf[2];

	// Set offset to where each segment's data is in VAF
	int segoffs[segcount];
	{
		int b = headlen;

		int s;
		for(s=0;s<segcount-1;s++)
		{
			segoffs[s] = b;

			b+= timlenos;

			int timlen;
			BTV(timlen, b, timlenun, timlenbc);
			b+= timlenbc + timlen*ampbc;
		}

		segoffs[segcount-1] = b;
	}

	// Determine length of stream
	{
		int max = 0;

		int s;
		for(s=0;s<segcount;s++)
		{
			float str;
			float att;
			float sus;
			float dec;
			BTV(str, segoffs[s]+freqbc+timebc*0, timeun, timebc);
			BTV(att, segoffs[s]+freqbc+timebc*1, timeun, timebc);
			BTV(sus, segoffs[s]+freqbc+timebc*2, timeun, timebc);
			BTV(dec, segoffs[s]+freqbc+timebc*3, timeun, timebc);
			int end = (str+att+sus+dec)*sampps;
			
			if(end>max) max = end;
		}

		strlen = max+100; // TODO non arbitrary number
		//strlen = 2*sampps+100; // TODO XXX
	}

	// Init stream data
	strdat = (float*)malloc(sizeof(float)*strlen);
	{
		int s;
		for(s=0;s<strlen;s++)
		{
			strdat[s] = 0.0f;
		}
	}

	// Parse VAF and populate strdat
	{
		int s;
		for(s=0;s<segcount;s++)
		{
			int os = segoffs[s];

			// Parse data
			float fre, str, att, sus, dec, amp, spr;
			int tml;
			BTV(fre, os, frequn,   freqbc);    os+= freqbc;
			BTV(str, os, timeun,   timebc);    os+= timebc;
			BTV(att, os, timeun,   timebc);    os+= timebc;
			BTV(sus, os, timeun,   timebc);    os+= timebc;
			BTV(dec, os, timeun,   timebc);    os+= timebc;
			BTV(amp, os, ampun,    ampbc);     os+= ampbc;
			BTV(spr, os, spreadun, spreadbc);  os+= spreadbc;
			BTV(tml, os, timlenun, timlenbc);  os+= timlenbc;
			float timbre[tml];
			{
				int t;
				for(t=0;t<tml;t++)
				{
					BTV(timbre[t], os, ampun, ampbc);
					os+= ampbc;
				}
			}

			// Update stream with segment
			addSeg(fre, str, att, sus, dec, amp, spr, tml, timbre, sampps, strlen, strdat);

		}
	}

	// Scale samples to be <= 1.0
	{
		// Determine max sample dist for scale factor
		float max = 1.0f; // TODO set to be max segment amp? for s in segs: if s.amp>max: max=amp ?
		                 //  If so, make sure to check that non-zero
		int s;
		for(s=0;s<strlen;s++)
		{
			if( strdat[s]>max)  max =  strdat[s];
			if(-strdat[s]>max)  max = -strdat[s];
		}

		// Calculate scale factor
		float scale = 1.0f/max;

		// Catch and fix any rounding errors
		while(scale*max>1.0f) scale*= 1023.0f/1024.0f;

		// Scale samples
		for(s=0;s<strlen;s++)
		{
			strdat[s]*= scale;
		}
	}

	// Set return values
	if(size) *size = strlen;
	if(stream) *stream = strdat;

	// Undefine Macros
	//{{{

#undef BTV

	//}}}

}
//}}}

//}}}


