/**
 * Author: Tanvir Tatla
 * Description: This is a server program that awaits a client's request to connect.
 *              The server then reads the data sent by the client and responds with
 *              the number of times the server took to read the data. 
**/
#include <iostream> // cout
#include <cstring> // memset
#include <stdexcept> // stoi exceptions
#include <sys/types.h> // socket, bind
#include <sys/socket.h> // socket, bind, listen, inet_ntoa
#include <netinet/in.h> // htonl, htons, inet_ntoa
#include <arpa/inet.h> // inet_ntoa
#include <netdb.h> // addrinfo
#include <unistd.h> // read, write, close
#include <strings.h> // bzero
#include <netinet/tcp.h> // SO_REUSEADDR
#include <sys/uio.h> // writev
#include <sys/time.h> // timeval and timersub

using namespace std;

const int BUFSIZE = 1500;
const int NUM_CONNECTIONS = 10;

int port; // server's port number
int repetition; // the repetition of client's data transmission activities. Should be same as client.

// validates the 2 arguments passed into main
// port must be between 1024 and 65535
// repetition must be 0 or greater
// return true if all are valid, false otherwise
bool validateArgs(char *args[]) {
    try {
        // convert string arguments to int
        port = stoi(args[1]);
        repetition = stoi(args[2]);
    } catch(const invalid_argument& e) {
        cout << "Please enter valid integers" << endl;
        return false;
    } catch(const out_of_range& e) {
        cout << "Integer overflow" << endl;
        return false;
    }

    if (port > 65535 || port < 1024) {
        cout << "Port must be between 1024 and 65535" << endl;
        return false;
    }

    if (repetition < 0) {
        cout << "Repetitions cannot be less than zero" << endl;
        return false;
    }

    return true;
}

// printStatistics calculates the time taken to receive all data from client
// and prints the receiving time. 
void printStatistics(struct timeval start, struct timeval stop) {
    struct timeval rec;
    timersub(&stop, &start, &rec); //get diff between stop and start, assign diff to rec
    long receiveTime = rec.tv_sec * 1000000L + rec.tv_usec; // time in microseconds
    cout << "data-receiving time = " + to_string(receiveTime) + " usec" << endl;
}

// evaluatePerformance records the time the server starts reading client's data
// and the time it finishes reading data. Then it sends the number of times the
// server called read( ) to the client. Finally, it prints the time takne to read
// all incoming data, and closes the connection to client. 
// evaluatePerformance is called by a pthread. 
void *evaluatePerformance(void *data) {
    int sd = *(int*) data;
    char databuf[BUFSIZE];
    int count = 0; // number of reads
    struct timeval start, stop;
    gettimeofday(&start , NULL); // record start time

    // repeat same number of repetitions as client.
    for (int i = 0; i < repetition; i++) {
        for ( int nRead = 0; 
            ( nRead += read( sd, databuf, BUFSIZE - nRead ) ) < BUFSIZE; 
            ++count );
        count++;
    }
    
    gettimeofday(&stop , NULL); // record end time
    int temp = htonl(count);
    write(sd, &temp, sizeof(temp)); // send number of reads
    printStatistics(start, stop); // print receive times
    close(sd); // close connection
}

// main creates a TCP socket that listens on the port given as an argument. 
// The server will accept an incoming connection and then create a new
// thread that will handle the connection. The new thread will read all the 
// data from the client and respond back to it. The details of the response 
// can be found in the evaluatePerformance function. 
// Returns 0 on success, or -1 on failure.
// numArgs is the number of arguments passed, *args[] is the arguments
// args must be formatted as ./ProgramName port repetition
int main(int numArgs, char *args[]) {
    if (numArgs != 3)
    {
        cout << "Incorrect number of arguments provided" << endl;
        return -1;
    }

    if (!validateArgs(args)) {
        cout << "One or more arguments was invalid." << endl;
        return -1;
    }

    struct addrinfo hints, *res; // res is a list of addresses
    memset( &hints, 0, sizeof (hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int status = getaddrinfo(NULL, args[1], &hints, &res);

    if (status != 0) {
        cout << gai_strerror(status) << endl;
        return -1;
    }

    struct addrinfo *p; // current address
    int serverSd, optResult, bindResult;
    
    // iterate over list of addresses.
    for (p = res; p != nullptr; p = res->ai_next) {
        // create TCP socket
        serverSd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (serverSd == -1) continue;

        // set socket option
        const int yes = 1;
        optResult = setsockopt( serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes,sizeof( yes ) );

        if (optResult == -1) continue;

        bindResult = bind( serverSd, p->ai_addr, p->ai_addrlen );

        if (bindResult == -1) {
            close(serverSd);
            continue;
        }

        break; // success
    }

    if (!p) {
        cout << "Unable to connect" << endl;
        return -1;
    }

    freeaddrinfo(res);
    int listenResult = listen(serverSd, NUM_CONNECTIONS); // prepare to accept connections

    if (listenResult == -1) {
        cout << "Unable to listen." << endl;
        return -1;
    }

    // run indefinitely to accept incoming connections
    while (true) {
        struct sockaddr_storage newSockAddr;
        socklen_t newSockAddrSize = sizeof( newSockAddr );
        // await connection request, open new socket upon connection
        int newSd = accept( serverSd, (struct sockaddr *)&newSockAddr, &newSockAddrSize );

        if (newSd == -1) {
            cout << "Unable to accept client connection request." << endl;
            continue;
        }

        pthread_t thread; // thread to handle new client
        int result = pthread_create(&thread, nullptr, evaluatePerformance, (void*) &newSd);

        if (result != 0) {
            cout << "Unable to create thread." << endl;
            continue;
        }
    }

    return 0;
}