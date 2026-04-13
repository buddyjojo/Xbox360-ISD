#!/usr/bin/env python3
import argparse
import socket
import struct

# TCP_IP = '10.42.0.140'
# TCP_PORT = 902

BUFFER_SIZE = 1

COMMANDS = {
    "INIT": 0xA0,
    "DEINIT": 0xA1,
    "READ_ID": 0xA2,
    "READ_FLASH": 0xA3,
    "ERASE_FLASH": 0xA4,
    "WRITE_FLASH": 0xA5,
    "PLAY_VOICE": 0xA6,
    "EXEC_MACRO": 0xA7,
    "RESET": 0xA8,
    "PLAYSPI": 0xA9,
    "READ_SPI": 0xB0,
    "READ_CFG": 0xB1,
    "SET_CFG": 0xB2,
    "CMD_FIN": 0xB3
}

def recv_exact(sock, size):
    buf = bytearray()
    while len(buf) < size:
        chunk = sock.recv(size - len(buf))
        if not chunk:
            raise ConnectionError("Socket closed early")
        buf.extend(chunk)
    return bytes(buf)


def main():
    parser = argparse.ArgumentParser(
        description="Send ISD commands to the 360 over TCP"
    )
    parser.add_argument(
        "ip",
        help="IP and port of the 360 e.g 10.42.0.140:902"
    )
    parser.add_argument(
        "cmd",
        choices=COMMANDS.keys()
    )
    parser.add_argument(
        "--arg",
        type=str,
        help="Argument to send e.g 5 for PLAYSPI, 0x2 for READ_CFG, 0x0060 for SET_CFG",
        required=False
    )
    parser.add_argument(
        "--norec",
        help="Don't wait for recived data",
        required=False
    )

    args = parser.parse_args()
    
    TCP_IP, TCP_PORT = args.ip.split(":")
    
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    s.connect((TCP_IP, int(TCP_PORT)))
    
    cmd_hex = COMMANDS[args.cmd]
    
    if args.arg is None:
        packet = bytes([cmd_hex])
        s.send(packet)
    else:
        is_hex = args.arg.lower().startswith("0x")
        arg_val = int(args.arg, 0)
        if is_hex and len(args.arg) <= 4:
            fmt = ">BB"
        else:
            fmt = ">BH"
        packet = struct.pack(fmt, cmd_hex, arg_val)
        s.send(packet)
        
    if args.norec is None:
        data = recv_exact(s, BUFFER_SIZE)
        s.close()
        print("Received data:", data.hex())
    else:
        s.close()

if __name__ == "__main__":
    main()
