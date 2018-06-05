#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <string>
#include "threadpool/ThreadPool.h"

#define PORT 5000

void respond(int sock);

std::string ROOT;


int main(int argc, char *argv[]) {
    int sockfd, portno = PORT;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    ROOT = getenv("PWD");

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Make port reusable */
    int tr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Bind socket */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen on socket */
    if (listen(sockfd, 1000) < 0) {
        perror("ERROR on listening");
        exit(-1);
    }

    printf("Server is running on port %d\n", portno);

    clilen = sizeof(serv_addr);

    ThreadPool pool(4);

    while (1) {
        /* Acccepting connections */
        int newsockfd = accept(sockfd, (struct sockaddr *) &serv_addr, (socklen_t *) &clilen);
        if (newsockfd < 0) {
            printf("ERROR on accepting");
            exit(-1);
        }

        pool.enqueue([](int newsock) {
                         respond(newsock);
                     }, newsockfd
        );
    }

    return 0;
}

void *make_long(void *b, char c, int len)
{
    char *b_char = (char *)b;

    if (b == NULL) return NULL;

    while(*b_char && len > 0)
    {
        *b_char = c;
        b_char++;
        len--;
    }

    return b; 
}


long get_file_size(std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    if(rc == 0) {
        return stat_buf.st_size; }
    else 
        return -1;
}

bool str_ending(const std::string &a, const std::string &b) {
    auto len = b.length();
    auto pos = a.length() - len;
    if (pos < 0)
        return false;
    auto pos_a = &a[pos];
    auto pos_b = &b[0];
    while (*pos_a)
        if (*pos_a++ != *pos_b++)
            return false;
    return true;
}




void respond(int sock) {
    int n;
    char buffer[9999];
    char abs_path[256];
    char send_buff[8192];
    std::ifstream in;
    std::string response;
    
    bzero(buffer, 9999);
    n = recv(sock, buffer, 9999, 0);
    if (n < 0) {
        printf("recv() error\n");
        return;
    } else if (n == 0) {
        printf("Client disconnected unexpectedly\n");
        return;
    } else {
        char *token = strtok(buffer, "\r\n\r\n");
        char cpy[256];
        strcpy(cpy, token);

        std::string path = strtok(cpy, " \t");

        path = strtok(NULL, " \t");

        if (strncmp("/", path.c_str(), path.length()) == 0) {
            path = "/index.html";
        }

    long sz = get_file_size(ROOT + path.c_str());
    std::string p = ROOT+path.c_str();

        if(!(str_ending(path, ".jpg")))
        {
            //long sz = get_file_size(ROOT + path.c_str());
            response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html; charset=UTF-8\r\n"
                       "Content-Encoding: UTF-8\r\n";
            
            //std::string p =ROOT+path.c_str();     

            in.open(p);

            std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            response += std::to_string(sz) + "\r\n";
            send(sock, response.c_str(), strlen(response.c_str()), 0);
            send(sock, "\n", 1, 0);
            send(sock, contents.c_str(), strlen(contents.c_str()), 0);

        }
        int ss = sizeof(send_buff);
        if (str_ending(path, ".jpg")) {

                    
            if(!in.eof()){
                if(in.is_open()){
                    make_long(send_buff, 0, ss);
                in.read(send_buff, ss);
                send(sock, send_buff, ss, 0);
                }
            }
            response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: image/jpeg\r\n";

            in.open(p, std::ios::binary);
            
            
            if(in.is_open()){

                response += std::to_string(sz) + "\r\n";
                int len = strlen(response.c_str());
                send(sock, response.c_str(), len, 0);
                send(sock, "\n", 1, 0);
            }

            while (!in.eof() )
            {if(in.is_open()) {

                make_long(send_buff, 0, ss);
                in.read(send_buff, ss);
                send(sock, send_buff, ss, 0);

            }
            else{

                send(sock, send_buff, ss, 0);
            }
        }
        } 

        

        
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

}