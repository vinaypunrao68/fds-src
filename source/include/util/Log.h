#define BOOST_LOG_DYN_LINK 1

#ifndef SOURCE_UTIL_LOG_H_
#define SOURCE_UTIL_LOG_H_

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef test
#undef test
#endif

#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/lockfree/detail/branch_hints.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <fds_defines.h>

#define FDS_LOG(lg) BOOST_LOG_SEV(lg.get_slog(), fds::fds_log::debug)
#define FDS_PLOG(lg_ptr) BOOST_LOG_SEV(lg_ptr->get_slog(), fds::fds_log::debug)

//If the build is a debug build we want source location exposed
#ifndef DONTLOGLINE
#define FDS_PLOG_SEV(lg_ptr, sev) BOOST_LOG_STREAM_WITH_PARAMS((lg_ptr->get_slog()), \
                                                               (fds::set_get_attrib("Location", (__LOC__), (__func__)))\
                                                               (boost::log::keywords::severity = (sev)) \
                                                              )
#define FDS_LOG_SEV(lg_ptr, sev) BOOST_LOG_STREAM_WITH_PARAMS((lg_ptr.get_slog()), \
                                                               (fds::set_get_attrib("Location", (__LOC__), (__func__)))\
                                                               (boost::log::keywords::severity = (sev)) \
                                                              )
//Otherwise - do not waste cycles setting attribute "Location"
#else 
#define FDS_PLOG_SEV(lg_ptr, sev) BOOST_LOG_SEV(lg_ptr->get_slog(), sev)
#define FDS_LOG_SEV(lg_ptr, sev) BOOST_LOG_SEV(lg_ptr.get_slog(), sev)
#endif
#define FDS_PLOG_COMMON(lg_ptr, sev) BOOST_LOG_SEV(lg_ptr->get_slog(), sev) << __FUNCTION__ << " " << __LINE__ << " " << log_string() << " "
#define FDS_PLOG_INFO(lg_ptr) FDS_PLOG_COMMON(lg_ptr, fds::fds_log::normal)
#define FDS_PLOG_WARN(lg_ptr) FDS_PLOG_COMMON(lg_ptr, fds::fds_log::warning)
#define FDS_PLOG_ERR(lg_ptr)  FDS_PLOG_COMMON(lg_ptr, fds::fds_log::error)

//For classes that expose the GetLog() fn .
#define LOGGERPTR  GetLog()
// For static functions
#define GLOGGERPTR  fds::GetLog()
// incase your current logger is different fom GetLog(),
// redefine macro [LOGGERPTR] at the top of your cpp file

#define LEVELCHECK(sev) if (LOGGERPTR->getSeverityLevel()<= fds::fds_log::sev)
#define GLEVELCHECK(sev) if (GLOGGERPTR->getSeverityLevel()<= fds::fds_log::sev)

#define LOGTRACE    LEVELCHECK(trace)        FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::trace)
#define LOGDEBUG    LEVELCHECK(debug)        FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::debug)
#define LOGMIGRATE  LEVELCHECK(migrate)      FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::migrate)
#define LOGIO       LEVELCHECK(io)           FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::io)
#define LOGNORMAL   LEVELCHECK(normal)       FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::normal)
#define LOGNOTIFY   LEVELCHECK(notification) FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::notification)
#define LOGWARN     LEVELCHECK(warning)      FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::warning)
#define LOGERROR    LEVELCHECK(error)        FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::error)
#define LOGCRITICAL LEVELCHECK(critical)     FDS_PLOG_SEV(LOGGERPTR, fds::fds_log::critical)

// for static functions inside classes
#define GLOGTRACE    GLEVELCHECK(trace)        FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::trace)
#define GLOGDEBUG    GLEVELCHECK(debug)        FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::debug)
#define GLOGMIGRATE  GLEVELCHECK(migrate)      FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::migrate)
#define GLOGIO       GLEVELCHECK(io)           FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::io)
#define GLOGNORMAL   GLEVELCHECK(normal)       FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::normal)
#define GLOGNOTIFY   GLEVELCHECK(notification) FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::notification)
#define GLOGWARN     GLEVELCHECK(warning)      FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::warning)
#define GLOGERROR    GLEVELCHECK(error)        FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::error)
#define GLOGCRITICAL GLEVELCHECK(critical)     FDS_PLOG_SEV(GLOGGERPTR, fds::fds_log::critical)


/*
 * Adds a thread local attribute to be printed at the log level
 * */
#define SCOPEDATTR(key,value) BOOST_LOG_NAMED_SCOPE((key ":" value))

// #define FUNCTRACING
#ifdef FUNCTRACING
#define TRACEFUNC fds::__TRACER__ __tt__(__PRETTY_FUNCTION__, __FILE__, __LINE__);
#else
#define TRACEFUNC
#endif

namespace fds {
struct __TRACER__ {
    __TRACER__(const std::string& prettyName, const std::string& filename, int lineno);
    ~__TRACER__();
    const std::string prettyName;
    const std::string filename;
    int lineno;
};

std::string cleanNameFromPrettyFunc(const std::string& prettyFunction, bool fClassOnly = false);
class fds_log {
  public:
    enum severity_level {
        trace,
        debug,
        migrate,
        io,
        normal,
        notification,
        warning,
        error,
        critical
    };

    enum log_options {
        record,
        pid,
        pname,
        tid
    };

  private:
    typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > text_sink;
    typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > file_sink;

    boost::shared_ptr< file_sink > sink;
    boost::log::sources::severity_logger_mt< severity_level > slg;

    void init(const std::string& logfile,
              const std::string& logloc,
              bool timestamp,
              bool severity,
              severity_level level,
              bool pname,
              bool pid,
              bool tid,
              bool record);

  public:

    /*
     * Constructs new log in specific location.
     */
    explicit fds_log(const std::string& logfile = "fds",
                     const std::string& logloc  = "",
                     severity_level level       = normal);

    ~fds_log();

    static severity_level getLevelFromName(std::string level);

    void setSeverityFilter(const severity_level &level);
    inline severity_level getSeverityLevel() { return severityLevel; }
    boost::log::sources::severity_logger_mt<severity_level>& get_slog() { return slg; }

    void flush() { sink->flush(); }
    void rotate();

  private :
    severity_level severityLevel = normal;
};

struct HasLogger {
    // get the class logger
    fds_log* GetLog() const;

    // set a new logger & return the old one.
    fds_log* SetLog(fds_log* logptr) const;
  protected:
    mutable fds_log* logptr= NULL;
};

// get the global logger;
fds_log* GetLog();

typedef boost::shared_ptr<fds_log> fds_logPtr;

/*
 * Adds and modifies attributes for the logger stream - helps logger see function, file, and line number as a
 * part of the attributes
 */
std::string set_get_attrib(const char* name, std::string value, const char * function_name);

}  // namespace fds
#endif  // SOURCE_UTIL_LOG_H_
