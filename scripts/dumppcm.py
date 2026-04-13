#!/usr/bin/env python3
import argparse
import socket
import struct
import sys

# TCP_IP = '10.42.0.140'
# TCP_PORT = 902

ISD1200_INIT = 0xA0
ISD1200_PLAY_VOICE = 0xA6
ISD1200_RESET = 0xA8
ISD1200_READ_SPI = 0xB0
ISD1200_SET_CFG  = 0xB2
ISD1200_CMD_FIN = 0xB3

BLOCK_SIZE = 512

def recv_exact(sock, size):
    buf = bytearray()
    while len(buf) < size:
        chunk = sock.recv(size - len(buf))
        if not chunk:
            raise ConnectionError("Socket closed early")
        buf.extend(chunk)
    return bytes(buf)

def update_progress(pct):
    sys.stdout.write(f"\rReceived: {pct} bytes")
    sys.stdout.flush()

def main():
    parser = argparse.ArgumentParser(
        description="Dump decompressed 16 bit pcm data from the ISD"
    )
    parser.add_argument(
        "ip",
        help="IP and port of the 360 e.g 10.42.0.140:902"
    )
    parser.add_argument(
        "filename",
        type=str,
        help="filename"
    )
    parser.add_argument(
        "index",
        type=int,
        help="VP index"
    )

    args = parser.parse_args()
    
    TCP_IP, TCP_PORT = args.ip.split(":")
    
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    s.connect((TCP_IP, int(TCP_PORT)))
    
    packet = struct.pack(">BBB", ISD1200_SET_CFG, 0x02, 0x41)
    s.send(packet)
    recv_exact(s, 1)
  
    packet = struct.pack(">BH", ISD1200_PLAY_VOICE, args.index)
    s.send(packet)
    recv_exact(s, 1)
   
    s.send(bytes([ISD1200_CMD_FIN]))
    recv_exact(s, 1)
    
    total_written = 0

    with open(args.filename, "wb") as f:
        while True:
            s.send(bytes([ISD1200_READ_SPI]))
            
            header = recv_exact(s, 1)
            
            if header == b"\xFF":
                print("\nFIN received, stopping.")
                break

            if header != b"\x01":
                raise ValueError(f"Unknown packet type: {header.hex()}")

            data = recv_exact(s, BLOCK_SIZE)

            f.write(data)
            
            total_written += len(data)
            update_progress(total_written)

    s.send(bytes([ISD1200_RESET]))
    recv_exact(s, 1)
    s.send(bytes([ISD1200_INIT]))
    recv_exact(s, 1)

    print("\nDone.")

if __name__ == "__main__":
    main()
