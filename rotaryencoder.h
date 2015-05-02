/*
based on:
rotaryencoder by astine
http://theatticlight.net/posts/Reading-a-Rotary-Encoder-from-a-Raspberry-Pi/
https://github.com/astine/rotaryencoder
*/

#pragma once

//17 pins / 2 pins per encoder = 8 maximum encoders
#define max_encoders 8

struct encoder
{
	int pin_a;
	int pin_b;
	volatile long value;
	volatile int lastEncoded;
};

extern struct encoder encoders[max_encoders];

/*
Should be run for every rotary encoder you want to control
Returns a pointer to the new rotary encoder structer
The pointer will be NULL is the function failed for any reason
*/
struct encoder *setupencoder(int pin_a, int pin_b);

void updateEncoders(void);
