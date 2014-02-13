#ifndef SOURCE_INCLUDE_NET_SOCKET_H_
#define SOURCE_INCLUDE_NET_SOCKET_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/concurrency/Monitor.h>
#include <string>
#include <thread>

namespace fds {
    namespace net {
        
        struct SocketEventHandler {
            SocketEventHandler() {}
            virtual void onSocketConnect();
            virtual void onSocketDisconnect();
            virtual void onSocketClose();
            virtual ~SocketEventHandler() {}
        };

        class Socket : public apache::thrift::transport::TSocket {
      public:
            Socket();
            virtual ~Socket();
            Socket(std::string host, int port);
            void setEventHandler(SocketEventHandler* eventHandler);

            // over rides

            virtual void write(const uint8_t* buf, uint32_t len);
            virtual uint32_t read(uint8_t* buf, uint32_t len);
            virtual void open();
            virtual void close();
            virtual bool peek();

            /**
             * try connecting the socket upto retryCount times.
             * after every unsuccessful attempt, 
             * wait for backoff milliseconds ( default 1s)
             * retryCount <=0 means keep trying.
             * the backoff will be double for every 5 failed attempts
             */
            bool connect(int retryCount = 50, int backoff = 1000);
            
            void shutDown() ;
      protected:
            void checkConnection();
            std::thread connChecker;
            apache::thrift::concurrency::Monitor monitor;
            apache::thrift::concurrency::Mutex mutex;
            SocketEventHandler* eventHandler = NULL;
            bool fShutDown = false;
            bool fConnected = false;
        };

    } // namespace net
} // namespace fds
#endif
