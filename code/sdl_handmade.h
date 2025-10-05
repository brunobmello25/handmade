#if !defined(SDL_HANDMADE_H)

struct SdlOffscreenBuffer
{
	// NOTE(casey): Pixels are alwasy 32-bits wide, memory Order BB GG RR XX
	SDL_Texture *texture;
	void *memory;
	int width;
	int height;
	int pitch;
};

struct SdlWindowDimension
{
	int width;
	int height;
};

struct SdlAudioRingBuffer
{
	int size;
	int writeCursor;
	int playCursor;
	void *data;
};

struct SdlSoundOutput
{
	int samplesPerSecond;
	uint32 runningSampleIndex;
	int bytesPerSample;
	int secondaryBufferSize;
	int latencySampleCount;
};

#define SDL_HANDMADE_H
#endif
