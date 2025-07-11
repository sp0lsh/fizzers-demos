#ifndef _OIDOS_H_
#define _OIDOS_H_

struct sample {
	short left,right;
};

extern "C" {
	// Fill the block of random data used by Oidos.
	// Must be called before Oidos_GenerateMusic.
	void __cdecl Oidos_FillRandomData();

	// Generate the whole music into the music buffer. When this function
	// returns, Oidos_MusicBuffer will be filled with sound data,
	// and Oidos_StartMusic can be called.
	void __cdecl Oidos_GenerateMusic();

	// Play the music
	void __cdecl Oidos_StartMusic();

	// Returns how much of the music has currently been played.
	// Use this function as the timer for the visuals in your intro.
	// Returned value is measured in music ticks (pattern rows).
	float __cdecl Oidos_GetPosition();

	// Buffer containing the music.
	extern struct sample Oidos_MusicBuffer[];

	// The tick rate of the music.
	extern const float Oidos_TicksPerSecond;

	// The length of the music in ticks.
	extern const unsigned int Oidos_MusicLength;

	// Wav file header to use if you want to write the music to disk.
	// Write these 44 bytes followed by Oidos_MusicBuffer with a
	// length of Oidos_WavFileHeader[10].
	extern unsigned int Oidos_WavFileHeader[11];

	// Block of random data used by Oidos.
	// Can also be useful as a 3D noise texture.
	#define NOISESIZE 64
	extern unsigned Oidos_RandomData[NOISESIZE * NOISESIZE * NOISESIZE];
};

// If you are using D3D11, you can re-use this GUID.
#ifdef GUID_DEFINED
extern GUID ID3D11Texture2D_ID;
#endif

#endif
