#include <sstream>
#include <ncurses.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Options.h"
#include "KeyboardInput.h"
#include "AudioData.h"

KeyboardInput::KeyboardInput() :
	fevdev(-1)
{
}

bool KeyboardInput::GrabKeyboard(std::string bus_device_id)
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

void KeyboardInput::ReleaseKeyboard()
{
	if (fevdev != -1)
	{
		ioctl(fevdev, EVIOCGRAB, 0); //Release grabbed keyboard
		close(fevdev);
	}
}

bool KeyboardInput::KeyPressedAction(RaspiVoiceOptions &opt, int ch)
{
	std::stringstream state_str;
	bool quit = false;
	switch (ch)
	{
		case '0':
			opt.mute = !opt.mute;
			state_str << ((opt.mute) ? "muted on" : "muted off");
			break;
		case '1':
			opt.negative_image = !opt.negative_image;
			state_str << ((opt.negative_image) ? "negative image on" : "negative image off");
			break;
		case '2':
			opt.zoom *= 2.0;
			if (opt.zoom > 4.01)
			{
				opt.zoom = 1.0;
			}
			state_str << "zoom factor" << opt.zoom;
			break;
		case '3':
			opt.blinders = (opt.blinders == 0) ? (opt.columns / 4) : 0;
			state_str << (opt.blinders == 0) ? "blinders off" : "blinders on";
			break;
		case '4':
			opt.edge_detection_opacity += 0.5;
			if (opt.edge_detection_opacity > 1.01)
			{
				opt.edge_detection_opacity = 0.0;
			}
			state_str << "edge detection " << opt.edge_detection_opacity;
			break;
		case '5':
			opt.threshold += (int)(0.25 * 255);
			if (opt.threshold > (int)(0.75 * 255) + 1)
			{
				opt.threshold = 0;
			}
			state_str << "threshold " << opt.threshold;
			break;
		case '6':
			if (opt.brightness == 0)
			{
				opt.brightness = 100;
				state_str << "brightness high";
			}
			else if (opt.brightness > 0)
			{
				opt.brightness = -100;
				state_str << "brightness low";
			}
			else
			{
				opt.brightness = 0;
				state_str << "brightness medium";
			}
			break;
		case '7':
			opt.contrast += 1.0;
			if (opt.contrast > 3.01)
			{
				opt.contrast = 1.0;
			}
			state_str << "contrast factor " << opt.contrast;
			break;
		case '8':
			opt.foveal_mapping = !opt.foveal_mapping;
			state_str << (opt.foveal_mapping ? "foveal mapping on" : "foveal mapping off");
			break;
		case KEY_BACKSPACE:
		case KEY_DC:
		case 'J':
		case ',':
		case '.':
			opt = GetCommandLineOptions();
			state_str << "default options";
			break;
		case 'q':
		case 27: // ESC key
			state_str << "goodbye";
			quit = true;
			break;
	}

	//Speak state_str?
	if (opt.speak)
	{
		AudioData::Speak(state_str.str(), opt.audio_card);
	}

	return quit;
}

int KeyboardInput::ReadKey()
{
	struct input_event ev[64];

	int rd;
	if ((rd = read(fevdev, ev, sizeof(struct input_event) * 64)) < sizeof(struct input_event))
	{
		return ERR;
	}

	int value = ev[0].value;
	if (value != ' ' && ev[1].value == 1 && ev[1].type == EV_KEY) //value=1: key press, value=0 key release
	{
		return(keyEventMap(ev[1].code));
	}

	return ERR;
}

int KeyboardInput::keyEventMap(int event_code)
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

	return ch;
}


void KeyboardInput::PrintInteractiveCommands()
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