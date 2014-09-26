#!/usr/bin/env python

import os, sys
import telnetlib
import re

def main():
    type = None
    if len (sys.argv) > 5:
        print "Too many arguments."
        sys.exit (0)

    if len (sys.argv) < 2:
        print "Enter the socket number"
        print "./pwrCycle.py <socket #> [powerStripName] [on|off|reboot]"
        sys.exit (0)
    try:
        cycle = int (sys.argv[1])
    except:
        print "Socket # must be a number"
        sys.exit (0)
    
    pser = "pdu1-rack-1"
    
    if len (sys.argv) > 2:
        pser = sys.argv[2]

    # For dual power supply
    if len (sys.argv) > 3:
        if len (sys.argv) == 4:
            if (sys.argv[3] in ("on", "off")):
                type = sys.argv[3]
        else:
            cycle2 = int (sys.argv[3])
            pser2 = sys.argv[4]
    if len (sys.argv) == 6:
        if (sys.argv[5] in ("on", "off")):
            type = sys.argv[5]

    print "Attempt to %s socket %s on %s"%(type, cycle, pser)
    Pcycler(cycle, pser, type)
    if len (sys.argv) > 4:
        print "Attempt to %s socket %s on %s"%(type, cycle2, pser2)
        Pcycler (cycle2, pser2, type)
    print "Power cycle complete."

class Pcycler(object):
    def __init__(self, cycle, pser, type=None):
        self.cycle = cycle
#        if pser.find (".lab") == -1:
#            self.pser = pser+".lab"
#        else:
        # type = on, off, reboot
        self.type=type
        self.pser = pser
        return self.__cycle()

    def __te(self, rc):
        if rc[0] == -1:
            print rc[2]

    def __cycle(self):
        login= re.compile (r"User Name : ")
        passwd = re.compile (r"Password  : ")
        prompt = re.compile (r"(?m)^> ")
        outletCCRE = re.compile (r"(?m)(\d)- Outlet Control/Configuration")
        outletmgntRE = re.compile (r"(?m)(\d)- Outlet Management")
        outletList = [outletmgntRE, outletCCRE, prompt]
        x = telnetlib.Telnet (self.pser)
        rc = x.expect ([login], 3)
        self.__te(rc)
        x.write ("apc\r")  # login
        rc = x.expect ([passwd], 3)
        self.__te(rc)
        x.write ("apc\r")  # Password
        rc = x.expect ([prompt], 3)
        self.__te(rc)
        x.write ("1\r") # Device Manager
        rc = x.expect (outletList, 3)
        self.__te(rc)
        if rc[0] == 0:
            # We are need outlet management
            # Since we got a line before the prompt
            # we need to consume the following prompt
            rc = x.expect ([prompt], 3)
            x.write ("2\r") # Device Manager
            # Need to expect another prompt
            rc = x.expect ([prompt], 3)
            x.write ("1\r") # Outlet Control/Config
        elif rc[0] == 1:
            # We see the outlet control
            # Since we got a line before the prompt
            # we need to consume the following prompt
            rc = x.expect ([prompt], 3)
            x.write ("3\r") # Outlet Control/Config
        elif rc[0] == 2:
            # We got a prompt, which meant we don't 
            # know what is in the previous menu
            print rc[2]
            # We gone to somewhere we don't want to.
            sys.exit (1)
        # After we select Control/config, there will
        # be a long list requiring a MORE...
        x.write ("\r")  # MORE
        rc = x.expect ([prompt], 3)
        self.__te(rc)
        x.write ("%s\r"%self.cycle)  # Select outlet
        rc = x.expect ([prompt], 3)
        self.__te(rc)
        x.write ("1\r") # Control Outlet
        rc = x.expect ([prompt], 3)
        self.__te(rc)
        if self.type == "on":
            x.write ("1\r")  # Power On
        elif self.type == "off":
            x.write ("2\r")  # Power Off
        else:
            x.write ("6\r")  # Reboot Delay
        rc = x.expect ([re.compile (r"Enter \'YES\' to continue or \<ENTER\> to cancel")], 3)
        self.__te(rc)
        x.write ("YES\r")
        rc = x.expect ([re.compile (r"Press \<ENTER\> to continue\.\.\.")], 3)
        self.__te(rc)
        x.write ("\r")
        x.write ("\r")
        x.close ()

if __name__ == "__main__":
    main()
