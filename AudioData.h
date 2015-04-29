#pragma once

#include <vector>
#include <string>
#include <cstdio>
#include <cinttypes>

class AudioData
{
private:
	const bool use_stereo;
	const int sample_freq_Hz;
	const int sample_count;
	std::vector<uint16_t> samplebuffer;

	void wi(FILE* fp, uint16_t i);
	void wl(FILE* fp, uint32_t l);

public:
	int CardNumber;
	bool Verbose;

	AudioData(int sample_freq_Hz, int sample_count, bool use_stereo = true);
	
	uint16_t *Data() { return &samplebuffer[0]; };

	void SaveToWavFile(std::string filename);
	
	void Play();
	void SetVolume(int newvolume);
	static int PlayWav(std::string filename, int cardnumber);
	static int SetVolume(int newvolume, int cardnumber);
	static bool Speak(std::string text, int cardnumber);
};

