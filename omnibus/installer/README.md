Formation Data Systems Install
==============================

Overview
--------
This installation process is intended to be performed at a customer site. You
should have a package (tar.gz) named with a particular version - for example:

`install_2015.06.19-c6307bd.tar.gz`

This installation requires that you have a pre-defined inventory file which
describes the domain topology (host IP addresses) and configuration options
for this installation. Below is an example inventory file:

    [deploy-nodes]
    10.2.10.[75:78]

    [deploy-nodes:vars]
    fds_build=release
		# fds_default_nic must be accurate for all hosts - if it is different
		# on some hosts you may define this per-host in the above section
    fds_default_nic=eth0
    fds_deploy_local_deps=True
		# fds_om_host must match a host in the top section
    fds_om_host=10.2.10.75
    fds_metrics_enables=false
    fds_metricsdb_ip=127.0.0.1
		# This should be a user with sudo rights
    ansible_ssh_user=ubuntu
    # This will be prompted if not defined
    ansible_ssh_pass=ubuntu
    # This will be prompted if not defined
    ansible_sudo_pass=ubuntu
		# Enable use of sudo - disable if using root user
		ansible_sudo=true

Installation
------------

1. Extract tarball - if you are reading this you've probably already done so.

2. Run `sudo ./install.sh -i <inventory file>`. The inventory file may reside outside
the install directory - this is recommended.

3. Monitor the deployment process - some notes on the output you see:

- Some output will be red with an "ignored" line below it, these are
  expected failures and are probably OK.
- Output which is red but is not ignored should stop the deploy process,
  these are unexpected failures and indicate a problem.
- Output which is yellow in color means a change was made, it should be
  prefixed with [changed]
- Output which is green in color means no change was made, it should be
  prefixed with [ok]
- Sections which do not show output were probably skipped, this may be
  expected in many circumstances but if things aren't working right this
  information may be relevant.

Post Installation Troubleshooting
---------------------------------

If you run into problems during the installation make sure you do the following:

1. Test all networking connectivity between hosts - sometimes networking problems between nodes will cause the domain to fail to start.

2. If the domain fails to start - use the `fds-util` utility to evaluate the
state of the nodes and the domain. More than likely a service has crashed
or is not responding.

3. If a service crashes, make sure to collect a coroner tarball ON THAT HOST.
This is a crucial step to being able to diagnose problems. You can run a
coroner tarball on any host by running (as root):

`# /fds/sbin/coroner.py collect --refid <refid>`

The refid may be a Jira issue (FS-NNNN) or a string which uniquely identifies
this issue - if nothing else use the customer name. The tarball will already
contain the date - you do not need to include this in the refid.

4. If you must collect a coroner tarball on all hosts - there is a playbook
included in this installer to do that. Note that they will be very large.

