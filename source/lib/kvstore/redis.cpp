/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/redis.h>
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <util/Log.h>
namespace atc = apache::thrift::concurrency;

namespace redis {

/******************************************************************************************
 *  Redis Reply structures 
 ******************************************************************************************/
Reply::Reply(void* v, bool fOwner) : r(reinterpret_cast<redisReply*>(v)), fOwner(fOwner) {
}

Reply::Reply(redisReply* r, bool fOwner) : r(r), fOwner(fOwner) {
}

Reply& Reply::operator=(const Reply& rhs) {
    if (this == &rhs) {
        return *this;
    }

    if (r && fOwner) {
        freeReplyObject(r);
    }
    r = rhs.r;
    rhs.fOwner = false;
    return *this;
}

Reply::~Reply() {
    if (r && fOwner) {
        freeReplyObject(r);
    }
}

bool Reply::isError() const {
    return r->type == REDIS_REPLY_ERROR;
}

bool Reply::isValid() const {
    return r != NULL;
}

std::string Reply::getString() const {
    return std::string(r->str, r->len);
}

long long Reply::getLong() const { //NOLINT
    checkError();
    if (r->type == REDIS_REPLY_INTEGER) {
        return r->integer;
    }

    if (r->type == REDIS_REPLY_STRING) {
        // TODO(prem) : check for length.
        // return atoll(getString().c_str());
        return atoll(r->str);
    }
    return -1;
}

void Reply::checkError() const {
    if (isError()) {
        throw RedisException(getString());
    }
}

bool Reply::isNil() const {
    return r->type == REDIS_REPLY_NIL;
}


std::string Reply::getStatus() {
    checkError();
    if (r->type != REDIS_REPLY_STATUS) {
        throw RedisException("invalid reply type");
    }
    return getString();
}

size_t Reply::getNumElements() const {
    checkError();
    if (r->type != REDIS_REPLY_ARRAY) {
        throw RedisException("invalid reply type");
    }
    return r->elements;
}

void Reply::toVector(std::vector<std::string>& vec) { // NOLINT
    for (size_t i = 0; i < r->elements ; ++i) {
        vec.push_back(std::string(r->element[i]->str, r->element[i]->len));
    }
}

void Reply::toVector(std::vector<long long>& vec) { // NOLINT
    for (size_t i = 0; i < r->elements ; ++i) {
        vec.push_back(strtoll(std::string(r->element[i]->str,
                                          r->element[i]->len).c_str(), NULL, 10));
    }
}

void Reply::dump() const {
    GLOGDEBUG << "redis reply ::: ";
    std::string strType="unknown";
    switch (r->type) {
        case REDIS_REPLY_STRING  : strType = "string"  ; break;
        case REDIS_REPLY_ARRAY   : strType = "array"   ; break;
        case REDIS_REPLY_INTEGER : strType = "integer" ; break;
        case REDIS_REPLY_NIL     : strType = "nil"     ; break;
        case REDIS_REPLY_STATUS  : strType = "status"  ; break;
        case REDIS_REPLY_ERROR   : strType = "error"   ; break;
    }

    std::ostringstream oss;
    for (uint i = 0; i < r->elements ; i++) {
        oss << i <<":(int:" << r->element[i]->integer << ") "
            << "(str:" << std::string(r->element[i]->str,
                                      r->element[i]->len) << ") ";
    }

    GLOGDEBUG << "[type:" << strType << "] "
             << "[intval:" << r->integer << "] "
             << "[str:" << getString() << "] "
             << "[numelements:" << r->elements << "] "
             << "[elements:" << oss.str() << "] ";
}


/******************************************************************************************
 *  Redis Connection
 ******************************************************************************************/
Connection::Connection(const std::string& host,
                       uint port) : host(host),
                                    port(port),
                                    ctx(0) {
    GLOGDEBUG << "instantiating a connection to " << host << ":" << port;
}

void Connection::connect() {
    if (ctx != 0) {
        return;
    }
    ctx = redisConnect(host.c_str(), port);
    if (ctx->err) {
        std::ostringstream oss;
        oss << "connect to " << host << ":"<< port << " failed - ["
            << ctx->err <<"] - " << ctx->errstr;
        RedisException e(oss.str());
        // redisFree(ctx);
        // ctx = 0;
        throw e;
    }
}

Reply Connection::getReply() {
    redisReply* r;

    if (redisGetReply(ctx, reinterpret_cast<void**>(&r)) != REDIS_OK) {
        RedisException e(ctx->errstr);
        redisFree(ctx);
        ctx = 0;
        throw e;
    }

    return Reply(r);
}

Connection::~Connection() {
    if (ctx) {
        redisFree(ctx);
    }
    ctx = 0;
}

/******************************************************************************************
 *  Redis Connection Pool
 ******************************************************************************************/
ConnectionPool::ConnectionPool(uint poolsize,
                               const std::string& host,
                               uint port) : monitor(&mutex) {
    GLOGDEBUG << "instantiating connection pool ["
             << poolsize << "] to " << host << ":" << port;
    for (uint i = 0; i < poolsize; i++) {
        connections.push(new Connection(host, port));
    }
}

ConnectionPool::~ConnectionPool() {
    atc::Synchronized s(monitor);
    GLOGDEBUG << "destroying connection pool [" << connections.size() << "]";
    while (!connections.empty()) {
        Connection* cxn = connections.front();
        connections.pop();
        delete cxn;
    }
}

Connection* ConnectionPool::get() {
    atc::Synchronized s(monitor);
    while (connections.empty()) {
        GLOGWARN << "waiting for a connection from the pool "
                << "- This should not happen often [mebbe increase poolsize??]!!!!!";
        monitor.wait();
    }
    Connection* cxn = connections.front();
    connections.pop();
    return cxn;
}

void ConnectionPool::put(Connection* cxn) {
    atc::Synchronized s(monitor);
    connections.push(cxn);
    if ( 1 == connections.size() ) {
        monitor.notifyAll();
    }
}

ScopedConnection::ScopedConnection(ConnectionPool* pool) : pool(pool) {
    cxn = pool->get();
    cxn->connect();
}

Connection* ScopedConnection::operator->() const {
    return cxn;
}

ScopedConnection::~ScopedConnection() {
    if (cxn != NULL) {
        pool->put(cxn);
    }
}

#define SCOPEDCXN(...) ScopedConnection cxn(&pool);

/******************************************************************************************
 *  Redis  Interface
 ******************************************************************************************/
void Redis::encodeHex(const std::string& binary, std::string& hex) {
    static const char *hexchar = "0123456789abcdef";
    hex.clear();
    hex.reserve(binary.length()*2);
    for (uint i = 0; i < binary.length(); i++) {
        hex.append(1, hexchar[(binary[i] >> 4) & 0x0F]);
        hex.append(1, hexchar[binary[i] & 0x0F]);
    }
}

void Redis::decodeHex(const std::string& hex, std::string& binary) {
    binary.clear();
    binary.reserve(hex.length()/2);
    char a, b;
    for (uint i = 0; i < hex.length(); i += 2) {
        a = hex[i];   if (a > 57) a -= 87; else a -= 48; // NOLINT
        b = hex[i+1]; if (b > 57) b -= 87; else b -= 48; // NOLINT
        binary.append(1, (a << 4 & 0xF0) + b);
    }
}

Redis::Redis(const std::string& host, uint port,
             uint poolsize) : pool(poolsize, host, port) {
}

Reply Redis::sendCommand(const char* cmdfmt, ...) {
    SCOPEDCXN();
    va_list ap;
    int ret;

    va_start(ap, cmdfmt);
    ret = redisvAppendCommand(cxn->ctx, cmdfmt, ap);
    va_end(ap);
    return cxn->getReply();
}

Reply Redis::set(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "set %s %s", key.c_str(), value.c_str()));
}

Reply Redis::get(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "get %s", key.c_str()));
}

Reply Redis::append(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "append %s", key.c_str()));
}

Reply Redis::incr(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "incr %s", key.c_str()));
}

Reply Redis::incrby(const std::string& key, long increment) { // NOLINT
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "incrby %s %ld", key.c_str(), increment));
}

// list commands
Reply Redis::lrange(const std::string& key, long start, long end) { //NOLINT
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "lrange %s %ld %ld", key.c_str(), start, end));
}

Reply Redis::llen(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "llen %s", key.c_str()));
}

Reply Redis::lpush(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "lpush %s %s", key.c_str(), value.c_str()));
}

Reply Redis::rpush(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "rpush %s %s", key.c_str(), value.c_str()));
}

Reply Redis::hset(const std::string& key, const std::string& field,
                  const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "hset %s %s %s",
                              key.c_str(), field.c_str(), value.c_str()));
}

Reply Redis::hget(const std::string& key, const std::string& field) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "hget %s %s", key.c_str(), field.c_str()));
}

Reply Redis::hgetall(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "hgetall %s", key.c_str()));
}

Reply Redis::hlen(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "hlen %s", key.c_str()));
}


Redis::~Redis() {
}

}  // namespace redis
