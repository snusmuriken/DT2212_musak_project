#include <SFML/Audio.hpp>
#include <cstring>
#include <math.h>
#include "fft.h"
const double pi = acos(-1);
using namespace sf;


class MidiStream : public SoundStream
{
public:
	MidiStream(unsigned int samplerate = 44100, unsigned int channel_count = 1)
	{
		sample_size = 1 << 12;
		samples = new Int16[sample_size];
		double base_f = 100;
		freq_count = 100;

		freqs = new double[freq_count];
		phases = new double[freq_count];
		amps = new double[freq_count];
		memset(freqs, 0, sizeof(double) * freq_count);
		memset(phases, 0, sizeof(double) * freq_count);
		memset(amps, 0, sizeof(double) * freq_count);

		
		initialize(channel_count, samplerate);
	}

private:
	Int16* samples;
	Int64 sample_size;

	unsigned int freq_count;
	double* freqs;
	double* phases;
	double* amps;

	bool onGetData(Chunk& data)
	{
		static int counter = 0;
		memset(samples, 0, sizeof(Int16) * sample_size);
		for (int f_i = 0; f_i < freq_count; f_i++)
		{
			if (abs(freqs[f_i]) < 1)
				break;
			for (size_t i = 0; i < sample_size; i++)
			{
				samples[i] += amps[f_i] * std::sin(phases[f_i] + (2 * pi * freqs[f_i] * i) / getSampleRate());
			}
			phases[f_i] += (2 * pi * freqs[f_i] * sample_size) / getSampleRate();
			if (phases[f_i] > 2 * pi)
				phases[f_i] -= (2 * pi) * int(phases[f_i] / (2 * pi));
		}
		for (unsigned int i = 0; i < counter; i++)
		{
			freqs[i] = 100 * (i + 1);
			amps[i] = (1 << 14) / (i + 1);
		}
		counter++;
		if (counter > freq_count)
			counter--;

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

	while (true);
	song.stop();
	
}