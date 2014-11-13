#!/usr/bin/python
import json
from optparse import OptionParser
import tabulate
import pprint

class TestList():
    def __init__(self):
        self.tests = None

    def dump_json(self):
        return json.dumps(self.tests)

    def get_tests(self):
        return self.tests

    def create_tests_fio(self):
        # test template
        template = {
            "test_type" : "fio",
            "nvols" : 4,
            "disk" : None,
            "bs" : 4096,
            "numjobs" : 1,
            "fio_jobname" : "seq_readwrite",
            "fio_type" : "readwrite",
            "iodepth" : 32,
            "runtime" : 120,
            "injector" : None
        }

        tests = []
        ############### Test definition ############
        #test_types = ["read", "write", "readwrite", "randwrite", "randread"]
        test = dict(template)
        test["bs"] = 4096
        test["fio_jobname"] = "write"
        test["fio_type"] = "write"
        test["numjobs"] = 1
        test["runtime"] = 0
        tests.append(test)
        test_types = ["randread"]
        for d in [4, 16, 32, 64, 128]:
            for j in [1, 2, 10]:
                for bs in [4096]:
                    for test_type in test_types:
                        test = dict(template)
                        test["bs"] = bs
                        test["fio_jobname"] = test_type
                        test["fio_type"] = test_type
                        test["numjobs"] = j
                        test["iodepth"] = d
                        test["runtime"] = 60
                        tests.append(test)
        self.tests = tests

    def create_tests_s3(self):
        # test template
        template = {
            "test_type" : "tgen",
            "nvols" : 4,
            "threads" : 1,
            "nreqs" : 10000,
            "type" : "PUT",
            "fsize" : 4096,
            "nfiles" : 1000,
            "injector" :None
        }

        # tests = []
        # #for size in [x*256*1024 for x in range(1:9)]:
        # for size in [4*1024]:
        #     test = dict(template)
        #     test["test_type"] = "amprobe"
        #     # test["type"] = "GET"
        #     test["nreqs"] = 1000000
        #     test["nfiles"] = 1000
        #     test["fsize"] = size
        #
        #     test["nvols"] = 4
        #     test["threads"] = 1
        #     tests.append(test)

        tests = []
        size = 4096   # 4096
        test = dict(template)
        test["type"] = "PUT"
        test["nreqs"] = 10000  # 100000
        test["nfiles"] = 1000  # 10000
        test["nvols"] = 1
        test["threads"] = 20
        test["fsize"] = size
        test["injector"] = None
        tests.append(test)
        for size in [4096]:
            for nvols in [1]:
                # for th in [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 35, 40]:
                for th in range(1,51):
                    #for outs in [2, 4, 10, 20, 50]:
                    for outs in [2]:
                        #for th in [4]:
                        #for th in [21]:
                        # for th in [30]:
                            total_reqs = max([th * 1000000 / 50, 100000])
                            test = dict(template)
                            test["type"] = "PUT"
                            test["nreqs"] = 100000
                            test["nfiles"] = 1000
                            test["nvols"] = nvols
                            test["threads"] = th
                            test["fsize"] = size
                            #tests.append(test)
                            test = dict(template)
                            # test["test_type"] = "tgen_java"
                            test["type"] = "GET"
                            test["nreqs"] = total_reqs
                            test["nfiles"] = 1000
                            test["nvols"] = nvols
                            test["threads"] = th
                            test["fsize"] = size
                            test["outstanding"] = outs
                            tests.append(test)

#            for nvols in [1]:#[1, 2]: # [1, 2, 3, 4]:
#                # for th in [5, 10, 15, 20, 25, 30, 35]:
#                for th in [30]:
#                    test = dict(template)
#                    test["type"] = "GET"
#                    test["nreqs"] = 100000
#                    test["nfiles"] = 1000
#                    test["nvols"] = nvols
#                    test["threads"] = th
#                    test["fsize"] = size
#                    #test["injector"] = [
#                #    #options.local_fds_root + "/source/Build/linux-x86_64.debug/bin/fdscli --volume-modify volume1 -s 1000000  -g 3200 -m 5000 -r 10",
#                #    "sleep = 20",
#                #    "/home/monchier/FDS/source/Build/linux-x86_64.debug/bin/fdscli --volume-modify volume1 -s 1000000  -g 3000 -m 5000 -r 10",
#                #    "sleep = 10",
#                #    "/home/monchier/FDS/source/Build/linux-x86_64.debug/bin/fdscli --volume-modify volume1 -s 1000000  -g 200 -m 1000 -r 10",
#                #    "sleep = 5"
#                #    "/home/monchier/FDS/source/Build/linux-x86_64.debug/bin/fdscli --volume-modify volume0 -s 1000000  -g 2800 -m 5000 -r 10",
#                #
#                #    ]
#                tests.append(test)
        self.tests = tests

def main():
    parser = OptionParser()
    parser.add_option("-j", "--json-file", dest = "json_file",
                      default = None,
                      help = "Dump output to json file")
    (options, args) = parser.parse_args()

    tl = TestList()
    # tl.create_tests_s3()
    tl.create_tests_fio()
    print "Tests:"
    pp = pprint.PrettyPrinter(indent=4)
    for e in tl.get_tests():
        pp.pprint(e)
    print "Number of tests:", len(tl.get_tests())
    json_string = tl.dump_json()
    if options.json_file != None:
        with open(options.json_file, "w") as _f:
            _f.write(json_string)

if __name__ == "__main__":
    main()
