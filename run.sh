apt install libjsoncpp-dev libssl-dev
g++ src/main.cpp -o bin/main -std=c++17 -ljsoncpp -lssl -lcrypto

if [ $? == 0 ]; then
    bin/main
else
    echo "Compilation failed"
fi