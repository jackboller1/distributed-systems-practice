#!/bin/bash
if [ ! -d temp ]; then
    mkdir temp;
fi;
if [ ! -d master ]; then
    mkdir master;
fi;
if [ ! -d slave ]; then
    mkdir slave;
fi;


./coord -p 3010 &
sleep 0.5

./tsd -d 1 -p 9010 -i 0.0.0.0 -c 3010 -t master &
./tsd -d 1 -p 9020 -i 0.0.0.0 -c 3010 -t slave &

./tsd -d 2 -p 9030 -i 0.0.0.0 -c 3010 -t master &
./tsd -d 2 -p 9040 -i 0.0.0.0 -c 3010 -t slave &

./tsd -d 3 -p 9050 -i 0.0.0.0 -c 3010 -t master &
./tsd -d 3 -p 9060 -i 0.0.0.0 -c 3010 -t slave &
