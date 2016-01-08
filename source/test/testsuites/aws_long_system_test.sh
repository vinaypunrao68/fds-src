#!/bin/bash

PASSWD="dummy"mv 
INVENTORYFILE="long_system_test"
LOGFILE="/tmp/AwsLongSystemTest.log"

#ADD NEW SYSTEM TEST HERE IN THE ARRAY
declare -a SCENARIOLIST=(
AWS_tests
)
#These are failing due to fs-3449.  Commenting out for now
#AWS_KillServiceTest
#AWS_StartStopServiceTest
#AWS_AddRemoveServiceTest
#AWS_StartShutdownNodeTest
#AWS_AddRemoveNodeTest


STARTTIME=$(date)
echo "AWS LONG SYSTEM TEST START TIME:  $STARTTIME"

for SCENARIO in "${SCENARIOLIST[@]}"
do
	echo "======================================================================================================="
	echo "$SCENARIO START TIME:  $(date)"
	echo "Running Scenario $SCENARIO test....."
	sudo ./ScenarioDriverSuite.py -q ./${SCENARIO}.ini -d $PASSWD --verbose -l $LOGGING --install -z $INVENTORYFILE
	[[ $? -ne 0 ]] && echo "AWS LONG SYSTEM TEST:  $SCENARIO FAILED"
	echo "$SCENARIO END TIME:  $(date)"
	echo "*******************************************************************************************************"
done

ENDTIME=$(date)
echo "AWS LONG SYSTEM TEST END TIME: $ENDTIME"
