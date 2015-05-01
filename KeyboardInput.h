#pragma once

#include <string>

#include "Options.h"

class KeyboardInput
{
private:
	int fevdev;
	int currentOptionIndex;

	int keyEventMap(int event_code);

	int changeIndex(int i, int maxindex, int changevalue);
	void cycleValues(int &current_value, std::vector<int> value_list, int changevalue);
	void cycleValues(float &current_value, std::vector<float> value_list, int changevalue);
public:
	KeyboardInput();

	void KeyPressedAction(RaspiVoiceOptions &opt, int ch);
	bool GrabKeyboard(std::string bus_device_id);
	void ReleaseKeyboard();
	int ReadKey();
	void PrintInteractiveCommands();
};

