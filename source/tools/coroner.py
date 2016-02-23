#!/usr/bin/env python
#
# This tool collects stats about a running or failed system and produces a
# bundle that may be used to diagnose failures or problems in the environment.
#
#  ******** IMPORTANT IF YOU ARE MODIFYING THIS TOOL ***********
# This tools is intended to be non-destructive and should not perform any tasks
# which could impact the functionality of a running system unless the
# operator is made VERY aware of any impact running this may have. Any option
# added which could impact the running of a system MUST be disabled by default
# and SHOULD provide a warning when enabled that requires acknowledgement
#
# TODO:
# - Need a disk space check before gathering data - if < 5G or so of free
#   space in the destination disk we shouldn't proceed or we should not
#   collect large artifacts (binaries, cores).

import os
import sys
import argparse
import time
import socket
import subprocess
import glob
import logging

class FDSCoroner(object):
    """ FDSCoroner - a general purpose system information collection class
        intended to be used for collection of data about a system"""

    def __init__(self, destdir='/tmp',
                 refid='noref',
                 fdsroot='/fds',
                 debug=False):
        self.check_uid() # Make sure user is uid 0
        self.destdir = destdir # Destination directory in which to create bag
        self.refid = refid # Reference ID used in naming bag
        self.debug = debug # Enable extra logging
        self.fdsroot = fdsroot # Where our fdsroot is
        self.report_content = {} # This contains details about what was found during collection
        self.bodybag = None
        self.logformat = "%(asctime)s:%(levelname)s:%(message)s"
        if self.debug:
            logging.basicConfig(format=self.logformat,level=logging.DEBUG)
        else:
            logging.basicConfig(format=self.logformat,level=logging.INFO)

    def check_uid(self, uid=0):
        """Exit if the current uid is not equal to uid (expected to be root)"""
        if not os.geteuid() == uid:
            sys.exit("\nThis script requires root access\n")

    # Generate a bag name to be used during collection
    def get_bag_name(self, date=None):
        """Return a time-based name to be used in setting up a new bag"""
        if date is None:
            date = time.strftime("%Y%m%d-%H%M%S")
        hostname = socket.gethostname()
        return("%s/fdscoroner_%s_%s_%s" % (self.destdir, date, hostname, self.refid))

    def make_dir(self, directory):
        """Generic directory creation method"""
        if not os.path.exists(directory):
            logging.debug("Creating directory %s" % directory)
            try:
                os.makedirs(directory)
            except OSError as e:
                logging.error("Failed to create directory " + directory)
                raise

    def get_dir_path(self, dirname):
        """returns the full path to the named directory including the bag path"""
        return "%s/%s" % (self.bodybag, dirname)

    # Prepare a directory where we can store information about the system
    def prep_bag(self):
        """This prepares the base collection directory by getting a unique
           time-based name which is where all further data is collected into"""
        bag_name = self.get_bag_name()
        if os.path.exists(bag_name):
            logging.error("%s already exists, cannot create bag" % bag_name)
            exit(1)

        logging.debug("Creating bag directory %s" % bag_name)
        os.makedirs(bag_name)
        self.bodybag = bag_name

    def prep_data_dir(self, dirname):
        """Prepare a named data directory for holding data. This method is
           used to create topical directories inside the main body of the
           collection directory"""
        directory = self.get_dir_path(dirname)
        self.make_dir(directory)
        return directory

    def collect_dir(self, directory, path):
        """This method allows collection of a particular directory and all
           of its contents as a tarball"""
        mydir = self.prep_data_dir(directory)
        if os.path.exists(path):
            # Strip any trailing /
            path = path.rstrip('/')
            basename = os.path.basename(path)
            path_base = os.path.dirname(path)

            logging.info("Collecting %s" % path)
            # If you change this command, fix the insert below so it is always before the "-cvhzf" entry
            cmd = ["/bin/tar", "-C", path_base, "--exclude-from", path_base + "/sbin/coroner.excludes", "-chzf",
                   mydir + '/' + basename + '.tar.gz', basename]
            subprocess.call(cmd)

    def collect_paths(self, directory, paths):
        """This method allows collection of all files within some path. This
           method DOES NOT RECURSE into directories, so use of glob patterns
           is suggested for collecting multiple files in a single path"""
        mydir = self.prep_data_dir(directory)
        for path in paths:
            for file_path in glob.glob(path):
                if os.path.exists(file_path):
                    logging.info("Copying %s to %s" % (file_path, mydir))
                    subprocess.call(['cp', file_path, mydir ])

    def collect_cmd(self, command, name=None):
        """This method allows collection of data using shell commands. Note
           that we intentionally do not set shell=True when calling subprocess
           to keep things secure, so chaining of commands may not work. If
           the name= value is passed in, this will be used as the filename
           for both stderr & stdout - each with their own suffix to identify
           them"""
        if name is None:
            name = command
            name.replace(" ", "_")
        # We run all cmd with nice -n 19 to reduce system impact
        nice = ['/usr/bin/nice', '-n', '19']
        parsed_command = nice + command.split()
        mydir = self.prep_data_dir('command-logs')
        logging.debug("Collecting cmd: %s" % command),
        sys.stdout.flush()
        p = subprocess.Popen(parsed_command,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (stdoutdata, stderrdata) = p.communicate()
        # Open files where we collect output for packaging
        mystdoutfile = open("%s/%s.out" % (mydir, name), 'w')
        mystderrfile = open("%s/%s.err" % (mydir, name), 'w')
        mystdoutfile.write(stdoutdata)
        mystdoutfile.write(stderrdata)
        # If something went wrong, let the operator know
        if p.returncode is not 0:
            logging.error(
                    "FAIL (exit %d) cmd: %s" % (p.returncode, command)
            )
            logging.error("Error output:")
            logging.error(stderrdata)
        else:
            logging.info("SUCCESS cmd: %s" % command)

    def compress_sparse_files(self, dirname):
        """Compress all files inside a given directory as individual files and
           treat them as sparse files to allow efficient decompression. This
           is exclusively used for core files right now"""
        mydir = self.get_dir_path(dirname)
        for filename in glob.glob("%s/*" % mydir):
            logging.debug("Compressing %s" % filename)
            cmd = ["/bin/tar", "--remove-files", "-C", mydir, "-Sczf",
                   filename + '.tar.gz', os.path.basename(filename)]
            subprocess.call(cmd)

    def collect_cores(self, paths):
        """Collect & Compress all core files from specified paths"""
        self.collect_paths('cores', paths)
        self.compress_sparse_files('cores')

    def collect_cli_dirs(self, dirlist):
        """Collect directories passed on the command line"""
        for entry in dirlist:
            (name, path) = entry.split(':')
            self.collect_dir(directory=name, path=path)

    def collect_cli_cmds(self, cmdlist):
        """Collect commands passed on the command line"""
        for entry in cmdlist:
            self.collect_cmd(command=entry)

    def close(self):
        """Tar & compress a bag and then remove original directory"""
        logging.info("Zipping up bag...")
        cmd = ["/bin/tar", "--remove-files", "-C", self.destdir, "-czf",
               self.bodybag + '.tar.gz',
               os.path.basename(self.bodybag)]
        subprocess.call(cmd)

    def report(self):
        """Print a coroners report detailing what was collected"""
        logging.info("Collection file: " + self.bodybag + ".tar.gz")

def run_collect(opts):
    """This is the main body of the data collection script. This sets up a new
       bag to be used for collection, runs all collection tasks, then tars up
       the collected data"""
    bodybag = FDSCoroner( destdir=opts['destdir'],
                          refid=opts['refid'],
                          fdsroot=opts['fdsroot'],
                          debug=opts['debug'])

    bodybag.prep_bag()
    # Collect up system logs
    bodybag.collect_paths(
        directory="system-logs",
        paths=["/var/log/kern*", "/var/log/syslog*"]
    )
    # Collect FDS artifacts
    if not opts['skip_binaries']:
        bodybag.collect_dir(directory="fds", path=bodybag.fdsroot + '/bin')
        bodybag.collect_dir(directory="fds", path=bodybag.fdsroot + '/sbin')

    bodybag.collect_dir(directory="fds", path=bodybag.fdsroot + '/etc')

    bodybag.collect_dir(directory="fds", path=bodybag.fdsroot + '/var')
    bodybag.collect_paths(
        directory="fds",
        paths=[ bodybag.fdsroot + '/dev/disk-map',
                bodybag.fdsroot + '/version-manifest.txt']
    )
    # Collect command output
    bodybag.collect_cmd(command='/bin/dmesg', name='dmesg')
    bodybag.collect_cmd(command='/usr/bin/lsof', name='lsof')
    bodybag.collect_cmd(command='/usr/bin/env', name='env')
    bodybag.collect_cmd(command='/bin/uname -a', name='uname')
    bodybag.collect_cmd(command='/usr/bin/dpkg -l', name='dpkg')
    bodybag.collect_cmd(command='/bin/df -a', name='df')
    bodybag.collect_cmd(command='/bin/mount', name='mount')
    bodybag.collect_cmd(command='/sbin/ifconfig -a', name='ifconfig')
    bodybag.collect_cmd(command='/bin/ps axfwwww', name='ps')
    bodybag.collect_cmd(command='/opt/fds-deps/embedded/bin/redis-cli keys *', name='redis-keys')
    bodybag.collect_cmd(command='/bin/netstat -nr', name='netstat_route')
    bodybag.collect_cmd(command='/bin/netstat -ni', name='netstat_interfaces')
    bodybag.collect_cmd(command='/bin/netstat -ns', name='netstat_stats')
    bodybag.collect_cmd(command='/sbin/fdisk -l', name='fdisk_list')
    bodybag.collect_cmd(command='/bin/ls -l %s/var/log/corefiles' % bodybag.fdsroot, name='ls_corefiles')
    if not opts['buildermode']:
        bodybag.collect_cmd(
            command='/opt/fds-deps/embedded/sbin/parted --list --script',
            name='parted'
        )
    bodybag.collect_cmd(command='/usr/bin/lshw', name='lshw')
    bodybag.collect_cmd(
        command='/usr/bin/find %s -type d -print -exec ls -al {} ;' % bodybag.fdsroot,
        name='fds_files'
    )
    if opts['checksum_files']:
        bodybag.collect_cmd(
            command="/usr/bin/find %s -type f -exec sha256sum {} ;" % bodybag.fdsroot,
            name='fds_shasums'
        )
    # Collect cores from all possible locations
    corepaths = [
       '%s/var/log/corefiles/*' % bodybag.fdsroot,
       '%s/var/cores/*' % bodybag.fdsroot,
       '%s/bin/*core*' % bodybag.fdsroot,
       '%s/bin/*.hprof' % bodybag.fdsroot,
       '%s/bin/*hs_err_pid*.log' % bodybag.fdsroot,
       '%s/var/log/*.hprof' % bodybag.fdsroot,
       '%s/var/log/*hs_err_pid*.log' % bodybag.fdsroot
    ]
    bodybag.collect_cores(corepaths)
    if opts['collect_dirs'] is not None:
        bodybag.collect_cli_dirs(opts['collect_dirs'])
    if opts['collect_cmds'] is not None:
        bodybag.collect_cli_cmds(opts['collect_cmds'])
    # Close up the bag
    bodybag.close()
    bodybag.report()

def main():

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest='sp_name', help='sub-command help')

    parser_collect = subparsers.add_parser(
            'collect',
            help='Collect information about the system & generate a support bundle')
    parser_collect.add_argument(
            '--destdir', dest="destdir", default='/tmp/',
            help='Destination directory to store support bundle')
    parser_collect.add_argument(
            '--refid', dest="refid", default='noref',
            help='Reference ID - either FS-NNNN or other reference')
    parser_collect.add_argument(
            '--buildermode', action='store_true', dest="buildermode", default=False,
            help='Use this when coroner is run in build environments')
    parser_collect.add_argument(
            '--skip-binaries', action='store_true', dest="skip_binaries", default=False,
            help='Use this to skip interacting with binaries (collection or otherwise)')
    parser_collect.add_argument(
            '--checksum-files', action='store_true', dest="checksum_files", default=False,
            help='Use this to checksum all files in /fdsroot')
    parser_collect.add_argument(
            '--fdsroot', dest="fdsroot", default='/fds',
            help='Location of FDS root directory - normally /fds')
    parser_collect.add_argument(
            '--include-fdsroot', action='store_true', default=False,
            help='Enable collection of actual /fds directory excluding some data')
    parser_collect.add_argument(
            '--collect-dirs', dest='collect_dirs', nargs='*', default=None,
            help='Collect additional directory, form "name:directory"')
    parser_collect.add_argument(
            '--collect-cmds', dest='collect_cmds', nargs='*', default=None,
            help='Collect additional command')
    parser_collect.set_defaults(func=run_collect)

    #parser.add_argument(
    #        '--url', default='http://localhost',
    #        help='url to run tests against')
    #parser.add_argument(
    #        '--fdsuser', dest="fdsuser", default='admin',
    #        help='define FDS user to use Tenant API for auth token')
    #parser.add_argument(
    #        '--fdspass', dest="fdspass", default='admin',
    #        help='define FDS pass to use Tenant API for auth token')
    parser.add_argument(
            '--debug', action="store_true", default=False,
            help='enable debugging')

    args = parser.parse_args()

    # Because argparse doesn't create an iterable arg list by default we
    # do this to allow checking options
    opts = vars(args)

    # This calls the function defined by the subparser that is called. So
    # if you call 'collect' above then this will call the function defined by
    # the func= in set_defaults
    args.func(opts)

if __name__ == '__main__':
    main()
