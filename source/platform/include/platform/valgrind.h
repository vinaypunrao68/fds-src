/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_VALGRIND_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_VALGRIND_H_

#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace fds
{

    struct ValgrindOptions
    {
        ValgrindOptions(char const* prog_name, char const* fds_root);
        ValgrindOptions(ValgrindOptions const& rhs) = delete;
        ValgrindOptions& operator=(ValgrindOptions const& rhs) = delete;
        virtual ~ValgrindOptions() = default;

        /** Returns a vector of all the options */
        std::vector<std::string const*> operator()();

     private:
        void initialize();
        bool suppFileExists(std::string const& path) const;

        std::string program;
        std::string root;
        std::vector<std::string> options;

        static std::string const base_config_path;

        std::once_flag initialized;
    };


}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_VALGRIND_H_
