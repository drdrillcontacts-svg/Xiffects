import importlib.util
import math
from pathlib import Path
import tempfile
import unittest
import wave
import numpy as np

spec = importlib.util.spec_from_file_location('analysis', Path(__file__).resolve().parents[1]/'Tools/ValidateReferenceWindows.py')
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)

class OfflineAnalysis(unittest.TestCase):
    def test_broadband_peak_and_wrong_octave(self):
        sr=48000
        t=np.arange(sr//2)/sr
        for hz in [100, 200.3, 440, 880, 1700]:
            measured=analysis.dominant(.2*np.sin(2*np.pi*hz*t),sr)
            self.assertLess(abs(1200*math.log2(measured/hz)),.1)
        octave=analysis.dominant(.2*np.sin(2*np.pi*880*t),sr)
        self.assertAlmostEqual(1200*math.log2(octave/440),1200,delta=.1)
        self.assertIsNone(analysis.dominant(np.zeros(480),sr))

    def test_signed_24_bit_pcm(self):
        with tempfile.TemporaryDirectory() as tmp:
            file=Path(tmp)/'test.wav'
            samples=[-8388608,-100,0,100,8388607]
            data=b''.join((v & 0xffffff).to_bytes(3,'little') for v in samples)
            with wave.open(str(file),'wb') as output:
                output.setnchannels(1);output.setsampwidth(3);output.setframerate(48000)
                output.writeframes(data)
            sr,actual=analysis.read_pcm(file)
            self.assertEqual(sr,48000)
            np.testing.assert_array_equal(actual,np.array(samples)/8388608)

if __name__=='__main__':
    unittest.main(verbosity=2)
