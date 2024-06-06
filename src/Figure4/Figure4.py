import Figure4

count = 0


def app(incoming: bytes):
  global count
  msg = f'#{count}: '.encode() + incoming
  count += 1
  return msg


Figure4.run(app)
