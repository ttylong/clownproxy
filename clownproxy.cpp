#include "stdio.h"
#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#include <errno.h>
#include <regex>
#include <limits>
#include <fstream>
#include <filesystem>
#include <random>

/**
 * References:
 * https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
 * TA provided code (various)
 * Provided slides
 * Assignment 0 palindrome server code
 * https://stackoverflow.com/questions/20911584/how-to-read-a-file-in-multiple-chunks-until-eof-c/52779086
 * https://stackoverflow.com/questions/7560114/random-number-c-in-some-range
 * https://stackoverflow.com/questions/11508798/conditionally-replace-regex-matches-in-string
 **/

// global constants
#define MAX_MESSAGE_LENGTH 2048
#define MAX_REC 2048 * 10
#define PORTNUM 80 // port to be accessed on web server
#define MAX_IMG_ID 3 // can be changed to accommodate how ever many clown pics are to be used

using namespace std;

int childsock; // glob variable

// signal handler to do graceful exit
void catcher(int sig){
    close(sig);
    exit(0);
}

// helper functions: https://stackoverflow.com/questions/47116974/remove-a-substring-from-a-string-in-c#:~:text=There%20is%20no%20predefined%20function,source%20and%20destination%20arrays%20overlap.
char *strremove(char *str, const char *sub) {
    size_t len = strlen(sub);
    if (len > 0) {
        char *p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}
// randomly selects a clown image using ID within a defined range
string clownSelector(){
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(1, MAX_IMG_ID); // define the range
    int id = distr(gen);
    string name = "./clown" + to_string(id) + ".jpg";
    return name;
}
// converts the word happy to silly
void clown(const char * webresponse, char * proxyresponse, const int64_t webbytes){

    regex n("happy", std::regex_constants::icase);
    string ret = regex_replace(webresponse, n, "Silly");
    const char * temp = ret.data();  
    copy_n(temp, webbytes, proxyresponse);
}
// detects whether content type is an image
bool isImg(const char * webresponse){

    string temp = webresponse;
    regex g ("Content-Type: text/html;");
    smatch m;
    if(regex_search(temp, m, g)){
      return false;
    }
    return true;
}
// detects whether an image content type is of type jpeg
bool isJPG(const char * webresponse){

    string temp = webresponse;
    regex g ("Content-Type: image/jpeg");
    smatch m;
    if(regex_search(temp, m, g)){
      return true;
    }
    return false;
}

int main(int argc, char *argv[]){

    if(argc < 2 || argc > 2){
        cout << "usage: ./clown [portnumber]" << endl;
        return -1;
    }
    int port = atoi(argv[1]);

    // setup
    pid_t pid;
    static struct sigaction act;
    // set up signal handler
    act.sa_handler = catcher;
    sigfillset(&(act.sa_mask));
    sigaction(SIGPIPE, &act, NULL);

    char clientRequest[MAX_MESSAGE_LENGTH];
    char HTTPResponse[MAX_REC];
    char proxyResponse[MAX_REC];
    char proxyRequest[MAX_MESSAGE_LENGTH];

    int bytes;
    int parentsock;

    // begin connection to proxy
    cout << "trying to connect to port " << port << endl;
    struct sockaddr_in address;
    address.sin_family = AF_INET; // using ipv4
    address.sin_port = htons(port); // set port
    address.sin_addr.s_addr = htonl(INADDR_ANY); // accept connection to all IPs of machine

    // creating socket for listening only
    parentsock = socket(AF_INET, SOCK_STREAM, 0);
    if(parentsock == -1){
        cout << "socket() call failed" << endl;
        exit(1);
    }
    // binding (server only)
    int bindstatus = bind(parentsock, (struct sockaddr *) & address, sizeof(struct sockaddr_in));
    if(bindstatus == -1){
        cout << "bind() call failed" << endl;
        cout << errno << endl;
        exit(1);
    }
    // begin listening (server only)
    int lstnstatus = listen(parentsock, 5);
    if(lstnstatus == -1){
        cout << "listen() call failed" << endl;
        exit(1);
    }

    cout <<"Listening to requests on TCP port: " << port << endl;

    // main infinite loop to listen
    string terminalInput;
    while(1){

        childsock = accept(parentsock, NULL, NULL); // accept a connection
        if(childsock == -1){
            printf("accept() call failed\n");
            exit(1);
        }
        else{
            printf("proxy connection accepted\n");
        }

        pid = fork(); // create a child process to deal with client

        // if fork failed
        if(pid < 0){
            printf("fork() failed\n");
            exit(1);
        }
        // if child process, do not need parent listener
        else if(pid == 0){
            close(parentsock);
            // processing the CLIENT request
            bytes = recv(childsock, clientRequest, MAX_MESSAGE_LENGTH, 0); // proxy receiving request from client
            while(bytes > 0){
                if(strcmp(clientRequest, "EXIT") == 0){
                    printf("EXIT detected\n");
                    close(childsock);
                    exit(0);
                }
                // processing/extracting the client's request
                char clientRequestCopy [strlen(clientRequest) + 1];
                strcpy(clientRequestCopy, clientRequest); // preserve original request before strtok
                char * getReq = strtok(clientRequest, "\r\n");

                char * hostName = strtok(NULL, "\r");
                hostName = strtok(hostName, " ");
                hostName = strtok(NULL, " ");

                char * getReqCopy = (char *)malloc(strlen(getReq) + 1);
                strcpy(getReqCopy, getReq); // preserve original get request before strtok

                char * url = strtok(getReq, " ");
                url = strtok(NULL, " ");

                char path[100];
                sscanf(url, "http://%s[^@]", path);
                char * pathPtr = strremove(path, hostName);

                // relevant information extracted, do it all again to make proxy act as client
                struct sockaddr_in server_addr;
                server_addr.sin_family = AF_INET; // using ipv4
                server_addr.sin_port = htons(PORTNUM); // set port
                struct hostent * address;
                address = gethostbyname(hostName);
                // set the IP of server socket to IP of server  
                bcopy((char *) address -> h_addr, (char *) &server_addr.sin_addr.s_addr, 
                    address -> h_length);

                // connect to specified port of server
                cout << "trying to connect to server port " << PORTNUM << endl;
                int serverSock = socket(AF_INET, SOCK_STREAM, 0);
                if(serverSock == -1){
                    cout << "socket() call to server failed, errno " << errno << endl;
                    exit(1);
                }
                // try to establish a connection with the server / website
                int serverStatus = connect (serverSock, (struct sockaddr *) & server_addr, sizeof(struct sockaddr_in));
                if(serverStatus == -1){
                    cout << "connect() call to server failed, errno " << errno << endl;
                    exit(1);
                }
                else{
                    cout << "connection to server accepted" << endl;
                }
                // create a request
                memset(HTTPResponse, 0, MAX_REC);
                sprintf(proxyRequest, "GET %s HTTP/1.1\r\nHost: %s\r\nContent-Type: */*\r\nConnection: keep-alive\r\n\r\n", pathPtr, hostName); // create an http request
                // send request to server
                int count = send(serverSock, proxyRequest, MAX_MESSAGE_LENGTH, 0);
                if(count == -1){
                    cout << "send() call to server failed" << endl;
                    exit(1);
                }
                // receive response from web server
                int webbytes = recv(serverSock, HTTPResponse, MAX_REC, 0);
                if(webbytes == -1){
                    cout << "receive call to server failed" << endl;
                    exit(1);
                }
                // while receiving data from the http response, do something
                while(webbytes > 0){
                    int responsestatus; // number of bytes returned
                    // detect if text or binary is returned
                    if(!isImg(HTTPResponse)){
                        clown(HTTPResponse, proxyResponse, webbytes); // html text detected, alter the html file to sub silly for happy
                    }
                    else{
                        // jpg detected. stop receiving response from the server, and send image of clown instead
                        if(isJPG(HTTPResponse)){
                            string filename = clownSelector(); // return a random clown image from a small set of images
                            ifstream img(filename, ios::binary);
                            if(img.is_open()){
                                vector<char> buffer(MAX_REC, 0);
                                size_t filesize = std::filesystem::file_size(filename); // get number of bytes of image to send in http response
                                // create a new http response and send to client
                                int httprespbytes = sprintf(proxyResponse, "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\nContent-Length: %zu\r\nConnection: close\r\nContent-Type: image/jpeg\r\n\r\n", filesize);
                                responsestatus = send(childsock, proxyResponse, httprespbytes, 0);
                                if(responsestatus == -1){
                                    cout << "send() call to client failed" << endl;
                                    cout << "ERRNO: " << errno << endl;
                                    exit(1);
                                }
                                // while loop is the equivalent of while recv() > 0, but for reading in local bin data
                                while(!img.eof()){
                                    img.read(buffer.data(), buffer.size());
                                    std::streamsize sendsize = img.gcount(); // get size of the bytes sent to buffer

                                    responsestatus = send(childsock, &buffer[0], sendsize, 0);
                                    if(responsestatus == -1){
                                        cout << "send() call to client failed" << endl;
                                        cout << "ERRNO: " << errno << endl;
                                        exit(1);
                                    }
                                }
                            }
                            else{
                                cout << "Failed to open local image\n";
                                exit(1);
                            }
                            img.close();
                            break; // break out of loop to stop receiving response from server for the jpg image, as a pic of a clown was sent instead
                        }
                        copy_n(HTTPResponse, webbytes, proxyResponse); // if not a jpg image, send along.
                    }
                    responsestatus = send(childsock, proxyResponse, webbytes, 0); // send bytes to the client
                    if(responsestatus == -1){
                        cout << "send() call to client failed" << endl;
                        cout << "ERRNO: " << errno << endl;
                        exit(1);
                    }
                    webbytes = recv(serverSock, HTTPResponse, MAX_REC, 0); // receive next batch of data
                }
                // getting next request
                bytes = recv(childsock, clientRequest, MAX_MESSAGE_LENGTH, 0);
            }
            // client no longer sending info, closing child
            close(childsock);
            exit(0);
        }
        // if parent process, get rid of childsock
        else{
            close(childsock);
        }
    }
} 
