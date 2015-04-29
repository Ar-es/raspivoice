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
#include <pthread.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>

#include "printtime.h"
#include "Options.h"
#include "RaspiVoice.h"

void *run_worker_thread(void *arg);
void setup_screen(void);
void print_interactive_commands(void);
bool key_pressed_action(int ch);
bool key_event_action(int event_code);
void daemon_startup(void);
bool grab_keyboard(std::string bus_device_id);

RaspiVoiceOptions rvopt, rvopt_defaults;
pthread_mutex_t rvopt_mutex;
int fevdev = -1;

bool quit_flag = false;
pthread_mutex_t quit_flag_mutex;
std::exception_ptr exc_ptr;

int main(int argc, char *argv[])
{
	if (!GetCommandLineOptions(rvopt, argc, argv))
	{
		return -1;
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
		AudioData::Speak(state_str.str(), rvopt_local.audio_card);
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
