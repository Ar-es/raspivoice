#include <sstream>
#include <ncurses.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <vector>
#include <stdexcept>

#include "Options.h"
#include "KeyboardInput.h"
#include "AudioData.h"

KeyboardInput::KeyboardInput() :
	fevdev(-1),
	currentOptionIndex(0)
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


int KeyboardInput::ReadKey()
{
	if (fevdev == -1)
	{
		return ERR;
	}
	
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


int KeyboardInput::changeIndex(int i, int maxindex, int changevalue)
{
	int new_index = 0;

	if (changevalue == -1)
	{
		new_index = i - 1;
		if (new_index < 0)
		{
			new_index = 0;
		}
	}
	else if (changevalue == 1)
	{
		new_index = i + 1;
		if (new_index > maxindex)
		{
			new_index = maxindex;
		}
	}
	else if (changevalue == 2)
	{
		new_index = i + 1;
		if (new_index > maxindex)
		{
			new_index = 0;
		}
	}
	else if (changevalue == -2)
	{
		new_index = i - 1;
		if (new_index < 0)
		{
			new_index = maxindex;
		}
	}

	return new_index;
}

void KeyboardInput::cycleValues(float &current_value, std::vector<float> value_list, int changevalue)
{
	for (int i = 0; i < value_list.size(); i++)
	{
		if (fabs(current_value - value_list[i]) < 1e-10)
		{
			current_value = value_list[changeIndex(i, value_list.size() - 1, changevalue)];
			return;
		}
	}
	current_value = value_list[0];
}

void KeyboardInput::cycleValues(int &current_value, std::vector<int> value_list, int changevalue)
{
	for (int i = 0; i < value_list.size(); i++)
	{
		if (current_value == value_list[i])
		{
			current_value = value_list[changeIndex(i, value_list.size() - 1, changevalue)];
			return;
		}
	}
	current_value = value_list[0];
}

void KeyboardInput::KeyPressedAction(RaspiVoiceOptions &opt, int ch)
{
	int option_cycle[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '+', '-', '.', 'q' };

	bool option_changed = false;
	int changevalue = 2; //-1: decrease value, 0: no change, 1: increase value, 2: cycle values
	std::stringstream state_str;
	AudioData audioData(opt.audio_card);

	//Menu navigation keys:
	switch (ch)
	{
		case 'a': //previous value
			changevalue = -1;
			break;
		case 's': //next option
			option_changed = true;
			if (currentOptionIndex > 0)
			{
				currentOptionIndex--;
			}
			break;
		case 'd': //next value
			changevalue = 1;
			break;
		case 'w': //previous option
			option_changed = true;
			if (currentOptionIndex < (sizeof(option_cycle)-1))
			{
				currentOptionIndex++;
			}
			break;
	}

	if (option_changed)
	{
		switch (option_cycle[currentOptionIndex])
		{
			case '0':
				state_str << "0: mute";
				break;
			case '1':
				state_str << "1: negative image";
				break;
			case '2':
				state_str << "2: zoom";
				break;
			case '3':
				state_str << "3: blinders";
				break;
			case '4':
				state_str << "4: edge detection";
				break;
			case '5':
				state_str << "5: threshold";
				break;
			case '6':
				state_str << "6: brightness";
				break;
			case '7':
				state_str << "7: contrast";
				break;
			case '8':
				state_str << "8: foveal mapping";
				break;
			case '+':
				state_str << "plus: volume up";
				break;
			case '-':
				state_str << "minus: volume down";
				break;
			case ',':
			case '.':
			case 263: //Backspace
				state_str << "period: default options";
				break;
			case 'q':
			case 27: //ESC
				state_str << "escape: quit";
				break;
		}
	}
	else
	{
		if ((changevalue == -1) || (changevalue == 1))
		{
			//use current option if navigation keys were used:
			ch = option_cycle[currentOptionIndex];
		}
		
		//change value:
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
				cycleValues(opt.zoom, {1.0, 2.0, 4.0}, changevalue);
				state_str << "zoom factor" << opt.zoom;
				break;
			case '3':
				opt.blinders = (opt.blinders == 0) ? (opt.columns / 4) : 0;
				state_str << (opt.blinders == 0) ? "blinders off" : "blinders on";
				break;
			case '4':
				cycleValues(opt.edge_detection_opacity, { 0.0, 0.5, 1.0 }, changevalue);
				state_str << "edge detection " << opt.edge_detection_opacity;
				break;
			case '5':
				cycleValues(opt.threshold, { 0, int(0.25*255), int(0.5*255), int(0.75*255) }, changevalue);
				state_str << "threshold " << opt.threshold;
				break;
			case '6':
				cycleValues(opt.brightness, { -100, 0, 100 }, changevalue);
				if (opt.brightness == 100)
				{
					state_str << "brightness high";
				}
				else if (opt.brightness == 0)
				{
					state_str << "brightness medium";
				}
				else
				{
					state_str << "brightness low";
				}
				break;
			case '7':
				cycleValues(opt.contrast, { 1.0, 2.0, 3.0 }, changevalue);
				state_str << "contrast factor " << opt.contrast;
				break;
			case '8':
				opt.foveal_mapping = !opt.foveal_mapping;
				state_str << (opt.foveal_mapping ? "foveal mapping on" : "foveal mapping off");
				break;
			case '+':
				cycleValues(opt.volume, { 1, 2, 4, 8, 16, 32, 64, 100 }, (changevalue > 0) ? 1: -1);
				audioData.SetVolume(opt.volume);
				state_str << "Volume up ";
				break;
			case '-':
				cycleValues(opt.volume, { 1, 2, 4, 8, 16, 32, 64, 100 }, (changevalue > 0) ? -1 : 1);
				audioData.SetVolume(opt.volume);
				state_str << "Volume down ";
				break;
			case ',':
			case '.':
			case 263:
				opt = GetCommandLineOptions();
				state_str << "default options";
				break;
			case 'q':
			case 27: // ESC key
				state_str << "goodbye";
				opt.quit = true;
				break;
		}
	}

	//Speak state_str?
	if ((opt.speak) && (state_str.str() != ""))
	{
		if (!audioData.Speak(state_str.str()))
		{
			throw(std::runtime_error("Error calling Speak(). Use verbose mode for more info."));
		}
	}

	return;
}


int KeyboardInput::keyEventMap(int event_code)
{
	int ch = 0;
	switch (event_code)
	{
		case KEY_A:
		case KEY_LEFT:
		case KEY_BACK:
		case KEY_STOP:
			ch = 'a';
			break;
		case KEY_S:
		case KEY_DOWN:
		case KEY_PREVIOUSSONG:
		case KEY_REWIND:
			ch = 's';
			break;
		case KEY_D:
		case KEY_RIGHT:
		case KEY_PLAYPAUSE:
		case KEY_PLAY:
		case KEY_PAUSE:
			ch = 'd';
			break;
		case KEY_W:
		case KEY_UP:
		case KEY_FORWARD:
		case KEY_NEXTSONG:
			ch = 'w';
			break;
		case KEY_0:
		case KEY_KP0:
		case KEY_MUTE:
		case KEY_NUMERIC_0:
			ch = '0';
			break;
		case KEY_1:
		case KEY_KP1:
			ch = '1';
			break;
		case KEY_2:
		case KEY_KP2:
			ch = '2';
			break;
		case KEY_3:
		case KEY_KP3:
			ch = '3';
			break;
		case KEY_4:
		case KEY_KP4:
			ch = '4';
			break;
		case KEY_5:
		case KEY_KP5:
			ch = '5';
			break;
		case KEY_6:
		case KEY_KP6:
			ch = '6';
			break;
		case KEY_7:
		case KEY_KP7:
			ch = '7';
			break;
		case KEY_8:
		case KEY_KP8:
			ch = '8';
			break;
		case KEY_9:
		case KEY_KP9:
			ch = '9';
			break;
		case KEY_NUMERIC_STAR:
		case KEY_KPASTERISK:
			ch = '*';
			break;
		case KEY_SLASH:
		case KEY_KPSLASH:
			ch = '/';
			break;
		case KEY_LINEFEED:
		case KEY_KPENTER:
			ch = 13;
			break;
		case KEY_EQUAL:
		case KEY_KPEQUAL:
			ch = '=';
			break;
		case KEY_DOT:
		case KEY_KPDOT:
		case KEY_KPCOMMA:
		case KEY_KPJPCOMMA:
		case KEY_BACKSPACE:
		case KEY_DC:
			ch = '.';
			break;
		case KEY_KPPLUS:
		case KEY_VOLUMEUP:
			ch = '+';
			break;
		case KEY_MINUS:
		case KEY_KPMINUS:
		case KEY_VOLUMEDOWN:
			ch = '-';
			break;
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
	printw(".: Restore defaults\n");
	printw("q, [Escape]: Quit\n");
}

