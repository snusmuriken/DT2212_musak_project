import pyaudio as pa


FORMAT = pa.paInt16
CHANNELS = 1
RATE = 44100
CHUNK = 4096

audio_in = pa.PyAudio()
audio_out = pa.PyAudio()

stream_out = audio_out.open(format=FORMAT, channels=CHANNELS, rate=RATE, output=True, frames_per_buffer=CHUNK)
stream_in = audio_in.open(format=FORMAT, channels=CHANNELS, rate=RATE, input=True, frames_per_buffer=CHUNK)

while True:
    in_data = stream_in.read(num_frames=CHUNK)
    stream_out.write(in_data)