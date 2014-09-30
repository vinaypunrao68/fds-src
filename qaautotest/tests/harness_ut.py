#!/usr/bin/env python

import logging

def main():
    params = {"log_level": logging.DEBUG}
    run_tests = []
    run_tests.append(PassingTestExample(params).run())
    run_tests.append(FailingTestExample(params).run())
    print(run_tests)


class PassingTestExample(object):
    """ Example of a test case that will pass."""
    def __init__(self, parameters):
        self.log = logging.getLogger("PassingTestExample")
        self.log.setLevel(parameters["log_level"])

    def run(self):
        self.log.info("Running the PassingTestExample() test case.")
        self.log.info("Test case passed.")
        return True


class FailingTestExample(object):
    """ Example of a test case that will fail."""
    def __init__(self, parameters):
        self.log = logging.getLogger("FailingTestExample")
        self.log.setLevel(parameters["log_level"])

    def run(self):
        self.log.info("Running the FailingTestExample() test case.")
        self.log.error("Test case failed.")
        return False


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG,
                        format='%(asctime)s %(name)s %(levelname)-8s %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')
    main()
