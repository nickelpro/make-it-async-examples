import sys
import Figure3

count = 0


def app(incoming: bytes):
  global count
  msg = f'#{count}: '.encode() + incoming
  count += 1
  return msg


if '-h' in sys.argv or '--help' in sys.argv or len(sys.argv) > 2:
  print(f'Usage: {sys.argv[0]} [host[:port]]')
  exit()

host = "localhost"
port = "8000"

if len(sys.argv) == 2:
  host_port = sys.argv[1].split(":")
  host = host_port[0]
  if len(host_port) > 1:
    port = host_port[1]

Figure3.run(app, host, port)
