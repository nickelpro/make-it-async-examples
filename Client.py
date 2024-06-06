import socket

s = socket.create_connection(("localhost", 8000))
msg = b"Hello World"

buf = len(msg).to_bytes(2, 'big') + msg

s.sendall(buf)

to_read = int.from_bytes(s.recv(2), 'big')
print(s.recv(to_read).decode())
