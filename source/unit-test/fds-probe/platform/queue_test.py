#!/usr/bin/env python

import random
import pycurl
from string import Template

class PushTemplate:

    def __init__(self):
        
        self.json_templ = Template('''
{
    "Queue-Workload": {
    	"Queue-Push": {
		"queue-name": "$t_name",
    		"value": $t_value
    	}
    }
}
''')

    def gen(self):
        value = random.randint(0, 1000)
        return self.json_templ.substitute(t_name = "test-queue", t_value = value)
        

class PopTemplate:
    
    def __init__(self):
        self.json_templ = Template('''
{
    "Queue-Workload": {
    	"Queue-Pop": {
    		"queue-name": "$t_name"
    	 }
    }
}
''')

    def gen(self):
        return self.json_templ.substitute(t_name = "test-queue")

def make_pycurl(url='http://localhost:8000/abc',
                verbose=0):
    '''
    Creates a pycurl object with correct settings for the probe
    '''
    c = pycurl.Curl()
    c.setopt(pycurl.URL, url)
    c.setopt(pycurl.POST, 1)
    c.setopt(pycurl.VERBOSE, verbose)
    
    return c

def do_queue_test():
    # Do pycurl creation
    pc = make_pycurl("http://localhost:8000/abc/")

    opts = [PushTemplate(), PopTemplate()]
    
    for i in range(1000):
        x = random.sample(opts, 1)
        x = x[0]
        pc.setopt(pycurl.POSTFIELDS, x.gen())
        pc.perform()
    

if __name__ == "__main__":

    do_queue_test()
