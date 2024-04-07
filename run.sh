# sudo apt install libjsoncpp-dev libssl-dev
g++ src/main.cpp -o bin/main -std=c++17 -ljsoncpp -lssl -lcrypto
bin/main