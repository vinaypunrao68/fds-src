#include <iostream>
#include <fds_assert.h>
#include <libconfig.h++>
#include <fds_config.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

using namespace std;
using namespace fds;

void cmd_line_parse_test()
{
    char *argv[] = {
        "prog",
        "--fds.log=10", 
        "--cnt=10", 
        "--fds.test_mode=false", 
        "--fds.name=config_test",
        "--fds.test.unknown=22"
    };
    FdsConfig config("conf_test.conf", sizeof(argv) / sizeof(char*), argv);
    /* Basic commnad line parsing and overriding should work */
    fds_verify(10 == config.get<fds_int32_t>("fds.log"));
    fds_verify(10 == config.get<int>("fds.log"));
    fds_verify(false == config.get<fds_bool_t>("fds.test_mode"));
    fds_verify("config_test" == config.get<std::string>("fds.name"));
    fds_verify("22" == config.get<std::string>("fds.test.unknown"));
    fds_verify(22 == config.get<int>("fds.test.unknown"));

    /* commandline args that aren't prefixed with "fds." shouldn't be parse */
    fds_verify(config.exists("cnt") == false);

    config.set("fds.test.intval", 101);
    fds_verify(101 == config.get<int>("fds.test.intval"));
    fds_verify("101" == config.get<std::string>("fds.test.intval"));

    config.set("fds.test.intval", "12345678901234");
    fds_verify("12345678901234" == config.get<std::string>("fds.test.intval"));
    fds_verify(12345678901234 == config.get<fds_uint64_t>("fds.test.intval"));
    
    config.set("fds.test.boolval", false);
    fds_verify("false" == config.get<std::string>("fds.test.boolval"));

    config.set("fds.test.boolval", true);
    fds_verify(config.get<fds_bool_t>("fds.test.boolval"));


    config.set("fds.test.strval", "12345");
    fds_verify(12345 == config.get<fds_int32_t>("fds.test.strval"));
    fds_verify(12345 == config.get<fds_uint64_t>("fds.test.strval"));
    fds_verify("12345" == config.get<std::string>("fds.test.strval"));



    /* If command line args contain the wrong value type, parsing should fail */
    char *argv2[] = {"prog", "--fds.log=blah"};
    FdsConfig config2;
    bool exception = false;
    try {
        config2.init("conf_test.conf", sizeof(argv2) / sizeof(char*), argv2);
    } catch (...) {
        exception = true;
    }
    fds_verify(exception == true);
    
    /* exist test */
    fds_verify(config2.exists("fds.log") == true);
    fds_verify(config2.exists("fds.blah") == false);
    std::cout << "config test ... done" <<std::endl;
}

int main(int ac, char *av[])
{
    cmd_line_parse_test();
    return 0;
}
