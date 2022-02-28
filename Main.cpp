#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <math.h>
#if defined(_WIN64)
#include <Windows.h>
#else
#include <cstring>
#include <iostream>
#endif
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
		return *this;
	}
};

class MidiStream : public SoundStream
{
public:
	MidiStream(unsigned int samplerate = 44100, unsigned int channel_count = 1)
	{
		sample_size = 1 << 9;
		samples = new Int16[sample_size];
		freq_count = samplerate;

		phases = new float[freq_count];
		memset(phases, 0, sizeof(float) * freq_count);

		amps = new float[freq_count];
		memset(amps, 0, sizeof(float) * freq_count);

		active_notes = new char[MIDIKEY_COUNT];
		memset(active_notes, -1, sizeof(char) * MIDIKEY_COUNT);

		initialize(channel_count, samplerate);
	}
	void handle_midi_message(Message message)
	{
		if ((message.parts[0] >> 4) == 0x9)
		{
			if (message.parts[2] > 0)
				active_notes[message.parts[1]] = message.parts[2];
			else
				active_notes[message.parts[1]] = -1;
		}
		else if ((message.parts[0] >> 4) == 0x8)
			active_notes[message.parts[1]] = -1;
	}

	Int16* samples;
	Int64 sample_size;
private:
	unsigned int freq_count;
	float* amps;
	float* phases;


	char* active_notes; //index is the note, value is the velocity

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
		unsigned int overtones = 30;
		memset(samples, 0, sizeof(Int16) * sample_size);
		memset(amps, 0, sizeof(float) * freq_count);

		for (int k = 0; k < MIDIKEY_COUNT; k++)
		{
			if (active_notes[k] != -1)
			{
				double f = pow(2, (k + 36.3763) / 12);
				double ampl = active_notes[k] * ((1 << 13) / 127);
				amps[int(f)] += ampl;
				for (int l = 1; l < overtones; l++)
					amps[int((int(f) * (l + 1)) * getInharmonicityFactor(l + 1, active_notes[k]))] += ampl / (l + 1);
				

			}
		}
		for (unsigned int f = 0; f < freq_count; f++)
		{
			if (abs(amps[f]) > 0.1)
			{
				double freq = f;
				for (size_t i = 0; i < sample_size; i++)
				{
					samples[i] += amps[f] * sin(phases[f] + (freq * i * 2 * pi) / getSampleRate());
				}
				phases[f] += (2 * pi * freq * sample_size) / getSampleRate();
				if (phases[f] > 2 * pi)
					phases[f] -= (2 * pi) * int(phases[f] / (2 * pi));
			}
			else
				phases[f] = 0;

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
	base_message.parts[2] = 60;
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