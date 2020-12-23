/**
 * Author: Tanvir Tatla
 * Description: This is a client program that connects to a web server and then
 *              sends a HTTP request to the server. The client then waits for
 *              a response and closes the connection upon receiving it. 
 *              If the server responds with a 200 OK, client saves the response
 *              body to output.txt and displays the output. Otherwise the 
 *              error is displayed.
**/
#include <iostream> // cout
#include <fstream> // file
#include <netdb.h> // addrinfo
#include <cstring> // memset
#include <unistd.h> // read, write, close
#include <sys/uio.h> // writev
#include <stdexcept> // stoi exceptions
#include<bits/stdc++.h> 


using namespace std;

string OUTPUT_FILE = "output.txt"; // Response body written here

char *serverPort; // server's port number
char *serverName; // server's IP address or hostname
char *filePath; // path of requested file

// parseHeader receives the response from the server one character at a
// time until a double line break is encountered.
// Returns the status code of the response;
// sd = socket file descriptor, status = string for storing status line,
// bufsize = int for storing buffer size
int parseHeader(int sd, string& status, int& bufsize) {
    string header;
    char curr;
    int code;

    // keep receiving until double line break encountered
    for(;;) {
        recv(sd, &curr, sizeof(char), 0);
        header += curr;
        if (header.find("\r\n\r\n") != -1) break;
    }

    cout << "Response header:" << endl << header;

    // beginning of header to first line break is status line
    status = header.substr(0, header.find("\r\n"));
    code = atoi(status.substr(9).c_str()); // Status code comes after 9th char
    string content = "";

    string temp(header);
    transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
    int c = temp.find("CONTENT-LENGTH: ");
    
    // if header contains Content-Length field
    if (c != -1) {
        content = header.substr(c + 16);
        bufsize = stoi(content.c_str()); // get length
    } else {
        bufsize = -1; // No content-length field
    }
    
    return code;
}

// parseBody receives the body of the response, saves it to a file, and 
// displays the output
void parseBody(int sd, int bufsize) {
    char buffer[bufsize]; // Allocate a buffer to store the content
    recv(sd, &buffer, bufsize, 0);

    ofstream file;
    file.open(OUTPUT_FILE);

    // loop over body
    for (char c : buffer) {
        file << c; // write body to file
        cout << c; // display output
    }

    file.close();
    cout << endl << endl;
}

// parseError receives the body of a 404 and displays it.
// sd = socket file descriptor, bufsize = size of buffer
void parseError(int sd, int bufsize) {
    string body;

    // if no body, return
    if (bufsize == 0) {
        return;
    }

    // if content-length field exists
    if (bufsize > 0) {
        char buffer[bufsize]; // Allocate a buffer to store the content
        recv(sd, &buffer, bufsize, 0); // get body
        body = string(&buffer[0], bufsize);
    } else {
      // otherwise read the body until the server stops sending data
        char curr = ' ';
        while (recv(sd, &curr, sizeof(char), 0) > 0) {
            body += curr;
        }
    }

    // loop over body
    for (char c : body) {
        cout << c; // output body
    }

    cout << endl << endl;
}

// makeRequest creates and sends a HTTP request to the web server.
// Then it receives the response and outputs it.
void makeRequest(int sd) {
    string request = "GET " + string(filePath) + " HTTP/1.1\r\n";
    request += "Host: " + string(serverName) + "\r\n";
    request += "\r\n";

    int success = send(sd, request.c_str(), strlen(request.c_str()), 0);
    if (success == -1) {
        cout << "Could not send request" << endl;
        return;
    }

    cout << "Sent Request" << endl << request;

    // get server's response
    string status;
    int bufsize;
    int code = parseHeader(sd, status, bufsize);
    
    cout << "Response Body:" << endl;

    if (code == 200) {
        parseBody(sd, bufsize);
    } else if (code == 404) {
        parseError(sd, bufsize);
    }
}

// openConnection creates a socket so this client can communicate with
// the web server. 
int openConnection() {
    // open connection
    struct addrinfo hints;
    struct addrinfo *servInfo; // list of socket addresses from getaddrinfo
    memset(&hints, 0, sizeof(hints) );
    hints.ai_family = AF_UNSPEC; // Address Family Internet
    hints.ai_socktype = SOCK_STREAM; // TCP
    int status = getaddrinfo(serverName, serverPort, &hints, &servInfo );

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

    return clientSd;
}

// main takes arguments from commandline and attempts to connect to server
// Once connected, client sends data to server and waits for response.
// Upon receiving response, it outputs response and then terminates.
// returns 0 for success, -1 for failure.
// numArgs is the number of arguments being passed, *args[] is the arguments.
// args should be in format ./ProgramName serverPort serverName filePath
int main (int numArgs, char *args[]) {
    if (numArgs != 4)
    {
        cout << "Incorrect number of arguments provided" << endl;
        return -1;
    }

    serverPort = args[1];
    serverName = args[2];
    filePath = args[3];

    // get output file's name
    OUTPUT_FILE = string(filePath);
    int oEnd = OUTPUT_FILE.find_last_of("/") + 1; // if file is in subdirectory
    OUTPUT_FILE.erase(0, oEnd);
    if (OUTPUT_FILE == ".txt" || OUTPUT_FILE.empty()) {
        OUTPUT_FILE = string(serverName) + ".txt";
    }

    int clientSd = openConnection(); // create socket

    if (clientSd == -1) {
        cout << "Could not connect to server." << endl;
        return -1;
    }

    makeRequest(clientSd); // send a request and output the response
    close(clientSd); // close connection
    cout << endl;
    return 0;
}