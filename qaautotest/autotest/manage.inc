<?php
###############################################
# This file will contain the functions for the 
# management page of the autotest database.
###############################################
require_once ('funclib.inc');
require_once ('constants.inc');


function make_management_right() {
    echo "<html><head><title>Management</title>";
    javascript_header();
    style_header();
    echo "</head>";
    echo "<body>";
    echo "<hr />";
    if (!empty($_GET['manage_id'])) {
        $mgnt_id = $_GET['manage_id'];
    } else {
        $mgnt_id = 0;
    }
    // Now depend on mgnt_id, do the right side.
    if ($mgnt_id == 1) {
        mgnt_right_add_test();
    } elseif ($mgnt_id == 2) {
        mgnt_right_suite_page();
    } elseif ($mgnt_id == 3) {
        mgnt_right_suite_page_old();
    } elseif ($mgnt_id == 4) {
        display_all_test_cases();
    } elseif ($mgnt_id == 5) {
        display_test_to_suite_matrix();
    } elseif ($mgnt_id == 6) {
        administer_database();
    } else {
        display_all_test_cases();
    }
    echo "<hr />";
    echo "</body>";
    echo "</html>";
}


function get_management_list() {
    // Each of these entries define what will
    // be displayed on the right side frame.
    $info[1]['name'] = 'Upload tests';
    $info[2]['name'] = 'Test suite management';
    //$info[3]['name'] = 'Old test suite management';
    $info[4]['name'] = 'Show all tests';
    $info[5]['name'] = 'Test to suite matrix';
    
    $ip = $_SERVER['REMOTE_ADDR'];
    $ini_array = parse_ini_file("database.ini");
    if (in_array($ip, split(',', $ini_array["admin"]))) {
        $info[6]['name'] = 'Database admin';
    }
    return $info;
}


function make_management_left() {
// This is the management page's controls.
    global $COLORS;
    $select = 1;
    echo "<html><head><title>Management Control Frame</title>";
    style_header();
    echo "</head>";
    if (!empty ($_POST['mgnt_list'])) {
        $select = $_POST['mgnt_list'];
        // Update the right side to match the left.
        $bodyLine = "<body onload=window.parent.management_right.location=\"{$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$select}";
        // These line will set the right window to the right suite, existing suites trump new suite name.
        // Select existing suite to none if you want to add new suite.
        if (!empty ($_POST['suiteName'])) {
            $bodyLine .= "&suiteName={$_POST['suiteName']}";
        } elseif (!empty ($_POST['suiteNameList'])) {
            foreach ($_POST['suiteNameList'] as $tempSuiteName) {
                $bodyLine .= "&suiteNameList[]={$tempSuiteName}";
            }
        } elseif (!empty ($_POST['newSuiteName'])) {
            $bodyLine .= "&newSuiteName={$_POST['newSuiteName']}";
        }
        $bodyLine .= "\">";
    } else {
        $bodyLine = "<body>";
    }
    echo $bodyLine."\n";
    echo "<form action=\"{$_SERVER['PHP_SELF']}\" name=\"task_form\" method=\"post\" target=\"_top\">";
    make_page_selector(2);
    echo "</form>";
    echo "<hr />";
    echo "<form action=\"{$_SERVER['PHP_SELF']}?frame=management_left\" name=\"Management_Control\" method=\"post\" target=\"management_left\">";
    echo "<select name=\"mgnt_list\" onchange=submit();>";
    $mgnt_page = get_management_list();
    while (list ($id, $mgnt_obj) = each($mgnt_page)) {
        $selected="";
        if ($select == $id) {$selected="selected";}
        echo "<option value=\"{$id}\" $selected> {$mgnt_obj['name']}</option>\n";
    }
    echo "</select>";
    echo "</form>";
    echo "<hr />";
    if ($select == 1) {
        // This is the test management page.
        ?>
        <table>
        <tr><th colspan=2 align=left>Update test database from .csv</th></tr>
        <tr><td colspan=2 align=left>* Choose a .csv file containing test cases to upload to the database.</td></tr>
        <tr><td colspan=2 align=left>* Duplicate tests in .csv will be ignored.</td></tr>
        <tr><td colspan=2 align=left>* Tests marked as XFAIL must have a defect ID after the run_as_root flag.</td></tr>
        <form method="POST" enctype="multipart/form-data" action="<?=$_SERVER['PHP_SELF']?>?frame=management_right<?php echo "&manage_id={$select}"?>" target="management_right">
        <input type="hidden" name="MAX_FILE_SIZE" value="100000" />
        <tr><th colspan=2 align=left>.csv file to upload:<br><input type="file" name="upfile"></th></tr>
        <tr><td colspan=2 align=left>
        <input type="checkbox" name="overrideDB" value="1" />Replace database values<br>
        </td></tr>
        <tr><td>
        <input type="submit" value="Update database" />
        </td></tr>
        </form>
        
        <tr><th align=left>Legend</th></tr>
        <?php
        echo "<tr bgcolor={$COLORS['GREEN']}><td align=center>Test successfully added to the database</td></tr>";
        echo "<tr bgcolor={$COLORS['PASTEY']}><td align=center>Test is already in the database</td></tr>";
        echo "<tr bgcolor={$COLORS['YELLOW']}><td align=center>Updated the database with new value</td></tr>";
        echo "<tr bgcolor={$COLORS['RED']}><td align=center>Failed database action</td></tr>";
        echo "<tr bgcolor={$COLORS['PORANGE']}><td align=center>Unable to process .csv line<br>Tests marked as 'skip' or 'xfail' must include a defect ID</td></tr>";
        echo "</table>";
        echo "<p></p>";
    } elseif ($select == 2) {
        $tlArray = mgnt_make_test_array();
        $suite_name = "";
        if (empty ($tlArray)) {
            // Should never get into here if both DB and WEB is on the same machine.
            echo "<br>Unable to load a list of tests from the database. ";
            echo "You must first add tests to the database before you can create a test suite.</br>";
            return;
        }
        // This is the suite management page.
        echo "\n<table>\n";
        // First we have a text entry for adding suites, this will add an item into the auto_suite_list.
        if (empty ($_POST['suiteNameList'])) {
            if (empty ($_POST['newSuiteName'])) {
				echo "";
            } else {
                if (add_suite($_POST['newSuiteName']) == -1) {
                    $tColor=$COLORS['PORANGE'];
                } else {
                    $tColor=$COLORS['GREEN'];
                    $_POST['suiteNameList'][] = $_POST['newSuiteName'];
                }
                echo "<tr><th colspan=2 align=center bgcolor={$tColor}> Suite: {$_POST['newSuiteName']}</th></tr>\n";
            }
        } else {
            // Print out each suite to the top
            echo "<tr><th colspan=2 align=center bgcolor={$COLORS['PORANGE']}> Suite(s): \n";
            foreach ($_POST['suiteNameList'] as $pSuiteName) {
                if ($pSuiteName == "") {
                    continue;
                }
                echo "{$pSuiteName} ";
            }
            echo "</th></tr>\n";
        }
        // Get all the suite names, we need it to be here because we may have added a suite from add_suite().
        $allSuiteName = mgnt_get_suite_names();
        echo "<form action={$_SERVER['PHP_SELF']}?frame=management_left method=\"post\" target=\"management_left\" name=\"Management_Control\">\n";
        echo "<input type=\"hidden\" name=\"mgnt_list\" value=\"2\" />\n";
        echo "<tr><th size=15 align=left>Create suite</th>\n<td><input type=\"text\" name=\"newSuiteName\" size=\"25\" /></td></tr>\n";
		echo "<tr><td></td><td><input type=\"submit\" value=\"Create\" /><br><br></td></tr>";
        echo "</form>\n\n";
		if (!$allSuiteName) {
		    echo "<tr><td colspan=2>Create a new test suite by entering a suite name into the text box and clicking \"Create\".</td></tr>\n";
		} else {
		    echo "<form action={$_SERVER['PHP_SELF']}?frame=management_left method=\"post\" target=\"management_left\" name=\"Management_Control\">\n";
            echo "<input type=\"hidden\" name=\"mgnt_list\" value=\"2\" />\n";
            echo "<tr>\n";
            echo "<th size=15 align=left valign=\"top\">or select an existing suite</th>\n";
            echo "<td>\n";
            echo "<select name=suiteNameList[] multiple size=10>\n";
            //echo "<option value=\"\">None</option>\n";
            foreach ($allSuiteName as $suiteName) {
                $selected = "";
                if (!empty ($_POST['suiteNameList'])) {
                    foreach ($_POST['suiteNameList'] as $csName) {
                        if ($csName == $suiteName) {
                            $selected = "selected";
                        }
                    }
                }
                echo "<option value=\"{$suiteName}\" $selected>{$suiteName}</option>\n";
            }
            echo "</select>";
            echo "<tr><td></td><td><input type=\"submit\" value=\"Set working suite\" /></td></tr>\n";
            echo "</form>\n\n";
            echo "<form action={$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$select} target=\"management_right\" method=\"post\">\n";
            if (!empty ($_POST['suiteNameList'])) {
                foreach ($_POST['suiteNameList'] as $gsName) {
                    echo "<input type=\"hidden\" name=\"suiteNameList[]\" value=\"{$gsName}\" />\n";
                }
            }
            echo "</td></tr>\n";
		    echo "<th size=15 align=left colspan=2><br><br>Add tests to the selected suite:</th>\n";
            echo "<tr><td colspan=2>\n";
            echo "<select name=addSuiteList[] multiple size=20>\n";
            foreach ($tlArray as $fileName => $fileItems) {
                echo "<optgroup label=\"{$fileName}\">\n";
                foreach ($fileItems as $test) {
                    echo "<option value=\"{$test['File']},{$test['Class']}\">{$test['Class']}</option>\n";
                }
                echo "</optgroup>\n";
            }
            echo "</select>\n";
            echo "</td></tr>\n";
            echo "<tr><td colspan=2><input type=\"submit\" name=\"UpdateSuite\" value=\"Add tests to suite\" /></td></tr>\n";
            echo "<tr><td colspan=2>Step 1: Create a new suite or select an existing suite.</td></tr>\n";
            echo "<tr><td colspan=2>Step 2: Select tests to add to the suite.</td></tr>\n";
            echo "<tr><td colspan=2>Step 3: Click \"Add tests to suite\".</td></tr>\n";
            echo "</form>\n\n";
		}
        // Second we will have a list of test items in a checkbox list.
        echo "</table>\n";
    } elseif ($select == 3) {
        $tlArray = mgnt_make_test_array();
        $suite_name = "";
        if (empty ($tlArray)) {
            // Should never get into here if both DB and WEB is on the same machine.
            echo "<br>Unable to load a list of tests from the database. ";
            echo "You must first add tests to the database before you can create a test suite.</br>";
            return; 
        }       
        // This is the suite management page.
        echo "<table>\n";
        // First we have a text entry for adding suites, this will add an item into the auto_suite_list.
        if (empty ($_POST['suiteName'])) {
            if (empty ($_POST['newSuiteName'])) {
                echo "<tr><th colspan=2 align=center>No working suite selected</th></tr>";
            } else {
                if (add_suite($_POST['newSuiteName']) == -1) {
                    $tColor=$COLORS['PORANGE'];
                } else {
                    $tColor=$COLORS['GREEN'];
                    $_POST['suiteName'] = $_POST['newSuiteName'];
                }
                echo "<tr><th colspan=2 align=center bgcolor={$tColor}> Suite: {$_POST['newSuiteName']}</th></tr>";
            }
        } else {
            echo "<tr><th colspan=2 align=center bgcolor={$COLORS['PORANGE']}> Suite: {$_POST['suiteName']}</th></tr>";
        }
        // Get all the suite names, we need it to be here because we may have added a suite from add_suite().
        $allSuiteName = mgnt_get_suite_names();
        if (!$allSuiteName) {
            echo "Failed to get suite names.";
            return;
        }
        ?>
        <form action="<?=$_SERVER['PHP_SELF']?>?frame=management_left" method="post" target="management_left" name="Management_Control">
        <input type="hidden" name="mgnt_list" value="3" />
        <tr><th size=15 align=left>New Suite</th><td><input type="text" name="newSuiteName" size="25" /></td></tr>
        <tr><th colspan=2 align=center>or select</th></tr>
        <tr>
        <th size=15 align=left>Old Suite</th>
        <td>
        <select name="suiteName" onchange=submit();>
        <?php
        echo "<option value=\"\">None</option>\n";
        foreach ($allSuiteName as $suiteName) {
            $selected = "";
            if ($suiteName == $_POST['suiteName']) {$selected = "selected";}
            echo "<option value=\"{$suiteName}\" $selected>{$suiteName}</option>\n";
        }
        ?>
        </select>
        <tr><td colspan=2><input type="submit" value="Set Working Suite" /></td></tr>
        </form>
        <form action="<?=$_SERVER['PHP_SELF']?>?frame=management_right<?php echo "&manage_id={$select}&suiteName={$_POST['suiteName']}"?>" target="management_right" method="post">
        </td></tr>
        <tr><td colspan=2>
        <select name=addSuiteList[] multiple>
        <?php
        foreach ($tlArray as $fileName => $fileItems) {
            echo "<optgroup label=\"{$fileName}\">\n";
            foreach ($fileItems as $test) {
                echo "<option value=\"{$test['File']},{$test['Class']}\">{$test['Class']}</option>\n";
            }
            echo "</optgroup>\n";
        }
        echo "</select>";
        echo "</td></tr>";
        echo "<tr><td colspan=2><input type=\"submit\" name=\"UpdateSuite\" value=\"Add tests to suite\" /></td></tr>";
        echo "</form>";
        // Second we will have a list of test items in a checkbox list.
        echo "</table>\n";
    } elseif ($select == 4) {
        // Simple display of all tests in autotest
        echo "This page contains a list of all tests entered into the database.";
    } elseif ($select == 5) {
        echo "This page shows all tests in the database and the suite(s) to which the tests are assigned. ";
        echo "Tests that are not assigned to any suite are highlighted.";
        echo "<table>";
        echo "<tr><th>LEGEND</th></tr>";
        echo "<tr bgcolor=\"{$COLORS['LIGHTBLUE']}\"><th>File Name</th></tr>";
        echo "<tr bgcolor=\"{$COLORS['PORANGE']}\"><th>Not in suite</th></tr>";
        echo "</table>";
    } elseif ($select == 6) {
        // Simple display of all tests in autotest
        echo "<strong>Select a table in the database to administer:</strong><br />";
        echo "<form action={$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$select} target=\"management_right\" method=\"post\"";
        echo "<tr><td colspan=2>";
        echo "<input type='radio' name='administer' value='test_suites'>Suites<br />";
        echo "<input type='radio' name='administer' value='test_cases'>Test Cases<br />";
        echo "<input type='radio' name='administer' value='test_results'>Test Results<br /><br />";
        echo "</td></tr>";
        echo "<tr><td colspan=2><input type=\"submit\" name=\"submit\" value=\"Submit\"></td></tr>";
        echo "</form>";
    }
    echo "</body>";
    echo "</html>";
}


function write_suite_to_csv_file($suiteName) {
    $query = "select atc.File File, atc.Class Class, atc.run_flag run_flag, atc.run_as_root run_as_root, atc.xfail_bug_id xfail_bug_id ".
        "from auto_test_case atc ".
#        "where atc.File in ".
#        "(select distinct test_file from auto_test_suites where auto_suite_name=\"".$suiteName."\") ".
#        "and atc.Class in (select distinct test_class from auto_test_suites where auto_suite_name=\"".$suiteName."\") ".
        "where atc.id in (select id from auto_test_suites where auto_suite_name=\"".$suiteName."\") order by atc.File, atc.Class";
    connect_db();
    $result = mysql_query($query);
    if (!$result) {
        die("Invalid query: ".mysql_error());
    }
    $myFile = "./csv/{$suiteName}.csv";
    $fh = fopen($myFile, "w") or die("Can't open file.");  // Replace old file.
    fwrite ($fh, "file_name,test_class,run_flag,run_as_root,xfail_bug_id\n");
    while ($row = mysql_fetch_assoc($result)) {
        foreach ($row as &$value) {
            if ($value == "NULL") {$value = "";}
        }
        unset($value);
        $stringData = "{$row['File']},{$row['Class']},{$row['run_flag']},{$row['run_as_root']},{$row['xfail_bug_id']}\n";
        fwrite ($fh, $stringData);
    }
    fclose($fh);
    mysql_free_result($result);
    return 1;
}


function write_all_tests_csv() {
    $query = "select atc.File File, atc.Class Class, atc.run_flag run_flag, atc.run_as_root run_as_root, atc.xfail_bug_id xfail_bug_id ".
        "from auto_test_case atc";
    connect_db();
    $result = mysql_query($query);
    if (!$result) {
        die("Invalid query: ".mysql_error());
    }
    $myFile = "./csv/all_tests.csv";
    $fh = fopen($myFile, "w") or die("Can't open file.");  // Replace old file.
    fwrite ($fh, "file_name,test_class,run_flag,run_as_root,xfail_bug_id\n");
    while ($row = mysql_fetch_assoc($result)) {
        foreach ($row as &$value) {
            if ($value == "NULL") {$value = "";}
        }
        unset($value);
        $stringData = "{$row['File']},{$row['Class']},{$row['run_flag']},{$row['run_as_root']},{$row['xfail_bug_id']}\n";
        fwrite ($fh, $stringData);
    }
    fclose($fh);
    mysql_free_result($result);
    return 1;
}


function mgnt_make_test_array() {
    $tempArray = array();
    $query = "select id, File, Class, run_flag, run_as_root, xfail_bug_id from auto_test_case order by File, Class";
    connect_db();
    $result = mysql_query($query);
    while ($row = mysql_fetch_assoc($result)) {
        if (array_key_exists ($row['File'], $tempArray)) {
            if (array_key_exists ($row['id'], $tempArray[$row['File']])) {
                // The file + Class already exist, probably a different version
                // Version will be done by run. So just ignore version
                continue;
            }
        } else {
            // If there isn't an array for this file name, create it.
            $tempArray[$row['File']] = array();
        }
        $tempArray[$row['File']][$row['id']] = array();
        $tempArray[$row['File']][$row['id']]['File'] = $row['File'];
        $tempArray[$row['File']][$row['id']]['Class'] = $row['Class'];
        $tempArray[$row['File']][$row['id']]['id'] = $row['id'];
        $tempArray[$row['File']][$row['id']]['run_flag'] = $row['run_flag'];
        $tempArray[$row['File']][$row['id']]['run_as_root'] = $row['run_as_root'];
        $tempArray[$row['File']][$row['id']]['xfail_bug_id'] = $row['xfail_bug_id'];
    }
    mysql_free_result($result);
    return $tempArray;
}


function mgnt_get_suite_names() {
    connect_db();
    $tempArray = array();
    $result = mysql_query("select distinct suite_name from auto_suite_list order by suite_name");
    if (!$result) {
        return;
    }
    while ($row = mysql_fetch_assoc($result)) {
        $tempArray[] = $row['suite_name'];
    }
    mysql_free_result($result);
    return $tempArray;
}


function mgnt_get_tests_in_suite($suiteName) {
    $tempArray = array();
    $query = "select * from auto_test_suites where auto_suite_name=\"{$suiteName}\" order by test_file, test_class";
    connect_db();
    $result = mysql_query($query);
    if (!$result) {
        echo "<br/> Unable to get test list for test suite \"{$suiteName}\".<br/>\n";
        return;
    }
    while ($row = mysql_fetch_assoc($result)) {
        $xarray = array();
        array_push ($tempArray, $xarray);
        $xarray['test_file'] = $row['test_file'];
        $xarray['test_class'] = $row['test_class'];
        $xarray['id'] = $row['id'];
    }
    mysql_free_result($result);
    return $tempArray;
}


function fetch_suites_that_include_test($file_class) {
    list($test_file, $test_class) = explode(":", $file_class);
    $query = "select auto_suite_name from auto_test_suites where test_file=\"$test_file\" and test_class=\"$test_class\"";
    connect_db();
    $query_info = mysql_query($query);
    $suites = array();
    while ($row = mysql_fetch_assoc($query_info)) {
        array_push($suites, $row["auto_suite_name"]);
    }
    mysql_free_result($query_info);
    return $suites;
}


function fetch_total_number_of_jobs_in_database() {
    $query = "select * from auto_test_jobs";
    connect_db();
    $result = mysql_query($query);
    $count = 0;
    while ($this_row = mysql_fetch_assoc($result)) {
        $count += 1;
    }
    mysql_free_result($result);
    return $count;
}


function display_all_test_cases() {
    global $COLORS;
    $test_count = 0;
    $query = "select File, Class, run_flag, xfail_bug_id, run_as_root from auto_test_case order by File, Class";
    connect_db();
    $result = mysql_query($query);
    echo "<table border=1>";
    echo "<tr><th>File Name</th><th>Test Class</th><th>Run Flag</th><th>Run as root</th><th>XFAIL Defect ID</th></tr>";
    while ($row = mysql_fetch_assoc($result)) {
        foreach ($row as &$value) {
            if ($value == "NULL") {$value = "";}
        }
        unset($value);
        echo "<tr bgcolor={$COLORS['PASTEY']}>";
        echo "<td>".$row['File']."</td>";
        echo "<td>".$row['Class']."</td>";
        echo "<td>".$row['run_flag']."</td>";
        echo "<td>".$row['run_as_root']."</td>";
        echo "<td>".$row['xfail_bug_id']."</td>";
        echo "</tr>";
        $test_count += 1;
    }
    mysql_free_result($result);
    echo "<tr><th colspan=6 align=\"left\">Total number of tests in the database: {$test_count}</th></tr>";
    echo "</table>";
}


function display_test_to_suite_matrix() {
    // Create a matrix of tests to suite, the rows will be tests, the columns will be suites.
    // Any test not included in any suite should be high lighted by a orange background. 
    global $COLORS;

    connect_db();

    // TODO: This may be really slow in the future
    // First create a array of arrays from the suite list.
    $query = "select File, Class, auto_suite_name ".
        "from auto_test_case ".
        "LEFT JOIN auto_test_suites ".
        "on auto_test_case.File=auto_test_suites.test_file ".
        "and auto_test_case.Class=auto_test_suites.test_class ".
        "order by File, Class, auto_suite_name";
    $suiteArray = array();
    $result = mysql_query($query);
    if (!$result) {
        die("Invalid query".mysql_error());
    }
    while ($row = mysql_fetch_assoc($result)) {
        if (!(array_key_exists ($row['File'], $suiteArray))) {
            // Create a new array for each suite
            $suiteArray[$row['File']] = array();
        }
        if (!(array_key_exists ($row['Class'], $suiteArray[$row['File']]))) {
            $suiteArray[$row['File']][$row['Class']] = array();
        }
        $suiteArray[$row['File']][$row['Class']][] = $row['auto_suite_name'];
    }
    mysql_free_result($result);
    echo "<table border=1>";
    echo "<tr><th>Class</th><th>Suites</th></tr>";
    foreach ($suiteArray as $file => $classList) {
        echo "<tr bgcolor=\"{$COLORS['LIGHTBLUE']}\"><th colspan=10 align=left>{$file}</th></tr>\n";
        foreach ($classList as $class => $suiteList) {
            $bgc = "\"".$COLORS['WHITE']."\"";
            $suites = join(", ", $suiteList);
            if ($suites == '') {
                $bgc = "\"".$COLORS["PORANGE"]."\"";
            }
            echo "<tr bgcolor={$bgc}>";
            echo "<td>{$class}</td><td>{$suites}</td>\n";
            echo "</tr>\n";
        }
    }
    echo "</table>";
}


function display_test_suite_table_with_checkboxes() {
    global $COLORS;
    echo "<table border=1 cellpadding=2>";
    echo "<form action=\"{$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$_GET['manage_id']}\" target=\"management_right\" method=\"post\" name=\"delete_suites\">\n";
    echo "<input type=\"hidden\" name=\"del_suite\" value=1/>\n";
    // A check all box
    echo "<tr><td align=center>\n";
    echo "<input type=\"checkbox\" name=\"checkall\" "; 
    echo "onclick=\"window.parent.checkAll(window.parent.management_right.document.delete_suites, window.parent.management_right.document.delete_suites.checkall);\"/></td><th>Suite Name</th></tr>\n";
    // Now create the check box for each suite.
    $all_suites = mgnt_get_suite_names();
    $count = 0;
    foreach ($all_suites as $this_suite) {
        if (($count % 2) == 0) {
            $bgc = $COLORS['LIGHTBLUE'];
        } else {
            $bgc = $COLORS['WHITE'];
        }
        echo "<tr bgcolor=\"$bgc\">";
        echo "<td align=center><input type=\"checkbox\" name=\"del_suites[]\" value=\"$this_suite\"/></td>";
        echo "<td>$this_suite</td>";
        echo "</tr>\n";
        $count += 1;
    }
    echo "<tr><td colspan=10>\n";
    echo "<input type=\"submit\" name=\"submit\" value=\"Delete suite(s)\" /></td></tr>\n";
    echo "</form>\n";
    echo "</table>\n";
    return;
}


function display_test_case_table_with_checkboxes() {
    global $COLORS;
    echo "<table border=1 cellpadding=2>";
    echo "<form action=\"{$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$_GET['manage_id']}\" target=\"management_right\" method=\"post\" name=\"delete_test_cases\">\n";
    echo "<input type=\"hidden\" name=\"del_test\" value=1/>\n";
    // A check all box
    echo "<tr><td align=center>\n";
    echo "<input type=\"checkbox\" name=\"checkall\" "; 
    echo "onclick=\"window.parent.checkAll(window.parent.management_right.document.delete_test_cases, window.parent.management_right.document.delete_test_cases.checkall);\"/></td><th>File Name</th><th>Test Class</th></tr>\n";
    // Now create the check box for each suite.
    $query = "select File, Class from auto_test_case order by File, Class";
    connect_db();
    $result = mysql_query($query);
    $count = 0;
    while ($row = mysql_fetch_assoc($result)) {
        if (($count % 2) == 0) {
            $bgc = $COLORS['LIGHTBLUE'];
        } else {
            $bgc = $COLORS['WHITE'];
        }
        echo "<tr bgcolor=\"$bgc\">";
        echo "<td align=center><input type=\"checkbox\" name=\"del_tests[]\" value=\"{$row['File']}:{$row['Class']}\"/></td>";
        echo "<td>".$row['File']."</td>";
        echo "<td>".$row['Class']."</td>";
        echo "</tr>";
        $count += 1;
    }
    echo "<tr><td colspan=10>\n";
    echo "<input type=\"submit\" name=\"submit\" value=\"Delete test(s)\" /></td></tr>\n";
    echo "</form>\n";
    echo "</table>\n";
    mysql_free_result($result);
}


function display_test_results_admin() {
    global $COLORS;
    $query = "select * from auto_test_jobs";
    connect_db();
    $result = mysql_query($query);
    $count = 0;
    $first_row;
    $last_row;
    $this_row;
    while ($this_row = mysql_fetch_assoc($result)) {
        if ($count == 0) {
            $first_row = $this_row;
        }
        $count += 1;
        $last_row = $this_row;
    }
    mysql_free_result($result);
    $now = getdate();
    $current_date = date('Y-m-d H:i:s', $now[0]);
    if ($first_row) {
        $oldest_date = date('Y-m-d H:i:s', $first_row['start_time']);
    } else {
        $oldest_date = "";
    }
    if ($last_row) {
        $newest_date = date('Y-m-d H:i:s', $last_row['start_time']);
    } else {
        $newest_date = "";
    }
    
    echo "<form action=\"{$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$_GET['manage_id']}\" target=\"management_right\" method=\"post\">\n";
    echo "<input type=\"hidden\" name=\"del_results\" value=1/>\n";
    echo "<strong>Total test runs in the database: </strong>$count<br />";
    echo "<strong>Today's date/time: </strong>$current_date<br />";
    echo "<strong>Date of the oldest test run: </strong>$oldest_date<br />";
    echo "<strong>Date of the most recent test run: </strong>$newest_date<br /><br />";
    echo "Delete records older than <select name=\"value\">";
    $count = 1;
    while ($count <= 30) {
        echo "<option value=\"$count\">$count</option>\n";
        $count += 1;
    }
    echo "</select>\n";
    echo "<select name=\"interval\">\n";
    echo "<option value=\"days\">days</option>\n";
    echo "<option value=\"weeks\">weeks</option>\n";
    echo "<option value=\"months\">months</option>\n";
    echo "</select>\n<br />";
    echo "<input type=\"submit\" name=\"submit\" value=\"Delete\" /></td></tr>\n";
    echo "</form>\n";
}


function add_csv_file_to_db($fileName, $className, $runTimeFlag, $run_as_root, $xfail_bug_id) {
    // Function to try to insert file , Class, into the
    // auto_test_case table.
    $hay = array (1=>'', 'skip', 'xfail');
    $selectQuery = "select File, Class, run_flag, run_as_root, xfail_bug_id ".
            "from auto_test_case ".
            "where File=\"{$fileName}\" ".
            "and Class=\"{$className}\"";
    connect_db();
    $info = array();
    $result = mysql_query($selectQuery);
    if (!$result) {
        echo "\n".$selectQuery."\n";
        die('Invalid query: '.mysql_error());
        return -1;
    }
    while ($row = mysql_fetch_assoc($result)) {
        if (array_key_exists ("File", $row)) {
			if (empty ($_POST['overrideDB'])) {
				return 1;
			} elseif ($_POST['overrideDB'] != 1) {
                return -1;
            }
			$updateValues = "";
			if (array_key_exists ("run_flag", $row)) {
				// We now check the run_flag, if this is different use the new one.
				$runTimeFlag = strtolower ($runTimeFlag);
				if (array_search ($runTimeFlag, $hay)) {
					if ($runTimeFlag != $row['run_flag']) {
						// The new flag is different from the one in DB.
						$updateValues = "run_flag=\"{$runTimeFlag}\"";
					}
					if (($runTimeFlag == "xfail") or ($runTimeFlag == "skip")) {
						// The xfail flag is up, we must to update the XFAIL BUG ID
                        if ($xfail_bug_id == "") {
                            return 3;
                        }
					}
                    if ($xfail_bug_id != $row['xfail_bug_id']) {
                        if (! (($xfail_bug_id == "") and ($row['xfail_bug_id'] == "NULL"))) {
                            if ($updateValues != "") {$updateValues .= ", ";}
                            if ($xfail_bug_id == "") {$xfail_bug_id = "NULL";}
                            $updateValues .= "xfail_bug_id=\"{$xfail_bug_id}\"";
                        }
					}
				}
			} 
			// Check if this test will be running as root.
			if (trim($run_as_root) != "") {
				if ($row['run_as_root'] != $run_as_root) {
					if ($updateValues != "") {$updateValues .= ", ";}
					$updateValues .= "run_as_root={$run_as_root}";
				}
			}
			if ($updateValues != "") {
				$updateQuery = "update auto_test_case set ".$updateValues.
					" where File=\"{$fileName}\"".
					" and Class=\"{$className}\"";
				$result = mysql_query($updateQuery);
				if ($result != TRUE) {
					echo "\n".$updateQuery."\n";
					die('Invalid query: '.mysql_error());
					return -1;
				} else {
					return 2;
				}
			} else {
				return 1;
			}
        }
            
    }
    mysql_free_result($result);
    // Looks like there arn't any stuff left.
    if ($xfail_bug_id == "") {$xfail_bug_id = "NULL";}
    $insertQuery = "insert into auto_test_case (File, Class, run_flag, run_as_root, xfail_bug_id) ".
                   "values (\"{$fileName}\", \"{$className}\", \"{$runTimeFlag}\", \"{$run_as_root}\", \"{$xfail_bug_id}\")";
    $result = mysql_query($insertQuery);
    if ($result != TRUE) {
        echo "\n".$insertQuery."\n";
        die('Invalid query: '.mysql_error());
        return -1;
    }
    return 0;
}


function add_suite($suiteName) {
    // This only adds the suite name to the auto_suite_list
    // First check if the suite already exist
    $result = mysql_query("select * from auto_suite_list where suite_name=\"{$suiteName}\"");
    if (!$result) {
        // there is already a suite of this name
        mysql_free_result($result);
        return -1;
    }
    $query = "insert into auto_suite_list (suite_name) values (\"{$suiteName}\")";
    $result = mysql_query($query);
    if (!$result) {
        echo $query;
        echo "<br>";
        die('Invalid query: '.mysql_error());
    }
    return 0;
}


function mgnt_right_add_test() {
    global $COLORS;
    $tempFilePath = $_FILES['upfile']['tmp_name'];
    // If we don't want to save the file, just operate on the tempFilePath
    // To save the file, move the temp file to the target_path
    if ($tempFilePath) {
        $fileArray = file ($tempFilePath, FILE_SKIP_EMPTY_LINES|FILE_TEXT);
        if (empty ($fileArray)) {
            echo "Upload a .csv file from the left frame.";
            return;
        }
    } else {
        echo "Upload a .csv file from the left frame.";
        return;
    }
    if (!empty ($_GET['manage_id'])) {
        echo "<table border=1>\n";
        foreach ($fileArray as $id => $line) {
            $itemList = split (",", stripcslashes($line));
            if (strstr($itemList[0], ".py")) {
                $iRC = add_csv_file_to_db (trim($itemList[0], "\""), trim($itemList[1],"\""), $itemList[2], $itemList[3], trim($itemList[4]));
                if ($iRC < 0) {
                    $bgc = "\"".$COLORS['RED']."\"";
                } elseif ($iRC == 0) {
                    $bgc = "\"".$COLORS['GREEN']."\"";
                } elseif ($iRC == 1) {
                    $bgc = "\"".$COLORS["PASTEY"]."\"";
                } elseif ($iRC == 2) {
                    $bgc = "\"".$COLORS["YELLOW"]."\"";
                } elseif ($iRC == 3) {
                    $bgc = "\"".$COLORS["PORANGE"]."\"";
                }
                echo "<tr bgcolor={$bgc}>\n";
				$this_file = trim($itemList[0], "\"");
				$this_class = trim($itemList[1],"\"");
				$query = "select File, Class, run_flag, run_as_root, xfail_bug_id ".
							   "from auto_test_case where File=\"{$this_file}\" ".
							   "and Class=\"{$this_class}\"";
				$result = mysql_query($query);
				$tc_details = mysql_fetch_assoc($result);
                foreach ($tc_details as $item) {
                    if ($item == "NULL") {$item = "";}
                    echo "<td>{$item}</td>";
                }
                echo "</tr>\n";
            } else {
                if ($id == 0) {
                    echo "<tr>\n";
                    foreach ($itemList as $item) {
                        if ($item == "NULL") {$item = "";}
                        echo "<th>{$item}</th>\n";
                    }
                    echo "</tr>\n";
                } else {
                    echo "<tr bgcolor={$COLORS['PORANGE']}><td colspan=2>{$line}</td></tr>";
                }
            }
        }
        echo "</table>\n";
        // Create a all test CSV file
        write_all_tests_csv();
    } else {
        echo "Select a management page";
    }
}


function mgnt_right_suite_page() {
    // This is a complete rewrite of the mgnt_right_suite_page
    global $COLORS;
    if (!$_GET['suiteNameList']) {
        if ($_POST['suiteNameList']) {
            $_GET['suiteNameList'] = $_POST['suiteNameList'];
        } elseif (!$_GET['newSuiteName']) {
            echo "Create a new suite or select an existing test suite from the left frame.";
        } else {
            echo "Please add tests to new test suite '{$_GET['newSuiteName']}' from the left frame.";
        }
    } 
    if ($_GET['suiteNameList']) {
        // We have a suiteName so print the suite (s).
        // First convert the POST value into a string.  Suite are not case sensitive.
        $suiteNameString = "(";
        $tempSuiteNameArray = $_GET['suiteNameList'];
        foreach ($_GET['suiteNameList'] as $gsName) {
            if ($gsName == "") {
                continue;
            }
            $suiteNameString .= "\"$gsName\",";
        }
        if ($suiteNameString == "(" and $gsName == "") {
            // There is no suite, just die
            echo "Create a new suite or select an existing test suite from the left frame.";
            return;
        }
        $suiteNameString = trim ($suiteNameString, ",");
        $suiteNameString .= ")";
        
        connect_db();
        if (!empty ($_POST['SuiteMod'])) {
            // First update all the suites base on SuiteMod
            // First generate a list for each suite and push it into old list
            $oldSuiteItemList = array();
            $dbResult = mysql_query("select * from auto_test_suites where auto_suite_name in $suiteNameString order by test_file, test_class, auto_suite_name");
            if (!$dbResult) {
                echo "Auto encounter database error<br>";
                die('Invalid query: '.mysql_error());
            }
            while ($row = mysql_fetch_assoc($dbResult)) {
                $oldSuiteItemList[] = $row['auto_suite_name'].",".$row['test_file'].",".$row['test_class'];
            }
            // Free the dbresult 
            mysql_free_result($dbResult);
            // Now match the 2 lists
            // The newStuff need to be added
            $newStuff = array_diff ($_POST['SuiteMod'], $oldSuiteItemList);
            // The oldStuff need to be deleted
            $oldStuff = array_diff ($oldSuiteItemList, $_POST['SuiteMod']);
            // Now delete the oldStuff first.
            if (!empty ($oldStuff)) {
                foreach ($oldStuff as $workItem) {
                    list ($workSuiteName, $workFileName, $workClassName) = split (",", $workItem);
                    $dbResult = mysql_query("delete from auto_test_suites where ".
                            "auto_suite_name=\"{$workSuiteName}\" ".
                            "and test_file=\"{$workFileName}\" ".
                            "and test_class=\"{$workClassName}\"");
                    if (!$dbResult) {
                        echo "Auto encounter database error<br>";
                        die('Invalid query: '.mysql_error());
                    }
                }
            }
            if (!empty ($newStuff)) {
                foreach ($newStuff as $workItem) {
                    list ($workSuiteName, $workFileName, $workClassName) = split (",", $workItem);
                    $idQuery = mysql_query("select id from auto_test_case where File=\"{$workFileName}\" and Class=\"{$workClassName}\"");
                    $testID = mysql_fetch_assoc($idQuery);
                    $addQuery = "insert into auto_test_suites ".
                        "(auto_suite_name, test_file, test_class, id) values ".
                        "(\"{$workSuiteName}\", \"{$workFileName}\", \"{$workClassName}\", {$testID['id']})";
                    $dbResult = mysql_query($addQuery);
                    if (!$dbResult) {
                        die('Invalid query: '.mysql_error());
                    }
                }
            }
        } elseif (!empty ($_POST['addSuiteList'])) {
            // then check for add
            // For each suite, add the test
            foreach ($_POST['addSuiteList'] as $testToAdd) {
                list ($adFile, $adClass) = split (",", $testToAdd);
                $idQuery = mysql_query("select id from auto_test_case where File=\"{$adFile}\" and Class=\"{$adClass}\"");
                if (!$idQuery) {
                    die('Invalid query: '.mysql_error());
                }
                $testID = mysql_fetch_assoc($idQuery);
                mysql_free_result($idQuery);
                foreach ($_GET['suiteNameList'] as $adToSuite) {
                    if ($adToSuite == "") {
                        continue;
                    }
                    // Check if it is already in the suite
                    $query = "select * from auto_test_suites where ".
                        "auto_suite_name=\"{$adToSuite}\" ".
                        "and test_file=\"{$adFile}\" ".
                        "and test_class=\"{$adClass}\"";
                    $idQuery = mysql_query($query);
                    $tQueryValue = mysql_fetch_assoc($idQuery);
                    if (empty ($tQueryValue)) {
                        // Suite don't have this test, lets add it
                        $addQuery = "insert into auto_test_suites ".
                            "(auto_suite_name, test_file, test_class, id) values ".
                            "(\"{$adToSuite}\", \"{$adFile}\", \"{$adClass}\", {$testID['id']})";
                        $dbResult = mysql_query($addQuery);
                        if (!$dbResult) {
                            die('Invalid query: '.mysql_error());
                        }
                    } else {
                        mysql_free_result($idQuery);
                    }
                }
            }
        }
        // Now print the suite(s)
        $dbResult = mysql_query("select * from auto_test_suites where auto_suite_name in $suiteNameString order by test_file, test_class, auto_suite_name");
        if (!$dbResult) {
            echo "Auto encounter database error<br>";
            die('Invalid query: '.mysql_error());
        }
        // First create an array of suite to test
        $test2SuiteA = array();
        while ($row = mysql_fetch_assoc($dbResult)) {
            if (!array_key_exists ($row['test_file'], $test2SuiteA)) {
                $test2SuiteA[$row['test_file']] = array();
            }
            if (!array_key_exists ($row['test_class'], $test2SuiteA[$row['test_file']])) {
                $test2SuiteA[$row['test_file']][$row['test_class']] = array();
            }
            // This is one messy 3D array
            $test2SuiteA[$row['test_file']][$row['test_class']][$row['auto_suite_name']] = 1;
        }
        // Free the db results structure.
        mysql_free_result($dbResult);
        // Order the suites by suite,fileName,TestName as key
		if (!$test2SuiteA) {
		    echo "Add test cases to the selected suite(s) by selecting the desired tests, then click \"Add tests to suite\".";
			return;
		}
        echo "<table border=1>";
//        echo "<tr><th colspan=10 align=center>{$_GET['suiteNameList']}</th></tr>\n";
        echo "<form action=\"{$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$_GET['manage_id']}&\" target=\"management_right\" method=\"post\" name=\"suite_case_mod\">\n";
        $_POST['suiteNameList'] = $_GET['suiteNameList'];
        foreach ($_GET['suiteNameList'] as $gsName) {
            if ($gsName == "") {
                continue;
            }
            echo "<input type=\"hidden\" name=\"suiteNameList[]\" value=\"{$gsName}\" />\n";
        }
        echo "<input type=\"hidden\" name=\"modSuiteItem\" value=1/>\n";
        echo "<tr><th>File</th><th>Class</th>\n";
        foreach ($_GET['suiteNameList'] as $gsName) {
            if ($gsName == "") {
                continue;
            }
            echo "<th>{$gsName}</th>\n";
        }
        echo "</tr>\n";
        // Now generate the check boxes using the test2SuiteA array
        $count = 0;
        foreach ($test2SuiteA as $test_file => $test_classList) {
            foreach ($test_classList as $test_class => $suiteList) {
                if (($count % 2) == 0) {
                    $bgc = $COLORS['LIGHTBLUE'];
                } else {
                    $bgc = $COLORS['WHITE'];
                }
				if ($count == 0) {
		                echo "<tr>\n";
                        echo "<td colspan=2></td>\n";
						foreach ($_GET['suiteNameList'] as $this_suite) {
                            $this_suite = str_replace("-", "_", $this_suite);
                            echo "<td align=center><input type=\"checkbox\" name=\"{$this_suite}_top\" onclick=\"window.parent.check_suite(window.parent.management_right.document.suite_case_mod, window.parent.management_right.document.suite_case_mod.{$this_suite}_top)\" /></td>\n";
						}
                        echo "</tr>\n";
						$count += 2;
		            }
                echo "<tr bgcolor={$bgc}>";
                echo "<td>{$test_file}</td><td>{$test_class}</td>\n";
                foreach ($_GET['suiteNameList'] as $gsName) {
                    if ($gsName == "") {
                        continue;
                    }
                    $temp_gsname = str_replace("-", "_", $gsName);
                    if (array_key_exists ($gsName, $suiteList)) {
                        echo "<td align=center><input type=\"checkbox\" id=\"{$temp_gsname}\" checked name=\"SuiteMod[]\" value=\"{$gsName},{$test_file},{$test_class}\"/></td>\n";
                    } else {
                        echo "<td align=center><input type=\"checkbox\" id=\"{$temp_gsname}\" name=\"SuiteMod[]\" value=\"{$gsName},{$test_file},{$test_class}\"/></td>\n";
                    }
                }
                echo "</tr>\n";
                $count += 1;
            }
        }
        echo "<tr><td colspan=10>\n";
        echo "<input type=\"submit\" name=\"ModStuffSuite\" value=\"Apply\" /></td></tr>\n";
        echo "</form>\n";
        echo "</table>\n";
		echo "Note: This table is an exclusive list of test cases in the selected suites.  Test cases that are not a part of any of the selected suites will not show up in the list.  Check the \"Test to suite matrix\" to see what tests may be omitted.<br /><br />";
        // Finally dump each suite to the csv file.
        foreach ($_GET['suiteNameList'] as $gsName) {
            if ($gsName == "") {
                continue;
            }
            echo "<a href=./csv/{$gsName}.csv>{$gsName}.csv</a><br>\n";
            write_suite_to_csv_file($gsName);
        }
    }
}


function mgnt_right_suite_page_old() {
    global $COLORS;

    if (!$_GET['suiteName']) {
        if (!$_GET['newSuiteName']) {
            echo "No test suite selected Old.";
        } else {
            echo "Please add tests to new test suite '{$_GET['newSuiteName']}' from the left frame list.";
        }
    } else {
        // We have a suiteName so print the suite.
        connect_db();
        // First if we have some tests to remove or add to suite before we continue with print.
        if (!empty ($_POST['del'])) {
            // If we are removing tests from suite.
            foreach ($_POST['del'] as $testToRemove) {
                list ($rmFile, $rmClass) = split (",",$testToRemove);
                $delQuery = "delete from auto_test_suites ".
                    "where auto_suite_name=\"{$_GET['suiteName']}\" and ".
                    "test_file=\"{$rmFile}\" and ".
                    "test_class=\"{$rmClass}\"";
                $dbResult = mysql_query($delQuery);
                if (!$dbResult) {
                    echo $delQuery;
                    die('Invalid query: '.mysql_error());
                }
            }
        } elseif (!empty ($_POST['addSuiteList'])) {
            // If we are adding tests to suite.
            foreach ($_POST['addSuiteList'] as $testToAdd) {
                list ($adFile, $adClass) = split (",", $testToAdd);
                // First get the test_case_id for the test.  If no Id, the test is
                // not in the DB, so continue.
                $idQuery = mysql_query("select id from auto_test_case where File=\"{$adFile}\" and Class=\"{$adClass}\"");
                if (!$idQuery) {
                    die('Invalid query: '.mysql_error());
                }
                $testID = mysql_fetch_assoc($idQuery);
                mysql_free_result($idQuery);
                $query = "select * from auto_test_suites where ".
                    "auto_suite_name=\"{$_GET['suiteName']}\" ".
                    "and test_file=\"{$adFile}\" ".
                    "and test_class=\"{$adClass}\"";
                $idQuery = mysql_query($query);
                $tQueryValue = mysql_fetch_assoc($idQuery);
                if (empty ($tQueryValue)) {
                    $addQuery = "insert into auto_test_suites ".
                        "(auto_suite_name, test_file, test_class, id) values ".
                        "(\"{$_GET['suiteName']}\", \"{$adFile}\", \"{$adClass}\", {$testID['id']})";
                    $dbResult = mysql_query($addQuery);
                    if (!$dbResult) {
                        die('Invalid query: '.mysql_error());
                    }
                }
                mysql_free_result($idQuery);
            }
        }
        $dbResult = mysql_query("select * from auto_test_suites where auto_suite_name=\"{$_GET['suiteName']}\" order by test_file, test_class");
        if (!$dbResult) {
            echo "Auto encounter database error<br>";
            mysql_free_result($dbResult);
            die('Invalid query: '.mysql_error());
        }
        echo "<table border=1>";
        echo "<tr><th colspan=10 align=center>{$_GET['suiteName']}</th></tr>\n";
        echo "<form action=\"{$_SERVER['PHP_SELF']}?frame=management_right&manage_id={$_GET['manage_id']}&suiteName={$_GET['suiteName']}\" target=\"management_right\" method=\"post\" name=\"suite_case_delete\">\n";
        echo "<input type=\"hidden\" name=\"delSuiteItem\" value=1/>\n";
        echo "<tr><th>Delete</th><th>Python File</th><th>Test Class</th></tr>\n";
        // A check all box
        echo "<tr><td align=center>\n";
        echo "<input type=\"checkbox\" name=\"checkall\" "; 
        echo "onclick=\"window.parent.checkAll(window.parent.management_right.document.suite_case_delete, window.parent.management_right.document.suite_case_delete.checkall);\"/></td><th colspan=10 align=left>Check All</th></tr>\n";
        // Now create the check box for each test.
        $count = 0;
        while ($row = mysql_fetch_assoc($dbResult)) {
            if (($count % 2) == 0) {
                $bgc = $COLORS['LIGHTBLUE'];
            } else {
                $bgc = $COLORS['WHITE'];
            }
            echo "<tr bgcolor={$bgc}>";
            echo "<td align=center><input type=\"checkbox\" name=\"del[]\" value=\"{$row['test_file']},{$row['test_class']}\"/></td>";
            echo "<td>{$row['test_file']}</td><td>{$row['test_class']}</td>";
            echo "</tr>\n";
            $count += 1;
        }
        echo "<tr><td colspan=10>\n";
        echo "<input type=\"submit\" name=\"RmStuffSuite\" value=\"Delete checked\" /></td></tr>\n";
        echo "</form>\n";
        echo "</table>\n";
        echo "<a href=./csv/{$_GET['suiteName']}.csv>{$_GET['suiteName']}.csv</a>";
        mysql_free_result($dbResult);
        write_suite_to_csv_file($_GET['suiteName']);
    }
}


function administer_database() {
    global $COLORS;
    if ($_POST["del_suites"]) {
        delete_suites($_POST["del_suites"]);
        return;
    } elseif ($_POST["del_tests"]) {
        delete_test_cases($_POST["del_tests"]);
        return;
    } elseif ($_POST["del_results"]) {
        delete_test_results($_POST["value"], $_POST["interval"]);
        return;
    }
    if (!$_POST['administer']) {
        echo "Select a database table to administer." ;
        return;
    }
    $selected_radio = $_POST['administer'];
    if ($selected_radio == "test_suites") {
        display_test_suite_table_with_checkboxes();
        return;
    } elseif ($selected_radio == "test_cases") {
        display_test_case_table_with_checkboxes();
        return;
    } elseif ($selected_radio == "test_results") {
        display_test_results_admin();
        return;
    }
    return;
}


function delete_suites($suites) {
    $count = 0;
    connect_db();
    foreach ($suites as $this_suite) {
        $del_from_list = "delete from auto_suite_list where suite_name=\"$this_suite\"";
        $del_suite = "delete from auto_test_suites where auto_suite_name=\"$this_suite\"";
        mysql_query($del_from_list);
        mysql_query($del_suite);
        unlink("./csv/{$this_suite}.csv");
        $count += 1;
    }
    display_test_suite_table_with_checkboxes();
    echo "<strong><br />Removed $count suite(s) from the database.</strong><br />";
}


function delete_test_cases($tests) {
    $count = 0;
    connect_db();
    foreach ($tests as $this_test) {
        list($test_file, $test_class) = explode(":", $this_test);
        $del_test_from_suites = "delete from auto_test_suites where test_file=\"$test_file\" and test_class=\"$test_class\"";
        $del_test_case = "delete from auto_test_case where File=\"$test_file\" and Class=\"$test_class\"";
        mysql_query($del_test_from_suites);
        mysql_query($del_test_case);
        $suites = fetch_suites_that_include_test($this_test);
        foreach ($suites as $this_suite) {
            write_suite_to_csv_file($this_suite);
        }
        $count += 1;
    }
    display_test_case_table_with_checkboxes();
    echo "<strong><br />Removed $count test case(s) from the database.</strong><br />";
}


function delete_test_results($value, $interval) {
    $now = getdate();
    $remove_older_than = 0;
    if ($interval == "days") {
        $remove_older_than = ($now[0] - (intval($value) * 86400));
    } elseif ($interval == "weeks") {
        $remove_older_than = ($now[0] - (intval($value) * 604800));
    } elseif ($interval == "months") {
        $remove_older_than = ($now[0] - (intval($value) * 2629743));
    }
    $before = fetch_total_number_of_jobs_in_database();
    
    connect_db();
    $query = "delete from auto_test_jobs where start_time < $remove_older_than";
    mysql_query($query);
    $query = "delete from auto_test_item where start_time < $remove_older_than";
    mysql_query($query);    
    
    $after = fetch_total_number_of_jobs_in_database();
    $removed = ($before - $after);
    
    display_test_results_admin();
    //echo "<strong><br />Current time: $now[0]</strong><br />";
    //echo "<strong><br />Remove records older than: $remove_older_than</strong><br />";
    echo "<strong><br />Removed $removed test records(s) from the database.</strong><br />";
    return;
}
?>
