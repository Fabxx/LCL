#!/bin/bash

mkdir -p build

cp info build

cd build 

cmake -DCORE="azahar" -DSYSTEM_NAME="Nintendo - Nintendo 3DS" .. && cmake --build .

cmake -DCORE="duckstation" -DSYSTEM_NAME="Sony - Playstation" .. && cmake --build .

cmake -DCORE="mgba" -DSYSTEM_NAME="Nintendo - Game Boy Advance" .. && cmake --build .

cmake -DCORE="melonds" -DSYSTEM_NAME="Nintendo - Nintendo DS" .. && cmake --build .

cmake -DCORE="pcsx2" -DSYSTEM_NAME="Sony - Playstation 2" .. && cmake --build .

cmake -DCORE="xemu" -DSYSTEM_NAME="Microsoft - Xbox" .. && cmake --build .

cmake -DCORE="xenia" -DSYSTEM_NAME="Microsoft - Xbox 360" .. && cmake --build .

find . -type f ! \( -name "*.so"\) -exec rm -f {} +
