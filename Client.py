import socket
import sys

if '-h' in sys.argv or '--help' in sys.argv or len(sys.argv) > 2:
  print(f'Usage: {sys.argv[0]} [host[:port]]')
  exit()

host = "localhost"
port = 8000

if len(sys.argv) == 2:
  host_port = sys.argv[1].split(":")
  host = host_port[0]
  if len(host_port) > 1:
    port = int(host_port[1])

s = socket.create_connection((host, port))

while True:
  msg = input("Send: ").encode()

  buf = len(msg).to_bytes(2, 'big') + msg

  s.sendall(buf)

  to_read = int.from_bytes(s.recv(2), 'big')
  print(f'Recv: {s.recv(to_read).decode()}')
