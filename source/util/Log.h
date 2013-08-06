#define BOOST_LOG_DYN_LINK 1

#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

#define FDS_LOG(lg) BOOST_LOG_SEV(lg.get_slog(), fds::fds_log::normal)
#define FDS_LOG_SEV(lg, sev) BOOST_LOG_SEV(lg.get_slog(), sev)
// #define FDS_LOG_A(lg) BOOST_LOG(lg.get_log())

namespace fds {

  // static src::logger lg;
  
  // Here we define our application severity levels.
  // enum severity_level;
  /*
  {
    normal,
    notification,
    warning,
    error,
    critical
  };
  */
  
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
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;

    boost::shared_ptr< file_sink > sink;

    src::severity_logger< severity_level > slg;

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
     * 
     */
    fds_log(const std::string& logfile,
            const std::string& logloc);
    
    ~fds_log();

    src::severity_logger<severity_level>& get_slog() { return slg; }
  };

}
