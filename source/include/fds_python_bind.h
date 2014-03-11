/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PYTHON_BIND_H_
#define SOURCE_INCLUDE_FDS_PYTHON_BIND_H_
#include <Python.h>

#include <shared/fds_types.h>
#include <util/Log.h>
#include <string>

namespace fds {

class PythonBind
{
  public:
    static void py_initialize(void);
    static void py_finalize(void);

    explicit PythonBind(const std::string &py_file);
    ~PythonBind();

    int load_module(void);
    int call(const char *py_func_name, int args_cnt, ...);

  private:
    std::string     p_filename;
    PyObject       *p_name;
    PyObject       *p_module;

    int             p_calls_made;
    std::string     p_func_last;

    static fds_log   *py_logger;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PYTHON_BIND_H_
