#!/usr/bin/env python
from __future__ import print_function
import sys
import random
import subprocess
import time
import errno

"""
Ghetto OpenMP for forceitbox ;)

Runs on localhost & over SSH
"""


class Worker(object):
    def __init__(self, host, cmd):
        self.host = host
        self.cmd = cmd
        self.proc = None

    def start(self, start_time, search_hash, range_start, range_end):
        assert self.proc is None
        args = [self.cmd, str(start_time), str(search_hash),
                str(range_start), str(range_end)]
        if self.host != 'localhost':
            args = ['ssh', '-q', '-a', '-k', '-tt', self.host] + args
        self.proc = subprocess.Popen(args, bufsize=1, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=None)

    def result(self):
        assert self.proc
        if self.proc.returncode == 0:
            return self.proc.stdout.readline().split()

    def finished(self):
        if not self.proc:
            return None
        if self.proc.poll() is not None:
            return True

    def stop(self):
        if self.proc:
            try:
                if self.host != 'localhost':
                    # Send Ctrl+C
                    self.proc.stdin.write(chr(3))
                    try:
                        self.proc.stdin.flush()
                    except IOError as e:
                        if e.errno != errno.EPIPE and e.errno != errno.EINVAL:
                            raise
                self.proc.terminate()
            except OSError:
                pass
            self.proc = None


def main(args):
    if len(args) != 2:
        print("Usage: ghettomp.py <time> <search-hash>")
        return 1
    search_time, search_hash = args

    hosts = [
        # Name of host, CPUs, path to executable
        ('localhost',   1,    './forceitbox'),
        ('root@derp', 4, './forceitbox/forceitbox'),
        # ('ubuntu@aws1', 8, './forceitbox'),
        # ('ubuntu@aws2', 8, './forceitbox'),
        # ('ubuntu@aws3', 8, './forceitbox'),
        # ('ubuntu@aws4', 8, './forceitbox'),
        # ('ubuntu@aws5', 8, './forceitbox'),
        # ('ubuntu@aws6', 8, './forceitbox'),
        # ('ubuntu@aws7', 8, './forceitbox'),
        # ('ubuntu@aws8', 8, './forceitbox'),

    ]

    # Split the work evenly across the CPUs
    cpu_count = sum([cpus for _, cpus, _ in hosts])
    unit_count = cpu_count * 10  # give each CPU 10 units of work
    work_range = 3333333 - 1111111
    unit_size = work_range / unit_count
    work_units = [
        (start, min((start+unit_size) - 1, 3333333))
        for start in range(1111111, 3333333, unit_size)
    ]
    work_unit_count = len(work_units)
    random.shuffle(work_units)

    # Now setup workers...
    worker_list = []
    for hostname, cpu_count, cmd in hosts:
        for N in range(0, cpu_count):
            worker_list.append(Worker(hostname, cmd))
    random.shuffle(worker_list)

    """
    # Test unit...
    # ./ghettomp.py 1468688982 c060571db8365796717b6ed51f296e574e965e16
    for worker in worker_list:
        worker.start(1468688982, 'c060571db8365796717b6ed51f296e574e965e16',
                     1111111, 3333333)
    """

    result = False
    while not result:
        try:
            failcnt = 0
            for worker in worker_list:
                if worker.proc is None:
                    # Needs work...
                    if not len(work_units):
                        failcnt += 1
                        continue
                    work = work_units.pop(0)
                    pct = str(len(work_units) / (work_unit_count / 100.0))+"%"
                    print(pct, worker.host, work)
                    worker.start(search_time, search_hash, work[0], work[1])
                if worker.finished():
                    tmp_result = worker.result()
                    worker.stop()
                    if tmp_result:
                        result = tmp_result
                        break
            if failcnt == len(worker_list):
                print("FAILURE!")
                break
            time.sleep(0.3)
        except KeyboardInterrupt:
            print("Finishing early...")
            break
    if result:
        print("")
        print(result)
    for worker in worker_list:
        worker.stop()
    return 0

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
