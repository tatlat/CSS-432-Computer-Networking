/**
 * Author: Tanvir Tatla
 * Description: This is a web server program that awaits a client's HTTP request.
 *              The server then sends a HTTP response to the client and closes
 *              the connection.
**/
#include <iostream> // cout
#include <fstream> // file
#include <sstream> // stringstream
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

const int BUFFSIZE = 1500;
const int NUM_CONNECTIONS = 10;

// HTTP Response Codes
const string BAD_REQUEST = "400 Bad Request\r\n\r\n";
const string FORBIDDEN = "403 Forbidden\r\n\r\n";
const string UNAUTHORIZED = "401 Unauthorized\r\n\r\n";
const string NOT_FOUND = "404 Not Found\r\n";
const string OK = "200 OK\r\n";

// Custom html pages
const string SECRET_FILE = "SecretFile.html";
const string NOT_FOUND_PAGE = "404.html";
const string HOME_PAGE = "index.html";

char* port; // server's port number

// isSecret checks if a file is unauthorized
// returns true if unauthorized access
bool isSecret(string file) {
    return (file.find(SECRET_FILE) != -1);
}

// prepareResponse prepares a response to the request
// returns the appropriate response based on the file.
// returns NOT_FOUND_PAGE if file not found
string prepareResponse(string filePath) {
    string response;
    string content;
    if (filePath == "./") {
        filePath = HOME_PAGE;
    }
    ifstream file(filePath); // attempt to open file

    // file not found
    if (!file) {
        response += NOT_FOUND;
        ifstream f(NOT_FOUND_PAGE); // open custom 404 page
        stringstream s;
        s << f.rdbuf();
        content = s.str(); // add 404 page to reponse body
    } else { // file exists
        response += OK;
        stringstream s;
        s << file.rdbuf();
        content = s.str(); // add file contents to response body
    }

    string contentLength = to_string(content.length()); // length of content
    response += "Content-Type: text/html\r\n"; // content-type header
    response += "Content-Length: " + contentLength + "\r\n\r\n"; //content-length header
    response += content; // append body to response

    return response;
}

// parseRequest receives the request from the client
// returns the appropriate response to the request.
// sd = socket file descriptor
string parseRequest(int sd) {
    string request;
    string filePath;
    char curr;

    char buffer[BUFFSIZE];
    while (recv(sd, &buffer, BUFFSIZE, 0) > 0)
    {
        request += buffer;
        int end = request.find_last_of("\r\n\r\n") + 1;
        if (end > 0) {
            request.erase(end, request.size() - end);
            break;
        }
    }
    

    cout << "Received Request:" << endl << request;
    int begin = request.find("GET") + 4; // start (index) of requested file
    int ending = request.find(" HTTP") - 4; // end of requested file

    // improper HTTP request if GET and HTTP not found
    if (begin == -1 || ending == -1) {
        return BAD_REQUEST;
    }

    try {
        filePath = "." + request.substr(begin, ending); // get file path
        if (filePath.find("./") == -1) { // filePath should be formatted /file (not just /)
            return BAD_REQUEST;
        }
    } catch(out_of_range e) {
        return BAD_REQUEST;
    }

    // trying to access parent directories is forbidden
    if (filePath.substr(0, 2) == "..") {
        return FORBIDDEN;
    }

    // if the file is unauthorized
    if (isSecret(filePath)) {
        return UNAUTHORIZED;
    }

    return prepareResponse(filePath); // return the response based on the file
}

// handleRequest sends a HTTP response to the client and then closes
// the connection.
// data = socket file descriptor (int)
// evaluatePerformance is called by a pthread. 
void *handleRequest(void *data) {
    int sd = *(int*) data; // cast to int and dereference
    string response;
    response = "HTTP/1.1 " + parseRequest(sd); // prepare response based on request
    cout << "Sending response:" << endl << response;
    if (response.substr(response.size() - 2) != "\r\n") {
        cout << endl;
    }
    const char* r = response.c_str();
    send(sd, r, strlen(r), 0); // send response
    cout << "Closing connection" << endl << endl;
    close(sd); // close connection
}

// createSocket opens a TCP socket for listening to clients
// returns the socket file descriptor
int createSocket() {
    struct addrinfo hints, *res; // res is a list of addresses
    memset( &hints, 0, sizeof (hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int status = getaddrinfo(nullptr, port, &hints, &res);

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
    return serverSd;
}

// main creates a TCP socket that listens on the port given as an argument. 
// The server will accept an incoming connection and then create a new
// thread that will handle the connection. The new thread will read all the 
// data from the client and respond back to it. The details of the response 
// can be found in the handleRequest function. 
// Returns 0 on success, or -1 on failure.
// arguments should be in format: ./program port
int main(int numArgs, char *args[]) {
    if (numArgs != 2) {
        cout << "Please enter a port number" << endl;
        return -1;
    }

    port = args[1];
    int serverSd = createSocket(); // create socket for listening
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

        cout << "Accepted a client." << endl;
        pthread_t thread; // thread to handle new client
        int result = pthread_create(&thread, nullptr, handleRequest, (void*) &newSd);

        if (result != 0) {
            cout << "Unable to create thread." << endl;
            continue;
        }
    }

    return 0;
}