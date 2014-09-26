<?php
/*
 * Created on Nov 29, 2007
 */
// Use constant.inc for any constants
// Use different inc files for each page.
// 2 pages in the beginning
// 1: Test execution page, this page will all access to see all the tests in 
//    the DB and provide ways to execute individual tests or group of tests.
//    The user will need to specify a Harness IP in the beginning, hopefully 
//    once the auto harness part is done, the user could simple say run, and 
//    allow the dispatcher to choose an harness.

// 2: Page 2 will be a reporting page, this page will contain 2 frames, the top 
//    frame will be a report selection frame,  the bottom frame will be the
//    report.  At first there should be a table report that should show all the
//    test jobs and their results.  Another page for reporting results from one
//    test, and finally one page for reporting the test define values.  Hopfully 
//    we could add a graph feature to the test define value page to graph the 
//    test values.

    // Set the require files.
    require 'constants.inc';
    require 'funclib.inc';
    require 'manage.inc';

    // Initial task is layout of the page
    $task = "layout";
    if ($_GET[frame]=="layout") {$task="layout";}
    elseif ($_GET[frame]=="page1_header") {$task="make_report_header";}
    elseif ($_GET[frame]=="report_left") {$task="make_report_left";}
    elseif ($_GET[frame]=="report_right") {$task="make_report_right";}
    elseif ($_GET[frame]=="management_left") {$task="make_management_left";}
    elseif ($_GET[frame]=="management_right") {$task="make_management_right";}
    
    // set a session.  We will only pull data from DB once for each session, or 
    // when we modify something.
    // Calling sessoin start for now, might want to remember sessions in the future
    // for that we will need to create a session_name () first.
    session_start ();

    // If we get a reset, reset the session.
    if (!empty ($_POST['Reset'])) {
        session_destroy ();
        session_start ();
        $task = "layout";
    }

    if (! $_SESSION[running]) {
        $_SESSION[running] = 1;
        $_SESSION['changed'] = false;
        // Create the default page. For now it will be the report page.
        // We'll load data from each page as they are created.
    }
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?php
// Primary page layout.
    if ($task == "layout") {
        $padding = 0; // use for tab spacing
        $page_id = 1; // 1: report, 2: summary ... will use 1 for now til a summary page could be put together.
        $b = $_SERVER['HTTP_USER_AGENT'];
        if (strpos($b, "MSIE") > 0) {
            // If we are posting to an IE add more padding.
            $padding = 15;
        }
        if (!empty($_POST['page_display'])) {
            $page_id = $_POST['page_display'];
        } elseif (!empty ($_GET['page_display'])) {
            $page_id = $_GET['page_display'];
        }

        // TODO: Later reset will repost from the selected page
        if ($page_id == 1) {
            report_layout ($padding);
        } elseif ($page_id == 2) {
            management_layout ($padding);
        }
    }
    elseif ($task == "make_header") {
        make_header ();  // We have common header for all page right now.
    } elseif ($task == "make_report_header") {
        make_report_header ();
    } elseif ($task == "make_report_left") {
        make_report_left ();
    } elseif ($task == "make_report_right") {
        // The right page is always the reporting page.
        $right_value = array ();  // This is use to provide direct link support.
        foreach ($_GET as $key => $value) {
            $right_value[$key] = $value;
        }
        make_report_right ($right_value);
    } elseif ($task == "make_management_left") {
        // The left side of the management page
        make_management_left ();
    } elseif ($task == "make_management_right") {
        // the right side of the management page
        make_management_right ();
    }
?>
