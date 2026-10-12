"""Append the test WAV to SDK tone packs, preserving existing resources.
Format: SDK/interface/utils/fs/sdfile.h; CRC16-CCITT, initial value 0.
"""
from pathlib import Path
import argparse
import binascii
import struct
import io
import wave

ROOT = Path(__file__).resolve().parents[1]
HEADER = struct.Struct("<HHIIBBH16s")

def validate_notice_wav(raw):
    with wave.open(io.BytesIO(raw), "rb") as audio:
        if (audio.getnchannels(), audio.getframerate(), audio.getsampwidth(), audio.getcomptype()) != (1, 16000, 2, "NONE"):
            raise ValueError("Reverse notice must be PCM16, mono, 16000 Hz")
        duration = audio.getnframes() / audio.getframerate()
        if not 0 < duration <= 4:
            raise ValueError("Reverse notice must contain ONE phrase (up to 4 seconds); trim the repeated source before packing")
    return duration

def crc(data):
    return binascii.crc_hqx(data, 0)

def header(data_crc, addr, size, attr, reserved, index, name):
    raw = HEADER.pack(0, data_crc, addr, size, attr, reserved, index, name)
    return struct.pack("<H", crc(raw[2:])) + raw[2:]

def read_pack(raw):
    root = HEADER.unpack_from(raw)
    assert root[0] == crc(raw[2:32]) and root[1] == crc(raw[32:])
    assert root[2] == 32 and root[3] == len(raw) and root[4] == 3
    limit = HEADER.unpack_from(raw, 32)[2]
    entries = []
    for off in range(32, limit, 32):
        entry = HEADER.unpack_from(raw, off)
        assert entry[0] == crc(raw[off + 2:off + 32])
        data = raw[entry[2]:entry[2] + entry[3]]
        assert len(data) == entry[3] and entry[1] == crc(data)
        entries.append((entry, data))
        if entry[6] == 1:
            break
    assert entries[-1][0][6] == 1
    return root, entries, limit

def append_wav(raw, wav):
    root, entries, limit = read_pack(raw)
    existing = [e for e in entries if e[0][7].split(b"\0")[0] == b"reverse.wav"]
    if existing:
        if existing[0][1] == wav:
            return raw
        # 替换语音时重建文件表，所有其他资源的内容保持一致。
        kept = [e for e in entries if e[0][7].split(b"\0")[0] != b"reverse.wav"]
        records, payload = [], b""
        start = 32 * (len(kept) + 2)
        for entry, data in kept:
            records.append(header(crc(data), start + len(payload), len(data), entry[4], entry[5], 0, entry[7]))
            payload += data
            payload += b"\xff" * ((-len(payload)) % 16)
        records.append(header(crc(wav), start + len(payload), len(wav), 2, 255, 1, b"reverse.wav\0".ljust(16, b"\xff")))
        payload += wav
        payload += b"\xff" * ((-len(payload)) % 16)
        body = b"".join(records) + payload
        result = header(crc(body), 32, 32 + len(body), root[4], root[5], root[6], root[7]) + body
        _, verified, _ = read_pack(result)
        assert [x[1] for x in verified[:-1]] == [x[1] for x in kept]
        assert verified[-1][1] == wav
        return result
    records = []
    for entry, data in entries:
        records.append(header(entry[1], entry[2] + 32, entry[3], entry[4], entry[5], 0, entry[7]))
    new_addr = len(raw) + 32
    name = b"reverse.wav\0".ljust(16, b"\xff")
    records.append(header(crc(wav), new_addr, len(wav), 2, 255, 1, name))
    body = b"".join(records) + raw[limit:] + wav
    body += b"\xff" * ((-(32 + len(body))) % 16)
    result = header(crc(body), 32, 32 + len(body), root[4], root[5], root[6], root[7]) + body
    _, verified, _ = read_pack(result)
    assert [x[1] for x in verified[:-1]] == [x[1] for x in entries]
    assert verified[-1][1] == wav
    return result

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-directory", type=Path, default=ROOT / "output")
    args = parser.parse_args()
    wav = (ROOT / "assets/audio/reverse_notice.wav").read_bytes()
    duration = validate_notice_wav(wav)
    print(f"reverse.wav: single-phrase asset, {duration:.3f}s")
    for group in ("tone_zh", "tone_en"):
        base = ROOT / f"SDK/cpu/br33/tools/{group}_dashboard.cfg"
        output = args.output_directory / f"{group}.cfg"
        output.parent.mkdir(parents=True, exist_ok=True)
        # GUI 刚生成的提示音优先；第一次没有输出时使用 SDK 原始包。
        source = output if output.exists() else base
        result = append_wav(source.read_bytes(), wav)
        temporary = output.with_suffix(".cfg.reverse.tmp")
        temporary.write_bytes(result)
        temporary.replace(output)
        # 下载目录没有项目输出配置时仍能通过 SDK 路径找到资源。
        (base.parent / f"{group}.cfg").write_bytes(result)
        print(f"{output.name}: reverse.wav included; existing payloads and all CRCs verified")
