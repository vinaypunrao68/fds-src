#!/bin/bash -le

###################################################################################
###################################################################################
###################################################################################
###################################################################################
# This script is deprecated by jenkins_tests_copy_for_build_test_coroner_cleanup.sh
###################################################################################
###################################################################################
###################################################################################
###################################################################################

echo "$0 is deprecated, please update your workflow to use jenkins_tests_copy_for_build_test_coroner_cleanup.sh"
exit 67

function run_coverage_test_runner_tests
{
   for directory in ${COVERAGE_TEST_RUNNER_DIRECTORIES}
   do
      cd ${directory}
      python -m CoverageTestRunner
      cd -
   done
}

function run_unittest_discovery
{
   for directory in ${PYTHON_UNITTEST_DISCOVERY_DIRECTORIES}
   do
      cd ${directory}
      python -m unittest discover -p "*_test.py"
      cd -
   done

}

root_dir="$(pwd)/.."

COVERAGE_TEST_RUNNER_DIRECTORIES="${root_dir}/source/platform/python/tests"
PYTHON_UNITTEST_DISCOVERY_DIRECTORIES="${root_dir}/source/tools"

run_coverage_test_runner_tests
run_unittest_discovery

exit 0
