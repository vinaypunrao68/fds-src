##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: test_report_parser.py                                                #
##############################################################################

# parse XML report files and create the appropriate data structures
# so that information can be stored to the database
import xmltodict
import os
import glob

def parse_xml_file(filepath):
    '''
    Given a xml file, parse it and return as a OrderedDict type.
    User can access elements using a dict type.

    Example:
    -------
    doc['mydocument']['@has'] # == u'an attribute'
    doc['mydocument']['and']['many'] # == [u'elements', u'more elements']
    doc['mydocument']['plus']['@a'] # == u'complex'
    doc['mydocument']['plus']['#text'] # == u'element as well'

    Arguments:
    ---------
    filepath: str
        the path to the .xml file

    Returns:
    --------
    OrderedDict: a dictionary with all input fields
    '''
    doc = None
    with open(filepath, 'r') as fd:
        doc = xmltodict.parse(fd.read())

    return doc

def read_test_reports(directory):
    '''
    Given a directory which contains *xml files, filter the files in that directory
    and only select those with *.xml at the end.

    Arguments:
    ----------
    directory: the path to the directory

    Returns:
    set: a set of all the *.xml files
    '''
    xml_files = os.path.join(directory, "*.xml")
    reports = set(glob.glob(xml_files))
    return reports
