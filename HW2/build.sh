#!/bin/bash
# if error, run dos2unix build.sh
g++ -std=c++11 -o server Server.cpp -lpthread
cd retriever_testing
g++ -std=c++11 -o retriever Retriever.cpp