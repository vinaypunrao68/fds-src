/*
 * Copyright 2016 Formation Data Systems, Inc.
 */
#include <util/properties.h>
#include <fds_value_type.h>
using boss = boost::log::formatting_ostream;
#ifndef SOURCE_INCLUDE_UTIL_ATTRIBUTES_H_
#define SOURCE_INCLUDE_UTIL_ATTRIBUTES_H_

/**
 * Helper macros for attributes
 * usage:
 *    LOGDEBUG << ATTR("ctx","delete") << ATTR ("vol", 23) << "will be deleted"
 */

#define ATTR(key, value) key << ":" << value << " "

#define ATTR_AT(v)          ATTR("at",          v)
#define ATTR_BLOB(v)        ATTR("blob",        v)
#define ATTR_CALLBACK(v)    ATTR("callback",    v)
#define ATTR_CB(v)          ATTR("callback",    v)
#define ATTR_CLONE(v)       ATTR("clone",       v)
#define ATTR_CODE(v)        ATTR("code",        v)
#define ATTR_CONTEXT(v)     ATTR("ctx",         v)
#define ATTR_COUNT(v)       ATTR("count",       v)
#define ATTR_CTX(v)         ATTR("ctx",         v)
#define ATTR_DEST(v)        ATTR("dest",        v)
#define ATTR_DISK(v)        ATTR("disk",        v)
#define ATTR_DURATION(v)    ATTR("duration",    v)
#define ATTR_ENABLED(v)     ATTR("enabled",     v)
#define ATTR_ERROR(v)       ATTR("err",         v)
#define ATTR_FEATURE(v)     ATTR("feature",     v)
#define ATTR_FILE(v)        ATTR("file",        v)
#define ATTR_FROM(v)        ATTR("from",        v)
#define ATTR_FUNC(v)        ATTR("func",        v)
#define ATTR_FUNCTION(v)    ATTR("func",        v)
#define ATTR_HANDLE(v)      ATTR("handle",      v)
#define ATTR_ID(v)          ATTR("id",          v)
#define ATTR_IDX(v)         ATTR("idx",         v)
#define ATTR_INCARNATION(v) ATTR("incarnation", v)
#define ATTR_INDEX(v)       ATTR("idx",         v)
#define ATTR_INITIATOR(v)   ATTR("initiator",   v)
#define ATTR_INTERFACE(v)   ATTR("interface",   v)
#define ATTR_IP(v)          ATTR("ip",          v)
#define ATTR_LINE(v)        ATTR("line",        v)
#define ATTR_MESSAGE(v)     ATTR("msg",         v)
#define ATTR_MSG(v)         ATTR("msg",         v)
#define ATTR_NAME(v)        ATTR("name",        v)
#define ATTR_NODE(v)        ATTR("node",        v)
#define ATTR_NUM(v)         ATTR("num",         v)
#define ATTR_OBJECTID(v)    ATTR("objid",       v)
#define ATTR_OBJID(v)       ATTR("objid",       v)
#define ATTR_OFFSET(v)      ATTR("offset",      v)
#define ATTR_OP(v)          ATTR("op",          v)
#define ATTR_OPID(v)        ATTR("opid",        v)
#define ATTR_PATH(v)        ATTR("path",        v)
#define ATTR_PEER(v)        ATTR("peer",        v)
#define ATTR_POLICY(v)      ATTR("policy",      v)
#define ATTR_PORT(v)        ATTR("port",        v)
#define ATTR_PRIMARY(v)     ATTR("primary",     v)
#define ATTR_PRIORITY(v)    ATTR("pri",         v)
#define ATTR_QOS(v)         ATTR("qos",         v)
#define ATTR_QUORUM(v)      ATTR("quorum",      v)
#define ATTR_RATE(v)        ATTR("rate",        v)
#define ATTR_REQID(v)       ATTR("reqid",       v)
#define ATTR_REQUESTID(v)   ATTR("reqid",       v)
#define ATTR_RESULT(v)      ATTR("result",      v)
#define ATTR_SENT(v)        ATTR("sent",        v)
#define ATTR_SERVICE(v)     ATTR("svcid",       v)
#define ATTR_SIZE(v)        ATTR("size",        v)
#define ATTR_SNAP(v)        ATTR("snap",        v)
#define ATTR_SNAPSHOT(v)    ATTR("snap",        v)
#define ATTR_SOURCE(v)      ATTR("src",         v)
#define ATTR_SRC(v)         ATTR("src",         v)
#define ATTR_STATE(v)       ATTR("state",       v)
#define ATTR_STATS(v)       ATTR("stats",       v)
#define ATTR_STATUS(v)      ATTR("status",      v)
#define ATTR_SVC(v)         ATTR("svcid",       v)
#define ATTR_THREAD(v)      ATTR("thread",      v)
#define ATTR_THREADPOOL(v)  ATTR("threadpool",  v)
#define ATTR_TIME(v)        ATTR("time",        v)
#define ATTR_TIMEOUT(v)     ATTR("timeout",     v)
#define ATTR_TO(v)          ATTR("to",          v)
#define ATTR_TOKEN(v)       ATTR("token",       v)
#define ATTR_TXN(v)         ATTR("txn",         v)
#define ATTR_TYPE(v)        ATTR("type",        v)
#define ATTR_URI(v)         ATTR("uri",         v)
#define ATTR_UUID(v)        ATTR("uuid",        v)
#define ATTR_VALUE(v)       ATTR("value",       v)
#define ATTR_VERSION(v)     ATTR("version",     v)
#define ATTR_VOL(v)         ATTR("vol",         v)

namespace fds {
using fds_volid_t = fds_value_type<uint64_t>;
namespace util {

struct Attributes : protected Properties {
    Attributes();
    ~Attributes();
    using Properties::hasValue;
    using Properties::set;
    using Properties::setInt;
    using Properties::setDouble;
    using Properties::setBool;
    using Properties::get;
    using Properties::getInt;
    using Properties::getDouble;
    using Properties::getBool;
    friend boss& operator<<(boss& out, const Attributes& attr);
    friend std::ostream& operator<<(std::ostream& out, const Attributes& attr);
    void setVolId(const std::string& key, const fds_volid_t volId);
    fds_volid_t getVolId(const std::string& key);
    
};

boss& operator<<(boss& out, const Attributes& attr);
std::ostream& operator<<(std::ostream& out, const Attributes& attr);

}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_ATTRIBUTES_H_
