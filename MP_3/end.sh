#!/bin/bash

kill -9 $(pgrep -f './coord -p 3010' -n)

kill -9 $(pgrep -f './tsd -d 1 -p 9010 -i 0.0.0.0 -c 3010 -t master' -n)
kill -9 $(pgrep -f './tsd -d 1 -p 9020 -i 0.0.0.0 -c 3010 -t slave' -n)

kill -9 $(pgrep -f './tsd -d 2 -p 9030 -i 0.0.0.0 -c 3010 -t master' -n)
kill -9 $(pgrep -f './tsd -d 2 -p 9040 -i 0.0.0.0 -c 3010 -t slave' -n)

kill -9 $(pgrep -f './tsd -d 3 -p 9050 -i 0.0.0.0 -c 3010 -t master' -n)
kill -9 $(pgrep -f './tsd -d 3 -p 9060 -i 0.0.0.0 -c 3010 -t slave' -n)

rm -rf temp
rm -rf master
rm -rf slave
