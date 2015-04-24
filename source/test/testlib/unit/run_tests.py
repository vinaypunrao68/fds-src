import logging
import unittest
import glob
import os

def get_test_modules():
    files = []
    for file in glob.glob("*.py"):
        name = file.replace(".py", "")
        if name == "__init__" or name == "run_tests":
            continue
        else:
            files.append(name)
            
    return files
        
def run_tests():
    test_modules = get_test_modules()

    suite = unittest.TestSuite()
    
    for test in test_modules:
        try:
            # If the module defines a suite() function, call it to get the suite.
            mod = __import__(test, globals(), locals(), ['suite'])
            suitefn = getattr(mod, 'suite')
            suite.addTest(suitefn())
        except (ImportError, AttributeError):
            # else, just load all the test cases from the module.
            suite.addTest(unittest.defaultTestLoader.loadTestsFromName(test))
    unittest.TextTestRunner().run(suite)
    
if __name__ == "__main__":
    run_tests()

