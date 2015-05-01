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

#include "printtime.h"
#include "Options.h"
#include "RaspiVoice.h"
#include "KeyboardInput.h"

void *run_worker_thread(void *arg);
void setup_screen(void);
void daemon_startup(void);
void main_loop(KeyboardInput &keyboardInput, bool use_ncurses);

RaspiVoiceOptions rvopt;
pthread_mutex_t rvopt_mutex;

std::exception_ptr exc_ptr;

int main(int argc, char *argv[])
{
	RaspiVoiceOptions cmdline_opt;
	KeyboardInput keyboardInput;

	if (!SetCommandLineOptions(argc, argv))
	{
		return -1;
	}

	cmdline_opt = GetCommandLineOptions();

	if (cmdline_opt.daemon)
	{
		std::cout << "raspivoice daemon started." << std::endl;
		daemon_startup();
	}

	//Start Program in worker thread:
	rvopt = cmdline_opt;
	//Warning: Do not read or write rvopt or quit_flag without locking after this.
	pthread_t thr;
	pthread_mutex_init(&rvopt_mutex, NULL);
	if (pthread_create(&thr, NULL, run_worker_thread, NULL))
	{
		std::cerr << "Error setting up thread." << std::endl;
		return -1;
	}

	if ((!cmdline_opt.verbose) && (!cmdline_opt.daemon))
	{
		//Show interactive screen:
		setup_screen();
		keyboardInput.PrintInteractiveCommands();

		main_loop(keyboardInput, true); //with ncurses

		//quit ncurses:
		refresh();
		endwin();
	}
	else if ((cmdline_opt.verbose) && (!cmdline_opt.daemon))
	{
		//Show debug messages:
		std::cout << "Verbose mode on, interactive mode disabled." << std::endl;
		std::cout << "Press Enter to quit." << std::endl << std::endl;
		getchar();

		pthread_mutex_lock(&rvopt_mutex);
		rvopt.quit = true;
		pthread_mutex_unlock(&rvopt_mutex);
	}
	else if (cmdline_opt.grab_keyboard != "")
	{
		if (!cmdline_opt.daemon)
		{
			std::cerr << "Grab keyboard device is only possible in daemon mode." << std::endl;
			return -1;
		}

		if (!keyboardInput.GrabKeyboard(cmdline_opt.grab_keyboard))
		{
			std::cerr << "Cannot grab keyboard device: " << cmdline_opt.grab_keyboard << "." << std::endl;
			return -1;
		}

		main_loop(keyboardInput, false); //without ncurses

		keyboardInput.ReleaseKeyboard();
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
}

void main_loop(KeyboardInput &keyboardInput, bool use_ncurses)
{
	RaspiVoiceOptions rvopt_local;
	rvopt_local.quit = false;

	while (!rvopt_local.quit)
	{
		int ch;

		if (use_ncurses)
		{
			ch = getch();
		}
		else
		{
			ch = keyboardInput.ReadKey();
		}

		if (ch != ERR)
		{
			//Local copy of options:
			pthread_mutex_lock(&rvopt_mutex);
			rvopt_local = rvopt;
			pthread_mutex_unlock(&rvopt_mutex);

			keyboardInput.KeyPressedAction(rvopt_local, ch);

			//Set new options:
			pthread_mutex_lock(&rvopt_mutex);
			if (rvopt.quit)
			{
				rvopt_local.quit = true;
			}
			rvopt = rvopt_local;
			pthread_mutex_unlock(&rvopt_mutex);
		}
	}
}

void *run_worker_thread(void *arg)
{
	//Copy current options:
	RaspiVoiceOptions rvopt_local;
	pthread_mutex_lock(&rvopt_mutex);
	rvopt_local = rvopt;
	pthread_mutex_unlock(&rvopt_mutex);

	try
	{
		//Init:
		RaspiVoice raspiVoice(rvopt_local);

		while (!rvopt_local.quit)
		{
			//Copy any new options:
			pthread_mutex_lock(&rvopt_mutex);
			rvopt_local = rvopt;
			pthread_mutex_unlock(&rvopt_mutex);

			//Read and play one frame:
			raspiVoice.PlayFrame(rvopt_local);
		}
	}
	catch (std::runtime_error err)
	{
		exc_ptr = std::current_exception();
		if (rvopt_local.verbose)
		{
			std::cout << err.what() << std::endl;
		}
		pthread_mutex_lock(&rvopt_mutex);
		rvopt.quit = true;
		pthread_mutex_unlock(&rvopt_mutex);
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
