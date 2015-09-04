/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <net/fdssocket.h>
#include <util/Log.h>
#include <string>

namespace att =  apache::thrift::transport;
namespace atc =  apache::thrift::concurrency;

namespace fds { namespace net {


void SocketEventHandler::onSocketConnect() {
    LOGDEBUG << "socket connected";
}

void SocketEventHandler::onSocketDisconnect() {
    LOGERROR << "socket disconnected";
}

void SocketEventHandler::onSocketClose() {
    LOGDEBUG << "socket closed ";
}

Socket::Socket() : TSocket(),
                   monitor(&mutex),
                   connChecker(&Socket::checkConnection, this) {
}

Socket::Socket(std::string host, int port) : att::TSocket(host, port),
                                             monitor(&mutex),
                                             connChecker(&Socket::checkConnection, this) { // NOLINT
    setLinger(false, 0);
}

void Socket::setEventHandler(SocketEventHandler* evtHandler) {
    this->eventHandler = evtHandler;
}

void Socket::close() {
    bool fWasOpen = fConnected;
    att::TSocket::close();
    fConnected = false;
    if (fWasOpen && eventHandler) eventHandler->onSocketClose();
}

void Socket::open() {
    LOGDEBUG << "in open";
    if (isOpen()) return;
    att::TSocket::open();
    fConnected = true;
    if (eventHandler) eventHandler->onSocketConnect();
    // atc::Synchronized s(monitor);
    // monitor.notify();
}

void Socket::write(const uint8_t* buf, uint32_t len) {
    try {
        att::TSocket::write(buf, len);
    } catch(const att::TTransportException& e) {
        if (e.getType() == att::TTransportException::TTransportExceptionType::NOT_OPEN ||
            e.getType() == att::TTransportException::TTransportExceptionType::UNKNOWN
            ) {
            if (eventHandler) eventHandler->onSocketDisconnect();
            fConnected = false;
        }
        throw;
    }
}

uint32_t Socket::read(uint8_t* buf, uint32_t len) {
    try {
        return att::TSocket::read(buf, len);
    } catch(const att::TTransportException& e) {
        if (e.getType() == att::TTransportException::TTransportExceptionType::NOT_OPEN ||
            e.getType() == att::TTransportException::TTransportExceptionType::UNKNOWN
            ) {
            if (eventHandler) eventHandler->onSocketDisconnect();
            fConnected = false;
        }
        throw;
    }
}

bool Socket::peek() {
    try {
        if (!isOpen()) return false;
        bool fHasData = att::TSocket::peek();
        if (!fHasData) {
            // if the conn was reset , peek will return false & close the socket
            if (!isOpen()) {
                if (eventHandler) eventHandler->onSocketDisconnect();
                fConnected = false;
            }
            return fHasData;
        }
        return fHasData;
    } catch(const att::TTransportException& e) {
        if (e.getType() == att::TTransportException::TTransportExceptionType::NOT_OPEN ||
            e.getType() == att::TTransportException::TTransportExceptionType::UNKNOWN
            ) {
            if (eventHandler) eventHandler->onSocketDisconnect();
            fConnected = false;
        }
        throw;
    }
}

bool Socket::connect(int retryCount, int backoff) {
    int numAttempts = 0;
    int waitTime = backoff*1000;
    if (isOpen()) return true;

    while (!isOpen() && numAttempts < retryCount) {
        ++numAttempts;
        // double the wait time every 5 attempts
        if (numAttempts % 5 == 0) waitTime *= 2;

        try {
            att::TSocket::open();
            fConnected = true;
        } catch(const att::TTransportException& e) {
            LOGDEBUG << "connection attempt [" << numAttempts
                     << "] failed . waiting for [" << waitTime <<"] micros";
            LOGGERPTR->flush();
            if (numAttempts < retryCount)  usleep(waitTime);
        }
    }
    if (isOpen()) {
        if (eventHandler) eventHandler->onSocketConnect();
        // atc::Synchronized s(monitor);
        // monitor.notify();
    }

    return isOpen();
}

void Socket::shutDown() {
    fShutDown = true;
    close();
    // atc::Synchronized s(monitor);
    // monitor.notify();
}

void Socket::checkConnection() {
    // LOGWARN << "Not running the check connection thread for now [WIN-92]";
    return;
    LOGDEBUG << "in checkConnection";
    for (; !fShutDown ;) {
        LOGDEBUG << "about to wait";
        {
            // wait for the connection to be opened.
            atc::Synchronized s(monitor);
            while (!fConnected) monitor.wait();
        }
        LOGDEBUG << "about to peek";
        if (isOpen() && !peek()) {
            if (eventHandler) eventHandler->onSocketDisconnect();
            fConnected = false;
        }
    }
    LOGDEBUG << "end checkConnection";
}

Socket::~Socket() {
    shutDown();
    connChecker.join();
}

}  // namespace net
}  // namespace fds
