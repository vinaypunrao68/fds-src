/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Volume catalog class. Provides a database for mapping
 * volume blob locations to objects.
 */

#ifndef SOURCE_LIB_VOLUMECATALOG_H_
#define SOURCE_LIB_VOLUMECATALOG_H_

#include <string>

#include "Catalog.h"

namespace fds {

  class VolumeCatalog : public Catalog {
 private:
 public:
    explicit VolumeCatalog(const std::string& _file);
    ~VolumeCatalog();

    using Catalog::Query;
    using Catalog::Update;
    using Catalog::GetFile;
  };

  class TimeCatalog : public Catalog {
 private:
 public:
    explicit TimeCatalog(const std::string& _file);
    ~TimeCatalog();
  };

}  // namespace fds

#endif  // SOURCE_LIB_VOLUMECATALOG_H_
