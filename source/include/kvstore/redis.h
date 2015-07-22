/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_REDIS_H_
#define SOURCE_INCLUDE_KVSTORE_REDIS_H_
#include <string>
#include <hiredis.h>
#include <queue>
#include <vector>
#include <thrift/concurrency/Monitor.h>

namespace redis {
    /**
     * For redis specific exception
     */
    struct RedisException : public std::exception {
        explicit RedisException(const char* cstr) {
            reason = std::string(cstr);
        }

        explicit RedisException(const std::string& reason) {
            this->reason = reason;
        }

        virtual ~RedisException() throw() {}

        virtual const char* what() const throw() {
            return reason.c_str();
        }
  private:
        std::string reason;
    };

    /**
     * Reply from redis server
     */
    struct Reply {
        explicit Reply(redisReply* r, bool fOwner = true);
        explicit Reply(void* v, bool fOwner = true);
        Reply& operator=(const Reply& rhs);
        virtual ~Reply();

        bool isError() const;
        bool isValid() const;
        bool isOk() const;
        bool wasModified() const;
        std::string getString() const;
        long long getLong() const; // NOLINT
        void checkValid() const;
        void checkError() const;
        bool isNil() const;
        std::string getStatus();
        size_t getNumElements() const;

        void toVector(std::vector<std::string>& vec); // NOLINT
        void toVector(std::vector<long long>& vec); // NOLINT
        void toVector(std::vector<uint>& vec); // NOLINT
        void toVector(std::vector<uint64_t>& vec); //NOLINT
        void toVector(std::vector<int64_t>& vec); //NOLINT

        void dump() const;

      protected:
        mutable bool fOwner = true;
        mutable redisReply* r;
    };

    /**
     * Connection to Redis
     */
    struct Connection {
        explicit Connection(const std::string& host = "localhost", uint port = 6379);
        void connect();
        Reply getReply();
        bool isConnected();
        ~Connection();
        bool setDB(uint db);

        redisContext* ctx;
        std::string host;
        uint port = 6379;
        uint db = 0;
    };

    /**
     * Redis Connection Pool
     */
    struct ConnectionPool {
        ConnectionPool(uint poolsize = 10,
                       const std::string& host = "localhost",
                       uint port = 6379);
        ~ConnectionPool();

        Connection* get();
        void put(Connection* cxn);
        bool setDB(uint db);
      protected:
        std::queue<Connection*> connections;
        apache::thrift::concurrency::Monitor monitor;
        apache::thrift::concurrency::Mutex mutex;
        friend struct Redis;
    };

    /**
     * RAII design for acquiring a connection from a pool 
     * & releasing it back into pool automatically
     */
    struct ScopedConnection {
        explicit ScopedConnection(ConnectionPool* pool);
        Connection* operator->() const;
        ~ScopedConnection();
      protected:
        ConnectionPool* pool = NULL;
        Connection* cxn = NULL;
    };

    /**
     * The actual interface to redis
     * - maintains a connection pool internally
     */

    struct Redis {
        static void encodeHex(const std::string& binary, std::string& hex);
        static void decodeHex(const std::string& hex, std::string& binary);
        Redis(const std::string& host = "localhost",
              uint port = 6379, uint poolsize = 10);
        ~Redis();

        bool isConnected();

        bool setDB(uint db);

        // send command
        Reply sendCommand(const char* cmdfmt, ...);

        // strings
        bool set(const std::string& key, const std::string& value);
        Reply get(const std::string& key);
        bool del(const std::string& key);
        Reply append(const std::string& key, const std::string& value);
        int64_t incr(const std::string& key, int64_t increment = 1);

        // lists
        Reply lrange(const std::string& key, long start = 0, long end = -1); // NOLINT
        Reply llen(const std::string& key);
        Reply lpush(const std::string& key, const std::string& value);
        Reply rpush(const std::string& key, const std::string& value);

        // hashes
        bool hset(const std::string& key, const std::string& field, const std::string& value); //NOLINT
        bool hset(const std::string& key, int64_t field, const std::string& value); //NOLINT
        bool hexists(const std::string& key, int64_t field);
        bool hexists(const std::string& key, const std::string field);
        Reply hget(const std::string& key, const std::string& field);
        Reply hget(const std::string& key, int64_t field);
        Reply hgetall(const std::string& key);
        int64_t hlen(const std::string& key);
        bool hdel(const std::string& key, const std::string& field);
        bool hdel(const std::string& key, int64_t field);

        // sets
        Reply smembers(const std::string& key);
        bool sismember(const std::string& key, const std::string& value);
        bool sismember(const std::string& key, const int64_t value);

        bool sadd(const std::string& key, const std::string& value);
        bool sadd(const std::string& key, const int64_t value);

        bool srem(const std::string& key, const std::string& value);
        bool srem(const std::string& key, const int64_t value);

      protected:
        ConnectionPool pool;
    };
}  // namespace redis

#endif  // SOURCE_INCLUDE_KVSTORE_REDIS_H_
