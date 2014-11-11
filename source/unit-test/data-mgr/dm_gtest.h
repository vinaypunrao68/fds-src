/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UNIT_TEST_DATA_MGR_DM_GTEST_H_
#define SOURCE_UNIT_TEST_DATA_MGR_DM_GTEST_H_

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <stdlib.h>

#include <vector>
#include <string>


#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
namespace po = boost::program_options;

static fds_uint32_t MAX_OBJECT_SIZE = 2097152;    // 2MB
static fds_uint64_t BLOB_SIZE = 1 * 1024 * 1024 * 1024;   // 1GB
static fds_uint32_t NUM_VOLUMES = 1;
static fds_uint32_t NUM_BLOBS = 1;
static bool  profile = false;

static const fds_uint32_t NUM_TAGS = 10;

static fds_bool_t PUTS_ONLY = false;
static fds_bool_t NO_DELETE = false;

static std::atomic<fds_uint32_t> volCount;
static std::atomic<fds_uint32_t> blobCount;
static std::atomic<fds_uint64_t> txCount;


#endif  // SOURCE_UNIT_TEST_DATA_MGR_DM_GTEST_H_
