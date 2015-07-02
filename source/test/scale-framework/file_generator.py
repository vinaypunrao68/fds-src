import os
import sys
import shutil
import subprocess

import config
import utils

class FileGenerator(object):
    '''
    Produce a number of random files, using dd, given the size, quantity and the
    unit in KB, MB or GB.

    Attributes:
    -----------
    size: the size of the file. i.e. 2
    quantity: now many of those files we want to produce
    unit: if the size is of KB, MB or GB
    '''
    units = ('K', 'M', 'G')

    def __init__(self, size, quantity, unit):
        assert quantity >= 1
        assert unit.upper() in self.units
        self.size = size
        self.quantity = quantity
        self.unit = unit.upper()
        utils.create_dir(config.RANDOM_DATA)

    def get_files(self):
        '''
        return a list of all the files present in the random data directory
        '''
        return os.listdir(config.RANDOM_DATA)

    def generate_files(self):
        '''
        Given the size, quantity and type (K, M or G), produce that number of
        files, storing them at the random_data directory.
        '''
        os.chdir(config.RANDOM_DATA)
        for i in xrange(0, self.quantity):
            current_size = "%s%s" % (self.size, self.unit)
            fname = "file_%s_%s" % (i, current_size)
	    if int(self.size) == 0:
		cmd = 'touch %s' %fname
	    else:
            	cmd = config.DD % (fname, current_size)
            subprocess.call([cmd], shell=True)

	os.chdir('../') #need to get back to scale-framework directory

    def clean_up(self):
        '''
        if the API user decided to delete the entire random_data directory
        '''
        shutil.rmtree(config.RANDOM_DATA)
