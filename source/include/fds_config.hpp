#ifndef _FDS_CONFIG_H_
#define _FDS_CONFIG_H_

#include <string>

#include <libconfig.h++>

namespace fds {

/**
 *
 */
class FdsConfig {
public:
    FdsConfig(const std::string &config_file);

    template<class T>
    T get(const std::string &key) const;

    template<class T>
    T get(const std::string &key, const T &default_value) const;

private:
    /* Config object */
    libconfig::Config config_;
};

} // namespace Fds

#endif
