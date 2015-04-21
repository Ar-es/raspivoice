// raspivoice
//
// Based on:
// http://www.seeingwithsound.com/hificode_OpenCV.cpp
// C program for soundscape generation. (C) P.B.L. Meijer 1996
// hificode.c modified for camera input using OpenCV. (C) 2013
// Last update: December 29, 2014; released under the Creative
// Commons Attribution 4.0 International License (CC BY 4.0),
// see http://www.seeingwithsound.com/im2sound.htm for details
// License: https://creativecommons.org/licenses/by/4.0/
//
// For a discussion of this kind of software on the raspberry Pi, see
// https://www.raspberrypi.org/forums/viewtopic.php?uid=144831&f=41&t=49634&start=0

#include <iostream>
#include <thread>
#include <iostream>
#include <cmath>
#include <cinttypes>
#include <unistd.h>
#include <getopt.h>

#include "printtime.h"
#include "RaspiVoice.h"

int main(int argc, char *argv[])
{
	int show_help = 0;

	RaspiVoiceOptions rvopt;
	rvopt.rows = 64;
	rvopt.columns = 176;
	rvopt.image_source = 1;
	rvopt.input_filename = "";
	rvopt.audio_device = "default";
	rvopt.preview = false;
	rvopt.use_bw_test_image = false;
	rvopt.verbose = false;
	rvopt.negative_image = false;
	rvopt.flip = 0;
	rvopt.read_frames = 2;
	rvopt.exposure = 0;
	rvopt.blinders = 0;
	rvopt.zoom = 1;
	rvopt.foveal_mapping = false;
	rvopt.threshold = 0;
	rvopt.edge_detection_opacity = 0.0;
	rvopt.edge_detection_threshold = 127;
	rvopt.freq_lowest = 500;
	rvopt.freq_highest = 5000;
	rvopt.sample_freq_Hz = 44100;
	rvopt.total_time_s = 1.05;
	rvopt.use_exponential = true;
	rvopt.use_stereo = true;
	rvopt.use_delay = true;
	rvopt.use_fade = true;
	rvopt.use_diffraction = true;
	rvopt.use_bspline = true;
	rvopt.speed_of_sound_m_s = 340;
	rvopt.acoustical_size_of_head_m = 0.20;

	static struct option long_options[] =
	{
		{ "help",				no_argument,		&show_help, 1 },
		{ "rows",				required_argument,	0, 'r' },
		{ "columns",			required_argument,	0, 'c' },
		{ "image_source",		required_argument,	0, 's' },
		{ "input_filename",		required_argument,  0, 'i' },
		{ "audio_device",		required_argument,	0, 'a' },
		{ "preview",			no_argument,		0, 'p' },
		{ "use_bw_test_image",	required_argument,	0, 'B' },
		{ "verbose",			no_argument,		0, 'v' },
		{ "negative_image",		no_argument,		0, 'n' },
		{ "flip",				required_argument,  0, 'f' },
		{ "read_frames",		required_argument,	0, 'R' },
		{ "exposure",			required_argument,	0, 'e' },
		{ "blinders",			required_argument,	0, 'b' },
		{ "zoom",				required_argument,	0, 'z' },
		{ "foveal_mapping",		no_argument,		0, 'm' },
		{ "edge_detection_opacity",		required_argument, 0, 'E' },
		{ "edge_detection_threshold",	required_argument, 0, 'G' },
		{ "freq_lowest",		required_argument,	0, 'l' },
		{ "freq_highest",		required_argument,	0, 'h' },
		{ "total_time_s",		required_argument,	0, 't' },
		{ "use_exponential",	required_argument,	0, 'x' },
		{ "use_delay",			required_argument,	0, 'd' },
		{ "use_fade",			required_argument,	0, 'F' },
		{ "use_diffraction",	required_argument,	0, 'D' },
		{ "use_bspline",		required_argument,	0, 'N' },
		{ "sample_freq_Hz",		required_argument,	0, 'S' },
		{ "threshold",			required_argument,	0, 'T' },
		{ "use_stereo",			required_argument,  0, 'O' },
		{ 0, 0, 0, 0 }
	};

	//Retrieve command line options:
	int option_index = 0;
	int opt;
	while ((opt = getopt_long_only(argc, argv, "r:c:i:a:pb:vne:f:r:s:", long_options, &option_index)) != -1)
	{
		switch (opt)
		{
			case 0:
				break;
			case 'r':
				rvopt.rows = atoi(optarg);
				break;
			case 'c':
				rvopt.columns = atoi(optarg);
				break;
			case 's':
				rvopt.image_source = atoi(optarg);
				break;
			case 'i':
				rvopt.input_filename = optarg;
				break;
			case 'a':
				rvopt.audio_device = optarg;
				break;
			case 'p':
				rvopt.preview = true;
				break;
			case 'B':
				rvopt.use_bw_test_image = (atoi(optarg) != 0);
				break;
			case 'v':
				rvopt.verbose = true;
				break;
			case 'f':
				rvopt.flip = atoi(optarg);
				break;
			case 'n':
				rvopt.negative_image = true;
				break;
			case 'E':
				rvopt.edge_detection_opacity = atof(optarg);
				break;
			case 'G':
				rvopt.edge_detection_threshold = atoi(optarg);
				break;
			case 'l':
				rvopt.freq_lowest = atof(optarg);
				break;
			case 'h':
				rvopt.freq_highest = atof(optarg);
				break;
			case 't':
				rvopt.total_time_s = atof(optarg);
				break;
			case 'x':
				rvopt.use_exponential = (atoi(optarg) != 0);
				break;
			case 'd':
				rvopt.use_delay = (atoi(optarg) != 0);
				break;
			case 'F':
				rvopt.use_fade = (atoi(optarg) != 0);
				break;
			case 'R':
				rvopt.read_frames = atoi(optarg);
				break;
			case 'e':
				rvopt.exposure = atoi(optarg);
				break;
			case 'b':
				rvopt.blinders = atoi(optarg);
				break;
			case 'z':
				rvopt.zoom = atof(optarg);
				break;
			case 'm':
				rvopt.foveal_mapping = true;
				break;
			case 'D':
				rvopt.use_diffraction = (atoi(optarg) != 0);
				break;
			case 'N':
				rvopt.use_bspline = (atoi(optarg) != 0);
				break;
			case 'S':
				rvopt.sample_freq_Hz = atof(optarg);
				break;
			case 'T':
				rvopt.threshold = atoi(optarg);
				break;
			case 'O':
				rvopt.use_stereo = (atoi(optarg) != 0);
				break;
			case '?':
				std::cout << "Type raspivoice --help for available options." << std::endl;
			default:
				return -1;
		}
	}

	if (optind < argc)
	{
		std::cerr << "Invalid argument: " << argv[optind] << std::endl;
		std::cerr << "Type raspivoice --help for available options." << std::endl;
		return -1;
	}

	if (show_help != 0)
	{
		std::cout << "Usage: " << std::endl;
		std::cout << "raspivoice {options}" << std::endl;
		std::cout << std::endl;
		std::cout << "Options [defaults]: " << std::endl;
		std::cout << "    --help\t\t\t\tThis help text" << std::endl;
		std::cout << "-r, --rows=[64]\t\t\t\tNumber of rows, i.e. vertical (frequency) soundscape resolution (ignored if test image is used)" << std::endl;
		std::cout << "-c, --columns=[178]\t\t\tNumber of columns, i.e. horizontal (time) soundscape resolution (ignored if test image is used)" << std::endl;
		std::cout << "-s, --image_source=[1]\t\t\tImage source: 0 for image file, 1 for RaspiCam, 2 for 1st USB camera, 3 for 2nd USB camera..." << std::endl;
		std::cout << "-i, --input_filename=[]\t\t\tPath to image file (bmp,jpg,png,ppm,tif). Reread every frame. Static test image is used if empty." << std::endl; 
		std::cout << "-a, --audio_device=[default]\t\tAudio output device, type aplay -L to get list" << std::endl;
		std::cout << "-p, --preview\t\t\t\tOpen preview window(s). X server required." << std::endl;
		std::cout << "-v, --verbose\t\t\t\tVerbose outputs." << std::endl;
		std::cout << "-n, --negative_image" << std::endl;
		std::cout << "-f, --flip=[0]\t\t\t\t0: no flipping, 1: horizontal, 2: verticel, 3: both" << std::endl;
		std::cout << "-R, --read_frames=[2]\t\t\tSet number of frames to read from camera before processing (>= 1). Optimize for minimal lag." << std::endl;
		std::cout << "-e  --exposure=[0]\t\t\tCamera exposure time setting, 1-100. Use 0 for auto." << std::endl;
		std::cout << "-b  --blinders=[0]\t\t\tBlinders left and right, pixel size (0-89 for default columns)" << std::endl;
		std::cout << "-z  --zoom=[1.0]\t\t\tZoom factor (>= 1.0)" << std::endl;
		std::cout << "-m  --foveal_mapping\t\tEnable foveal mapping (barrel distortion magnifying center region)" << std::endl;
		std::cout << "-T, --threshold=[0]\t\t\tEnable threshold for black/white image if > 0. Range 1-255, use 127 as a starting point. 255=auto." << std::endl;
		std::cout << "-E, --edge_detection_opacity=[0.0]\tEnable edge detection if > 0. Opacity of detected edges between 0.0 and 1.0." << std::endl;
		std::cout << "-G  --edge_detection_threshold=[127]\tEdge detection threshold value 1-255." << std::endl;
		std::cout << "-l, --freq_lowest=[500]" << std::endl;
		std::cout << "-h, --freq_highest=[5000]" << std::endl;
		std::cout << "-t, --total_time_s=[1.05]" << std::endl;
		std::cout << "-x  --use_exponential=[1]" << std::endl;
		//std::cout << "-o  --use_stereo=[1]" << std::endl;
		std::cout << "-d, --use_delay=[1]" << std::endl;
		std::cout << "-F, --use_fade=[1]" << std::endl;
		std::cout << "-D  --use_diffraction=[1]" << std::endl;
		std::cout << "-N  --use_bspline=[1]" << std::endl;
		std::cout << "-S  --sample_freq_Hz=[44100]" << std::endl;
		std::cout << std::endl;
		return 0;
	}

	//Start program:
	try
	{
		RaspiVoice raspiVoice(rvopt);

		while (true)
		{
			raspiVoice.PlayFrame();
		}
	}
	catch (std::runtime_error err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return -1;
	}
	catch (std::exception err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return -1;
	}

	return(0);
}


