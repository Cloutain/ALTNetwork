#pragma once
#include "../iobase/rio.h"
#include <functional>
#include <vector>
#include <thread>

using ALTNetMessageType = enum {
	ALTNetUnknowMessage = -1,
	ALTNetConnectMessage = 0,
	ALTNetQuitMessage = 1,
	ALTNetNormalMessage = 2,
};

using ALTNetMessageHandle = std::function< void (int clientID,
                                               ALTNetMessageType type,
                                               const char *data,
                                               ssize_t size) >;


using ALTNetTcpPacket = struct _ALTNetTcpPacket {
	unsigned char header[4] = { 0 };
	char *data = NULL;
    
	_ALTNetTcpPacket(char *json = NULL) {
		if (json) {
			size_t length = strlen(json) + 1;
			data = (char *)malloc(length);
			memset(data, 0, length);
			header[0] = (unsigned char)(length >> 24);
			header[1] = (unsigned char)(length >> 16);
			header[2] = (unsigned char)(length >> 8);
			header[3] = (unsigned char)(length);
			strcpy(data, json);
		}
	}
    
	~_ALTNetTcpPacket() {
		if (data) {
			free(data);
			data = NULL;
		}
	}
    
	size_t length() const {
        if (*(unsigned int *)header != 0) {
			return (((unsigned char)header[0]) << 24) +
                    (((unsigned char)header[1]) << 16) +
                    (((unsigned char)header[2]) << 8) +
                    (unsigned char)header[3];
        }
		return 0;
	}
};

class ALTNetworkServer {
public:
	ALTNetworkServer(std::string ip, unsigned short port = 0);
    ~ALTNetworkServer();

public:
	int run(ALTNetMessageHandle callback);
	int send(unsigned int clientID, const char *json);
	void stop();

private:
	void workThread();

private:
	std::string ip;
	unsigned short port;
    
	int listenfd;
	int maxfd;
    
	std::thread runloop;
	ALTNetMessageHandle messageHandle;

private:
	bool isWorking;
	std::vector<rio_t> clientfds;
};

