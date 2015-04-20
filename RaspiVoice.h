#pragma once

#include <vector>
#include <raspicam/raspicam_cv.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "ImageToSoundscape.h"

typedef struct
{
	int rows;
	int columns;
	int image_source;
	std::string input_filename;
	std::string audio_device;
	bool preview;
	bool use_bw_test_image;
	bool verbose;
	bool negative_image;
	int flip;
	int read_frames;
	int threshold;
	float edge_detection_opacity;
	int edge_detection_threshold;
	double freq_lowest;
	double freq_highest;
	int	sample_freq_Hz;
	double total_time_s;
	bool use_exponential;
	bool use_stereo;
	bool use_delay;
	bool use_fade;
	bool use_diffraction;
	bool use_bspline;
	float speed_of_sound_m_s;
	float acoustical_size_of_head_m;
} RaspiVoiceOptions;

class RaspiVoice
{
private:
	int rows;
	int columns;
	int image_source;
	bool preview;
	bool use_bw_test_image;
	bool verbose;
	RaspiVoiceOptions opt;

	ImageToSoundscapeConverter *i2ssConverter;
	raspicam::RaspiCam_Cv raspiCam;
	cv::VideoCapture cap;
	std::vector<float> *image;

	RaspiVoice(const RaspiVoice& other) = delete;
	RaspiVoice& operator=(const RaspiVoice&) = delete;

	void init();
	void initFileImage();
	void initTestImage();
	void initRaspiCam();
	void initUsbCam();
	void readImage();
	int playWav(std::string filename);
public:
	RaspiVoice(RaspiVoiceOptions opt);
	~RaspiVoice();
	void PlayFrame();
};

