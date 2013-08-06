/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#define BOOST_LOG_DYN_LINK 1

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes/current_process_name.hpp>
#include <boost/log/attributes/current_process_id.hpp>

#include "util/Log.h"

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
      "notification",
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

void fds_log::init(const std::string& logfile,
                   const std::string& logloc,
                   bool timestamp,
                   bool severity,
                   severity_level level,
                   bool pname,
                   bool pid,
                   bool tid,
                   bool record) {

  /*
   * Create the with file name and rotation.
   */
  sink = boost::make_shared< file_sink >(keywords::file_name = logfile + "_%N.log",
                                         keywords::rotation_size = ROTATION_SIZE);

  /*
   * Setup log sub-directory location
   */
  if (logloc.empty()) {
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = "."));
  } else {
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = logloc));
  }

  /*
   * Set to defaulty scan for existing log files and
   * pickup where it left previously off.
   */
  sink->locked_backend()->scan_for_files();

  /*
   * Set the filter to not print messages below
   * a certain level.
   */
  sink->set_filter(
      expr::attr<severity_level>("Severity").or_default(normal) >= level);
  
  /*
   * Setup the attributes
   */
  attrs::counter< unsigned int > RecordID(1);
  logging::core::get()->add_global_attribute("RecordID", RecordID);
  attrs::local_clock TimeStamp;
  logging::core::get()->add_global_attribute("TimeStamp", TimeStamp);
  logging::core::get()->add_global_attribute(
      "ProcessName",
      attrs::current_process_name());  
  logging::core::get()->add_global_attribute(
      "ProcessID",
      attrs::current_process_id());
  logging::core::get()->add_global_attribute(
      "ThreadID",
      attrs::current_thread_id());

  /*
   * Set the format
   */
  sink->set_formatter(expr::stream
                      << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
                      << "] [" << expr::attr< severity_level >("Severity")
                      << "] ["
                      << expr::attr< attrs::current_thread_id::value_type >("ThreadID")
                      << "] - "
                      << expr::smessage);

  /*
   * Add the sink to the core.
   */
  logging::core::get()->add_sink(sink);  
}

fds_log::fds_log() {

  init("fds",
       "",
       true,   // timestamp
       true,   // severity
       normal, // minimum sev level
       false,  // process name
       false,  // process id
       true,   // thread id
       false); // record id
  
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

fds_log::~fds_log() {
}

}
