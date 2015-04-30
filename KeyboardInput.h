#pragma once

#include <string>

#include "Options.h"

class KeyboardInput
{
private:
	int fevdev;

	int keyEventMap(int event_code);

public:
	KeyboardInput();

	bool KeyPressedAction(RaspiVoiceOptions &opt, int ch);
	bool GrabKeyboard(std::string bus_device_id);
	void ReleaseKeyboard();
	int ReadKey();
	void PrintInteractiveCommands();
};

