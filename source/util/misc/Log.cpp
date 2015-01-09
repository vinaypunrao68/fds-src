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
#include <fds_globals.h>
namespace fds {

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
                "normal",
                "notify",
                "io",
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

fds_log::severity_level fds_log::getLevelFromName(std::string level) {
    boost::to_upper(level);
    boost::trim(level);
    if (0 == level.compare(0, 5, "TRACE")) return trace;
    if (0 == level.compare(0, 5, "DEBUG")) return debug;
    if (0 == level.compare(0, 6, "NORMAL")) return normal;
    if (0 == level.compare(0, 4, "INFO")) return normal;
    if (0 == level.compare(0, 5, "NOTIF")) return notification;
    if (0 == level.compare(0, 2, "IO")) return io;
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

    sink->locked_backend()->auto_flush(true);

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
