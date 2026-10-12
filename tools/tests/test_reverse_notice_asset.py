"""Check the actual firmware assets, including the former 8-second repeat regression."""
import io
import sys
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import pack_reverse_notice as pack


def silent_wav(seconds):
    stream = io.BytesIO()
    with wave.open(stream, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(16000)
        wav.writeframes(b"\0\0" * round(seconds * 16000))
    return stream.getvalue()


class ReverseNoticeAssetTest(unittest.TestCase):
    def test_one_phrase_duration_and_format(self):
        raw = (ROOT / "assets/audio/reverse_notice.wav").read_bytes()
        self.assertAlmostEqual(pack.validate_notice_wav(raw), 2.085, places=3)

    def test_old_multi_phrase_duration_rejected(self):
        with self.assertRaisesRegex(ValueError, "ONE phrase"):
            pack.validate_notice_wav(silent_wav(7.94))

    def test_language_packs_contain_new_recording_and_preserve_other_tones(self):
        voice = (ROOT / "assets/audio/reverse_notice.wav").read_bytes()
        for language in ("tone_zh", "tone_en"):
            with self.subTest(language=language):
                base = ROOT / f"SDK/cpu/br33/tools/{language}_dashboard.cfg"
                _, original, _ = pack.read_pack(base.read_bytes())
                expected = {e[7].split(b"\0")[0]: data for e, data in original}
                for directory in (ROOT / "output", base.parent):
                    binary = (directory / f"{language}.cfg").read_bytes()
                    _, entries, _ = pack.read_pack(binary)  # all SDK header/data CRCs
                    actual = {e[7].split(b"\0")[0]: data for e, data in entries}
                    self.assertEqual(actual.pop(b"reverse.wav"), voice)
                    self.assertEqual(actual, expected)
                    self.assertEqual(len(entries), len(original) + 1)
                    self.assertEqual(pack.append_wav(binary, voice), binary)


if __name__ == "__main__":
    unittest.main()
