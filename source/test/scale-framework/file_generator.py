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
        assert size >= 1
        assert quantity >= 1
        assert unit.upper() in self.units

        self.size = size
        self.quantity = quantity
        self.unit = unit.upper()
        self.root_path = os.path.join(os.getcwd(), config.RANDOM_DATA)
        utils.create_dir(self.root_path)

    def get_files(self):
        '''
        return a list of all the files present in the random data directory
        '''
        return os.listdir(self.root_path)

    def move_files(self, dst):
        files = self.get_files()
        for f in files:
            path = os.path.join(self.root_path, f)
            shutil.copy(path, dst)

    def sudo_copy_files(self, dst):
        files = self.get_files()
        for f in files:
            path = os.path.join(self.root_path, f)
            utils.execute_cmd("sudo cp %s %s" % (path, dst))

    def generate_files(self):
        '''
        Given the size, quantity and type (K, M or G), produce that number of
        files, storing them at the random_data directory.
        '''
        os.chdir(self.root_path)
        for i in xrange(0, self.quantity):
            current_size = "%s%s" % (self.size, self.unit)
            fname = "file_%s_%s" % (i, current_size)
            cmd = config.DD % (fname, current_size)
            subprocess.call([cmd], shell=True)
        os.chdir(os.path.normpath(os.getcwd() + os.sep + os.pardir))

    def clean_up(self):
        '''
        if the API user decided to delete the entire random_data directory
        '''
        if os.path.exists(self.root_path):
            shutil.rmtree(self.root_path)

if __name__ == "__main__":
    fgen = FileGenerator(10, 1, 'M')
    fgen.generate_files()
    path = os.path.join(os.getcwd(), "tests")
    fgen.move_files(path)
    fgen.clean_up()
