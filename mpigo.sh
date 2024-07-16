#!/usr/bin/env sh

(nohup mpirun -np 4 build/mpisort > nohup.log 2>&1 &) # Start your MPI program (example with 4 processes)

