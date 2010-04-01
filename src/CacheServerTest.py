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
import timeit

nread = 0
retries = 0

class client:
    def __init__(self, socknam, fmanager, clid, nreps):
        self.socknam = socknam
        self.fmanager = fmanager
        self.clid = clid
        self.nreps = nreps
    def get(self, name, sz):
        global nread, retries
        while (True):
            sock = socket.socket(socket.AF_UNIX)
            sock.connect(self.socknam)
            sock.sendall(name + "\r\n")
            torecv = sz + 2 + len(name)
            recvd = bytearray()
            newstuff = sock.recv(1<<12)
            while len(newstuff) > 0:
                recvd += newstuff
                nread += len(newstuff)
                newstuff = sock.recv(1<<12)
            # I.e. an error at the server; retry.
            if len(recvd) == 2+len(name):
                retries += 1
                continue
            break
        return recvd

    def __call__(self):
        global nread, retries
        for i in range(self.nreps):
            (name, stuff, sz) = self.fmanager.getrandom_wolock()
            print("client " + str(self.clid) + ": requesting "
                  + name + " [" + str(i+1) + "/" + str(self.nreps) + "]")
            recvd = self.get(name, sz)
            if recvd[(2+len(name)):] != stuff:
                print("client " + str(self.clid) + " FAILED: " + name)
                os.abort()

class client_modifier(client):
    def __call__(self):
        global nread, retries
        for i in range(self.nreps):
            (name, stuff, sz) = self.fmanager.getrandom_wolock()
            print("client " + str(self.clid) + ": requesting "
                  + name + " [" + str(i+1) + "/" + str(self.nreps) + "]")
            recvd = self.get(name, sz)
            if recvd[(2+len(name)):] != stuff:
                print("client " + str(self.clid) + " FAILED: " + name)
                os.abort()
            print("client " + str(self.clid) + ": modifying "
                  + name + " [" + str(i+1) + "/" + str(self.nreps) + "]")
            stuff = self.fmanager.modify(name)
            print("client " + str(self.clid) + ": requesting "
                  + name + " [" + str(i+1) + "/" + str(self.nreps) + "]")
            recvd = self.get(name, sz)
            if recvd[(2+len(name)):] != stuff:
                print("client " + str(self.clid) + " FAILED: " + name)
                os.abort()
            
class file_manager:
    def __init__(self):
        self.randbytes = open("/dev/urandom", "rb")
        self.flock = threading.Lock()
        self.files = {}
    def __del__(self):
       self.randbytes.close()
    def write_rand_bytes(self, f, sz):
        content = self.randbytes.read(sz)
        f.write(content)
        return content
    def create_files(self, cachesz, single_file_max=None):
        if single_file_max == None:
            single_file_max = cachesz
        print("creating random files to fill up " + str(cachesz) + " bytes")
        free = cachesz
        while free > 0:
            sz = min(random.randint(1, free), single_file_max)
            tmp = tempfile.NamedTemporaryFile(delete=False)
            content = self.write_rand_bytes(tmp, sz)
            tmp.close()
            self.flock.acquire()
            self.files[tmp.name] = content
            self.flock.release()
            free -= sz
            print("wrote " + tmp.name + " " + str(sz))
    def getrandom_wolock(self):
        name = random.choice(list(self.files.keys()))
        return (name, self.files[name], len(self.files[name]))
    def modify(self, name):
        self.flock.acquire()
        sz = len(self.files[name])
        f = open(name, "wb")
        stuff = self.write_rand_bytes(f, sz)
        self.files[name] = stuff
        f.close()
        self.flock.release()
        return stuff
    def clear(self):
        global nread, retries
        nread = 0
        retries = 0
        self.flock.acquire()
        map(os.unlink, self.files.keys())
        self.files = {}
        self.flock.release()

class concurrent_test:
    def __init__(self, fmanager, msg, socknam, nclients, nreps, invalidate=False):
        self.fmanager = fmanager
        self.msg = msg
        self.socknam = socknam
        self.nclients = nclients
        self.nreps = nreps
        self.invalidate = invalidate
    def __call__(self):
        print(self.msg)
        print(str(self.nclients) + " clients each read " + str(self.nreps)
              + " random files from cache and compare to disk")
        clients = []
        threads = []
        if self.invalidate:
            for i in range(self.nclients):
                clients.append(client_modifier(self.socknam, fm, i, self.nreps))
        else:
            for i in range(self.nclients):
                clients.append(client(self.socknam, fm, i, self.nreps))
        for c in clients:
            threads.append(threading.Thread(target=c))
        for th in threads:
            th.start()
        for th in threads:
            th.join()

if len(sys.argv) != 2:
    print("usage: " + sys.argv[0] + " <cache size (MB)>")
    exit(0)

pid = os.fork()

if pid == 0:
    os.execvp("valgrind", ["valgrind", "./standalone-cache", "-nbucket", "-m/tmp",
                           "-s" + str((1<<20) * int(sys.argv[1]))])
else:
    print("waiting for cache to start up")
    time.sleep(5)
    cachesz = (1<<20) * int(sys.argv[1])
    fm = file_manager()

    fm.create_files(cachesz)
    test1 = concurrent_test(fm, "concurrent test, load factor 1", "bucket", 10, 100)
    t = timeit.timeit(test1, number=1)
    print(str(retries) + " retries, " + str(nread/t) + "bytes/s")

    os.kill(pid, signal.SIGCONT)
    time.sleep(1)
    fm.clear()

    fm.create_files(2*cachesz, cachesz)
    test2 = concurrent_test(fm, "concurrent test, load factor 2", "bucket", 10, 100) 
    t = timeit.timeit(test2, number=1)
    print(str(retries) + " retries, " + str(nread/t) + "bytes/s")

    os.kill(pid, signal.SIGCONT)
    time.sleep(1)
    fm.clear()

    fm.create_files(cachesz)
    test3 = concurrent_test(fm, "sequential invalidate test, load factor 1", "bucket", 1, 100,
                            invalidate=True) 
    t = timeit.timeit(test3, number=1)
    print(str(retries) + " retries, " + str(nread/t) + "bytes/s")

    os.kill(pid, signal.SIGTERM)
    os.waitpid(pid, 0)
    fm.clear()
