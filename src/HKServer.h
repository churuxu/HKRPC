#pragma once

//#include "HKSession.h"
#include "pch.h"

typedef int ConnectionPtr;

typedef std::function<void(ConnectionPtr)> ConnectionHandler;
typedef std::function<void(ConnectionPtr, const Buffer& data)> ConnectionDataHandler;

class HKServer {
public:
	static HKServer* getInstance();

	virtual void start() = 0;

	virtual void stop() = 0;

	virtual void setOnConnect(ConnectionHandler handler) = 0;

	virtual void setOnClose(ConnectionHandler handler) = 0;

	virtual void setOnData(ConnectionDataHandler handler) = 0;
	
	virtual void sendData(ConnectionPtr conn, const Buffer& data)=0;

	virtual void close(ConnectionPtr conn) = 0;
};
