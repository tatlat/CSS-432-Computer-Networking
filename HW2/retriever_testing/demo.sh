#!/bin/bash
# Remember to run server-demo first
./retriever 3400 $1 /test.txt # 200 OK
./retriever 3400 $1 /fake.txt # 404 NOT FOUND
./retriever 3400 $1 /SecretFile.html # 401 UNAUTHORIZED
./retriever 3400 $1 ../upper.txt # 403 FORBIDDEN
./retriever 3400 $1 test.txt # 400 BAD REQUEST
./retriever 3400 $1 /files/test2.txt # 200 OK in lower directory
./retriever 80 corndog.io / # Access real server