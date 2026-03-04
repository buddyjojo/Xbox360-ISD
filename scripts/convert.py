#!/usr/bin/env python3
import argparse
import sys

def bytearray_to_int(b):
    # Little-endian 4‑byte integer
    return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)


def wav_to_raw(wav: bytes) -> bytes:
    raw = bytearray()
    magic = b"data"
    length_bytes = bytearray(4)

    got = 0
    data_found = False
    offset = 0
    i = 0

    counter = 0
    wav_len = len(wav)

    while counter < wav_len:
        if not data_found:
            # Searching for "data"
            if got >= 3:
                # Found "data"
                data_found = True

                # Read 4‑byte little-endian length
                length_bytes[0] = wav[counter + 4]
                length_bytes[1] = wav[counter + 3]
                length_bytes[2] = wav[counter + 2]
                length_bytes[3] = wav[counter + 1]

                counter += 4
                offset = counter
            else:
                if wav[counter] == magic[got]:
                    got += 1
                else:
                    got = 0
        else:
            # Extracting PCM data
            data_length = bytearray_to_int(length_bytes)

            if counter >= data_length + offset:
                break

            # Insert 0x78 every 0xFFB bytes
            # if (i % 0xFFB) == 0:
            #     raw.append(0x18)
            #     i += 1

            counter += 1
            raw.append(wav[counter])
            i += 1

        counter += 1

    return bytes(raw)


def main():
    parser = argparse.ArgumentParser(
        description="Extract RAW data from a WAV file."
    )
    parser.add_argument("input", help="Input WAV file")
    parser.add_argument("output", help="Output RAW file")

    args = parser.parse_args()

    try:
        with open(args.input, "rb") as f:
            wav_data = f.read()
    except IOError as e:
        print(f"Error reading input file: {e}", file=sys.stderr)
        sys.exit(1)

    print("Converting WAV → RAW...")
    raw_data = wav_to_raw(wav_data)

    try:
        with open(args.output, "wb") as f:
            f.write(raw_data)
    except IOError as e:
        print(f"Error writing output file: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Done. Wrote {len(raw_data)} bytes to {args.output}")


if __name__ == "__main__":
    main()

# Disclaimer: scripts may or may not be entirely ai generated.
