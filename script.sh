#!/bin/bash
gcc -o ring_client main.c myRing.c -I . -Wall -lpthread -g
gcc -o ring_server myRingServer.c -I . -Wall -g

exit 0

