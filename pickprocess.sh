#!/usr/bin/env sh

sleep 1                     # Give it a moment to start
pgrep -fo "^build/mpisort$" # Retrieve the PIDs of the started processes
