
typedef struct lnxSfxRaw* lnxSfx;


lnxSfx lnxSoundCreateSfx(float stream[], int samplecount);
void lnxSoundDestroySfx(lnxSfx sfx);

void lnxSoundInit(int dsampps, int* osampps);
void lnxSoundDeinit();

void lnxSoundShiftTimeReference(float time);
void lnxSoundPlaySfx(lnxSfx sound, float starttime, float vols[]);

