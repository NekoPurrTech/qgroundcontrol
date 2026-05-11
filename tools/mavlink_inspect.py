#!/usr/bin/env python3
"""
mavlink_inspect.py

Simple tool to extract and inspect MAVLink v1/v2 frames from:
 - a text log containing hex dumps (e.g. QGC raw-hex lines),
 - a direct hex string passed with --hex, or
 - a pcap file (requires scapy installed).

It scans the provided bytes for MAVLink frame magic (0xFD for v2, 0xFE for v1),
parses headers, and prints msgid/sysid/compid/payload info. Helpful to
confirm whether custom msgids (12922/12923/12924) are present.

Usage examples:
  python tools/mavlink_inspect.py --log qgc_recv_log.txt
  python tools/mavlink_inspect.py --hex "FD 80 00 00 ..."
  python tools/mavlink_inspect.py --pcap capture.pcap

"""
import argparse
import re
import sys
import binascii
import struct
from typing import List, Tuple

HEX_RE = re.compile(r"([0-9A-Fa-f]{2}(?:[ \t:,-]*[0-9A-Fa-f]{2})*)")


def extract_hex_from_line(line: str) -> List[bytes]:
    matches = HEX_RE.findall(line)
    out = []
    for m in matches:
        s = re.sub(r"[^0-9A-Fa-f]", "", m)
        if len(s) >= 2 and len(s) % 2 == 0:
            try:
                out.append(binascii.unhexlify(s))
            except Exception:
                pass
    return out


def scan_frames(data: bytes) -> List[Tuple[int, dict]]:
    """Scan data for MAVLink v2 (0xFD) and v1 (0xFE) frames.
    Returns list of (offset, info) dicts.
    """
    results = []
    i = 0
    L = len(data)
    while i < L:
        b = data[i]
        if b == 0xFD and i + 10 <= L:
            # MAVLink v2 header
            try:
                payload_len = data[i+1]
                incompat = data[i+2]
                compat = data[i+3]
                seq = data[i+4]
                sysid = data[i+5]
                compid = data[i+6]
                msgid = data[i+7] | (data[i+8] << 8) | (data[i+9] << 16)
                header_len = 10
                sig = (incompat & 0x01) != 0
                frame_len = header_len + payload_len + 2 + (13 if sig else 0)
                if i + frame_len <= L:
                    payload = data[i+header_len:i+header_len+payload_len]
                    checksum = data[i+header_len+payload_len:i+header_len+payload_len+2]
                    results.append((i, {
                        'version': 2,
                        'seq': seq,
                        'sysid': sysid,
                        'compid': compid,
                        'msgid': msgid,
                        'payload_len': payload_len,
                        'payload': payload,
                        'checksum': checksum,
                        'incompat': incompat,
                        'compat': compat,
                        'sig': sig,
                    }))
                    i += frame_len
                    continue
            except Exception:
                pass
        elif b == 0xFE and i + 6 <= L:
            # MAVLink v1 header
            try:
                payload_len = data[i+1]
                seq = data[i+2]
                sysid = data[i+3]
                compid = data[i+4]
                msgid = data[i+5]
                header_len = 6
                frame_len = header_len + payload_len + 2
                if i + frame_len <= L:
                    payload = data[i+header_len:i+header_len+payload_len]
                    checksum = data[i+header_len+payload_len:i+header_len+payload_len+2]
                    results.append((i, {
                        'version': 1,
                        'seq': seq,
                        'sysid': sysid,
                        'compid': compid,
                        'msgid': msgid,
                        'payload_len': payload_len,
                        'payload': payload,
                        'checksum': checksum,
                    }))
                    i += frame_len
                    continue
            except Exception:
                pass
        i += 1
    return results


def pretty_print_frame(offset: int, info: dict):
    print(f"Frame @ offset {offset}: MAVLink v{info['version']}")
    print(f"  msgid: {info['msgid']}  seq:{info.get('seq', '?')}  sysid:{info.get('sysid','?')}  compid:{info.get('compid','?')}")
    print(f"  payload_len: {info['payload_len']}")
    payload = info['payload']
    print(f"  payload(hex): {payload.hex().upper()}")
    # ASCII preview if printable
    try:
        s = payload.decode('ascii', errors='replace')
        if any(c.isalpha() for c in s):
            print(f"  payload(ascii): {s}")
    except Exception:
        pass
    # hint for common custom IDs
    if info['msgid'] in (12922, 12923, 12924):
        print("  >>> Matches custom message ID (12922/12923/12924)")


def handle_log_file(path: str):
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        all_bytes = bytearray()
        for line in f:
            hex_chunks = extract_hex_from_line(line)
            for chunk in hex_chunks:
                all_bytes.extend(chunk)
    if not all_bytes:
        print('No hex sequences found in log file.')
        return
    frames = scan_frames(bytes(all_bytes))
    if not frames:
        print('No MAVLink frames found in extracted hex. Parsed bytes length:', len(all_bytes))
        return
    for off, info in frames:
        pretty_print_frame(off, info)


def handle_hex_string(hexstr: str):
    s = re.sub(r"[^0-9A-Fa-f]", "", hexstr)
    if len(s) < 2:
        print('No hex data provided')
        return
    try:
        data = binascii.unhexlify(s)
    except Exception as e:
        print('Failed to parse hex:', e)
        return
    frames = scan_frames(data)
    if not frames:
        print('No frames found in provided hex. Raw length:', len(data))
        return
    for off, info in frames:
        pretty_print_frame(off, info)


def handle_pcap(path: str):
    try:
        from scapy.all import rdpcap, UDP
    except Exception:
        print('scapy is required to parse pcap files. Install with: pip install scapy')
        return
    pkts = rdpcap(path)
    all_bytes = bytearray()
    for p in pkts:
        if UDP in p:
            try:
                payload = bytes(p[UDP].payload)
            except Exception:
                continue
            all_bytes.extend(payload)
    if not all_bytes:
        print('No UDP payloads found in pcap')
        return
    frames = scan_frames(bytes(all_bytes))
    if not frames:
        print('No MAVLink frames found in pcap-extracted UDP payloads')
        return
    for off, info in frames:
        pretty_print_frame(off, info)


def main():
    p = argparse.ArgumentParser(description='Inspect MAVLink frames in hex/log/pcap')
    group = p.add_mutually_exclusive_group(required=True)
    group.add_argument('--log', help='Text log file (extract hex sequences)')
    group.add_argument('--hex', help='Direct hex string to parse')
    group.add_argument('--pcap', help='PCAP file to parse UDP payloads (needs scapy)')
    args = p.parse_args()
    if args.log:
        handle_log_file(args.log)
    elif args.hex:
        handle_hex_string(args.hex)
    elif args.pcap:
        handle_pcap(args.pcap)


if __name__ == '__main__':
    main()
