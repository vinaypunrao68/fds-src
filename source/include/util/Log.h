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
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#define FDS_LOG(lg) BOOST_LOG_SEV(lg.get_slog(), fds::fds_log::normal)
#define FDS_LOG_SEV(lg, sev) BOOST_LOG_SEV(lg.get_slog(), sev)
#define FDS_PLOG(lg_ptr) BOOST_LOG_SEV(lg_ptr->get_slog(), fds::fds_log::normal)
#define FDS_PLOG_SEV(lg_ptr, sev) BOOST_LOG_SEV(lg_ptr->get_slog(), sev)

namespace fds {

  class fds_log {
 public:
    enum severity_level {
      trace,
      debug,
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
     * New log with all defaults.
     */
    fds_log();
    /*
     * Constructs a new log with the file name and
     * all other defaults.
     */
    explicit fds_log(const std::string& logfile);
    /*
     * Constructs new log in specific location.
     */
    fds_log(const std::string& logfile,
            const std::string& logloc);
    /*
     * Constructs new log in specific location.
     */
    fds_log(const std::string& logfile,
                     const std::string& logloc,
                     severity_level level);
    
    ~fds_log();
    
    void setSeverityFilter(const severity_level &level);

    boost::log::sources::severity_logger_mt<severity_level>& get_slog() { return slg; }
  };

}  // namespace fds

#endif  // SOURCE_UTIL_LOG_H_
