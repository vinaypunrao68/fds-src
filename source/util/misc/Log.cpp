/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#define BOOST_LOG_DYN_LINK 1

#include <boost/log/expressions.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes/current_process_name.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/algorithm/string.hpp>
#include <util/Log.h>
#include <fds_process.h>
#include <dynamic_git_rev.h>
#include <dynamic_machine_arch.h>

namespace fds {

std::string cleanNameFromPrettyFunc(const std::string& prettyFunction, bool fClassOnly) {
    size_t colons = prettyFunction.find("::");
    if (colons == std::string::npos)
        return "::";
    colons = prettyFunction.find("(");
    if (fClassOnly) {
        // move before the fn.
        colons = prettyFunction.rfind("::", colons);
    }
    size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
    size_t end = colons - begin;

    return prettyFunction.substr(begin,end);
}

__TRACER__::__TRACER__(const std::string& _prettyName, const std::string& _filename, int _lineno) 
        : prettyName(cleanNameFromPrettyFunc(_prettyName)),filename(_filename),lineno(_lineno) {
    GLOGDEBUG << "enter : " << prettyName << ":" << filename << ":" << lineno;
}

__TRACER__::~__TRACER__() {
    GLOGDEBUG << "exit  : " << prettyName << ":" << filename << ":" << lineno;
}


/*
 * Rotate log when reachs N bytes
 */
#define ROTATION_SIZE 50 * 1024 * 1024

BOOST_LOG_ATTRIBUTE_KEYWORD(process_name, "ProcessName", std::string)

/*
 * The formatting logic for the severity level
 */
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, fds_log::severity_level lvl)
{
    static const char* const str[] =
            {
                "trace",
                "debug",
                "migrate",
                "io",
                "normal",
                "notify",
                "warning",
                "error",
                "CRITICAL"
            };
    if (static_cast< std::size_t >(lvl) < (sizeof(str) / sizeof(*str)))
        strm << str[lvl];
    else
        strm << static_cast< int >(lvl);
    return strm;
}

void writeHeader(boost::log::sinks::text_file_backend::stream_type& file) {
    char buildStr[256];

    if (g_fdsprocess == nullptr) {
        snprintf(buildStr, sizeof(buildStr), buildStrTmpl, "?", machineArch, gitRev);
    } else {
        std::string unknownManager = "Unknown Manager(" + g_fdsprocess->getProcId() + ")";
        snprintf(buildStr, sizeof(buildStr), buildStrTmpl,
                 SERVICE_NAME_FROM_EXE_NAME(unknownManager.c_str()), machineArch, gitRev);
    }

    file << buildStr;

}

fds_log::severity_level fds_log::getLevelFromName(std::string level) {
    boost::to_upper(level);
    boost::trim(level);
    if (0 == level.compare(0, 5, "TRACE")) return trace;
    if (0 == level.compare(0, 5, "DEBUG")) return debug;
    if (0 == level.compare(0, 7, "MIGRATE")) return migrate;
    if (0 == level.compare(0, 2, "IO")) return io;
    if (0 == level.compare(0, 6, "NORMAL")) return normal;
    if (0 == level.compare(0, 4, "INFO")) return normal;
    if (0 == level.compare(0, 5, "NOTIF")) return notification;
    if (0 == level.compare(0, 4, "WARN")) return warning;
    if (0 == level.compare(0, 5, "ERROR")) return error;
    if (0 == level.compare(0, 4, "CRIT")) return critical;

    // to support numbers
    if (level.size() == 1) {
        int value = level[0] - '0';
        if (value >= trace  && value <= critical) return (fds_log::severity_level)value;
    }

    // default
    return normal;
}

void fds_log::init(const std::string& logfile,
                   const std::string& logloc,
                   bool timestamp,
                   bool severity,
                   severity_level level,
                   bool Pname,
                   bool Pid,
                   bool Tid,
                   bool Record) {
    /*
     * Create the with file name and rotation.
     */
    sink = boost::make_shared< file_sink >(boost::log::keywords::file_name = logfile + "_%N.log",
                                           boost::log::keywords::rotation_size = ROTATION_SIZE);

    /*
     * Setup log sub-directory location
     */
    if (logloc.empty()) {
        sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector(
            boost::log::keywords::target = "."));
    } else {
        sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector(
            boost::log::keywords::target = logloc));
    }

    /*
     * Set to defaulty scan for existing log files and
     * pickup where it left previously off.
     */
    sink->locked_backend()->scan_for_files();

#ifdef DEBUG
    sink->locked_backend()->auto_flush(true);
#endif

    /*
     * Set the filter to not print messages below
     * a certain level.
     */
    sink->set_filter(
        boost::log::expressions::attr<severity_level>("Severity").or_default(normal) >= level);
    severityLevel = level;

    /*
     * Setup the attributes
     */
    boost::log::attributes::counter< unsigned int > RecordID(1);
    boost::log::core::get()->add_global_attribute("RecordID", RecordID);
    boost::log::attributes::local_clock TimeStamp;
    boost::log::core::get()->add_global_attribute("TimeStamp", TimeStamp);
    boost::log::core::get()->add_global_attribute(
        "ProcessName",
        boost::log::attributes::current_process_name());
    boost::log::core::get()->add_global_attribute(
        "ProcessID",
        boost::log::attributes::current_process_id());
    boost::log::core::get()->add_global_attribute(
        "ThreadID",
        boost::log::attributes::current_thread_id());

    /*
     * Set the format
     */
    sink->set_formatter(boost::log::expressions::stream
                        << "["
                        << boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
                        << "] [" << boost::log::expressions::attr< severity_level >("Severity")
                        << "] ["
                        << boost::log::expressions::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID")
                        << "] - "
                        << boost::log::expressions::smessage);

    /*
     * Set the log header.
     */
    sink->locked_backend()->set_open_handler(&writeHeader);

    /*
     * Add the sink to the core.
     */
    boost::log::core::get()->add_sink(sink);
}

fds_log::fds_log() {
    init("fds",
         "",
         true,   // timestamp
         true,   // severity
         normal,  // minimum sev level
         false,  // process name
         false,  // process id
         true,   // thread id
         false);  // record id
}

fds_log::fds_log(const std::string& logfile) {
    init(logfile,
         "",
         true,   // timestamp
         true,   // severity
         normal, // minimum sev level
         false,  // process name
         false,  // process id
         true,   // thread id
         false); // record id
}

fds_log::fds_log(const std::string& logfile,
                 const std::string& logloc) {
    init(logfile,
         logloc,
         true,   // timestamp
         true,   // severity
         normal, // minimum sev level
         false,  // process name
         false,  // process id
         true,   // thread id
         false); // record id
}

fds_log::fds_log(const std::string& logfile,
                 const std::string& logloc,
                 severity_level level) {
    init(logfile,
         logloc,
         true,   // timestamp
         true,   // severity
         level, // minimum sev level
         false,  // process name
         false,  // process id
         true,   // thread id
         false); // record id
}

fds_log::~fds_log() {
}

void fds_log::setSeverityFilter(const severity_level &level) {
    sink->reset_filter();
    sink->set_filter(
        boost::log::expressions::attr<severity_level>("Severity").or_default(normal) >= level);
    severityLevel= level;
}

}  // namespace fds
