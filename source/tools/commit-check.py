#!/usr/env python

'''
Script will perform the following steps:
1. Call git status --porecelain
2. Parse the output, removing any tracked files
3. Check for a .git_status_prev file
   a. If .git_status_prev exists compare to git status output
   b. If git status has more untracked files than .git_status_prev
      alert the user
4. Write git status output to .git_status_prev
'''

import subprocess
import argparse
import sys
import os 
import stat

def do_check(top_level):
    
    outfile = os.path.join(top_level, '.git_status_prev')
    
    if outfile is None:
        top_level = subproces.check_output(['git', 
                                            'rev-parse', 
                                            '--show-toplevel'])
        outfile = os.path.join(top_level, '.git_status_prev')

    STATUS_CMD = ['git', 'status', '--porcelain']
    output = ''

    # Call git status
    try:
        output = subprocess.check_output(STATUS_CMD)
    except CalledProcessError, e:
        print "Error: {}".format(e)
        sys.exit(1)
    
    # Verify that there is some output
    if output != '':
        # First find only the untracked files
        output = output.split('\n')
        # Remove any empty lines
        output = filter(lambda x: len(x) > 0, output)
        # Remove any tracked files
        output = filter(lambda x: len(x) > 0 and x[:2] != ' M', output)
        output = set(output)
    else:
        print "No output found from git status!"
        sys.exit(1)

    # Open the output file to read/write
    try:
        fh = open(outfile, 'r')
    except IOError, e:
        # No previous file
        fh = None

    # If we had a previous, do the diff
    diff = []
    all_lines = set()
    if fh is not None:
        all_lines = set(fh.read().splitlines())
        # Do difference
        diff = output - all_lines
        
        if len(diff) > 0:
            # If diff is > 0 then there are new untracked changes
            # Ask if user would like to continue
            sys.stdin = open('/dev/tty')
            print "WARNING: There are {} new untracked changes!".format(len(diff))
            for line in diff:
                print line
            result = ''
            while result.lower() != 'y' or result.lower() != 'n':
                result = raw_input('Do you wish to proceed? [Y/n]: ')
                if result.lower() == '':
                    break
                if result.lower() == 'n':
                    print "STOPPING git commit!"
                    sys.exit(1)

        fh.close()

    # Do the writes
    fh = open(outfile, 'w+')
    fh.writelines(map(lambda x: x + '\n', list(output)))
    fh.close()

    
    sys.exit(0)

def do_setup(top_level):
    fname = os.path.join(top_level, '.git/hooks/pre-commit')
    fh = open(fname, 'w+')
    fh.write('python {}'.format(
        os.path.join(top_level, 'source/tools/commit-check.py')))
    fh.close()
    os.chmod(fname, (stat.S_IREAD | stat.S_IWRITE | stat.S_IEXEC))

if __name__ == "__main__":

    # We'll need this in either case
    top_level = subprocess.check_output(['git', 
                                         'rev-parse', 
                                         '--show-toplevel'])
    if '\n' in top_level:
        top_level = top_level.rstrip()

    parser = argparse.ArgumentParser(description='Check if there are new'
                                     ' untracked files in git.')
    parser.add_argument('--setup', action='store_true',
                        help='Setup git pre-commit hook '
                        'and setup initial .git_status_prev file.')

    args = parser.parse_args()
    if args.setup is not None and args.setup == True:
        do_setup(top_level)
    else:
        print "FIRED!"
        do_check(top_level)

