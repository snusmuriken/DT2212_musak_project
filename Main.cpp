#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <math.h>
#if defined(_WIN64)
#include <Windows.h>
#else
#include <cstring>
#include <iostream>
#endif

#include <queue>
#include <vector>

#include "fft.h"
#include <time.h>
#include <functional> 

#include "RtMidi.h"
/*
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define  MIDI_DEVICE  "/dev/snd/midiC1D0"
*/

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
		//overtones = 1;
		inharmonicity = 0;
		sampleSize = 1 << 9;
		samples = new Int16[sampleSize];

		activeNotes = new char[MIDIKEY_COUNT];
		memset(activeNotes, -1, sizeof(char) * MIDIKEY_COUNT);

		prevNoteStatus = new char[MIDIKEY_COUNT];
		memset(prevNoteStatus, -1, sizeof(char) * MIDIKEY_COUNT);

		initialize(channel_count, samplerate);
	}
	
	void handleMidiMessage(Message message){
		/// Read the incoming MIDI message and set active notes 
		///(to be played) according to its contents.
		switch ((message.parts[0] >> 4))
		{
		case 0x9: //Note on
			if (message.parts[2] > 0)
				activeNotes[message.parts[1]] = message.parts[2];
			else
				activeNotes[message.parts[1]] = -1;
			break;
		case 0x8: //Note off
			activeNotes[message.parts[1]] = -1;
			break;
		case 0xb: //Control change
			handleControlChange(message.parts[1], message.parts[2]);
			break;
		default:
			break;
		}

	}

	Int16* samples;
	Int64 sampleSize;
private:

	struct AmpPhase
	{
		float ampl;
		float phase;
	};
	const sf::Uint32 accurary = 10000;
	
	//maps frequencies to amplitudes and phases to avoid getting clicks in playback
	std::map<sf::Uint32, AmpPhase> ampsPhases; 

	float inharmonicity;
	//unsigned int overtones;

	char* activeNotes; //index is the note, value is the velocity
	char* prevNoteStatus; // Keeps track of whether the note was on and off before

	void handleControlChange(char control_type, char control_value) {
		/// Handles a change of the modulation wheel, increasing inharmonicity.
		switch (control_type)
		{
		case 0x1: // Modulation Wheel
			//overtones = (int(control_value) * 40) / 127;
			inharmonicity = 0.001 * double(control_value) / 127;
			break;
		}
	}
	
	double getAWeight(double f) {
		/// Weights the ouput amplitude according to frequency.
		double f_sq = f * f;
		return (12194 * 12194 *  f_sq * f_sq) / (
				(f_sq + 20.6 * 20.6) * sqrt((f_sq + 107.7 * 107.7) * (f_sq + 737.9 * 737.9)) * (f_sq + 12194 * 12194));
	}

	double getInharmonicityFactor(Int16 n, char velocity) {
		/// Depending on velocity of keypress, create corrseponding inharmonicity factor.
		/*double inharmony = 1;
		if (double(velocity) > 127/2){
			double factor = double(velocity) / 127;
			double B = factor * 0.001;
			inharmony = sqrt(1 + pow(n, 2) * 2 * inharmonicity);	// See Askenfelt Strings eq. 25
		}

		return inharmony;*/
		return sqrt(1 + pow(n, 2) * 2 * inharmonicity);
	}

	// THIS FUNCTION GENERATES THE SOUND
	bool onGetData(Chunk& data) {
		memset(samples, 0, sizeof(Int16) * sampleSize);

		for (int k = 0; k < MIDIKEY_COUNT; k++) {
			if (activeNotes[k] != -1) {	// If -1 the note off
				int overtones = activeNotes[k] > 45 ? (activeNotes[k] - 45)/2.5 : 0;
				double f = pow(2, (k + 36.3763) / 12);
				double ampl = activeNotes[k] * ((1 << 13) / 127);
				
				double maxAmpl = 1;
				for (int l = 1; l < overtones; l++)
					maxAmpl += 1/pow(l + 1, 1.5);
				

				sf::Uint32 freqKey = Uint32(f * accurary);
				if (ampsPhases.count(freqKey))
					ampsPhases[freqKey].ampl += ampl / maxAmpl;
				else
					ampsPhases[freqKey].ampl = ampl / maxAmpl;
				for (int l = 1; l < overtones; l++) {
					double inharmonicityFactor = getInharmonicityFactor(l + 1, activeNotes[k]);
					double fN = f * inharmonicityFactor * (l + 1);
					if (fN > getSampleRate() / 2) break;
					else {
						freqKey = Uint32(fN * accurary);
						if (ampsPhases.count(freqKey))
							ampsPhases[freqKey].ampl += ampl / (pow(l + 1, 1.5) * maxAmpl);
						else
							ampsPhases[freqKey].ampl = ampl / (pow(l + 1, 1.5) * maxAmpl);
					}
				}
			}
			else if (activeNotes[k] == -1 && prevNoteStatus[k] != -1) {
				
			}
			
		}
		std::map<Uint32, AmpPhase>::iterator it = ampsPhases.begin();
		float maxAmpl = 0;
		for(; it != ampsPhases.end(); ++it)
		{
			if(abs((it->second).ampl) > maxAmpl)
				maxAmpl = (it->second).ampl;
		}
		it = ampsPhases.begin();
		int counter = 0;
		while(it != ampsPhases.end())
		{
			float f = float(it->first) / accurary;
			float& ampl = (it->second).ampl;
			float& phase = (it->second).phase;
			if (abs(ampl) > 0.1)
			{
				for (size_t i = 0; i < sampleSize; i++)
					samples[i] += (ampl* (1 << 12) /maxAmpl ) * sin(phase + (f * i * 2 * pi) / getSampleRate());
				
				phase += (2 * pi * f * sampleSize) / getSampleRate();
				if (phase > 2 * pi)
					phase -= (2 * pi) * int(phase / (2 * pi));
				ampl = 0;
				++it;
			}
			else 
				ampsPhases.erase(it++);
			
		}
		data.sampleCount = sampleSize;
		data.samples = samples;
		return true;

	}
	void onSeek(Time timeOffset)
	{

	}

};

void getSignalFFT(MidiStream* song_p) {
	MidiStream& song = *song_p;

	const unsigned int win_size = 512; 

	RenderWindow window(VideoMode(win_size + 20, win_size), "Wave");
	VertexArray wave;
	wave.resize(512);
	for (int i = 0; i < 512; i++)
		wave[i].position.x = 2 * i + 10;
	wave.setPrimitiveType(PrimitiveType::LinesStrip);

	std::complex<double>* compArr = new std::complex<double>[win_size];

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
			default:
				break;
			}
		}

		

		for (int i = 0; i < win_size; i++) {
			compArr[i] = song.samples[i];
		}

		fft(compArr, win_size, 1);

		for (int i = 0; i < win_size/2; i++)
		{
			wave[i].position.y = -log(abs(compArr[i])) * 10 + win_size/2;
		}
		
		window.clear();
		window.draw(wave);
		window.display();
	}
	song.stop();
}

/*
func decay(val) {
	if (val < thresh) return;
	else {
		return
	}
}
*/

int main()
{
	using namespace std;
	Message inpacket;	// Initialize MIDI message
	queue<Message> inputQueue;
	MidiStream song;	// Initialize an object of class MidiStream, defined above
	
	// Start playing the stream whose content will be updated 
	// by handling MIDI messages in onGetData
	song.play();
	
	// Create thread that will handle the song
	sf::Thread midiVisTh(std::bind(&getSignalFFT, &song));
	midiVisTh.launch();
	
	RtMidiIn *midiin = new RtMidiIn();	// Creates the MIDI device
	std::cout << "Midi input devices available: " << midiin->getPortCount() << endl;
  	midiin->openPort(1);

  	std::vector<unsigned char> message;
	int nBytes, i;
  	double stamp;

	// Start loop that handles incoming MIDI messages  
	while(true)
	{
		stamp = midiin->getMessage( &message );
		if(message.size() == 3)		// Only handle standard messages
		{
			inpacket.parts[0] = message[0];
			inpacket.parts[1] = message[1];
			inpacket.parts[2] = message[2];

			// Go to our own function that handles MIDI messages.
			song.handleMidiMessage(inpacket);
		}
		
		sleep(sf::milliseconds(10));
	}

  	

	/*
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
	*/
	
}
