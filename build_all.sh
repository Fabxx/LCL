#!/bin/bash

mkdir -p build

cp info build

cd build 

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="azahar" -DSYSTEM_NAME="Nintendo - Nintendo 3DS" .. && cmake --build .

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="duckstation" -DSYSTEM_NAME="Sony - Playstation" .. && cmake --build .

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="mgba" -DSYSTEM_NAME="Nintendo - Game Boy Advance" .. && cmake --build .

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="melonds" -DSYSTEM_NAME="Nintendo - Nintendo DS" .. && cmake --build .

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="pcsx2" -DSYSTEM_NAME="Sony - Playstation 2" .. && cmake --build .

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="xemu" -DSYSTEM_NAME="Microsoft - Xbox" .. && cmake --build .

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="xenia" -DSYSTEM_NAME="Microsoft - Xbox 360" .. && cmake --build .

find . -type f ! \( -name "*.dll" -o -name "*.so" \) -exec rm -f {} +
