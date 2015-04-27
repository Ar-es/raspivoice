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
#include <cmath>
#include <cinttypes>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>

#include "printtime.h"
#include "RaspiVoice.h"

void *run_worker_thread(void *arg);
void setup_screen(void);
void print_interactive_commands(void);
bool key_pressed_action(int ch);
bool key_event_action(int event_code);
void daemon_startup(void);
bool grab_keyboard(std::string bus_device_id);
bool speak(std::string text);

RaspiVoiceOptions rvopt, rvopt_defaults;
pthread_mutex_t rvopt_mutex;
int fevdev = -1;

bool quit_flag = false;
pthread_mutex_t quit_flag_mutex;
std::exception_ptr exc_ptr;

int main(int argc, char *argv[])
{
	int show_help = 0;

	rvopt.rows = 64;
	rvopt.columns = 176;
	rvopt.image_source = 1;
	rvopt.input_filename = "";
	rvopt.output_filename = "";
	rvopt.audio_device = "default";
	rvopt.preview = false;
	rvopt.use_bw_test_image = false;
	rvopt.verbose = false;
	rvopt.negative_image = false;
	rvopt.flip = 0;
	rvopt.read_frames = 2;
	rvopt.exposure = 0;
	rvopt.brightness = 0;
	rvopt.contrast = 1.0;
	rvopt.blinders = 0;
	rvopt.zoom = 1;
	rvopt.foveal_mapping = false;
	rvopt.threshold = 0;
	rvopt.edge_detection_opacity = 0.0;
	rvopt.edge_detection_threshold = 50;
	rvopt.freq_lowest = 500;
	rvopt.freq_highest = 5000;
	rvopt.sample_freq_Hz = 48000;
	rvopt.total_time_s = 1.05;
	rvopt.use_exponential = true;
	rvopt.use_stereo = true;
	rvopt.use_delay = true;
	rvopt.use_fade = true;
	rvopt.use_diffraction = true;
	rvopt.use_bspline = true;
	rvopt.speed_of_sound_m_s = 340;
	rvopt.acoustical_size_of_head_m = 0.20;
	rvopt.mute = false;
	rvopt.daemon = false;
	rvopt.grab_keyboard = "";
	rvopt.speak = false;

	static struct option long_options[] =
	{
		{ "help",				no_argument,		&show_help, 1 },
		{ "rows",				required_argument,	0, 'r' },
		{ "columns",			required_argument,	0, 'c' },
		{ "image_source",		required_argument,	0, 's' },
		{ "input_filename",		required_argument,  0, 'i' },
		{ "output_filename",	required_argument,	0, 'o' },
		{ "audio_device",		required_argument,	0, 'a' },
		{ "preview",			no_argument,		0, 'p' },
		{ "use_bw_test_image",	required_argument,	0, 'I' },
		{ "verbose",			no_argument,		0, 'v' },
		{ "negative_image",		no_argument,		0, 'n' },
		{ "flip",				required_argument,  0, 'f' },
		{ "read_frames",		required_argument,	0, 'R' },
		{ "exposure",			required_argument,	0, 'e' },
		{ "brightness",			required_argument,	0, 'B' },
		{ "contrast",			required_argument,	0, 'C' },
		{ "blinders",			required_argument,	0, 'b' },
		{ "zoom",				required_argument,	0, 'z' },
		{ "foveal_mapping",		no_argument,		0, 'm' },
		{ "edge_detection_opacity",		required_argument, 0, 'E' },
		{ "edge_detection_threshold",	required_argument, 0, 'G' },
		{ "freq_lowest",		required_argument,	0, 'l' },
		{ "freq_highest",		required_argument,	0, 'h' },
		{ "total_time_s",		required_argument,	0, 't' },
		{ "use_exponential",	required_argument,	0, 'x' },
		{ "use_delay",			required_argument,	0, 'y' },
		{ "use_fade",			required_argument,	0, 'F' },
		{ "use_diffraction",	required_argument,	0, 'D' },
		{ "use_bspline",		required_argument,	0, 'N' },
		{ "sample_freq_Hz",		required_argument,	0, 'Z' },
		{ "threshold",			required_argument,	0, 'T' },
		{ "use_stereo",			required_argument,  0, 'O' },
		{ "daemon",				no_argument,		0, 'd' },
		{ "grab_keyboard",		required_argument,	0, 'g' },
		{ "speak",				no_argument,		0, 'S' },
		{ 0, 0, 0, 0 }
	};

	//Retrieve command line options:
	int option_index = 0;
	int opt;
	while ((opt = getopt_long_only(argc, argv, "r:c:i:o:a:pI:vnf:R:e:B:C:b:z:mE:G:l:h:t:x:y:d:F:D:N:Z:T:O:dg:S", long_options, &option_index)) != -1)
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
			case 'o':
				rvopt.output_filename = optarg;
				break;
			case 'a':
				rvopt.audio_device = optarg;
				break;
			case 'p':
				rvopt.preview = true;
				break;
			case 'I':
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
			case 'y':
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
			case 'B':
				rvopt.brightness = atoi(optarg);
				break;
			case 'C':
				rvopt.contrast = atof(optarg);
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
			case 'Z':
				rvopt.sample_freq_Hz = atof(optarg);
				break;
			case 'T':
				rvopt.threshold = atoi(optarg);
				break;
			case 'O':
				rvopt.use_stereo = (atoi(optarg) != 0);
				break;
			case 'd':
				rvopt.daemon = true;
				break;
			case 'g':
				rvopt.grab_keyboard = optarg;
				break;
			case 'S':
				rvopt.speak = true;
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
		std::cout << "-d  --daemon\t\t\t\tDaemon mode (run in background)" << std::endl;
		std::cout << "-r, --rows=[64]\t\t\t\tNumber of rows, i.e. vertical (frequency) soundscape resolution (ignored if test image is used)" << std::endl;
		std::cout << "-c, --columns=[178]\t\t\tNumber of columns, i.e. horizontal (time) soundscape resolution (ignored if test image is used)" << std::endl;
		std::cout << "-s, --image_source=[1]\t\t\tImage source: 0 for image file, 1 for RaspiCam, 2 for 1st USB camera, 3 for 2nd USB camera..." << std::endl;
		std::cout << "-i, --input_filename=[]\t\t\tPath to image file (bmp,jpg,png,ppm,tif). Reread every frame. Static test image is used if empty." << std::endl; 
		std::cout << "-o, --output_filename=[]\t\tPath to output file (wav). Written every frame if not muted." << std::endl;
		std::cout << "-a, --audio_device=[default]\t\tAudio output device, type aplay -L to get list" << std::endl;
		std::cout << "-S, --speak\t\t\t\tSpeak out option changes (espeak)." << std::endl;
		std::cout << "-g  --grab_keyboard=[]\t\t\tGrab keyboard device for exclusive access. Use device number 0, 1, 2... from /dev/input/event*" << std::endl;
		std::cout << "-p, --preview\t\t\t\tOpen preview window(s). X server required." << std::endl;
		std::cout << "-v, --verbose\t\t\t\tVerbose outputs." << std::endl;
		std::cout << "-n, --negative_image\t\t\tSwap bright and dark." << std::endl;
		std::cout << "-f, --flip=[0]\t\t\t\t0: no flipping, 1: horizontal, 2: verticel, 3: both" << std::endl;
		std::cout << "-R, --read_frames=[2]\t\t\tSet number of frames to read from camera before processing (>= 1). Optimize for minimal lag." << std::endl;
		std::cout << "-e  --exposure=[0]\t\t\tCamera exposure time setting, 1-100. Use 0 for auto." << std::endl;
		std::cout << "-B  --brightness=[0]\t\t\tAdditional brightness, -255 to 255." << std::endl;
		std::cout << "-C  --contrast=[1.0]\t\t\tContrast enhancement factor >= 1.0" << std::endl;
		std::cout << "-b  --blinders=[0]\t\t\tBlinders left and right, pixel size (0-89 for default columns)" << std::endl;
		std::cout << "-z  --zoom=[1.0]\t\t\tZoom factor (>= 1.0)" << std::endl;
		std::cout << "-m  --foveal_mapping\t\t\tEnable foveal mapping (barrel distortion magnifying center region)" << std::endl;
		std::cout << "-T, --threshold=[0]\t\t\tEnable threshold for black/white image if > 0. Range 1-255, use 127 as a starting point. 255=auto." << std::endl;
		std::cout << "-E, --edge_detection_opacity=[0.0]\tEnable edge detection if > 0. Opacity of detected edges between 0.0 and 1.0." << std::endl;
		std::cout << "-G  --edge_detection_threshold=[50]\tEdge detection threshold value 1-255." << std::endl;
		std::cout << "-l, --freq_lowest=[500]" << std::endl;
		std::cout << "-h, --freq_highest=[5000]" << std::endl;
		std::cout << "-t, --total_time_s=[1.05]" << std::endl;
		std::cout << "-x  --use_exponential=[1]" << std::endl;
		//std::cout << "-o  --use_stereo=[1]" << std::endl;
		std::cout << "-d, --use_delay=[1]" << std::endl;
		std::cout << "-F, --use_fade=[1]" << std::endl;
		std::cout << "-D  --use_diffraction=[1]" << std::endl;
		std::cout << "-N  --use_bspline=[1]" << std::endl;
		std::cout << "-Z  --sample_freq_Hz=[48000]" << std::endl;
		std::cout << std::endl;
		return 0;
	}

	if (rvopt.daemon)
	{
		std::cout << "raspivoice daemon started." << std::endl;
		daemon_startup();
	}

	if (rvopt.grab_keyboard != "")
	{
		if (!rvopt.daemon)
		{
			std::cerr << "Grab keyboard device is only possible in daemon mode." << std::endl;
			return -1;
		}

		if (!grab_keyboard(rvopt.grab_keyboard))
		{
			std::cerr << "Cannot grab keyboard device: " << rvopt.grab_keyboard << "." << std::endl;
			return -1;
		}
	}

	//Start Program in worker thread:
	//Warning: Do not read or write rvopt or quit_flag without locking after this.
	rvopt_defaults = rvopt;
	pthread_t thr;
	pthread_mutex_init(&quit_flag_mutex, NULL);
	pthread_mutex_init(&rvopt_mutex, NULL);
	if (pthread_create(&thr, NULL, run_worker_thread, NULL))
	{
		std::cerr << "Error setting up thread." << std::endl;
		return -1;
	}

	if ((!rvopt_defaults.verbose) && (!rvopt_defaults.daemon))
	{
		//Show interactive screen:
		setup_screen();

		bool quit = false;
		while (!quit)
		{
			int ch = getch();
			if (ch != ERR)
			{
				quit = key_pressed_action(ch);
			}

			pthread_mutex_lock(&quit_flag_mutex);
			if (quit_flag)
			{
				quit = true;
			}
			pthread_mutex_unlock(&quit_flag_mutex);
		}

		//quit ncurses:
		refresh();
		endwin();
	}
	else if ((rvopt_defaults.verbose) && (!rvopt_defaults.daemon))
	{
		//Show debug messages:
		std::cout << "Verbose mode on, interactive mode disabled." << std::endl;
		std::cout << "Press Enter to quit." << std::endl << std::endl;
		getchar();

		pthread_mutex_lock(&quit_flag_mutex);
		quit_flag = true;
		pthread_mutex_unlock(&quit_flag_mutex);
	}
	else if (rvopt.grab_keyboard != "")
	{
		struct input_event ev[64];
		bool quit = false;
		while (!quit)
		{
			int rd;
			if ((rd = read(fevdev, ev, sizeof(struct input_event) * 64)) < sizeof(struct input_event))
			{
				break;
			}

			int value = ev[0].value;
			if (value != ' ' && ev[1].value == 1 && ev[1].type == EV_KEY) //value=1: key press, value=0 key release
			{
				quit = key_event_action(ev[1].code);
			}

			pthread_mutex_lock(&quit_flag_mutex);
			if (quit_flag)
			{
				quit = true;
			}
			pthread_mutex_unlock(&quit_flag_mutex);
		}

		if (fevdev != -1)
		{
			ioctl(fevdev, EVIOCGRAB, 0); //Release grabbed keyboard
			close(fevdev);
		}
	}


	//Wait for worker thread:
	pthread_join(thr, nullptr);

	//Check for exception from worker thread:
	if (exc_ptr != nullptr)
	{
		try
		{
			std::rethrow_exception(exc_ptr);
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			return(-1);
		}
	}

	return(0);
}


void setup_screen()
{
	//ncurses screen setup:
	initscr();
	clear();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	timeout(20); //ms

	print_interactive_commands();
}

void print_interactive_commands()
{
	printw("raspivoice\n");
	printw("Press key to cycle settings:\n");
	printw("0: Mute [off, on]\n");
	printw("1: Negative image [off, on]\n");
	printw("2: Zoom [off, x2, x4]\n");
	printw("3: Blinders [off, on]\n");
	printw("4: Edge detection [off, 50%%, 100%%]\n");
	printw("5: Threshold [off, 25%%, 50%%, 75%%]\n");
	printw("6: Brightness [low, normal, high]\n");
	printw("7: Contrast [x1, x2, x3]\n");
	printw("8: Foveal mapping [off, on]\n");
	printw("[Backspace]: Restore defaults\n");
	printw("q, [Escape]: Quit\n");
}


bool key_pressed_action(int ch)
{
	//Local copy of options:
	RaspiVoiceOptions rvopt_local;
	pthread_mutex_lock(&rvopt_mutex);
	rvopt_local = rvopt;
	pthread_mutex_unlock(&rvopt_mutex);

	std::stringstream state_str;
	bool quit = false;
	switch (ch)
	{
		case '0':
			rvopt_local.mute = !rvopt_local.mute;
			state_str << ((rvopt_local.mute) ? "muted on": "muted off");
			break;
		case '1':
			rvopt_local.negative_image = !rvopt_local.negative_image;
			state_str << ((rvopt_local.negative_image) ? "negative image on" : "negative image off");
			break;
		case '2':
			rvopt_local.zoom *= 2.0;
			if (rvopt_local.zoom > 4.01)
			{
				rvopt_local.zoom = 1.0;
			}
			state_str << "zoom factor" << rvopt_local.zoom;
			break;
		case '3':
			rvopt_local.blinders = (rvopt_local.blinders == 0) ? (rvopt_local.columns / 4) : 0;
			state_str << (rvopt_local.blinders == 0) ? "blinders off": "blinders on";
			break;
		case '4':
			rvopt_local.edge_detection_opacity += 0.5;
			if (rvopt_local.edge_detection_opacity > 1.01)
			{
				rvopt_local.edge_detection_opacity = 0.0;
			}
			state_str << "edge detection " << rvopt_local.edge_detection_opacity;
			break;
		case '5':
			rvopt_local.threshold += (int)(0.25 * 255);
			if (rvopt_local.threshold > (int)(0.75 * 255)+1)
			{
				rvopt_local.threshold = 0;
			}
			state_str << "threshold " << rvopt_local.threshold;
			break;
		case '6':
			if (rvopt_local.brightness == 0)
			{
				rvopt_local.brightness = 100;
				state_str << "brightness high";
			}
			else if (rvopt_local.brightness > 0)
			{
				rvopt_local.brightness = -100;
				state_str << "brightness low";
			}
			else
			{
				rvopt_local.brightness = 0;
				state_str << "brightness medium";
			}
			break;
		case '7':
			rvopt_local.contrast += 1.0;
			if (rvopt_local.contrast > 3.01)
			{
				rvopt_local.contrast = 1.0;
			}
			state_str << "contrast factor " << rvopt_local.contrast;
			break;
		case '8':
			rvopt_local.foveal_mapping = !rvopt_local.foveal_mapping;
			state_str << (rvopt_local.foveal_mapping ? "foveal mapping on": "foveal mapping off");
			break;
		case KEY_BACKSPACE:
		case KEY_DC:
		case 'J':
		case ',':
		case '.':
			rvopt_local = rvopt_defaults;
			state_str << "default options";
			break;
		case 'q':
		case 27: // ESC key
			pthread_mutex_lock(&quit_flag_mutex);
			quit_flag = true;
			pthread_mutex_unlock(&quit_flag_mutex);
			state_str << "goodbye";
			quit = true;
			break;
	}

	//Speak state_str?
	if (rvopt_local.speak)
	{
		speak(state_str.str());
	}

	//Set new options:
	pthread_mutex_lock(&rvopt_mutex);
	rvopt = rvopt_local;
	pthread_mutex_unlock(&rvopt_mutex);

	return quit;
}


bool key_event_action(int event_code)
{
	int ch = 0;
	switch (event_code)
	{
		case KEY_KP0: ch = '0';	break;
		case KEY_KP1: ch = '1';	break;
		case KEY_KP2: ch = '2';	break;
		case KEY_KP3: ch = '3';	break;
		case KEY_KP4: ch = '4';	break;
		case KEY_KP5: ch = '5';	break;
		case KEY_KP6: ch = '6';	break;
		case KEY_KP7: ch = '7';	break;
		case KEY_KP8: ch = '8';	break;
		case KEY_KP9: ch = '9';	break;
		case KEY_KPDOT: ch = '.';	break;
		case KEY_KPMINUS: ch = '-';	break;
		case KEY_KPPLUS: ch = '+';	break;
		case KEY_KPASTERISK: ch = '*';	break;
		case KEY_KPSLASH: ch = '/';	break;
		case KEY_KPENTER: ch = 13;	break;
		case KEY_KPEQUAL: ch = '=';	break;
		case KEY_KPCOMMA: ch = ',';	break;
		case KEY_KPJPCOMMA: ch = ',';	break;
	}

	bool quit = false;
	if (ch != 0)
	{
		 quit = key_pressed_action(ch);
	}
	
	return quit;
}

void *run_worker_thread(void *arg)
{
	try
	{
		//Copy current options:
		RaspiVoiceOptions rvopt_local;
		pthread_mutex_lock(&rvopt_mutex);
		rvopt_local = rvopt;
		pthread_mutex_unlock(&rvopt_mutex);

		//Init:
		RaspiVoice raspiVoice(rvopt_local);

		bool quit = false;
		while (!quit)
		{
			//Copy any new options:
			pthread_mutex_lock(&rvopt_mutex);
			rvopt_local = rvopt;
			pthread_mutex_unlock(&rvopt_mutex);

			//Read and play one frame:
			raspiVoice.PlayFrame(rvopt_local);

			//Check for quit flag:
			pthread_mutex_lock(&quit_flag_mutex);
			if (quit_flag)
			{
				quit = true;
			}
			pthread_mutex_unlock(&quit_flag_mutex);
		}
	}
	catch (std::runtime_error err)
	{
		exc_ptr = std::current_exception();
		pthread_mutex_lock(&quit_flag_mutex);
		quit_flag = true;
		pthread_mutex_unlock(&quit_flag_mutex);
	}
	catch (std::exception err)
	{
		exc_ptr = std::current_exception();
		pthread_mutex_lock(&quit_flag_mutex);
		quit_flag = true;
		pthread_mutex_unlock(&quit_flag_mutex);
	}

	pthread_exit(nullptr);
}

void daemon_startup(void)
{
	pid_t pid, sid;

	pid = fork();
	if (pid < 0)
	{
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	umask(0);

	sid = setsid();
	if (sid < 0)
	{
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0)
	{
		exit(EXIT_FAILURE);
	}

	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

bool grab_keyboard(std::string bus_device_id)
{
	int device_id = atoi(bus_device_id.c_str());

	std::string devpath("/dev/input/event" + bus_device_id);

	fevdev = open(devpath.c_str(), O_RDONLY);
	if (fevdev == -1)
	{
		return false;
	}

	if (ioctl(fevdev, EVIOCGRAB, 1) != 0) //grab keyboard
	{
		return false;
	}

	return true;
}

bool speak(std::string text)
{
	std::string command = "espeak --stdout \"" + text + "\" | aplay -q -D" + rvopt_defaults.audio_device;
	int res = system(command.c_str());
	return (res == 0);
}
