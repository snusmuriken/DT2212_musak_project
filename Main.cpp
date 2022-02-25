#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <math.h>
#include <Windows.h>
#include "fft.h"
const double pi = acos(-1);
#define MIDIKEY_COUNT 127
#pragma comment(lib,"Winmm.lib") 

using namespace sf;

union Message
{
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
		sample_size = 1 << 10;
		samples = new Int16[sample_size];
		double base_f = 100;
		freq_count = 100;

		freqs = new double[freq_count];
		phases = new double[freq_count];
		amps = new double[freq_count];
		memset(freqs, 0, sizeof(double) * freq_count);
		memset(phases, 0, sizeof(double) * freq_count);
		memset(amps, 0, sizeof(double) * freq_count);

		active_notes = new char[MIDIKEY_COUNT];
		memset(active_notes, -1, sizeof(char) * MIDIKEY_COUNT);

		initialize(channel_count, samplerate);
	}
	void handle_midi_message(Message message)
	{
		Message data(message);

		
		if ((data.parts[0] >> 4) == 0x9)
		{
			if (data.parts[2] > 0)
				active_notes[data.parts[1]] = data.parts[2];
			else
				active_notes[data.parts[1]] = -1;
		}
		else if ((data.parts[0] >> 4) == 0x8)
			active_notes[data.parts[1]] = -1;
	}

	Int16* samples;
	Int64 sample_size;
private:
	
	unsigned int freq_count;
	double* freqs;
	double* phases;
	double* amps;


	char* active_notes; //index is the note, value is the velocity

	bool onGetData(Chunk& data)
	{
		Int16 last_ampl = samples[sample_size - 1];
		memset(samples, 0, sizeof(Int16) * sample_size);
		for (int i = 0; i < MIDIKEY_COUNT; i++)
		{
			if (active_notes[i] != -1)
			{
				double f = pow(2, (i + 36.3763) / 12);
				double ampl = (active_notes[i] * (1 << 13)) / 127;
				for (size_t i = 0; i < sample_size; i++)
					samples[i] += ampl * sin((2 * pi * f * i) / getSampleRate());
			}
		}
		/*
		for (int f_i = 0; f_i < freq_count; f_i++)
		{
			if (abs(freqs[f_i]) < 1)
				break;
			for (size_t i = 0; i < sample_size; i++)
			{
				samples[i] += amps[f_i] * sin(phases[f_i] + (2 * pi * freqs[f_i] * i) / getSampleRate());
			}
			phases[f_i] = phases[f_i] + (2 * pi * freqs[f_i] * sample_size) / getSampleRate();
			if (phases[f_i] > 2 * pi)
				phases[f_i] -= (2 * pi) * int(phases[f_i] / (2 * pi));
		}*/
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
	cout << "created" << endl;
	cout << "opened" << endl;
	song.play();
	cout << "started" << endl;



	RenderWindow window(VideoMode(500, 500), "Wave");
	VertexArray wave;
	wave.resize(500);
	for (int i = 0; i < 500; i++)
		wave[i].position.x = i;
	wave.setPrimitiveType(PrimitiveType::LinesStrip);

	Message base_message;
	base_message.parts[0] = 144;
	base_message.parts[2] = 100;
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
				base_message.parts[0] = 144;
				base_message.parts[1] = ev.key.code + 20;
				printf("%d\n", ev.key.code + 20);
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