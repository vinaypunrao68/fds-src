/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_H_
#define INCLUDE_AM_ENGINE_H_

#include <fds_module.h>
#include <string>

namespace fds {

class AMEngine : public Module
{
  public:
    AMEngine(char const *const name) :
        Module(name), eng_signal(), eng_etc("etc"),
        eng_logs("logs"), eng_conf("etc/fds.conf") {}

    ~AMEngine() {}

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
    void run_server();

  private:
    std::string              eng_signal;
    char const *const        eng_etc;
    char const *const        eng_logs;
    char const *const        eng_conf;
};

extern AMEngine              gl_AMEngine;

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_H_ */
