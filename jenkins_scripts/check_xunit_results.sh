#!/bin/bash
#
# This script does a quick search of xunit results and
# returns a simple pass/fail result. This is a protection
# mechanism because we found that Jenkins sometimes doesn't
# mark builds as failed with failed xunit output.
#

echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo "# This script is deprecated by jenkins_tests_copy_for_build_test_coroner_cleanup.sh"
echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo ""
echo "$0 is deprecated, please update your workflow to use jenkins_build_test_coroner_cleanup.sh (pausing 30 seconds)"

curl "http://beacon.formationds.com/?JENKINS_DEPRECATED_SCRIPTS=true&script=${0}&JOB_NAME=${JOB_NAME}&PWD=${PWD}&BUILD_NUMBER=${BUILD_NUMBER}" &

sleep 30

EXIT=0

function message
{
   echo "================================================================================"
   echo "$*"
   echo "================================================================================"
}

function check_xunit_failures
{
	message "Checking xunit output for failure"
	grep -e 'failures="[1-9].*"' `find source/cit/ -name '*.xml'`
	if [[ $? -eq 0 ]] ; then
		echo "XUNIT FAILURES FOUND..."
		EXIT=97
	fi
}

function check_xunit_errors
{
	message "Checking xunit output for errors"
	grep -e 'errors="[1-9].*"' `find source/cit/ -name '*.xml'`
	if [[ $? -eq 0 ]] ; then
		echo "XUNIT ERRORS FOUND..."
		EXIT=97
	fi
}

check_xunit_errors
check_xunit_failures

exit $EXIT
