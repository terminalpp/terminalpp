#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

#include "helpers/helpers.h"



int main(int argc, char * argv[]) {
    MARK_AS_UNUSED(argc);
    MARK_AS_UNUSED(argv);

    return EXIT_SUCCESS;

    int sockFd = 0;
    int opt = 1;

    OSCHECK(sockFd = socket(AF_INET, SOCK_STREAM, 0)) << "Unable to create socket";


    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8123);

    bind(sockFd, (struct sockaddr *)&address, sizeof(address));
    listen(sockFd, 3); // 
    



    return EXIT_SUCCESS;
}
