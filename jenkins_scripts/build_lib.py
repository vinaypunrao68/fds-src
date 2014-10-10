import subprocess
import os

def shell_retry(cmd, max_retry = 5):
    for i in xrange(max_retry):
        proc = subprocess.Popen(cmd, shell=True)
        (stdout, stderr) = proc.communicate()
        if proc.returncode != 0:
            print("*" * 30)
            print("Command ", cmd, " failed, retrying ", i, " of ", max_retry)
            print("*" * 30)
            print stderr
        else:
            print stdout
            print "Success"
            break

def create_lockfiles():
    for file in [ "/tmp/.devsetup.chk", os.environ["HOME"] + "/.ihaveaswapfile" ]:
        touch(file)

def touch(fname):
    try:
        os.utime(fname, None)
    except:
        open(fname, 'a').close()

