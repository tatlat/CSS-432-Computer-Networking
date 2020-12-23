/**
 * Author: Tanvir Tatla
 * Description: This is a client program that connects to a server and then
 *              sends data to the server. The client also records the time
 *              it took to send the data and the time it took for the server
 *              to respond. The client should receive the number of reads made
 *              by the server.
**/
#include <iostream> // cout
#include <netdb.h> // addrinfo
#include <cstring> // memset
#include <sys/time.h> // timeval and timersub
#include <unistd.h> // read, write, close
#include <sys/uio.h> // writev
#include <stdexcept> // stoi exceptions


using namespace std;

// write types
const int MULTIPLE_WRITES = 1;
const int WRITEV = 2;
const int SINGLE_WRITE = 3;
const int BUFSIZE = 1500; // cumulative size of all data buffers (nbufs * bufsize)

int serverPort; // server's port number
char *serverName; // server's IP address or hostname
int repetition; // the number of iterations a client performs on data transmission using write type
int nbufs; // number of data buffers
int bufsize; // size of each data biffer
int type; // the type of transfer scenario: 1, 2, or 3

// validates the 6 arguments passed into main
// return true if all are valid, false otherwise
bool validateArgs(char *args[]) {
    try {
        // convert strings to int
        serverPort = stoi(args[1]);
        serverName = args[2];
        repetition = stoi(args[3]);
        nbufs = stoi(args[4]);
        bufsize = stoi(args[5]);
        type = stoi(args[6]);
    } catch(const invalid_argument& e) {
        cout << "Please enter valid integers" << endl;
        return false;
    } catch(const out_of_range& e) {
        cout << "Integer overflow";
        return false;
    }

    // highest port number is 65535. ports less than 1024 require priviliged account (root permissions) 
    if (serverPort > 65535 || serverPort < 1024) {
        cout << "Port must be between 1024 and 65535" << endl;
        return false;
    }

    // IP address or hostname can't be empty
    string temp = serverName;
    if (temp.empty()) {
        cout << "Please enter a server name" << endl;
        return false;
    }

    // negative repetitions are not possible
    if (repetition < 0) {
        cout << "Repetitions cannot be less than zero" << endl;
        return false;
    }

    // Total buffsize should be 1500
    if (nbufs * bufsize != BUFSIZE)
    {
        cout << "Number of buffers times buffer size does not equal" 
        + to_string(BUFSIZE) << endl;
        return false;
    }

    if (type < 1 || type > 3)
    {
        cout << "Type must be between 1 and 3" << endl;
        return false;
    }
    
    return true;
}

// multipleWrites invokes the write( ) system call for each data buffer, 
// thus resulting in calling as many write( )s as the number of data buffers, 
// (i.e., nbufs). 
void multipleWrites(int clientSd) {
    char databuf[nbufs][bufsize]; // where nbufs * bufsize = 1500
    for ( int j = 0; j < nbufs; j++ ) {
        write( clientSd, databuf[j], bufsize ); // clientSd: socket descriptor
    }
}

// writevHelper allocates an array of iovec data structures, each having its *iov_base field
// point to a different data buffer as well as storing the buffer size in
// its iov_len field; and thereafter calls writev( ) to send all data buffers at once.
void writevHelper(int clientSd) {
    char databuf[nbufs][bufsize]; // where nbufs * bufsize = 1500
    struct iovec vector[nbufs];
    for ( int j = 0; j < nbufs; j++ ) {
        vector[j].iov_base = databuf[j];
        vector[j].iov_len = bufsize;
    }
    writev( clientSd, vector, nbufs ); // clientSd: socket descriptor
}

// singleWrite allocates an nbufs-sized array of data buffers, and thereafter calls
// write( ) to send this array, (i.e., all data buffers) at once. 
void singleWrite(int clientSd) {
    char databuf[nbufs][bufsize]; // where nbufs * bufsize = 1500
    write( clientSd, databuf, nbufs * bufsize ); // clientSd: socket descriptor
}

typedef void(&func)(int); // using func to return void functions

// getTest returns the corresponding function to type
func getTest() {
    if (type == MULTIPLE_WRITES) return multipleWrites; // type 1
    if (type == WRITEV) return writevHelper; // type 2
    if (type == SINGLE_WRITE) return singleWrite; // type 3
}

// printStatistices calculates the transmission time and roundtrip time using timeval and
// then prints the transmission time, round-trip time, and the number of times the server
// called read( ). 
// start is right before client started the write test, lap is when the write test finished,
// end is when the server responded. numReads is the number of times server called read( ).
void printStatistics(struct timeval start, struct timeval lap, struct timeval stop, int numReads) {
    struct timeval trans, round;
    timersub(&lap, &start, &trans); // trans = lap - start
    timersub(&stop, &start, &round); // round = stop - start

    // get lapsed time in usec
    long transmissionTime = trans.tv_sec * 1000000L + trans.tv_usec;
    long roundTripTime = round.tv_sec * 1000000L + round.tv_usec;
    // print times
    cout << "Test 1: data-transmission time = " + to_string(transmissionTime) + " usec, ";
    cout << "round-trip time = " + to_string(roundTripTime) + " usec, ";
    cout << "#reads = " + to_string(numReads) << endl;
}

// main takes arguments from commandline and attempts to connect to server
// Once connected, client sends data to server and waits for response.
// Upon receiving response, it outputs response and then terminates.
// returns 0 for success, -1 for failure.
// numArgs is the number of arguments being passed, *args[] is the arguments.
// args should be in format ./ProgramName serverPort serverName repetition nbufs bufsize type
int main (int numArgs, char *args[]) {
    // first argument is program name so there should be 7 arguments in total
    if (numArgs != 7)
    {
        cout << "Incorrect number of arguments provided" << endl;
        return -1;
    }

    // terminate if invalid arguments
    if (!validateArgs(args)) {
        cout << "One or more arguments was invalid" << endl;
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo *servInfo; // list of socket addresses from getaddrinfo
    memset(&hints, 0, sizeof(hints) );
    hints.ai_family = AF_UNSPEC; // Address Family Internet
    hints.ai_socktype = SOCK_STREAM; // TCP
    int status = getaddrinfo(serverName, args[1], &hints, &servInfo );

    // terminate if getaddrinfo failed
    if (status != 0) {
        cout << gai_strerror(status) << endl;
        return -1;
    }

    struct addrinfo *p; // current address
    int clientSd, connection;

    // iterate over list of socket addresses
    for (p = servInfo; p != nullptr; p = servInfo->ai_next) {
        // create socket
        clientSd = socket( p->ai_family, p->ai_socktype, p->ai_protocol );

        // try next address
        if (clientSd == -1) continue;

        // open connection on socket file descriptor (clientSd)
        connection = connect( clientSd, p->ai_addr, p->ai_addrlen);

        // try next address if connect failed
        if (connection == -1) continue;
        
        break; // stop if socket and connect succeeded
    }

    // all addresses failed, terminate
    if (!p) {
        cout << "Unable to connect" << endl;
        return -1;
    }

    freeaddrinfo(servInfo);
    struct timeval start , lap , stop;
    gettimeofday(&start , NULL); // start time

    func test = getTest(); // test is the function that corresponds to type (e.g. singleWrite)

    // call test until repetitions satisfied
    for (int i = 0; i < repetition; i++) {
        test(clientSd);
    }

    gettimeofday(&lap, nullptr);
    int temp, numReads;

    // get server's response
    int readResult = read(clientSd, &temp, sizeof(temp));

    if (readResult == -1) {
        cout << "Unable to read." << endl;
        return -1;
    }

    numReads = ntohl(temp);
    gettimeofday(&stop, nullptr);
    printStatistics(start, lap, stop, numReads);
    close(clientSd);
    return 0;
}