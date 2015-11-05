/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/redis.h>
// #include <stdlib.h>
#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <util/Log.h>
namespace atc = apache::thrift::concurrency;

namespace redis {
using fds::GetLog;
/******************************************************************************************
 *  Redis Reply structures
 ******************************************************************************************/
Reply::Reply(void* v, bool fOwner) : r(reinterpret_cast<redisReply*>(v)), fOwner(fOwner) {
}

Reply::Reply(redisReply* r, bool fOwner) : r(r), fOwner(fOwner) {
}

Reply& Reply::operator= (const Reply& rhs) {
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
    checkValid();
    return (r->type == REDIS_REPLY_ERROR);
}

bool Reply::isValid() const {
    return r != NULL;
}

bool Reply::isOk() const {
    checkValid();
    return ((r->type == REDIS_REPLY_STATUS) && (getString() == "OK")) ||
            ((r->type == REDIS_REPLY_INTEGER) && (r->integer == 1));
}

bool Reply::wasModified() const {
    checkValid();
    return (r->type == REDIS_REPLY_INTEGER) && (r->integer > 0);
}

std::string Reply::getString() const {
    checkValid();
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

void Reply::checkValid() const {
    if (!isValid()) {
        throw RedisException("connection is not valid");
    }
}

bool Reply::isNil() const {
    checkValid();
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
    checkValid();
    for (size_t i = 0; i < r->elements; ++i) {
        vec.push_back(std::string(r->element[i]->str, r->element[i]->len));
    }
}

void Reply::toVector(std::vector<long long>& vec) { // NOLINT
    checkValid();
    for (size_t i = 0; i < r->elements; ++i) {
        vec.push_back(std::stoll(std::string(r->element[i]->str,
                                             r->element[i]->len), NULL, 10));
    }
}

void Reply::toVector(std::vector<uint>& vec) { // NOLINT
    checkValid();
    for (size_t i = 0; i < r->elements; ++i) {
        vec.push_back(std::stoi(std::string(r->element[i]->str,
                                            r->element[i]->len), NULL, 10));
    }
}

void Reply::toVector(std::vector<uint64_t>& vec) { // NOLINT
    checkValid();
    for (size_t i = 0; i < r->elements; ++i) {
        vec.push_back(std::stoll(std::string(r->element[i]->str,
                                             r->element[i]->len), NULL, 10));
    }
}

void Reply::toVector(std::vector<int64_t>& vec) { // NOLINT
    checkValid();
    for (size_t i = 0; i < r->elements; ++i) {
        vec.push_back(std::stoll(std::string(r->element[i]->str,
                                             r->element[i]->len), NULL, 10));
    }
}

void Reply::toVector(std::vector<std::int32_t>& vec) { // NOLINT
    checkValid();
    for (size_t i = 0; i < r->elements; ++i) {
        vec.push_back(std::stoi(std::string(r->element[i]->str,
                                            r->element[i]->len), NULL, 10));
    }
}

void Reply::dump() const {
    checkValid();
    LOGDEBUG << "redis reply ::: ";
    std::string strType= "unknown";
    switch (r->type) {
        case REDIS_REPLY_STRING  : strType = "string"; break;
        case REDIS_REPLY_ARRAY   : strType = "array"; break;
        case REDIS_REPLY_INTEGER : strType = "integer"; break;
        case REDIS_REPLY_NIL     : strType = "nil"; break;
        case REDIS_REPLY_STATUS  : strType = "status"; break;
        case REDIS_REPLY_ERROR   : strType = "error"; break;
    }

    std::ostringstream oss;
    for (uint i = 0; i < r->elements; i++) {
        oss << i <<":(int:" << r->element[i]->integer << ") "
            << "(str:" << std::string(r->element[i]->str,
                                      r->element[i]->len) << ") ";
    }

    LOGDEBUG << "[type:" << strType << "] "
             << "[intval:" << r->integer << "] "
             << "[len:" << r->len << "]"
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
    LOGDEBUG << "instantiating a connection to " << host << ":" << port;
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
        redisFree(ctx);
        ctx = 0;
        throw e;
    } else {
        setDB(db);
    }
}

bool Connection::setDB(uint db) {
    if (db > 0) {
        redisAppendCommand(ctx, "selectdb %d", db);
        try {
            Reply reply = getReply();
            reply.checkError();
            this->db = db;
        } catch(RedisException& e) { // NOLINT
            LOGWARN << "error setting db to " << db << " : " << e.what();
            redisFree(ctx);
            ctx = 0;
            throw e;
        }
    }
    return true;
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

bool Connection::isConnected() {
    if (ctx == 0) return false;
    redisAppendCommand(ctx, "ping");
    try {
        Reply reply = getReply();
        if (reply.getStatus() == "PONG") {
            return true;
        } else {
            LOGWARN << "unknown status : " << reply.getStatus();
        }
    } catch(RedisException& e) { // NOLINT
        LOGWARN << "error checking connection : " << e.what();
    }
    return false;
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
    LOGDEBUG << "instantiating connection pool ["
             << poolsize << "] to " << host << ":" << port;
    for (uint i = 0; i < poolsize; i++) {
        connections.push(new Connection(host, port));
    }
}

ConnectionPool::~ConnectionPool() {
    atc::Synchronized s(monitor);
    LOGDEBUG << "destroying connection pool [" << connections.size() << "]";
    while (!connections.empty()) {
        Connection* cxn = connections.front();
        connections.pop();
        delete cxn;
    }
}

Connection* ConnectionPool::get() {
    atc::Synchronized s(monitor);
    while (connections.empty()) {
        LOGWARN << "waiting for a connection from the pool "
                << "- This should not happen often [mebbe increase poolsize??]!!!!!";
        monitor.wait();
    }
    Connection* cxn = connections.front();
    connections.pop();
    return cxn;
}

void ConnectionPool::put(Connection* cxn) {
    atc::Synchronized s(monitor);
    try {
        if (!cxn->isConnected()) cxn->connect();
    } catch(const RedisException& e) {
        LOGWARN << "unable to reconect to redis server";
    }
    connections.push(cxn);
    if ( 1 == connections.size() ) {
        monitor.notifyAll();
    }
}

bool ConnectionPool::setDB(uint db) {
    atc::Synchronized s(monitor);
    uint poolsize = connections.size();
    for (uint i = 0; i < poolsize; i++) {
        Connection* cxn = connections.front();

        if (cxn->isConnected()) {
            cxn->setDB(db);
        } else {
            // if it is not connected yet, just
            // set the variable, it will be set
            // later on connect.
            cxn->db = db;
        }
        connections.pop();
        connections.push(cxn);
    }
    return true;
}

ScopedConnection::ScopedConnection(ConnectionPool* pool) : pool(pool) {
    try {
        cxn = pool->get();
        cxn->connect();
    } catch(const RedisException& e) {
        LOGWARN << "unable to get redis connection : " << e.what();
        if (cxn != NULL) pool->put(cxn);
        throw e;
    }
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

bool Redis::setDB(uint db) {
    return pool.setDB(db);
}

bool Redis::isConnected() {
    try {
        SCOPEDCXN();
        return cxn->isConnected();
    } catch(const RedisException& e) {
        return false;
    }
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

bool Redis::set(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "set %s %b", key.c_str(), value.data(), value.length()));
    return reply.isOk();
}

Reply Redis::get(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "get %s", key.c_str()));
}

bool Redis::exists(const std::string& key) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "exists %s", key.c_str()));
    reply.checkError();
    return reply.getLong() == 1;
}

bool Redis::del(const std::string& key) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "del %s", key.c_str()));
    reply.checkError();
    return reply.isOk();
}


Reply Redis::append(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "append %s", key.c_str()));
}

int64_t Redis::incr(const std::string& key, int64_t increment) {
    SCOPEDCXN();
    if (1 == increment) {
        Reply reply(redisCommand(cxn->ctx, "incr %s", key.c_str()));
        return reply.getLong();
    } else {
        Reply reply(redisCommand(cxn->ctx, "incrby %s %ld", key.c_str(), increment));
        return reply.getLong();
    }
}

// list commands
Reply Redis::lrange(const std::string& key, long start, long end) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "lrange %s %ld %ld", key.c_str(), start, end));
}

Reply Redis::llen(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "llen %s", key.c_str()));
}

Reply Redis::lpush(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "lpush %s %b", key.c_str(), value.data(), value.length()));
}

Reply Redis::rpush(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "rpush %s %b", key.c_str(), value.data(), value.length()));
}

bool Redis::hset(const std::string& key, const std::string& field, const std::string& value) {
    SCOPEDCXN();
    Reply reply(redisCommand(
            cxn->ctx, "hset %s %s %b", key.c_str(), field.c_str(), value.data(), value.length()));
    reply.checkError();
    return reply.getLong() == 1;
}

bool Redis::hset(const std::string& key, int64_t field, const std::string& value) {
    SCOPEDCXN();
    Reply reply(redisCommand(
            cxn->ctx, "hset %s %ld %b", key.c_str(), field, value.data(), value.length()));
    reply.checkError();
    return reply.getLong() == 1;
}

bool Redis::hset(const std::string& key, const std::string& field, const std::int64_t value) {
    SCOPEDCXN();
    Reply reply(redisCommand(
            cxn->ctx, "hset %s %s %ld", key.c_str(), field.c_str(), value));
    reply.checkError();
    return reply.getLong() == 1;
}

bool Redis::hexists(const std::string& key, int64_t field) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "hexists %s %ld", key.c_str(), field));
    return reply.getLong() == 1;
}

bool Redis::hexists(const std::string& key, const std::string field) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "hexists %s %ld", key.c_str(), field.c_str()));
    return reply.getLong() == 1;
}

Reply Redis::hget(const std::string& key, const std::string& field) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "hget %s %s", key.c_str(), field.c_str()));
}

Reply Redis::hget(const std::string& key, int64_t field) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "hget %s %ld", key.c_str(), field));
}

Reply Redis::hgetall(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "hgetall %s", key.c_str()));
}

int64_t Redis::hlen(const std::string& key) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "hlen %s", key.c_str()));
    return reply.getLong();
}

bool Redis::hdel(const std::string& key, const std::string& field) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "hdel %s %s", key.c_str(), field.c_str()));
    return reply.getLong() > 0;
}

bool Redis::hdel(const std::string& key, int64_t field) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "hdel %s %ld", key.c_str(), field));
    return reply.getLong() > 0;
}

Reply Redis::smembers(const std::string& key) {
    SCOPEDCXN();
    return Reply(redisCommand(cxn->ctx, "smembers %s", key.c_str()));
}

bool Redis::sismember(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    Reply reply(redisCommand(
            cxn->ctx, "sismember %s %b", key.c_str(), value.data(), value.length()));
    return reply.getLong() == 1;
}

bool Redis::sismember(const std::string& key, const int64_t value) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "sismember %s %ld", key.c_str(), value));
    return reply.getLong() == 1;
}

bool Redis::sadd(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "sadd %s %b", key.c_str(), value.data(), value.length()));
    return reply.wasModified();
}

bool Redis::sadd(const std::string& key, const int64_t value) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "sadd %s %ld", key.c_str(), value));
    reply.checkError();
    return reply.wasModified();
}

bool Redis::sadd(const std::string& key, const std::int32_t value) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "sadd %s %d", key.c_str(), value));
    reply.checkError();
    return reply.wasModified();
}

bool Redis::srem(const std::string& key, const std::string& value) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "srem %s %b", key.c_str(), value.data(), value.length()));
    return reply.wasModified();
}

bool Redis::srem(const std::string& key, const int64_t value) {
    SCOPEDCXN();
    Reply reply(redisCommand(cxn->ctx, "srem %s %ld", key.c_str(), value));
    return reply.wasModified();
}

Redis::~Redis() {
}

}  // namespace redis
