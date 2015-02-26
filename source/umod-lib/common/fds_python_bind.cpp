/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <Python.h>

#include <fds_python_bind.h>
#include <string>
#include <ostream>
#include <fds_assert.h>
#include <stdlib.h>

#define __PB_DONE_ERR(_err, _val)                                                   \
    do {                                                                            \
        (_err) = (_val);                                                            \
        goto done;                                                                  \
    } while (0)

namespace fds {

fds_log *PythonBind::py_logger = NULL;

void
PythonBind::py_initialize(void)
{
    fds_verify(PythonBind::py_logger == NULL);
    // XXX: Pass in logfile from parent process
    PythonBind::py_logger = new fds_log("/tmp/python_bind.log", "/tmp");
    fds_verify(PythonBind::py_logger);


    setenv("PYTHONPATH", ".", 0);

    Py_Initialize();
}

void
PythonBind::py_finalize(void)
{
    Py_Finalize();
    fds_verify(PythonBind::py_logger);
    delete PythonBind::py_logger;
}

PythonBind::PythonBind(const std::string &py_file) : p_filename(py_file),
                                                     p_name(NULL),
                                                     p_module(NULL),
                                                     p_calls_made(0),
                                                     p_func_last()
{
    fds_assert(p_filename.length() != 0);
    p_name = PyString_FromString(p_filename.c_str());
    fds_assert(p_name);
}

PythonBind::~PythonBind()
{
    fds_assert(p_name);
    Py_DECREF(p_name);

    if (p_module) {
        Py_DECREF(p_module);
    }
}

int
PythonBind::load_module(void)
{
    int             err = 0;

    fds_assert(p_module == NULL);
    p_module = PyImport_Import(p_name);
    if (p_module == NULL) {
        PyErr_Print();
        FDS_LOG_SEV((*py_logger), fds::fds_log::critical)
                    << "Fail to load python module: " << p_filename << std::endl;
        __PB_DONE_ERR(err, 1);
    }

    done:
        fds_assert(p_module);
        return (err);
}

int
PythonBind::call(const char *py_func_name, int args_cnt, ...)
{
    int             err     = 0;
    PyObject       *p_func  = NULL;
    PyObject       *p_args  = NULL;
    PyObject       *p_value = NULL;

    int             i       = 0;
    int             res     = 0;
    char           *arg_str = NULL;
    va_list         va_args;

    fds_assert(p_module);

    p_func = PyObject_GetAttrString(p_module, py_func_name);
    fds_assert(p_func);
    if (p_func == NULL) {
        __PB_DONE_ERR(err, 1);
    }

    if (PyCallable_Check(p_func)) {
        /* parse the arguments and make a tuple */
        p_args = PyTuple_New(args_cnt);
        va_start(va_args, args_cnt);
        for (i = 0; i < args_cnt; ++i) {
            arg_str = va_arg(va_args, char *);
            fds_assert(arg_str);
            p_value = PyString_FromString(arg_str);
            fds_assert(p_value);
            if (p_value == NULL) {
                __PB_DONE_ERR(err, 1);
            }

            /* p_value reference stolen here: */
            res = PyTuple_SetItem(p_args, i, p_value);
            fds_assert(res == 0);
            p_value = NULL;
        }
        va_end(va_args);

        /* call the python function */
        p_value = PyObject_CallObject(p_func, p_args);
        if (p_value == NULL) {
            PyErr_Print();
            FDS_LOG_SEV((*py_logger), fds::fds_log::critical)
                        << "Python call failed: " << py_func_name << std::endl;
            __PB_DONE_ERR(err, 1);
        }

        FDS_LOG_SEV((*py_logger), fds::fds_log::normal)
                    << "Python call result: " << PyInt_AsLong(p_value) << std::endl;


    } else {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        FDS_LOG_SEV((*py_logger), fds::fds_log::critical)
                    << "Cannot find function: " << py_func_name << std::endl;
        __PB_DONE_ERR(err, 1);
    }

    done:
        p_calls_made++;
        if (p_func) {
            Py_XDECREF(p_func);
            p_func_last = py_func_name;
        }
        if (p_args) {
            Py_DECREF(p_args);
        }
        if (p_value) {
            Py_DECREF(p_value);
        }

    return (err);
}

}  // namespace fds
