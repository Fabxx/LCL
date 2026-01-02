mkdir -p build

cp -r "info" "build"

cd build

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="azahar" -DSYSTEM_NAME="Nintendo - Nintendo 3DS" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="duckstation" -DSYSTEM_NAME="Sony - Playstation" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="mgba" -DSYSTEM_NAME="Nintendo - Game Boy Advance" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="melonds" -DSYSTEM_NAME="Nintendo - Nintendo DS" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="pcsx2" -DSYSTEM_NAME="Sony - Playstation 2" .. && cmake --build .  --parallel

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="xemu" -DSYSTEM_NAME="Microsoft - Xbox" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="xenia" -DSYSTEM_NAME="Microsoft - Xbox 360" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCORE="windows" -DSYSTEM_NAME="Microsoft - Windows" .. && cmake --build . --parallel


find . -type f ! \( -name "*.dll" -o -name "*.so" \) -exec rm -f {} +