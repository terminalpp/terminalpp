#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "helpers/helpers.h"


int main(int argc, char * argv[]) {
    MARK_AS_UNUSED(argc);
    MARK_AS_UNUSED(argv);

    int sockFd = 0;
    int opt = 1;

    OSCHECK((sockFd = socket(AF_INET, SOCK_STREAM, 0)) != 0) << "Unable to create socket";
    OSCHECK(! setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) << "Unable to set socket options";

    struct sockaddr_in address;
    int addrlen = sizeof(address);     
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8123);

    OSCHECK(! bind(sockFd, (struct sockaddr *)&address, sizeof(address))) << "Unable to bind";
    OSCHECK(! listen(sockFd, 3)) << "cannot start listening";

    int clientSock;

    OSCHECK((clientSock = accept(sockFd, (struct sockaddr *)&address, (socklen_t*) & addrlen)) > 0) <<     "cannot get client";
    std::cout << "Client accepted!" << std::endl;
    char buffer[1024];
    int cnt = read(clientSock, (void*) buffer, 1024);
    std::cout << "Read " << cnt << " bytes: "; 
    for (int i = 0; i < cnt; ++i)
        std::cout << buffer[i];
    std::cout << std::endl;
    std::string test = "Hello all this is a test of the stuff how it works and so on";
    send(clientSock, test.c_str(), test.size(), 0);
    std::cout << "data sent.";

    return EXIT_SUCCESS;
}
