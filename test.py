import pyaudio as pa
import numpy as np

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
    buffer = np.frombuffer(in_data, dtype = np.int16)
    buffer.flags.writable = True
    for i in range(1, len(buffer)):
        buffer[i] += 0.8 * buffer[i-1] 
    stream_out.write(buffer.tobytes())