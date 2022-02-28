#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <math.h>
#if defined(_WIN64)
#include <Windows.h>
#else
#include <cstring>
#include <iostream>
#endif

#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define  MIDI_DEVICE  "/dev/snd/midiC1D0"

const double pi = acos(-1);
#define MIDIKEY_COUNT 127
#pragma comment(lib,"Winmm.lib") 

using namespace sf;

union Message
{
	/*
	Message consists of 4 parts:
	- parts[0] is the type and channel
	- parts[1] primary value of the type
	- parts[2] secondary value of the type
	- parts[3] is not used.

	Content of all parts depend on first part.
	*/

	unsigned int word;
	unsigned char parts[4];
	Message()
	{
		word = 0;
	}
	Message(unsigned int in)
	{
		word = in;
	}
	Message& operator=(unsigned int in)
	{
		word = in;
	}
};

class MidiStream : public SoundStream
{
public:
	MidiStream(unsigned int samplerate = 44100, unsigned int channel_count = 1)
	{
		overtones = 1;

		sample_size = 1 << 9;
		samples = new Int16[sample_size];

		active_notes = new char[MIDIKEY_COUNT];
		memset(active_notes, -1, sizeof(char) * MIDIKEY_COUNT);

		initialize(channel_count, samplerate);
	}
	void handle_midi_message(Message message)
	{
		switch ((message.parts[0] >> 4))
		{
		case 0x9: //Note on
			if (message.parts[2] > 0)
				active_notes[message.parts[1]] = message.parts[2];
			else
				active_notes[message.parts[1]] = -1;
			break;
		case 0x8: //Note off
			active_notes[message.parts[1]] = -1;
			break;
		case 0xb: //Control change
			handle_control_change(message.parts[1], message.parts[2]);
			break;
		default:
			break;
		}

	}

	Int16* samples;
	Int64 sample_size;
private:

	struct AmpPhase
	{
		float ampl;
		float phase;
	};
	const sf::Uint32 accurary = 10000;
	std::map<sf::Uint32, AmpPhase> amps_phases; //maps frequencies to amplitudes and phases

	unsigned int overtones;


	char* active_notes; //index is the note, value is the velocity

	void handle_control_change(char control_type, char control_value)
	{
		switch (control_type)
		{
		case 0x1: //Modulation Wheel
			overtones = (int(control_value) * 60) / 127;
			break;
		}
	}

	double getInharmonicityFactor(Int16 n, char velocity) {
		// without string specifics: n * (1 + pow(n, 2) * J)
		// -> J decided by velocity, from lab: B = 0.00082 = 2 * J, J = 0.00041
		// From lab sqrt(1 + pow(n,2) * B)

		double B = 0.0008;
		double max = sqrt(1 + pow(n, 2) * 10 * B);
		double factor = double(velocity) / 127;

		return max * factor;
	}

	bool onGetData(Chunk& data)
	{
		memset(samples, 0, sizeof(Int16) * sample_size);

		for (int k = 0; k < MIDIKEY_COUNT; k++)
		{
			if (active_notes[k] != -1)
			{
				double f = pow(2, (k + 36.3763) / 12);
				double ampl = active_notes[k] * ((1 << 13) / 127);
				sf::Uint32 freq_key = Uint32(f * accurary);
				if (amps_phases.count(freq_key))
					amps_phases[freq_key].ampl += ampl;
				else
					amps_phases[freq_key].ampl = ampl;
				for (int l = 1; l < overtones; l++)
				{
					freq_key = Uint32(f * (l + 1) * accurary);
					if (amps_phases.count(freq_key))
						amps_phases[freq_key].ampl += ampl / l;
					else
						amps_phases[freq_key].ampl = ampl / l;
				}
				/*amps[int(f)] += ampl;
				for (int l = 1; l < overtones; l++)
				{
					amps[int((int(f) * (l + 1)))] += ampl / (l + 1)
				}*/
				

			}
		}
		std::map<Uint32, AmpPhase>::iterator it = amps_phases.begin();
		int counter = 0;
		while(it != amps_phases.end())
		{
			float f = float(it->first) / accurary;
			float& ampl = (it->second).ampl;
			float& phase = (it->second).phase;
			if (abs(ampl) > 0.1)
			{
				for (size_t i = 0; i < sample_size; i++)
					samples[i] += ampl * sin(phase + (f * i * 2 * pi) / getSampleRate());
				
				phase += (2 * pi * f * sample_size) / getSampleRate();
				if (phase > 2 * pi)
					phase -= (2 * pi) * int(phase / (2 * pi));
				ampl = 0;
				++it;
			}
			else 
				amps_phases.erase(it++);
			
		}
		data.sampleCount = sample_size;
		data.samples = samples;
		return true;

	}
	void onSeek(Time timeOffset)
	{

	}

};

int main()
{

	//writeSinTest("D:\\Musik\\sin.wav");
	using namespace std;
	MidiStream song;
	song.play();
	
	Message inpacket;
	// first open the sequencer device for reading.
	int seqfd = open(MIDI_DEVICE, 00);
	
	if (seqfd == -1) {
		printf("Error: cannot open %s\n", MIDI_DEVICE);
		return 1;
	} 

	// now just wait around for MIDI bytes to arrive and print them to screen.
	while (1) {
		read(seqfd, &inpacket, sizeof(inpacket));
	
		song.handle_midi_message(inpacket);
		// print the MIDI byte if this input packet contains one
		//printf("received MIDI byte: %d\n", inpacket.parts[1]);
	}

	RenderWindow window(VideoMode(500, 500), "Wave");
	VertexArray wave;
	wave.resize(500);
	for (int i = 0; i < 500; i++)
		wave[i].position.x = i;
	wave.setPrimitiveType(PrimitiveType::LinesStrip);

	Message base_message;
	base_message.parts[0] = 144;
	base_message.parts[2] = 60;

	Int16 counter = 0;
	while (window.isOpen())
	{
		Event ev;
		while (window.pollEvent(ev))
		{
			switch (ev.type)
			{
			case Event::Closed:
				window.close();
				break;
			case Event::KeyPressed:
				if (ev.key.code == Keyboard::Escape)
				{
					base_message.parts[0] = (0xb << 4);	// Type control
					base_message.parts[1] = 0x1;		// Modulo
					base_message.parts[2] = counter;			// Value of modulo
					//printf("Increase\n");
					
					sleep(sf::seconds(0.001));
					if (counter < 127) counter++;
					else counter = 0;
				}
				else
				{
					base_message.parts[0] = 144;
					base_message.parts[1] = ev.key.code + 20;
					base_message.parts[2] = 100;
					printf("%d\n", ev.key.code + 20);
				}
				song.handle_midi_message(base_message);
				
				break;
			case Event::KeyReleased:
				base_message.parts[0] = 128;
				base_message.parts[1] = ev.key.code + 20;
				song.handle_midi_message(base_message);
				break;
			}
		}

		for (int i = 0; i < 500; i++)
			wave[i].position.y = double(song.samples[i])/(1 << 10) * 10 + 250;
		window.clear();
		window.draw(wave);
		window.display();
	}
	song.stop();
	
}