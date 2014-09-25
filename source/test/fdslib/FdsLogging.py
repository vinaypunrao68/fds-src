#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import os
import sys
import logging
import logging.handlers

@staticmethod
def _setup_logging(logger_name, log_name, dir, log_level, num_threads, max_bytes=100*1024*1024, rollover_count=5):
    # Set up the core logging engine
    logging.basicConfig(level=log_level,
                format='%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s',
                datefmt='%Y-%m-%d %H:%M:%S')
    log = logging.getLogger(logger_name)
    logging.addLevelName(100, "REPORT")

    # Set up the log file and log file rotation
    dir = os.path.abspath(dir)
    log_file = os.path.join(dir, log_name)
    print("Logfile: %s"
              %log_file)
    if num_threads > 1:
        log_fmt = logging.Formatter("%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s",
                                    "%Y-%m-%d %H:%M:%S")
    else:
        log_fmt = logging.Formatter("%(asctime)s %(name)-24s %(levelname)-8s %(message)s",
                                    "%Y-%m-%d %H:%M:%S")
    if not os.access(dir, os.W_OK):
       log.warning("There is no write access to the specified log "
                         "directory %s  No log file will be created." %dir)
    else:
        try:
            log_handle = logging.handlers.RotatingFileHandler(log_file, "w",
                                                              max_bytes,
                                                              rollover_count)
            log_handle.setFormatter(log_fmt)
            logging.getLogger("").addHandler(log_handle)
            if os.path.exists(log_file):
                log_handle.doRollover()
        except:
            log.warning("Failed to rollover the log file.  The "
                             "results will not be recorded.")
            logging.getLogger("").removeHandler(log_handle)
    log.log(100, " ".join(sys.argv))

    return log

