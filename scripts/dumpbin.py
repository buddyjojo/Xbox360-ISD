import socket
import struct
import sys

TCP_IP = '10.42.0.140'
TCP_PORT = 902

ISD1200_READ_FLASH = 0xA3

BLOCK_SIZE = 512
TOTAL_SIZE = 0xB000
NUM_BLOCKS = TOTAL_SIZE // BLOCK_SIZE

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

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((TCP_IP, TCP_PORT))

    with open("output.bin", "wb") as f:
        for i in range(NUM_BLOCKS):

            packet = struct.pack(">BI", ISD1200_READ_FLASH, i)
            s.send(packet)

            data = recv_exact(s, BLOCK_SIZE)

            f.write(data)

            pct = int((i * 100) / NUM_BLOCKS)
            update_progress(pct)

        update_progress(100)

print("\nDone.")
