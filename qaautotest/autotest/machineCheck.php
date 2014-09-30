<?php
/*
 * Create on March 10, 2008
 */
/* * * * * * * * * * * * * * * * *
 * NOTE: Must have the ssh permission set for this to work.
 *
 */
// This page will define the frames for managing the 
// checking out of lab machines from the lab pool.
// This is only for manual user interaction.

	// Set the require files
	require 'funclib.inc';  // This is the function group for the autotest page, just need the db stuff from here.
	require 'constants.inc'; // For the colors
//	require 'machineCheck.inc';  // This is the function groups for machine checkout

	// This page is a stand alone page with its own frames.
	// Basic task is a layout of the standard page.
	$task = "layout";
	if ($_GET[frame]=="layout") {$task="layout";}
	// Main header will provide status infomation
	// TODO: Need to implement this page
	elseif ($_GET[frame]=="main_header") {$task="main_header";}
	// The left hand side is the checkout page.
	elseif ($_GET[frame]=="checkout_page") {$task="checkout_page";}
	// The Free machine list is on the right hand side, it should
	// be updated every few seconds
	elseif ($_GET[frame]=="machine_list") {$task="machine_list";}
	// start a session
	session_start ();

	// If we want to reset, reset the session
	if (!empty ($_POST['Reset'])) {
		session_destroy ();
		session_start ();
		$task = "layout";
	}

	if (! $_SESSION[running]) {
		$_SESSION[running]=1;
		$_SESSION['changed'] = false;
	}
?>

<?php
/* This part is the functions I plan to plance in machineCheck.inc */
function get_user_list () {
	$query = "select email from user_list order by email";
	connect_db ();
	$result = mysql_query ($query);
	$users = array ();
	if (!$result) {
		die ("Invalid Query: ".mysql_error());
	}
	while ($row = mysql_fetch_assoc ($result)) {
		$users[] = $row['email'];
	}
	mysql_free_result ($result);
	return $users;
}


function get_OS_type_list () {
	$query = "select id, os_type from harness_os_list order by os_type";
	connect_db ();
	$result = mysql_query ($query);
	$osType = array ();
	if (!$result) {
		die ("Invalid Query: ".mysql_error());
	}
	while ($row = mysql_fetch_assoc ($result)) {
		$osType[$row['id']] = $row['os_type'];
	}
	mysql_free_result ($result);
	return $osType;
}


function get_free_machine_list () {
	$query = "select hname from harness_list where reserve_user=\"\" order by hname";
    connect_db ();
	$result = mysql_query ($query);
	$freeList = array ();
    if (!$result) {
        die ("Invalid Query: ".mysql_error());
    }
    while ($row = mysql_fetch_assoc ($result)) {
        $freeList[] = $row['hname'];
    }
    mysql_free_result ($result);
    return $freeList;
}


function machine_check_layout ($padding) {
	global $COLORS;
	$hd = 50+$padding;  // Header padding
	?>
	<html><head><title>Machine User Control</title>
	<?php javascript_header (); // from funclib.inc ?>
	<?php style_header (); // CSS from funclib.inc ?>
	<frameset rows="<?=$hd?>,*,1">
		<frame src=<?=$_SERVER['PHP_SELF']?>?frame=main_header name=main_header scrolling="no" />
		<frameset cols="30%, *">
			<frame src=<?=$_SERVER['PHP_SELF']?>?frame=checkout_page name=checkout_page />
			<frame src=<?=$_SERVER['PHP_SELF']?>?frame=machine_list name=machine_list />
		</frameset>
		<frame src="" name=junk_frame scrolling="no" />
	</frameset>
	</head>
	</html>
	<?php
}

function make_main_header () {
	?>
	<html><head><title>Main Header</title>
	<style type="text/css">
	body {
		font: 16pt Verdana,sans-serif;
		color: darkblue;
	}
	</style>
	</head>
	<body>
	<?php
	connect_db ();
	$query = "select count(*) \"count\" from harness_list where reserve_user = \"\"";
	$result = mysql_query ($query);
	$row = mysql_fetch_assoc ($result);
	$fcount = $row['count'];
	mysql_free_result ($result);
	$query = "select count(*) \"count\" from harness_list";
	$result = mysql_query ($query);
    $row = mysql_fetch_assoc ($result);
	$tcount = $row['count'];
	mysql_free_result ($result);
	$ucount = $tcount-$fcount;
	echo ("{$tcount} Machines total, {$ucount} in use, {$fcount} free.");
	?>
	</body>
	</html>
	<?php
}

function make_checkout_page () {
	$users = get_user_list ();
	$osType = get_OS_type_list ();
    $freeList = get_free_machine_list ();
	if (!empty ($_POST['user_select'])) {
		$bline = "<body onload=window.parent.machine_list.location=\"{$_SERVER['PHP_SELF']}?frame=machine_list&user={$_POST['user_select']}\">\n";
	} else {
		$bline = "<body>\n";
	}
	?>
	<html><head><title>Check Out Page</title>
	<?php style_header(); ?>
	</head>
	<?php echo $bline; ?>
	<table border=0 cellpadding=0 width=100%>
		<tr><th align=left>I am</th>
		<td>
		<form action="<?=$_SERVER['PHP_SELF']?>?frame=checkout_page" method="post" name="userName" target="checkout_page">
			<select name="user_select" onchange=submit();>
			<?php
				echo "<option value=\"Noone\">Noone</option>\n";
				foreach ($users as $user) {
					$selected = "";
					if ($user == $_POST['user_select']) {$selected = "selected";}
					echo ("<option value=\"{$user}\" $selected>{$user}</option>\n");
				}
			?>
		</select>
		</form>
		</td>
		</tr>
		<?php /*
			On submit of this form change the right side page and update the machines own by current
		user, but first check if I am Noone.
		*/ ?>
		<form action="<?=$_SERVER['PHP_SELF']?>?frame=machine_list" name="pickMachine" method="post" target="machine_list">
		<?php
		if (!empty ($_POST['user_select'])) {
		// If we have selected a user name, echo this.
			echo ("<input type=\"hidden\" name=\"user\" value=\"{$_POST['user_select']}\" />");
		} else {
			echo ("<input type=\"hidden\" name=\"user\" value=\"Noone\" />");
		}
		?>
		<tr><th align=left colspan=10>Give Me Machines</th></tr>
		<tr><th align=right>OS Type</th>
		<td><select name="osType">
			<?php
			foreach ($osType as $osv => $osName) {
				echo ("<option value=\"{$osv}\">{$osName}</option>\n");
			}
			?>
		</td></tr>
		<tr><th align=right>Machine count</th><td><input type="text" name="machineCount" size="3" value="0"></td></tr>
		<tr><th colspan=10>Or select from free machine list</th></tr>
		<tr><th align=right>Free Machine List</th>
		<td>
        <select name=machineSelect[] multiple size=20>
		<?php 
			foreach ($freeList as $machine) {
				echo ("<option value=\"{$machine}\" >{$machine}</option>\n");
			}
		?>
		</select>
		</td></tr>
		<tr><td colspan=10><input type="submit" value="Give Me Machine(s)"/></td></tr>
		</form>
		<tr><td align=left colspan=10>Select from list supercedes machine count select.</td></tr>
		<tr><td align=left colspan=10>Checkout over 72 hours will be flagged.</td></tr>
		<tr><th align=left colspan=10>NOTE: If machine failed to reboot to chosen OS, ssh in and do a manual reboot on that machine.</th></tr>
	</table>
	</body>
	</html>
	<?php
}

function make_machine_list_page () {
	global $COLORS;
	?>
	<html><head><title>Machine List</title>
	<?php style_header(); ?>
	</head>
	<?php
	echo "<body onload=window.parent.main_header.location=\"{$_SERVER['PHP_SELF']}?frame=main_header\">\n";
	// Get the osType names to id array
	$osType = get_OS_type_list ();
	// Check if there is a user defined, if it is we will list the machines
	// checkout by the user.
	if (!empty ($_GET['user'])) {
		$user = $_GET['user'];
		$where = " where reserve_user=\"{$user}\" ";
	} else {
		$user = "";
		$where = " ";
	}
	// Check if it is sent by POST
	if (!empty ($_POST['user'])) {
		if ($_POST['user'] == "Noone") {
			$user = "";
			$where = " ";
		} else {
			$user = $_POST['user'];
			$where = " where reserve_user=\"{$user}\" ";
		}
    }
	// Connect to the DB.
	connect_db (); // Use use the same presistent connection as autotest
	// If we got a machineCount value, osType value, we need to assign a machine.
	if (!empty ($_POST['osType']) and (!empty ($_POST['machineCount']) or !empty ($_POST['machineSelect']))) {
		if (empty ($_POST['user']) or ($_POST['user'] == 'Noone')) {
			echo ("</br>Missing user name or user is 'Noone', please select your userID. </br>");
			return;
		} elseif (!empty ($_POST['machineSelect'])) {
//			print_r ($_POST['machineSelect']);
			$mList = "";
			foreach ($_POST['machineSelect'] as $selMachine) {
				if ($mList == "") {
					$mList .= "\"{$selMachine}\"";
				} else {
					$mList .= ",\"{$selMachine}\"";
				}
			}
			echo ("mList = {$mList}");
			// We have a list of machines to checkout, so check if these machines are free.
			$query = "select * from harness_list where hname in ({$mList})";
			$result = mysql_query ($query);
			if (!$result) {
				die ("Invalid Query: ".mysql_error ());
			}
			$bList = "";
			$macList = array ();
			while ($row = mysql_fetch_assoc ($result)) {
				if ($row['reserve_user'] != "") {
					$bList .= "{$row['hname']} ";
				}
				$macList [] = $row['MAC_ADDR'];
			}
			// Free the mysql result obj
			mysql_free_result ($result);
			if ($bList != "") {
				echo ("Several machines ({$bList}) have just been checkout by another person, please refresh the page and try again.");
				return;
			}
			// Now assign the machines to one person.
			$ctime = time ();
			$query = "update harness_list set reserve_user=\"{$user}\",reserve_time={$ctime},currentOS={$_POST['osType']} where hname in ({$mList})";
			echo $query;
			$result = mysql_query ($query);
			if ($result == FALSE) {
				die ("Invalid Query :".mysql_error());
			}
		} elseif ($_POST['machineCount'] > 0) {
			// We are good, lets assign the machine.
//			echo ("I am {$_POST['user']}, I got os type {$_POST['osType']} for {$_POST['machineCount']} machines.");
			// First search for any machine that is free.
			$query = "select * from harness_list where reserve_user=\"\" and supportedOS like \"%{$_POST['osType']}%\"";
			$result = mysql_query ($query);
			if (!$result) {
				die ("Invalid Query: ".mysql_error ());
			}
			$tempMachineList = array ();
			$tempMachineCount = 0;
			// Next pick the a number of machine = to machineCount and change their currentOS to osType and mark them as checkout by user.
			while ($row = mysql_fetch_assoc ($result)) {
				$tempMachineList[] = $row;
				$tempMachineCount += 1;
				if ($tempMachineCount == $_POST['machineCount']) {
					break;
				}
			}
			// Free the $result array
			mysql_free_result ($result);
			// If we don't have enough machine, output a message that we don't have enough machine to complete request.
			if ($tempMachineCount != $_POST['machineCount']) {
				// We don't have enough machine to complete the request.
				$oname = $osType[(int)$_POST['osType']];
				echo "We don't have enough machine to complete the request.  Requested {$_POST['machineCount']} machine with {$oname} OS.";
				return;
			}
			// TODO: Now check if we have selected a machine List, if so we ignore the machine count and check if the machines selected are
            //    in the free list, if not, return false, someone already check out xxx machine.
			// Now create the query to update the database, since we are only allowing one set of updates compile a id list first.
			$idList = "";
			$macList = array ();
			foreach ($tempMachineList as $machine) {
				$macList[] = $machine['MAC_ADDR'];
				if ($idList == "") {
					$idList .= $machine['id'];
				} else {
					$idList .= ",".$machine['id'];
				}
			}
			$ctime = time ();
			$updateString = "update harness_list set reserve_user=\"{$user}\",reserve_time={$ctime},currentOS={$_POST['osType']} where id in ({$idList})";
			// Update the database
			$result = mysql_query ($updateString);
			if ($result == FALSE) {
				die ("Invalid Query :".mysql_error());
			}
			// Now check the status of each of these machine
			// TODO
		}
		// Now exec the external function.
		foreach ($macList as $mac) {
			// Call the lease.py script.
			unset ($result);
			exec ("ssh root@192.168.17.3 \"python /opt/qa/autotest/lab/lease.py {$mac}\"", $result);
			// Since I am kicking it into a screen, there will be no return values.
			if (! empty ($result)) {
				echo ("<br />CLI output MAC_ADDR {$mac},</ br> result:<br />\n");
				print_r ($result);
				echo ("\n<br />\n");
			}
		}

	} elseif (!empty ($_POST['retMachine'])) {
		// We are doing return checkedout machine back into the system.
		$machineList = "";
		foreach ($_POST['retMachine'] as $mac) {
			if ($machineList == "") {
				$machineList .= "\"".$mac."\"";
			} else {
				$machineList .= ",\"".$mac."\"";
			}
		}
		$query = "update harness_list set reserve_user=\"\", reserve_time=0 where MAC_ADDR in ({$machineList})";
		echo ($query);
		$result = mysql_query ($query);
		if ($result == FALSE) {
			die ("Invalid Query :".mysql_error());
        }
//		// After returning the machine to the pool, power down the machine.
//		$query = "select pdu_name, pdu_port, pdu2_name, pdu2_port from harness_list where MAC_ADDR in ({$machineList})";
//		$result = mysql_query ($query);
//		if ($result == FALSE) {
//			die ("Invalid Query:".mysql_error());
//		}
//		while ($row = mysql_fetch_assoc ($result)) {
//			print_r ($row);
//			exec ("ssh root@192.168.17.3 \"python /opt/qa/autotest/lab/pwrCycle.py {$row['pdu_port']} {$row['pdu_name']} off\"", $rcResult);
//			if (array_key_exists ("pdu2_name", $row)) {
//				exec ("ssh root@192.168.17.3 \"python /opt/qa/autotest/lab/pwrCycle.py {$row['pdu2_port']} {$row['pdu2_name']} off\"", $rcResult);
//			}
//		}
//		mysql_free_result ($result);
	}
	// Now that we are done adding any new machine, create a list of machine assign to a user.
	// The query for building the table.
	$query = "select hl.id id, hl.primary_ip primary_ip, hl.hname hname, hl.reserve_user reserve_user, hl.reserve_time rtime, hol.os_type currentOS, ".
		"hl.MAC_ADDR MAC_ADDR, hl.SerialCon SerialTerm, hl.SerialCon_port SCport, hl.pdu_name pdu_name, hl.pdu_port pdu_port, hl.pdu2_name pdu2_name, hl.pdu2_port pdu2_port ".
		"from harness_list hl ".
		"left join harness_os_list hol ".
		"on hl.currentOS=hol.id{$where}order by hl.hname";//, hl.reserve_user, hl.currentOS, hl.id";
	$result = mysql_query ($query);
	if (!$result) {
		die ("Invalid Query: ".mysql_error());
	}
	echo ("<table border=1>");
	// Check if we have a user, if there is one, have a title and do the delete column too
	if ($user != "Noone" and $user != "") {
		echo ("<tr><th colspan=15>Machines assigned to {$user}</th></tr>");
		// If user we'll create a for a delete list.
//		echo ("<form onsubmit=\"confirmCheck ()\" action=\"{$_SERVER['PHP_SELF']}?frame=machine_list&user={$user}\" name=\"returnMachineList\" method=\"post\" target=\"machine_list\">\n");
		echo ("<form onsubmit=\"return confirm('Return Check Machines?')\" action=\"{$_SERVER['PHP_SELF']}?frame=machine_list&user={$user}\" name=\"returnMachineList\" method=\"post\" target=\"machine_list\">\n");
	} else {
		// If no $user or $user == Noone, set it to empty to make it easier later.
		$user = "";
	}
	echo ("<tr>");
	if ($user != "") {
		// Add a new column call return, this is the check box for return values.
		echo ("<th></th>");
	}
	echo ("<th>IP</th><th>MAC_ADDR</th><th>Host Name</th><th>User</th><th>Current OS</th><th>SerialCon</th><th>Serial Port</th><th>PDU1</th><th>PDU2</th><th>Cycle Pwr</th><th>Lease time</th></tr>");
	if ($user != "") {
		// Add the check all checkbox
		echo "<tr><td>\n";
		echo "<input type=\"checkbox\" name=\"checkall\"";
		echo "onclick=\"window.parent.checkAll(window.parent.machine_list.document.returnMachineList, window.parent.machine_list.document.returnMachineList.checkall);\"/>";
		echo "</td><td colspan=15>Check All</td></tr>";
	}
	$ctime = time ();
	$count = 0;
	while ($row = mysql_fetch_assoc ($result)) {
		if (($count % 2) == 0) {
            $bgc = $COLORS['LIGHTBLUE'];
        } else {
            $bgc = $COLORS['WHITE'];
        }
		$count += 1;
		echo ("<tr bgcolor={$bgc}>");
		if ($user != "") {
			echo "<td><input type=\"checkbox\" name=\"retMachine[]\" value=\"{$row['MAC_ADDR']}\" /></td>\n";
		}
		echo ("<td>{$row['primary_ip']}</td>\n");
		echo ("<td>{$row['MAC_ADDR']}</td>\n");
		echo ("<td>{$row['hname']}</td>\n");
		echo ("<td>{$row['reserve_user']}</td>\n");
		echo ("<td>{$row['currentOS']}</td>\n");
		echo ("<td>{$row['SerialTerm']}</td>\n");
		echo ("<td>{$row['SCport']}</td>\n");
		echo ("<td>{$row['pdu_name']}:{$row['pdu_port']}</td>\n");
		echo ("<td>{$row['pdu2_name']}:{$row['pdu2_port']}</td>\n");
		if ($user != "") {
			echo ("<td><a href=\"powerCycle.php?pdu_name={$row['pdu_name']}&pdu_port={$row['pdu_port']}&pdu2_name={$row['pdu2_name']}&pdu2_port={$row['pdu2_port']}\" target=\"junk_frame\">Cycle</a></td>\n");
		} else {
			echo ("<td></td>");
		}
		if ($row['rtime'] > 0) {
			$dtime = (int)($ctime-$row['rtime'])/60/60;
			if ($dtime > 3*24) {
				$bgc = $COLORS['PORANGE'];
			}
			printf ("<td bgcolor={$bgc}>%01.2f hr(s)</td>\n",$dtime);
		} else {
			echo ("<td>Free</td>\n");
		}
		echo ("</tr>");
	}
	if ($user != "") {
		echo "<tr><td colspan=15>\n";
        echo "<input type=\"submit\" name=\"retMachineButton\" value=\"Return checked\"/></td></tr>\n";
		echo "</form>";
	}
	echo ("</table>");
	mysql_free_result ($result);
	?>
	</body>
	</html>
	<?php
}
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<?php
// Main layout
	if ($task == "layout") {
		$padding = 0;
		$page_id = 1;
		$b = $_SERVER ['HTTP_USER_AGENT'];

		if (strpos($b, "MSIE") > 0) {
			$padding = 15;
		}
		// Remeber the page posted.
		// TODO: Make it general, this is probably not needed.
		if (!empty($_POST['page_display'])) {
            $page_id = $_POST['page_display'];
        } elseif (!empty ($_GET['page_display'])) {
            $page_id = $_GET['page_display'];
        }
		// There is only one layout
		machine_check_layout ($padding);
	} elseif ($task == "main_header") {
		// TODO: Implement the following function
		make_main_header ();
	} elseif ($task == "checkout_page") {
		// TODO: Implement the following function
		make_checkout_page ();
	} elseif ($task == "machine_list") {
		// TODO: Implement the following function
		make_machine_list_page ();
	}
?>
