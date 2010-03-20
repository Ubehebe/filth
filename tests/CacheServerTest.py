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
    def __init__(self, nreps):
        self.passed = True
        self.nreps = nreps
    def __call__(self):
        global files
        # I believe iteration over a dictionary is thread-safe in Python
        for i in range(self.nreps):
            name = random.choice(list(files.keys()))
            (sz, stuff) = files[name]
            sock = socket.socket(socket.AF_UNIX)
            sock.connect("./bucket")
            sock.sendall(name + "\r\n")
            torecv = sz + 2 + len(name)
            recvd = bytearray()
            while len(recvd) < torecv:
                recvd += sock.recv(1<<12)
            if recvd[(2+len(name)):] != stuff:
                self.passed = False
                break

# this takes hella long
def randbytes(n):
    return bytearray(map(lambda x: random.randint(0,255), range(n)))

def create_files(cachesz, singlefilemax=None):
    if singlefilemax == None:
        singlefilemax = cachesz
    global files
    print("creating random files to fill up " + str(cachesz) + " bytes")
    free = cachesz
    while free > 0:
        sz = min(random.randint(1, free), singlefilemax)
        tmp = tempfile.NamedTemporaryFile(delete=False)
        content = randbytes(sz)
        tmp.write(content)
        tmp.close()
        files[tmp.name] = (sz, content)
        free -= sz
        print("wrote " + tmp.name + " " + str(sz))

def clear_files():
    global files
    for name in files.keys():
        os.unlink(name)

def concurrent_test(nclients, nreps):
    print("simple concurrent test: " + str(nclients) + " clients each read "
          + str(nreps) + " random files from cache and compare to disk")
    clients = []
    threads = []
    for i in range(nclients):
        clients.append(client(nreps))
    for c in clients:
        threads.append(threading.Thread(target=c))
    for th in threads:
        th.start()
    for th in threads:
        th.join()
    return all(map(lambda c: c.passed, clients))

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
    cacheszmb = int(sys.argv[1]) * (1<<20)
#    create_files(cacheszmb)
#    if (concurrent_test(10,100)):
#        print("passed")
#    else:
#        print("FAILED")

#    os.kill(pid, signal.SIGUSR1)
#    time.sleep(1)
#    clear_files()

    create_files(2*cacheszmb, cacheszmb)
    if (concurrent_test(1,10)):
        print("passed")
    else:
        print("FAILED")

    os.kill(pid, signal.SIGTERM)
    os.waitpid(pid, 0)
    clear_files()
    

