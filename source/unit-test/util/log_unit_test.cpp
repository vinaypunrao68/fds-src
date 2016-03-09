/*
 * Copyright 2013 Formation Data Systems, Inc.
 */


#include <util/Log.h>

int main(int argc, char* argv[]) {

  fds::fds_log logger("tester", "");

  FDS_LOG(logger) << "Once more!";

  for (int i = 0; i < 1000000; i++) {
    if ((i % 5) == 0) {
      SCOPEDATTR("Key","CriticalContext");
      FDS_LOG_SEV(logger, fds::fds_log::critical) << "Logging a critical error";
      {
        SCOPEDATTR("Inner", "CriticalContext");
        FDS_LOG_SEV(logger, fds::fds_log::critical) << "Logging a second critical error";
      }
    } else if ((i % 4) == 0) {
      FDS_LOG_SEV(logger, fds::fds_log::error) << "Logging an error";
    } else if ((i % 3) == 0) {
      FDS_LOG_SEV(logger, fds::fds_log::warning) << "Logging a warning";
    }  else if ((i % 2) == 0) {
      SCOPEDATTR("Key","NotifyContext");
      SCOPEDATTR("Phillip", "NotifyContext2");
      FDS_LOG_SEV(logger, fds::fds_log::notification) << "Logging an notification";
    } else {
      FDS_LOG(logger) << "Logging normally";
    }
  }
  return 0;
}