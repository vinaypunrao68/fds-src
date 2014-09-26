<?php
echo ("<html><body>");
if (!empty ($_GET['pdu_name'])) {
	exec ("ssh root@192.168.17.3 \"python /opt/qa/autotest/lab/pwrCycle.py {$_GET['pdu_port']} {$_GET['pdu_name']}\"", $result);
	if (!empty ($result)) {
		print_r ($result);
	}
}
unset ($result);
if (!empty ($_GET['pdu2_name'])) {
	exec ("ssh root@192.168.17.3 \"python /opt/qa/autotest/lab/pwrCycle.py {$_GET['pdu2_port']} {$_GET['pdu2_name']}\"", $result);
	if (!empty ($result)) {
		print_r ($result);
	}
}
echo ("</body></html>");
?>
