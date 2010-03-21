#! /usr/bin/python

import os
import random
import resource
import signal
import socket
import sys
import tempfile
import threading
import time

files = {}
nread = 0
retries = 0

class client:
    def __init__(self, clid, nreps):
        self.clid = clid
        self.nreps = nreps
        self.passed = True
    def __call__(self):
        global files, nread, retries
        # I believe iteration over a dictionary is thread-safe in Python
        for i in range(self.nreps):
            name = random.choice(list(files.keys()))
            (sz, stuff) = files[name]
            sock = socket.socket(socket.AF_UNIX)
            sock.connect("./bucket")
            sock.sendall(name + "\r\n")
            torecv = sz + 2 + len(name)
            recvd = bytearray()
            print("client " + str(self.clid) + ": requesting "
                  + name + " " + str(torecv) + " "
                  + " [" + str(i+1) + "/" + str(self.nreps) + "]")
            while len(recvd) < torecv:
                recvd += sock.recv(1<<12)
                if len(recvd) == 3+len(name) and recvd[-1] == 0:
                    retries += 1
                    recvd = bytearray()
                    sock = socket.socket(socket.AF_UNIX)
                    sock.connect("./bucket")
                    sock.sendall(name + "\r\n")
            nread += torecv
            if recvd[(2+len(name)):] != stuff:
                self.passed = False
                break

def create_files(cachesz, singlefilemax=None):
    if singlefilemax == None:
        singlefilemax = cachesz
    global files
    print("creating random files to fill up " + str(cachesz) + " bytes")
    randbytes = open("/dev/urandom", "rb")
    free = cachesz
    while free > 0:
        sz = min(random.randint(1, free), singlefilemax)
        tmp = tempfile.NamedTemporaryFile(delete=False)
        content = randbytes.read(sz)
        tmp.write(content)
        tmp.close()
        files[tmp.name] = (sz, content)
        free -= sz
        print("wrote " + tmp.name + " " + str(sz))
    randbytes.close()

def clear_state():
    global files, retries
    for name in files.keys():
        os.unlink(name)
    files = {}
    nread = 0
    retries = 0

def concurrent_test(nclients, nreps):
    print(str(nclients) + " clients each read " + str(nreps)
          + " random files from cache and compare to disk")
    clients = []
    threads = []
    for i in range(nclients):
        clients.append(client(i, nreps))
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

    create_files(cacheszmb)
    print("simple concurrent test:")
    before = resource.getrusage(resource.RUSAGE_SELF)
    result = concurrent_test(10,100)
    after = resource.getrusage(resource.RUSAGE_SELF)
    if result:
        print("passed")
    else:
        print("FAILED")
    print(str(retries) + " retries, "
          + str(nread/(after.ru_utime + after.ru_stime - before.ru_utime - before.ru_stime))
          + "bytes/s")

    os.kill(pid, signal.SIGUSR1)
    time.sleep(1)
    clear_state()

    create_files(2*cacheszmb, cacheszmb)
    print("overloaded concurrent test:")
    before = resource.getrusage(resource.RUSAGE_SELF)
    result = concurrent_test(10,10)
    after = resource.getrusage(resource.RUSAGE_SELF)
    if result:
        print("passed")
    else:
        print("FAILED")
    print(str(retries) + " retries, "
          + str(nread/(after.ru_utime + after.ru_stime - before.ru_utime - before.ru_stime))
          + "bytes/s")

    os.kill(pid, signal.SIGTERM)
    os.waitpid(pid, 0)
    clear_state()
