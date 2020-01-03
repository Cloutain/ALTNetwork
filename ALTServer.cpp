#include "ALTServer.h"
#include <algorithm>
#include <iostream>

#ifdef WIN32
#include <WS2tcpip.h>
#endif

#define MaxListenFd 1024

static unsigned short getAvaliablePort() {
#ifdef WIN32
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0) 
		return 0;
#endif
    
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (SA *)&addr, sizeof(addr)) != 0)
		return 0;

	struct sockaddr_in connAddr;
	int len = sizeof(connAddr);
	if (getsockname(sockfd, (SA *)&connAddr, (socklen_t *)&len) != 0)
		return 0;

	if (close(sockfd) != 0)
		return 0;

	return ntohs(connAddr.sin_port);
}


ALTNetworkServer::ALTNetworkServer(std::string _ip,
                                   unsigned short _port)
: ip(_ip), port(_port) {
	if (port == 0) {
		port = getAvaliablePort();
		if (port <= 0) {
            std::cout << "getAvaliablePort failed" << std::endl;
			exit(-1);
		}
	}
}


ALTNetworkServer::~ALTNetworkServer() {
	stop();
}

int ALTNetworkServer::run(ALTNetMessageHandle handle) {
#ifdef WIN32
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		return -1;
	}
#endif
    
    do {
        if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            std::cout << "create listen socket failed" << std::endl;
            break;
        }
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        if (bind(listenfd, (SA *)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cout << "bind listen socket failed" << std::endl;
            break;
        }
        
        if (listen(listenfd, MaxListenFd) < 0) {
            std::cout << "listen failed" << std::endl;
            break;
        }

        maxfd = listenfd;
        messageHandle = handle;
        runloop = std::thread{ &ALTNetworkServer::workThread, this };
        runloop.detach();
        return 0;
        
    } while (0);
    
	return -1;
}

int ALTNetworkServer::send(unsigned int clientID, const char *json) {
	ALTNetTcpPacket packet((char *)json);
    
    if (rio_write(clientID, packet.header, 4) == 4) {
        return (int)rio_write(clientID, packet.data, packet.length());
    }
	return -1;
}

void ALTNetworkServer::stop() {
	if (isWorking) {
		isWorking = false;
		close(listenfd);
		listenfd = -1;
        maxfd = -1;
	}
}

void ALTNetworkServer::workThread() {
	isWorking = true;

	fd_set readSet;
	int nReady;

	while (isWorking) {
		FD_ZERO(&readSet);
		FD_SET(listenfd, &readSet);
        
		std::for_each(std::begin(clientfds),
                      std::end(clientfds),
                      [&readSet](rio_t rp) {
			FD_SET(rp.rio_fd, &readSet);
		});

		if ((nReady = select(maxfd + 1, &readSet, NULL, NULL, NULL)) < 0) {
			std::cout << "select failed" << std::endl;
			break;
		} 

		if (FD_ISSET(listenfd, &readSet)) {
			struct sockaddr_in newClientAddr;
			socklen_t newClientAddrLen = sizeof(sockaddr_in);
			int connfd = accept(listenfd, (SA *)&newClientAddr, &newClientAddrLen);
			if (connfd > 0) {
				FD_SET(connfd, &readSet);
                maxfd = std::max(maxfd, connfd);
				rio_t rp;
				rio_readinitb(&rp, connfd);
				clientfds.push_back(rp);
				messageHandle(connfd,
                              ALTNetConnectMessage, //client connect
                              "",
                              0);
			}
            
			if (--nReady == 0) continue;
		}

		for (auto it = clientfds.begin(); it != clientfds.end();) {
			rio_t& rp = (*it);
            
			if (FD_ISSET(rp.rio_fd, &readSet)) {
				ALTNetTcpPacket packet;
				ssize_t nRead = rio_buf_readn(&rp, &packet.header, 4);
                
				if (nRead <= 0) {   //client quit
                    
					messageHandle(rp.rio_fd,
                                  ALTNetQuitMessage,
                                  "",
                                  0);
					close(rp.rio_fd);
					FD_CLR(rp.rio_fd, &readSet);
					clientfds.erase(it++);
				} else if (nRead == 4) {
                    
				    ssize_t total = packet.length();
					if (total > 0) {
						packet.data = (char *)malloc(total);
						memset(packet.data, 0, total);
                        
						if (rio_buf_readn(&rp, packet.data, total) != total) {
							messageHandle(rp.rio_fd,
                                          ALTNetUnknowMessage, //client message error
                                          "",
                                          0);
							free(packet.data);
							packet.data = NULL;
						} else {
                            
							messageHandle(rp.rio_fd,
                                          ALTNetNormalMessage, //client message
                                          packet.data,
                                          packet.length());
						}
					}
					it++;
				}
                
				if (--nReady == 0) break;
			} else {
				it++;
			}
		}
	}
    
	if (listenfd > 0) {
		close(listenfd);
		listenfd = -1;
        maxfd = -1;
	}
    
	isWorking = false;
}
