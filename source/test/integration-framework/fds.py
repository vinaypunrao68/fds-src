import os
import re
import subprocess
import sys

import config
import utils 

class FDS(object):
    
    def __init__(self):
        pass
    
    def check_status(self):
        cmd = config.FDS_CMD % (config.FDS_TOOLS, "status")
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
        # skip [WARN]
        proc.stdout.readline()
        # look for interesting data in remaining
        for line in proc.stdout:
            # terminate after processing the [INFO] section
            if line.startswith('[INFO] :  redis'):
                self.status['redis'] = True
                for line in proc.stdout:
                    pass
                break
            # get the process status
            name, remaining = line.split(' ', 1)
            if 'Not running' in remaining:
                return False
            # print "%s starts with '%s'" % (line.strip(), name)
        proc.wait()
        return True

    def stop_single_node(self):
        cmd = config.FDS_CMD % (config.FDS_TOOLS, "stop")
        try:
            subprocess.call([cmd], shell=True)
            if self.check_status() is not False:
                raise Exception("FDS failed to stop")
        except Exception, e:
            utils.log.exception(e)
            sys.exit(1)
    
    def start_single_node(self):
        cmd = config.FDS_CMD % (config.FDS_TOOLS, "cleanstart")
        try:
            subprocess.call([cmd], shell=True)
            if not self.check_status():
                raise Exception("FDs failed to start")
        except Exception, e:
            utils.log.exception(e)
            sys.exit(1)

if __name__ == "__main__":
    fds = FDS()
    print fds.check_status()
    fds.start_single_node()