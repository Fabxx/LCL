mkdir -p build

cp .\info\ .\build\

cd build

cmake -DCORE="azahar" -DSYSTEM_NAME="Nintendo - Nintendo 3DS" .. && cmake --build . --config Release --parallel

cmake -DCORE="duckstation" -DSYSTEM_NAME="Sony - Playstation" .. && cmake --build . --config Release --parallel

cmake -DCORE="mgba" -DSYSTEM_NAME="Nintendo - Game Boy Advance" .. && cmake --build . --config Release --parallel

cmake -DCORE="melonds" -DSYSTEM_NAME="Nintendo - Nintendo DS" .. && cmake --build . --config Release --parallel

cmake -DCORE="pcsx2" -DSYSTEM_NAME="Sony - Playstation 2" .. && cmake --build . --config Release  --parallel

cmake -DCORE="xemu" -DSYSTEM_NAME="Microsoft - Xbox" .. && cmake --build . --config Release --parallel

cmake -DCORE="xenia" -DSYSTEM_NAME="Microsoft - Xbox 360" .. && cmake --build . --config Release --parallel

cd Release

Get-ChildItem -Path . -Recurse -File |
    Where-Object { $_.Extension -notin ".dll", ".so" } |
    Remove-Item -Force