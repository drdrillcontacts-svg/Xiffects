import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import wave

spec = importlib.util.spec_from_file_location('fixtures', Path(__file__).resolve().parents[1] / 'Tools/GenerateReferenceFixtures.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class Fixtures(unittest.TestCase):
    def test_repeatable_and_labelled(self):
        with tempfile.TemporaryDirectory() as tmp:
            a, b = Path(tmp)/'a', Path(tmp)/'b'
            module.generate(a, [48000]); module.generate(b, [48000])
            self.assertEqual((a/'manifest.json').read_bytes(), (b/'manifest.json').read_bytes())
            manifest = json.loads((a/'manifest.json').read_text())
            self.assertEqual(len(manifest['entries']), 16)
            for entry in manifest['entries']:
                data = (a/entry['file']).read_bytes()
                self.assertEqual(hashlib.sha256(data).hexdigest(), entry['sha256'])
                self.assertEqual(data, (b/entry['file']).read_bytes())
                with wave.open(str(a/entry['file'])) as wav:
                    self.assertEqual(wav.getnframes(), entry['samples'])
                    self.assertEqual(wav.getframerate(), 48000)
                    self.assertEqual(wav.getnchannels(), 1)
                    if entry['id'].startswith('silence'):
                        self.assertEqual(set(wav.readframes(wav.getnframes())), {0})
                middle = entry['truth'][100]
                if entry['id'].startswith('detuned_452'):
                    self.assertEqual(middle['hz'], 452)
                if entry['id'].startswith('above_range'):
                    self.assertFalse(middle['in_detector_range'])

if __name__ == '__main__':
    unittest.main(verbosity=2)
