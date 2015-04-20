// raspivoice
// Based on:
// http://www.seeingwithsound.com/hificode_OpenCV.cpp
// C program for soundscape generation. (C) P.B.L. Meijer 1996
// hificode.c modified for camera input using OpenCV. (C) 2013
// Last update: December 29, 2014; released under the Creative
// Commons Attribution 4.0 International License (CC BY 4.0),
// see http://www.seeingwithsound.com/im2sound.htm for details
// License: https://creativecommons.org/licenses/by/4.0/

#include <cstring>
#include <stdexcept>
#include <iostream>
#include <sstream>

#include "AudioData.h"

AudioData::AudioData(int sample_freq_Hz, int sample_count, bool use_stereo) :
	sample_freq_Hz(sample_freq_Hz),
	sample_count(sample_count),
	use_stereo(use_stereo),
	Verbose(false),
	DeviceName("default"),
	samplebuffer(std::vector<uint16_t>((use_stereo ? 2 : 1) * sample_count))
{
}

void AudioData::wi(FILE* fp, uint16_t i)
{
	int b1, b0;
	b0 = i % 256;
	b1 = (i - b0) / 256;
	putc(b0, fp);
	putc(b1, fp);
}

void AudioData::wl(FILE* fp, uint32_t l)
{
	unsigned int i1, i0;
	i0 = l % 65536L;
	i1 = (l - i0) / 65536L;
	wi(fp, i0);
	wi(fp, i1);
}

void AudioData::SaveToWavFile(std::string filename)
{
	FILE *fp;
	int bytes_per_sample = (use_stereo ? 4 : 2);

	// Write 8/16-bit mono/stereo .wav file
	fp = fopen(filename.c_str(), "wb");
	fprintf(fp, "RIFF");
	wl(fp, sample_count * bytes_per_sample + 36L);
	fprintf(fp, "WAVEfmt ");
	wl(fp, 16L);
	wi(fp, 1);
	wi(fp, use_stereo ? 2 : 1);
	wl(fp, 0L + sample_freq_Hz);
	wl(fp, 0L + sample_freq_Hz * bytes_per_sample);
	wi(fp, bytes_per_sample);
	wi(fp, 16);
	fprintf(fp, "data");
	wl(fp, sample_count * bytes_per_sample);

	fwrite(samplebuffer.data(), bytes_per_sample, sample_count, fp);
	fclose(fp);
}


void AudioData::Play()
{
	int bytes_per_sample = (use_stereo ? 4 : 2);

	std::stringstream cmd;
	cmd << "aplay --nonblock -r" << sample_freq_Hz << " -c" << (use_stereo ? 2 : 1) << " -fS16_LE -D" << DeviceName;
	if (!Verbose)
	{
		cmd << " -q";
	}
	else
	{
		std::cout << cmd.str() << std::endl;
	}

	FILE* p = popen(cmd.str().c_str(), "w");
	fwrite(samplebuffer.data(), bytes_per_sample, sample_count, p);
	pclose(p);
}

int AudioData::PlayWav(std::string filename, std::string devicename)
{
	char command[256] = "";
	int status;
	snprintf(command, 256, "aplay %s -D%s", filename.c_str(), devicename.c_str());
	status = system(command);
	return status;
}
