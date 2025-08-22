mkdir -p build

cp .\info\ .\build\

cd build

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="azahar" -DSYSTEM_NAME="Nintendo - Nintendo 3DS" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="duckstation" -DSYSTEM_NAME="Sony - Playstation" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="mgba" -DSYSTEM_NAME="Nintendo - Game Boy Advance" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="melonds" -DSYSTEM_NAME="Nintendo - Nintendo DS" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="pcsx2" -DSYSTEM_NAME="Sony - Playstation 2" .. && cmake --build .  --parallel

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="xemu" -DSYSTEM_NAME="Microsoft - Xbox" .. && cmake --build . --parallel

cmake -DCMAKE_BUILD_TYPE=Release -DCORE="xenia" -DSYSTEM_NAME="Microsoft - Xbox 360" .. && cmake --build . --parallel

cd Debug

Get-ChildItem -Path . -Recurse -File |
    Where-Object { $_.Extension -notin ".dll", ".so" } |
    Remove-Item -Force