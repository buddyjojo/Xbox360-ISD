import socket
import struct

TCP_IP = '10.42.0.140'
TCP_PORT = 902

BUFFER_SIZE = 1

ISD1200_INIT = 0xA0
ISD1200_DEINIT = 0xA1
ISD1200_READ_ID = 0xA2
ISD1200_READ_FLASH = 0xA3
ISD1200_ERASE_FLASH = 0xA4
ISD1200_WRITE_FLASH = 0xA5
ISD1200_PLAY_VOICE = 0xA6
ISD1200_EXEC_MACRO = 0xA7
ISD1200_RESET = 0xA8
ISD1200_PLAYSPI = 0xA9

def recv_exact(sock, size):
    buf = bytearray()
    while len(buf) < size:
        chunk = sock.recv(size - len(buf))
        if not chunk:
            raise ConnectionError("Socket closed early")
        buf.extend(chunk)
    return bytes(buf)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.connect((TCP_IP, TCP_PORT))

packet = bytes([ISD1200_PLAYSPI])
s.send(packet)

# packet = struct.pack(">BH", ISD1200_PLAY_VOICE, 5)
# s.send(packet)
#
# data = recv_exact(s, BUFFER_SIZE)

# s.close()
#
# print("Received data:", data.hex())
