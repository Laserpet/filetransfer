

//#include <winsock2.h>
//#include <ws2tcpip.h>
#include <iostream>


#include "domain.h"


int main() {
    ServerNode* dfDomain = new ServerNode();
    dfDomain->init();

    while(1)
    {
        sleep(1);
    }
    return 0;
}