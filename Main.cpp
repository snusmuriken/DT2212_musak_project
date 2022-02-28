#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <math.h>
#include <Windows.h>
#include <map>
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
		overtones = 1;

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
	unsigned int freq_count;
	float* amps;
	float* phases;

	struct AmpAhase
	{
		float ampl;
		float phase;
	};
	const sf::Uint32 accurary = 10000;
	std::map<sf::Uint32, AmpAhase> amps_phases; //maps frequencies to amplitudes and phases

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
		memset(amps, 0, sizeof(float) * freq_count);

		for (int k = 0; k < MIDIKEY_COUNT; k++)
		{
			if (active_notes[k] != -1)
			{
				double f = pow(2, (k + 36.3763) / 12);
				double ampl = active_notes[k] * ((1 << 13) / 127);
				sf::Uint32 freq_key = Uint32(f * accurary);
				if (amps_phases.count(freq_key))
					amps_phases[freq_key].ampl += ampl;
				//amps[int(f)] += ampl;
				//for (int l = 1; l < overtones; l++)
				//	amps[int((int(f) * (l + 1)) /** getInharmonicityFactor(l + 1, active_notes[k])*/)] += ampl / (l + 1);
				

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
				if (ev.key.code == Keyboard::Escape)
				{
					base_message.parts[0] = (0xb << 4);
					base_message.parts[1] = 0x1;
					base_message.parts[2] = 50;
					printf("Increase\n");
					
				}
				else
				{
					base_message.parts[0] = 144;
					base_message.parts[1] = ev.key.code + 20;
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