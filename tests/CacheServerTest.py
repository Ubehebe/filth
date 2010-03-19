#! /usr/bin/python

import os
import random
import signal
import socket
import sys
import tempfile
import threading
import time

files = {}

class client:
    def __init__(self):
        self.passed = True
    def __call__(self):
        global files
        # I believe iteration over a dictionary is thread-safe in Python
        for (name, (sz, stuff)) in files.items():
            sock = socket.socket(socket.AF_UNIX)
            sock.connect("./bucket")
            sock.sendall(name + "\r\n")
            torecv = sz + 2 + len(name)
            recvd = bytearray()
            while len(recvd) < torecv:
                recvd += sock.recv(1<<12)
            if recvd[(2+len(name)):] != stuff:
                self.passed = False
                break;

# this takes hella long
def randbytes(n):
    return bytearray(map(lambda x: random.randint(0,255), range(n)))

def bye():
    global files
    os.kill(pid, signal.SIGINT)
    os.waitpid(pid, 0)
    for name in files.keys():
        os.unlink(name)

if len(sys.argv) != 2:
    print("usage: " + sys.argv[0] + " <cache size (MB)>")
    exit(0)

pid = os.fork()

if pid == 0:
#    os.execv("./../bin/cash", ["cash", "-n./bucket", "-m/tmp", "-s" + sys.argv[1]])
    os.execvp("valgrind", ["valgrind", "./../bin/cash", "-n./bucket", "-m/tmp",
                           "-s" + sys.argv[1]])
else:
    print("waiting for cache to start up")
    time.sleep(5)
    
    cachesz = int(sys.argv[1]) * (1<<20)

    print("creating some random files...")
    free = cachesz
    while free > 0:
        sz = random.randint(1, free)
        tmp = tempfile.NamedTemporaryFile(delete=False)
        content = randbytes(sz)
        tmp.write(content)
        tmp.close()
        files[tmp.name] = (sz, content)
        free -= sz
        print("wrote " + tmp.name + " " + str(sz))

    print("test: 10 clients, read the files through the cache")
    clients = []
    threads = []
    for i in range(10):
        clients.append(client())
    for c in clients:
        threads.append(threading.Thread(target=c))
    for th in threads:
        th.start()
    for th in threads:
        th.join()
    if all(map(lambda c: c.passed, clients)):
        print("passed")
    else:
        print("failed")

    bye()
