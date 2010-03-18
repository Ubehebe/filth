#! /usr/bin/python

import os
import random
import sys
import socket
import tempfile

def randbytes(n):
    return bytearray(map(lambda x: random.randint(0,255), range(n)))

if len(sys.argv) != 2:
    print("usage: " + sys.argv[0] + " <sockname> [assumed to be mounted at /tmp]")
    exit(0)


os.chdir("/tmp")
tmp = tempfile.NamedTemporaryFile(delete=False)
sz = 5 * (1<<20)
backup = randbytes(sz)
tmp.write(backup)
tmp.close()
sock = socket.socket(socket.AF_UNIX)
sock.connect(sys.argv[1])
sock.sendall(tmp.name + "\r\n")
torecv = sz + 2 + len(tmp.name)
recvd = bytearray()
while len(recvd) < torecv:
    recvd += sock.recv(1<<12)
if recvd[(2+len(tmp.name)):] == backup:
    print("whew!")
else:
    print("uh oh")





