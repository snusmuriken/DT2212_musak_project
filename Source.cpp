#include <windows.h>
#include <stdio.h>
#include <mmeapi.h>
#include <math.h>

#define pa system("pause")

#pragma comment(lib,"Winmm.lib") 
using namespace std;

HMIDIIN input = NULL;


unsigned short input_id = 0;

bool input_available = true;


union bringer
{
	unsigned int word;
	unsigned char parts[4];
	bringer()
	{
		word = 0;
	}
	bringer(unsigned int in)
	{
		word = in;
	}
	bringer& operator=(unsigned int in)
	{
		word = in;
	}
};



void CALLBACK midiInputCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) 
{
	
	bringer data;
	switch (wMsg) 
	{
	case MIM_MOREDATA:
	case MIM_DATA:
		data = bringer(dwParam1);
		if (data.parts[0] == 144)
			Beep(int(pow(2, (data.parts[1] + 36.3763) / 12)), 100);
		break;
	}
}



void reset_devices()
{
	midiInReset(input);
	midiInClose(input);
}
void open_input_device()
{
	while (midiInOpen(&input, input_id, (DWORD)&midiInputCallback, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR);
	{
		printf("Error opening MIDI in port %d%c", input_id, '\n');
		input_id++;
		if (input_id > 15)
			input_id = 0;

	}
	printf("Opening midi input %d\n", input_id);
	midiInStart(input);
}
/*
int main(int argc, char** argv)
{
	unsigned char indev_count = midiInGetNumDevs();
	input_id = 0;

	if (indev_count > 1)
	{
		printf("Enter midi input device number (0-%d", (indev_count-1));
		scanf_s("%d", &input_id);
	}

		
	reset_devices();
	input_available = midiInGetNumDevs();

	if (input_available)
		open_input_device();
	else
		printf("No input device detected\n");
		
	const int command_max_length = 50;
	char commands[command_max_length];
	memset(commands, 0, command_max_length);

	scanf_s("%50s", commands);
	
	return 0;
}*/