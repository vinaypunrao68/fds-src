/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "platform/valgrind.h"

extern "C" {
#include <valgrind/valgrind.h>
}
#include <cstdio>
#include<boost/tokenizer.hpp>

#include "fds_process.h"
#include "fds_config.hpp"

namespace fds
{

/**** Valgrind constants ****/
static constexpr int unnested_valgrind = 1;

// Added to the end of any path in case the trailing slash was forgotten
static std::string const dir_delimiter = "/";

std::string const ValgrindOptions::base_config_path = "fds.common.valgrind.";

ValgrindOptions::ValgrindOptions(char const* prog_name, char const* fds_root) :
    program(prog_name), root(fds_root + dir_delimiter)
{
}

void ValgrindOptions::initialize()
{
    // Options come from platform.conf
    auto config(g_fdsprocess->get_conf_helper());
    config.set_base_path(base_config_path);

    options.emplace_back(config.get<std::string>("app_name"));
    auto base_dir = config.get<std::string>("base_dir") + dir_delimiter;

    auto common_options = config.get<std::string>("common_options", "");
    {
        boost::tokenizer<boost::escaped_list_separator<char> > tok(common_options);
        for(auto const& option : tok) {
            options.emplace_back(option);
        }
    }

    auto specific_options = std::string("process_specific.") + program;
    if (config.exists(specific_options)) {
        auto spec_options = config.get<std::string>(specific_options);
        boost::tokenizer<boost::escaped_list_separator<char> > tok(spec_options);
        for (auto const& option : tok) {
            options.emplace_back(option);
        }
    }

    auto suppressions_option = config.get<std::string>("suppressions.suppressions_option");
    auto suppressions_file = root + base_dir + program
                             + config.get<std::string>("suppressions.suppressions_suffix");
    if (suppFileExists(suppressions_file))
    {
        options.emplace_back(suppressions_option + suppressions_file);
    }

    options.emplace_back(config.get<std::string>("output.results_option") +
                         root +
                         base_dir +
                         program +
                         config.get<std::string>("output.results_suffix"));
}

std::vector<std::string const*> ValgrindOptions::operator()()
{
    std::call_once(initialized, &ValgrindOptions::initialize, this);

    std::vector<std::string const*> return_value;
    for (auto const& option : options)
    {
        return_value.push_back(&option);
    }

    return return_value;
}

bool ValgrindOptions::runningOnUnnestedValgrind() const
{
    return (unnested_valgrind == RUNNING_ON_VALGRIND);
}

bool ValgrindOptions::suppFileExists(std::string const& path) const
{
    if (auto file = fopen(path.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

}  // namespace fds
