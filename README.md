os
==

### lab 0:
- Compile the kernel using KVM.
- Trace the kernel

### lab 1:
- Create an multi-process application using clone where processes share the address space.
- Benchmark by setting cpu affinity and use cgroups to improve throughput.
- Optimize fairness among worker processes

### lab 2:
- Write a user level networked file system.
- Used Fuse and libssh to support basic file system operations like ls, cat, open, read, close
- Cache the remote file in /tmp.
- Compare with NFS

### lab 3:
- Write a program to load an elf executable.
- basic loader : map all the pages including bss segments
- demand loader: map the pages on demand.
- Hybrid loader: map .bss segments based on some prediction algorithm.

### lab 4:
- Implement the basic paxos methods in the provided distributed system simulator

