#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import socket
import struct
import time
import logging

from FDS_ProtocolInterface.ttypes import *
from fds_service.ttypes import *
from fds_service.constants import *
from fds_service import SMSvc
from fds_service import DMSvc
from fds_service import PlatNetSvc

from FdsException import *

log = logging.getLogger(__name__)

##
# @brief Returns thrift object's member variable type by looking into
# thrift spec
#
# @param th_obj
# @param mem_var
#
# @return 
def get_th_mem_var_type(th_obj, mem_var):
    for mem_spec in th_obj.thrift_spec:
        if mem_spec and mem_spec[2] == mem_var:
            if mem_spec[1] is TType.STRUCT:
                return mem_spec[3][0]
            return mem_spec[1]
    raise AttributeError("Unknown attribute: {}".format(mem_var))

##
# @brief Convert thrift primitive type into python primitive type
#
# @param t thrift primitive type
# @param val value to cast
#
# @return value converted to python primitive
# NOTE: Not all primitives are supported
def cast_th_type(t, val):
    if val is None:
        return None
    if t is TType.I32:
       return int(val) 
    elif t is TType.I64:
        return long(val)
    elif t is TType.BOOL:
        return bool(val)
    elif t is TType.DOUBLE:
        return float(val)
    elif t is TType.STRING:
        return str(val)
    raise Exception("invalid type")

##
# @brief Converts dictionary d into thrift object.  d can be a nested dictionary.
#
# @param th_type thrift type to convert the dictionary d into
# @param d dictionary
#
# @return thrift object
def dict_to_th_obj(th_type, d):
    th_obj = th_type()
    for mem_var_name in d.keys():
        th_type = get_th_mem_var_type(th_obj, mem_var_name)
        if type(d[mem_var_name]) is dict:
            assert(th_type is not TType.MAP)
            # make sure it's not a dictionary
            setattr(th_obj, mem_var_name, dict_to_th_obj(th_type, d[mem_var_name]))
        else:
            setattr(th_obj, mem_var_name, cast_th_type(th_type, d[mem_var_name])) 
    return th_obj

##
# @brief Converts thrift object into dictionary
#
# @param th_type
#
# @return dictionary
def th_obj_to_dict(th_type):
    d = dict()
    th_obj = th_type()
    for spec_item in th_obj.thrift_spec:
        if spec_item is None:
            continue
        if spec_item[1] is TType.STRUCT:
            d[spec_item[2]] = th_obj_to_dict(spec_item[3][0])
        else:
            d[spec_item[2]] = None
    return d
