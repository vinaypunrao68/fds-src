<?php
// This file contain all the page and frame dealing with test result reports.
require_once ('constants.inc');  // Probably don't need this.
require_once ('funclib.inc');

function get_report_list () {
    $info = array();
    $info[1]['name'] = 'All jobs';
    $info[2]['name'] = 'Recent jobs';
    $info[3]['name'] = 'Suite report';
    $info[10]['name'] = 'Test data';
    //$info[11]['name'] = 'Report page test report';
    return $info;
}

function get_result_list () {
    ?>
    <option value="">all</option>
    <option value="PASS">Pass</option>
    <option value="FAIL">Fail</option>
    <option value="XFAIL">XFail</option>
    # If it is a ran test, there will not be any skipped.
    # Adding skip simply have centralized get result list function.
    <option value="SKIP">SKIP</option>
    <?php
}


function make_status_filter () {
    ?>
    <select name=statusList[] multiple size=5>
    <option value=all>all</option>
    <option value="PASS">PASS</option>
    <option value="FAIL">FAIL</option>
    <option value="XFAIL">XFAIL</option>
    <optuon value="SKIP">SKIP</option>
    </select>
    <?php
}

function make_machine_filter () {
    ?>
    <select name=machineList[] multiple size=5>
    <?php
    connect_db ();
    $qstring = "select distinct test_host from auto_test_jobs";
    $result = mysql_query ($qstring);
    echo "<option value=all>all</option>\n";
    while ($row = mysql_fetch_assoc ($result)) {
        echo "<option value=\"{$row['test_host']}\">".
            "{$row['test_host']} </option>\n";
    }
    mysql_free_result ($result);
    ?>
    </select>
    <?php
}


function make_job_list () {
    ?>
    <select name=job_idList[] multiple size=5>
    <?php
    connect_db ();
    $qstring = "select id from auto_test_jobs order by id desc";
    $result = mysql_query ($qstring);
    echo "<option value=all>all</option>\n";
    while ($row = mysql_fetch_assoc ($result)) {
        echo "<option value=\"{$row['id']}\">".
            "{$row['id']} </option>\n";
    }
    mysql_free_result ($result);
    ?>
    </select>
    <?php
}

function suiteListHTML () {
    // Test Suite List, no multiple.
    $qstring = "select * from auto_suite_list order by suite_name";
    connect_db ();
    $result = mysql_query ($qstring);
    if (!$result) {
        die ("Failed to get suite list from DB.".mysql_error());
    }
    echo "<select name=suiteName size=5>\n";
    while ($value = mysql_fetch_assoc ($result)) {
        echo "<option value=\"{$value['id']}\">{$value['suite_name']}</option>\n";
    }
    mysql_free_result ($result);
    echo "</select>";
}

function fileFilterHTML ($multi) {
    // Test file query string.
    $qstring = "select distinct test_file from auto_test_item order by test_file";
    connect_db();
    $result = mysql_query ($qstring);
    if (!$result) {
        die ('Failed test file list query on auto_test_item'.mysql_error());
    }
    echo "<select name=test_file[] {$multi} size=5>";
    if ($multi) {
        echo "<option value=all>all</option>";
    }
    while ($value = mysql_fetch_assoc ($result)) {
        // <option value="test1.py">test1.py</option>
        echo "<option value=\"".$value['test_file']."\">".$value['test_file']."</option>\n";
    }
    mysql_free_result ($result);
    echo "</select>";
}

function testFilterHTML ($multi, $whereData="") {
    // Test Class query string.
    $qstring = "select distinct test_class from auto_test_item ".$whereData." order by test_class";
    connect_db ();
    $result = mysql_query ($qstring);
    if (!$result) {
        die ('Failed test class list query on auto_test_item'.mysql_error());
    }
    echo "<select name=test_class[] {$multi} size=5>";
    if ($multi) {
        echo "<option value=all>all</option>";
    }
    while ($value = mysql_fetch_assoc ($result)) {
        // <option value="test1.py">test1.py</option>
        echo "<option value=\"".$value['test_class']."\">".$value['test_class']."</option>";
    }
    mysql_free_result ($result);
    echo "</select>";
}



function make_report_left () {
// Page1 is a reporting page so the left side should be selecting reports
// and report parameters.  First we'll have just a test select and date and
// time select for now.
    $select = 1;
    if (!empty ($_POST['report_list'])) {
        $select = $_POST['report_list'];
    }
    ?>
    <html><head><title>Report Selection Frame</title>
    <?php style_header(); ?>
    </head>
    <body>
    <form action="<?=$_SERVER['PHP_SELF']?>" name="task_form" method="post" target="_top">
    <?php make_page_selector (1); ?>
    </form>
    <hr />
    Select Report Type
    <form action="<?=$_SERVER['PHP_SELF']?>?frame=report_left" name="report_selection" method="post" target="report_left">
    <select name="report_list" onchange=submit();>
    <?php
    $report = get_report_list ();
    while (list ($id, $report_obj) = each ($report)) {
        $selected="";
        if ($select == $id) {$selected="selected";}
        echo "<option value=\"{$id}\" $selected> {$report_obj['name']}</option>\n";
    }
    ?>
    </select>
    </form>
    <hr />
    <form action="<?=$_SERVER['PHP_SELF']?>?frame=report_right<?php echo "&report_id={$select}"?>" name="report_options" method="post" target="report_right">
    <input type="hidden" name="report_id" value="<?=$select?>"/>
    <?php
    if ($select == 1 or $select == 2 ) {
        // There are no options for select 1, this is just a page list of test runs and results.
        ?>
        <table>
        <tr>
        <th colspan=2 align=center>Job Report Filters</th>
        </tr>
        <tr>
        <th colspan=2 align=left>Please use the filters.  Attempts to list all jobs can take several minutes.</th>
        </tr>
        <tr><td colspan=2>
        File Name Filter<br/>
        <?php fileFilterHTML ("multiple");?>
        </td></tr>
        <tr><td colspan=2>
        <br/>Test Name Filter<br/>
        <?php testFilterHTML ("multiple");?>
        </td></tr>
        <tr><td>
        <br/>Job ID Filter<br/>
        <?php make_job_list ();?>
        </td>
        <td><br/>Host Filter<br/>
        <?php make_machine_filter ();?>
        </td></tr>
        </table>
    <?php
    } elseif ($select == 3) {
        // This is the suite report page.
        ?>
        <table>
        <tr><th colspan=2 align="center">Suite Report Filters</th></tr>
        <tr><th colspan=2 align="left">Select a suite</th></tr>
        <tr><td colspan=2>
        <?php suiteListHTML ();?>
        </td></tr>
        <tr><th colspan=2 align="left">Select a test machine</th></tr>
        <tr><td colspan=2>
        <?php make_machine_filter ();?>
        </td></tr>
        <tr><th colspan=2 align="left">Results history</th></tr>
        <tr><td colspan=2>
        <select name=SuiteReportDepth>
        <?php
        echo "<option value=1>1</option>";
        $sel_cnt=10;
        for ($cnt = 5; $cnt <=50; $cnt +=5) {
            $selected = "";
            if ($sel_cnt == $cnt) {$selected="selected";}
            echo "<option value={$cnt} {$selected}>{$cnt}</option>";
        }
        ?>
        </td></tr>
        </select>
        </table>
    <?php
    } elseif ($select == 10) {
        // This is the test data report.
        ?>
        <table>
        <th>Test Data Report</th>
<?php /*
        <tr><td colspan=2>
        <?php fileFilterHTML ('');?>
        </td></tr>
*/?>
        <tr><td colspan=2>
        <br/>File Name Filter<br/>
        <?php testFilterHTML ('onchange=\'submit()\'', "where test_data != \"\"");?>
        </td></tr>
        <tr><td>
        <br/>Job ID Filter<br/>
        <?php make_job_list ();?>
        </td>
        <td><br/>Host Filter<br/>
        <?php make_machine_filter ();?>
        </td></tr>
        </table>
    <?php
    } elseif ($select == 11) {
        ?>
        <table>
        <tr><td colspan=2>
        File Name Filter<br/>
        <?php fileFilterHTML ();?>
        <?php testFilterHTML ();?> 
        </td></tr>
        </table>
    <?php
    }
    ?>
    <hr />
    <input type="submit" name="show_report" value="Report" />
    </form>
    </body>
    </html>
    <?php
} 


function make_report_right ($rightArray) {
    ?>
    <html><head><title>Report</title>
    <?php javascript_header();?>
    <?php style_header();?>
    </head>
    <body>
    <hr />
    <?php
    $whereClause = '';
    if (!empty($_POST['report_id'])) {
        $reportType = $_POST['report_id'];
    } elseif (!empty($rightArray['report_id'])) {
        $reportType = $rightArray['report_id'];
    } else {
        $reportType = 0;
    }

    // File name filter
    if (!empty($_POST['test_file'])) {
        $fileName = $_POST['test_file'];
    } elseif (!empty($rightArray['test_file'])) {
        $fileName = $rightArray['test_file'];
    } else {
        $fileName = '';
    }

    if ($fileName != '') {
        $fileWhereLine = '';
        foreach ($fileName as $tFileName) {
            if ($tFileName == "all") {
                $fileWhereLine = '';
                break;
            }
            if ($fileWhereLine != '') {
                $fileWhereLine .= ",";
            }
            $fileWhereLine .="\"".$tFileName."\"";
        }
        if ($fileWhereLine != '') {
            $whereClause .= "test_file in (".$fileWhereLine.")";
        }
    }
            

    // Test name filter
    if (!empty($_POST['test_class'])) {
        $testName = $_POST['test_class'];
    } elseif (!empty($rightArray['test_class'])) {
        $testName = $rightArray['test_class'];
    } else {
        $testName = '';
    }

    if ($testName != '') {
        $testWhereLine = '';
        foreach ($testName as $tTestName) {
            if ($tTestName == "all") {
                $testWhereLine = '';
                break;
            }
            if ($testWhereLine != '') {
                $testWhereLine .= ",";
            }
            $testWhereLine .="\"".$tTestName."\"";
        }
        if ($testWhereLine != '') {
            if (!($whereClause=='')) {
                $whereClause .= " and ";
            }
            $whereClause .= "test_class in (".$testWhereLine.")";
        }
    }
    
    if (!empty ($_POST['job_idList'])) {
        $job_id = $_POST['job_idList'];
    } elseif (!empty($rightArray['job_id'])) {
        $job_id = $rightArray['job_id'];
    } else {
        $job_id = '';
    }

    if (!empty ($_POST['machineList'])) {
        $machineArray = $_POST['machineList'];
    } elseif (!empty ($rightArray['machineList'])) {
        $machineArray = $rightArray['machineList'];
    } else {
        $machineArray = '';
    }

    if ($machineArray != '') {
        $acceptableID= array();
        $tempString = '';
        // We'll need to check the DB for this.
        foreach ($machineArray as $machine) {
            if ($tempString != '') {
                $tempString.=",\"".$machine."\"";
            } else {
                $tempString.="(\"".$machine."\"";
            }
        }
        $tempString .= ")";
        connect_db ();
        $qstring = "select distinct id from auto_test_jobs where test_host in ".$tempString;
        $result = mysql_query ($qstring);
        while ($id = mysql_fetch_assoc ($result)) {
            array_push ($acceptableID, $id['id']);
        }
        mysql_free_result ($result);

        // Now check the job_id list and only keep the ones with the right Machine.
        // Could have done this as a sub query or join in the SQL, but this may
        // probably take less processing when we have lots of data in DB.
        if ($job_id != '') {
            $acceptableID = array_intersect ($job_id, $acceptableID);
        }
    } elseif ($job_id != '') {
        $acceptableID = $job_id;
    }
    
    if ($acceptableID != '') {
        $testjob_idLine = '';
        foreach ($acceptableID as $tjob_id) {
            if ($tjob_id == "all") {
                $testjob_idLine = '';
                break;
            }
            if ($testjob_idLine != '') {
                $testjob_idLine .= ",";
            }
            $testjob_idLine .= $tjob_id;
        }
        if ($testjob_idLine != '') {
            if (!($whereClause=='')) {
                $whereClause .= " and ";
            }
            $whereClause .= "job_id in (".$testjob_idLine.")";
        }
    }
            
    
    if (($reportType != 0) and ($reportType != 3)) {
        echo "<a href={$_SERVER['PHP_SELF']}?frame=report_right&report_id={$reportType}";
        if ($fileName != '') { 
            foreach ($fileName as $tFileName) {
                echo "&test_file[]={$tFileName}";
            }
        }
        if ($testName != '') {
            foreach ($testName as $tTestName) {
                echo "&test_class[]={$tTestName}";
            }
        }
        if ($job_id != '') {
            foreach ($job_id as $tjob_id) {
                echo "&job_id[]={$tjob_id}";
            }
        }
        if ($machineArray != '') {
            foreach ($machineArray as $machine) {
                echo "&machineList[]={$machine}";
            }
        }
        echo ">Direct Link</a>";
    }

    if ($reportType != 0) {
        if ($reportType == 1) {
            report_allTest ($whereClause);
        } elseif ($reportType == 2) {
            report_latestTest ($whereClause);
        } elseif ($reportType == 3) {
            report_suite_result ($whereClause);
        } elseif ($reportType == 10) {
            report_test_data ($whereClause);
        } else {
            echo "NO REPORT SELECTED";
        }
    } else {
        //foreach ($rightArray as $key => $value) {
        //    echo "{$key} => {$value}\n";
        //}
        echo "Please select a report.";
    }
    ?>
    <hr />
    </body>
    </html>
    <?php
}

function get_status_color ($status) {
    global $STCOLOR;
    global $COLORS;
        if (strstr ($status, "PASS")) {
            $bgc = "\"".$STCOLOR['PASS']."\"";
        } elseif (strstr ($status, "XFAIL")) {
            $bgc = "\"".$STCOLOR["XFAIL"]."\"";
        } elseif (strstr ($status, "FAIL")) {
            $bgc = "\"".$STCOLOR['FAIL']."\"";
        } elseif (strstr ($status, "SKIP")) {
            $bgc = "\"".$STCOLOR["SKIP"]."\"";
        } else {
            $bgc = "\"".$COLORS["GREY"]."\"";
        }
    return $bgc;
}

function report_allTest ($whereClause) {
    //echo $whereClause;
    // Type one report is just a list of test results from the database
    $pass=0;
    $fail=0;
    $skip=0;
    $xfail=0;
    connect_db();
    $info = array ();
    if (!($whereClause == '')) {
        $whereLine = " where ".$whereClause;
    } else {
        $whereLine = "";
    }
    $qstring = "select * from auto_test_item".$whereLine." order by test_file, test_class, job_id desc";
    //echo "<br/>{$qstring}<br/>";
    $result = mysql_query ($qstring);
    if (!$result) {
        die ('Invalid query: '.mysql_error());
    }
    // Put the result into the info array
    ?>
    <table border=1>
    <tr><th colspan=11>All Jobs Test Report
    </th></tr>
    <tr><th>File Name</th>
    <th>Test Class</th>
    <th>Test Status</th>
    <th>Build</th>
    <th>Start Time</th>
<!--    <th>End Time</th> -->
    <th>Run Time</th>
    <th>User Time</th>
    <th>System Time</th>
    <th>Job ID</th>
<!--
    <th>test repot HTML</th>
    <th>Test Data String</th>
-->
    </tr>
    <?php
    while ($row = mysql_fetch_assoc ($result)) {
        $start_time = date ('Y-m-d H:i:s', $row['start_time']);
        $end_time = date ('Y-m-d H:i:s', $row['end_time']);
        $runTime = $row['end_time'] - $row['start_time'];
        echo "<tr>\n";
        echo "<td>{$row['test_file']}</td>\n";
        echo "<td>{$row['test_class']}</td>\n";
        $bgc = get_status_color ($row['test_status']);
        echo "<td bgcolor=".$bgc.">{$row['test_status']}</td>\n";
        echo "<td>{$row['build']}</td>\n";
        echo "<td>{$start_time}</td>\n";
//        echo "<td>{$end_time}</td>\n";
        echo "<td>{$runTime} sec</td>\n";
        echo "<td>{$row['utime']} sec</td>\n";
        echo "<td>{$row['stime']} sec</td>\n";
        echo "<td>{$row['job_id']}</td>\n";
//        echo "<td>{$row['testHTML']}</td>\n";
//        echo "<td>{$row['test_data']}</td>\n";
        echo "</tr>\n";
    }
    mysql_free_result ($result);
    ?>
    </table>
    <?php
}

function report_latestTest ($whereClause) {
    global $STCOLOR, $COLORS;
    //echo $whereClause;
    // Type 2 report
    $pass=0;
    $fail=0;
    $skip=0;
    $xfail=0;
    connect_db ();
    $info = array ();
    if (!($whereClause == '')) {
        $whereLine = " where ".$whereClause;
    } else {
        $whereLine = "";
    }   
    //$qstring = "select * from auto_test_item".$whereLine." order by job_id";
    $qstring = "select * from (select * from auto_test_item".$whereLine.") jt join auto_test_case on jt.test_file=auto_test_case.File and jt.test_class=auto_test_case.Class order by job_id";
    //echo "<br/>{$qstring}<br/>";
    $result = mysql_query ($qstring);
    if (!$result) {
        die ("Invalid query:".mysql_error());
    }
    $currentTime = time ();
    while ($row=mysql_fetch_assoc ($result)) {
        $itemName = $row['test_file'].$row['test_class'];
        if (!empty($info[$itemName])) {
            if ($info[$itemName]['start_time'] > $row['start_time']) {
                continue;
            }
        }
        $info[$itemName]['test_file'] = $row['test_file'];
        $info[$itemName]['test_class'] = $row['test_class'];
        $info[$itemName]['job_id'] = $row['job_id'];
        $info[$itemName]['start_time'] = $row['start_time'];
        $info[$itemName]['end_time'] = $row['end_time'];
        $info[$itemName]['stime'] = $row['stime'];
        $info[$itemName]['utime'] = $row['utime'];
        $info[$itemName]['age'] = (int)(($currentTime - $row['end_time'])/86400);
        $info[$itemName]['test_status'] = $row['test_status'];
        $info[$itemName]['test_data'] = $row['test_data'];
        $info[$itemName]['build'] = $row['build'];
        $info[$itemName]['xfail_bug_id'] = $row['xfail_bug_id'];
    }
    ?>
    <table border=1>
    <tr><th colspan=12>Latest Test Report
    </th></tr>
    <tr><th>File Name</th>
    <th>Test Class</th>
    <th>Test Status</th>
    <th>XFAIL Bug ID</th>
    <th>Build</th>
    <th>Start Time</th>
    <th>Run Time</th>
    <th>User Time</th>
    <th>System Time</th>
<!--    <th>End Time</th> -->
    <th>Age (days)</th>
    <th>Job ID</th>
<!--    
    <th>test repot HTML</th>
--!>
    <th>Test Data String</th> 
    </tr>
    <?php
    foreach ($info as $row) {
        $start_time = date ('Y-m-d H:i:s', $row['start_time']);
        $end_time = date ('Y-m-d H:i:s', $row['end_time']);
        $runTime = $row['end_time'] - $row['start_time'];
        $bgc = get_status_color ($row['test_status']);
        if ($row['age'] > 90) {
            $agebc = "red";
        } elseif ($row['age'] > 30) {
            $agebc = "orange";
        } elseif ($row['age'] > 14) {
            $agebc = "yellow";
        } else {
            $agebc = "lightgreen";
        }
        echo "<tr>\n";
        echo "<td>{$row['test_file']}</td>\n";
        echo "<td>{$row['test_class']}</td>\n";
        echo "<td bgcolor=".$bgc.">{$row['test_status']}</td>\n"; 
        echo "<td>{$row['xfail_bug_id']}</td>\n";
        echo "<td>{$row['build']}</td>\n";
        echo "<td>{$start_time}</td>\n";
//        echo "<td>{$end_time}</td>\n";
        echo "<td>{$runTime} sec</td>\n";
        echo "<td>{$row['utime']} sec</td>\n";
        echo "<td>{$row['stime']} sec</td>\n";
        echo "<td bgcolor=".$agebc.">{$row['age']} day(s)</td>\n";
        echo "<td>{$row['job_id']}</td>\n";
//        echo "<td>{$row['testHTML']}</td>\n";
        echo "<td>{$row['test_data']}</td>\n";
        echo "</tr>\n";
    }   
    mysql_free_result ($result);
    ?>  
    </table> 
    <?php
}

function report_suite_result () {
    global $COLORS;
    // Suite report is different from the job reports, so we will not take a where clauseInput.
    if (!empty ($_POST['suiteName'])) {
        $suite_id = $_POST['suiteName'];
    }
    if (!$suite_id) {
        echo "Please select a test suite";
        return;
    }
    $whereClause = "where suite_id={$suite_id} ";
    if (!empty ($_POST['SuiteReportDepth'])) {
        $depth = $_POST['SuiteReportDepth'];
    } else {
        $depth = 1;
    }
    if (!empty ($_POST['machineList'])) {
        $machineArray = $_POST['machineList'];
        $tempString = '';
        foreach ($machineArray as $machine) {
            if ($machine == "all") {
                break;
            }
            if ($tempString != '') {
                $tempString.=",\"".$machine."\"";
            } else {
                $whereClause .= "and ";
                $tempString.="(\"".$machine."\"";
            }
        }
        if ($tempString != "") {
            $tempString .= ")";
            $whereClause .= "test_host in {$tempString}";
        }
    }
    connect_db ();
    // Query the job list for all the jobs run as the chosen suite upto a number of run deep.
    $query = "select id, start_time, end_time, test_host, suite_id from auto_test_jobs ".$whereClause." order by start_time desc, id desc limit {$depth}";
    $dbResult = mysql_query ($query);
    //echo "<br>{$query}<br>";
    if (!$dbResult) {
        echo $query;
        die ("Invalid query:".mysql_error());
    }
    // Id list will be use by a second query for test items in a job.
    $idList = "";
    $jobList = array ();
    $jobArray = array ();
    // Gather the job queries into a array
    while ($row = mysql_fetch_assoc ($dbResult)) {
        // Gather the job Rows
        $jobArray[$row['id']] = $row;
        $jobList[] = $row['id'];
        if ($idList == "") {
            $idList.= "{$row['id']}";
        } else {
            $idList.= ",{$row['id']}";
        }
    }

    if ($idList == "") {
        echo "The selected test suite has not been run yet.";
        return;
    }
    // Free the result
    mysql_free_result ($dbResult);

    // Now get the file name and class name from the suite.
    $query = "SELECT ats.test_file, ats.test_class, ats.auto_suite_name suite_name, atc.xfail_bug_id xfail_bug_id ".
        "FROM auto_test_suites ats, auto_suite_list asl, auto_test_case atc ".
        "WHERE asl.suite_name=ats.auto_suite_name and atc.File=ats.test_file and atc.Class=ats.test_class and asl.id={$suite_id} ".
        "ORDER BY ats.test_file, ats.test_class";
    $dbResult = mysql_query ($query);
    //echo "<br>{$query}<br>";
    if (!$dbResult) {
        echo $query;
        die ("Invalid query:".mysql_error());
    }   
    // Now create the test case array
    $testArray = array ();
    while ($row = mysql_fetch_assoc ($dbResult)) {
        if (!array_key_exists ($row['test_file'], $testArray)) {
            // Create an array
            $testArray[$row['test_file']] = array ();
        }
        $testArray[$row['test_file']][$row['test_class']] = $row['xfail_bug_id'];
    }
    mysql_free_result ($dbResult);

    // Last DB query, get the job items.
    $query = "SELECT test_file, test_class, test_status, job_id, build ".
        "FROM auto_test_item ".
        "WHERE job_id in ({$idList}) ".
        "ORDER BY job_id desc";
    $dbResult = mysql_query ($query);
    //echo "<br>{$query}<br>";
    if (!$dbResult) {
        echo $query;
        die ("Invalid query:".mysql_error());
    }
    $itemArray = array ();
    while ($row = mysql_fetch_assoc ($dbResult)) {
        if (!array_key_exists ($row['job_id'], $itemArray)) {
            $itemArray[$row['job_id']] = array ();
        }
        if (!array_key_exists ($row['test_file'], $itemArray[$row['job_id']])) {
            $itemArray[$row['job_id']][$row['test_file']] = array ();
        }
        $itemArray[$row['job_id']][$row['test_file']][$row['test_class']] = $row['test_status'];
        $itemArray[$row['job_id']]['build'] = $row['build'];
    }
    mysql_free_result ($dbResult);

    // TODO: Maybe reduce the queries by joining some of the tables or doing
    // sub queries.  Join might be too process expensive later with large tables.

    // Now that we got all the junk we could put it into something useful.
    echo "<table border=1>\n";
    // Each row is either a fileName which will be colspan depth
    $depth += 2;
    echo "<tr><th>Tests</th><th>XFAIL Bug ID</th>";
    foreach ($jobList as $job) {
        $etime = date ('Y-m-d H:i:s', $jobArray[$job]['start_time']);
        $end_time = date ('Y-m-d H:i:s', $jobArray[$job]['end_time']);
        if ($jobArray[$job]['end_time'] == $jobArray[$job]['start_time']) {
            $detaTime = 0;
        } else {
            $detaTime = $jobArray[$job]['end_time']-$jobArray[$job]['start_time'];
        }
        $build = $itemArray[$job]['build'];
        echo "<th width=18>Job ID: {$job}<hr />{$etime}<hr />Build: {$build}<hr />{$detaTime} sec</th>";
    }
    echo "</tr>";
    foreach ($testArray as $test_file => $testItems) {
        echo "<tr><th colspan={$depth} bgcolor={$COLORS['LIGHTBLUE']} align=left>{$test_file}</th></tr>\n";
        foreach ($testItems as $test_class => $testXFID) {
            echo "<tr><th align=left>{$test_class}</th>";
            if ($testXFID == "NULL") {
                echo "<td></td>";
            }
            elseif ($testXFID != "") {
                echo "<td bgcolor={$COLORS['PORANGE']}>{$testXFID}</td>";
            } else {
                echo "<td>{$testXFID}</td>";
            }
            //echo "<th>{$test_class['xfail_bug_id']}</th>";
            // Now for each job in the $itemArray            
            foreach ($jobList as $job) {
                if (!empty ($itemArray[$job][$test_file][$test_class])) {
                    $status = $itemArray[$job][$test_file][$test_class];
                } else {
                    $status = "NOT RUN";
                }
                if ($status == "PASS") {
                    $bgc = $COLORS['GREEN'];
                } elseif ($status == "FAIL") {
                    $bgc = $COLORS['RED'];
                } elseif ($status == "XFAIL") {
                    $bgc = $COLORS['PORANGE'];
                } elseif ($status == "SKIP") {
                    $bgc = $COLORS['PASTEY'];
                } else {
                    $bgc = $COLORS['YELLOW'];
                }
                echo "<td bgcolor={$bgc}>{$status}</td>";
            }
            echo "</tr>";
        }
    }
    // Each column is a job.
    echo "</table>\n";
}

function report_test_data ($whereClause) {
    global $STCOLOR;
    global $COLORS;

    $data = array ();
    $tableKey = array ();

    if (!($whereClause == '')) {
        $whereLine = " where ".$whereClause." and job_id=atj.id";
    } else {
        $whereLine = "where job_id=atj.id";
    }   
    $qstring = "select distinct job_id, auto_test_item.end_time end_time, test_data, atj.test_host pc from auto_test_item, auto_test_jobs atj {$whereLine} order by atj.test_host, job_id, auto_test_item.end_time";
//    echo ("<br/>".$qstring."<br/>");
    connect_db ();
    $result = mysql_query ($qstring);
    if (!$result) {
        die ("Invalid query:".mysql_error());
    }

    // Push the result data into the $data array.  Key1 is the testUnit name.
    while ($row=mysql_fetch_assoc ($result)) {
        if ($row['test_data'] != '') {
            $data[$row['pc']][$row['job_id']]['end_time'] = $row['end_time']; 
            $test_dataArray = split (",", $row['test_data']);
            foreach ($test_dataArray as $tAitem) {
                if ($tAitem != '') {
                    list($itemKey, $itemValue) = split (":", $tAitem);
                    if (!in_array ($itemKey, $tableKey)) {
                        array_push ($tableKey, $itemKey);
                    }
                    $data[$row['pc']][$row['job_id']][$itemKey] = $itemValue;
                }
            }
        }
    }
    $keyCount = count ($tableKey) + 2;
    mysql_free_result ($result);

    echo "<table border=1>\n";
    echo "<tr><th colspan={$keyCount}>Test Data Report</th></tr>\n";
    echo "<tr><th>Job ID</th>\n";
    echo "<th>End Time</th>\n";
    foreach ($tableKey as $tHead) {
        echo "<th>{$tHead}</th>\n";
    }
    echo "</tr>\n";
    foreach ($data as $mkey => $minfo) {
        echo "<tr><th colspan={$keyCount} align=\"left\" bgcolor={$COLORS['LIGHTBLUE']}>Machine: {$mkey}</th></tr>";
        foreach ($minfo as $job_id => $mpoint) {
            echo "<tr>\n";
            echo "<td>{$job_id}</td>\n";
            $etime = date ('Y-m-d H:i:s', $mpoint['end_time']);
            echo "<td>{$etime}</td>\n";
            if (count($tableKey) > 0) {
                foreach ($tableKey as $tHead) {
                    if (array_key_exists ($tHead, $mpoint)) {
                        echo "<td> {$mpoint[$tHead]} </td>\n";
                    } else {
                        echo "<td></td>\n";
                    }
                }
            }
            echo "</tr>\n";
        }
    }
    echo "</table>\n";
}

?>
