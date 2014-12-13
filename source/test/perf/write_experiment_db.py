#!/usr/bin/python
import os,re,sys
import dataset
import time
from optparse import OptionParser

def main():
    parser = OptionParser()
    parser.add_option("-f", "--database_file", dest = "db_file", help = "Database file")
    parser.add_option("-d", "--results-directory", dest = "directory", help = "Results directory")
    parser.add_option("-s", "--description", dest = "description", help = "Description")
    parser.add_option("-r", "--read", dest = "read", default = False, action="store_true", help = "Read db")

    (options, args) = parser.parse_args()

    db = dataset.connect('sqlite:///%s' % options.db_file)
    table = {}
    table["directory"] = options.directory
    table["description"] = options.description
    table["date"] = time.localtime()
    db["experiment_directories"].insert(table)

    if options.read:
        for e in db["experiment_directories"].all():
            print e

if __name__ == "__main__":
    main()
