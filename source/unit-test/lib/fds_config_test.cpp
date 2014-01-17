#include <iostream>
#include <fds_assert.h>
#include <libconfig.h++>
#include <fds_config.hpp>
namespace po = boost::program_options;

using namespace std;
using namespace fds;

void cmd_line_parse_test()
{
    char *argv[] = {"prog", 
        "--fds.log=10", 
        "--cnt=10", 
        "--fds.test_mode=false", 
        "--fds.name=config_test"
    };
    std::cout << "size " << sizeof(argv) << std::endl;
    FdsConfig config("conf_test.conf", sizeof(argv) / sizeof(char*), argv);
    /* Basic commnad line parsing and overriding should work */
    fds_verify(10 == config.get<int>("fds.log"));
    fds_verify(false == config.get<bool>("fds.test_mode"));
    fds_verify("config_test" == config.get<std::string>("fds.name"));
    /* basic set and get should work */
    config.set<int>("fds.log", 5);
    fds_verify(5 == config.get<int>("fds.log"));
    /* commandline args that aren't prefixed with "fds." shouldn't be parse */
    fds_verify(config.exists("cnt") == false);
    
    /* If command line args contain the wrong value type, parsing should fail */
    char *argv2[] = {"prog", "--fds.log=blah"};
    FdsConfig config2;
    bool exception = false;
    try {
        config2.init("conf_test.conf", sizeof(argv2) / sizeof(char*), argv2);
    } catch (boost::bad_lexical_cast e) {
        exception = true;
    }
    fds_verify(exception == true);
    
    /* exist test */
    fds_verify(config2.exists("fds.log") == true);
    fds_verify(config2.exists("fds.blah") == false);
}

int main(int ac, char *av[])
{
    cmd_line_parse_test();
    return 0;
}
