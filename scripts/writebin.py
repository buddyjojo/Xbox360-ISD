#!/usr/bin/env python3
import argparse
import socket
import struct
import os
import sys
import time

# TCP_IP = '10.42.0.140'
# TCP_PORT = 902

ISD1200_INIT = 0xA0
ISD1200_DEINIT = 0xA1
ISD1200_READ_ID = 0xA2
ISD1200_READ_FLASH = 0xA3
ISD1200_ERASE_FLASH = 0xA4
ISD1200_WRITE_FLASH = 0xA5
ISD1200_PLAY_VOICE = 0xA6
ISD1200_EXEC_MACRO = 0xA7
ISD1200_RESET = 0xA8

TOTAL_SIZE = 0xB000

def recv_exact(sock, size):
    buf = bytearray()
    while len(buf) < size:
        chunk = sock.recv(size - len(buf))
        if not chunk:
            raise ConnectionError("Socket closed early")
        buf.extend(chunk)
    return bytes(buf)


def update_progress(pct):
    sys.stdout.write(f"\rProgress: {pct}%")
    sys.stdout.flush()


def write_flash(filename):

    BLOCK_SIZE = 16
    NUM_BLOCKS = TOTAL_SIZE // BLOCK_SIZE

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))

    filesize = os.path.getsize(filename)
    print(f"Writing {filesize} bytes from '{filename}'")

    with open(filename, "rb") as f:
        for i in range(NUM_BLOCKS):

            data = f.read(BLOCK_SIZE)
            if not data or len(data) != BLOCK_SIZE:
                print("\nReached end of file early.")
                break

            packet = struct.pack(">BI", ISD1200_WRITE_FLASH, i) + data
            s.send(packet)

            ret_byte = recv_exact(s, 1)
            ret = ret_byte[0]

            if ret != 0:
                print(f"\nError: {ret:08X}")
                quit()

            pct = int((i * 100) / NUM_BLOCKS)
            update_progress(pct)

        update_progress(100)
        print("\nDone writing flash.")

    s.close()

def verify_flash(filename):
    print("Verifying flash contents...")

    READBACK_BLOCK = 512
    NUM_READBACK_BLOCKS = TOTAL_SIZE // READBACK_BLOCK

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))

    with open(filename, "rb") as f:
        for i in range(NUM_READBACK_BLOCKS):

            expected = f.read(READBACK_BLOCK)
            if not expected or len(expected) != READBACK_BLOCK:
                print("\nReached end of file early during verify.")
                break

            packet = struct.pack(">BI", ISD1200_READ_FLASH, i)
            s.send(packet)

            received = recv_exact(s, READBACK_BLOCK)

            if received != expected:
                print(f"\nMISMATCH at block {i}")
                print(f"Expected: {expected.hex()}")
                print(f"Received: {received.hex()}")
                s.close()
                return

            pct = int((i * 100) / NUM_READBACK_BLOCKS)
            update_progress(pct)

        update_progress(100)
        print("\nFlash verification successful")

    s.close()

def erase_flash():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))

    s.send(bytes([ISD1200_ERASE_FLASH]))

    ret_byte = recv_exact(s, 1)
    ret = ret_byte[0]

    if ret != 0:
        print(f"\nError: {ret:08X}")
        quit()

    s.close()

def init_isd():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))

    s.send(bytes([ISD1200_INIT]))

    ret_byte = recv_exact(s, 1)
    ret = ret_byte[0]

    if ret != 0:
        print(f"\nError: {ret:08X}")
        quit()

    s.close()


def main():
    global TCP_IP, TCP_PORT

    parser = argparse.ArgumentParser(
        description="Write ISD flash data to 360 over TCP"
    )
    parser.add_argument(
        "ip",
        help="IP and port of the 360 e.g 10.42.0.140:902"
    )
    parser.add_argument(
        "filename",
        help="Binary file to write to flash"
    )

    args = parser.parse_args()

    if not os.path.isfile(args.filename):
        print(f"Error: File '{args.filename}' not found.")
        sys.exit(1)

    TCP_IP, TCP_PORT = args.ip.split(":")
    TCP_PORT = int(TCP_PORT)

    init_isd()

    erase_flash()

    time.sleep(0.250)

    write_flash(args.filename)

    verify_flash(args.filename)

if __name__ == "__main__":
    main()
