#!/usr/bin/python
import os, sys, re
import time
import Pyro4
import paramiko
import SocketServer
import multiprocessing
import threading
import Queue
import tempfile
import shutil
import requests
import counter_server
import shlex
import is_fds_up
sys.path.append('../fdslib')
sys.path.append('../fdslib/pyfdsp')
from SvcHandle import SvcMap
import perf_framework_utils as utils

class Monitors():
    def __init__(self, outdir, options):
        self.options = options
        self.outdir = outdir 
        self.cmds = {}
        self.infos = {}
        for n in options.nodes:
            self.cmds[n] = []

    def compute_monitors_cmds(self, agent_pid_map):
        for n, m in agent_pid_map.iteritems():
            monitors = ["collectl -P --all",
                        "iostat -kxdp -d 1 -k"]
            top_cmd = "top -b "
            for a, p in agent_pid_map[n].iteritems():
                top_cmd += "-p" + str(p) + " "
            monitors.append(top_cmd)
            self.options.logger.debug(monitors)
            self.cmds[n] = monitors

    def run(self):
        self.infos = self.start()

    def start(self):
        infos = {}
        for s in self.options.nodes:
            rex = utils.RemoteExecutor(s, self.options)
            infos[rex] = {}
            for cmd in self.cmds[s]:
                infos[rex][cmd] = {"pid" : rex.execute(cmd), "node" : s, "cmd" : cmd}
        return infos

    def terminate(self):
        for rex, infos in self.infos.items():
            for cmd, info in infos.items(): 
                node = info['node']
                pid = info['pid']
                directory = self.outdir + "/" + node + "/" + cmd[:3]
                if not os.path.exists(directory):
                    os.makedirs(directory)
                rex.terminate(pid, directory)

class CounterServerPull:
    def __init__(self, outdir, options, opt_outfile =  None):
        self.options = options
        self.opt_outfile = opt_outfile
        self.outdir = outdir
        self.stop = threading.Event()
        if opt_outfile != None:
            self.datafname = opt_outfile + ".counters"
            self.datafile = os.open(self.datafname, os.O_RDWR|os.O_CREAT)
            self.javafname = opt_outfile + ".java_counters"
            self.javafile = os.open(self.javafname, os.O_RDWR|os.O_CREAT)
        else:
            self.datafile, self.datafname = tempfile.mkstemp(prefix = "counters")
            self.javafile, self.javafname = tempfile.mkstemp(prefix = "javacounters")
        task_args = ("counter_server", self.datafile, self.stop)
        # FIXME: move this in its own function
        self.thread = threading.Thread(target = self._task, args = task_args)
        self.thread.start()

    def _get_counters(self):
        counters = []
        tstamp = long(time.time())
        for node in self.options.nodes:
            ip = self.options.nodes[node]
            port=7020
            try:
                svc_map = SvcMap(ip, port)
            except:
                # self.options.logger.debug("Failed to get svc_map, sleeping")
                time.sleep(60)
                return {}
            svclist = svc_map.list()
            for e in svclist:
                svcid = e[0]
                svc = e[1]
                ip = e[3]
                if svc != "om" and svc != "None" and re.match("10\.\d+\.\d+\.\d+", ip):
                    try:
                        cntrs = svc_map.client(svcid).getCounters('*')
                    except:
                        # self.options.logger.debug("Cannot get counters for " + svc)
                        cntr = {}
                    for c, v in cntrs.iteritems():
                        line = node + " " + svc + " " + c + " " + str(v) + " " + str(tstamp)
                        counters.append(line)
        return counters

    def _get_java_counters(self):
        address = "http://%s:8000/diagnostic/async/stats" % (self.options.nodes[self.options.main_node])
        r = requests.get(address)
        # assert r.status_code == requests.codes.ok, "Java counters get failed " + str(r.status_code) + " " + str(r.text)
        if r.status_code == requests.codes.ok:
            return "tstamp = " + str(time.time()) + "\n" + r.text + "\n"
        else:
            return "tstamp = "+ str(time.time()) + "\n"

    def _task(self, name, datafile, stop):
        # self.options.logger.debug("starting counter task " + name)
        # pull counters
        time.sleep(2)
        while stop.isSet() == False:
            time.sleep(self.options.counter_pull_rate)
            counters = self._get_counters()
            # write to file
            for e in counters:
                os.write(datafile, e + "\n")
            if self.options.java_counters:
                java_counters = self._get_java_counters()
                os.write(self.javafile, java_counters)

    def terminate(self):
        self.stop.set()
        time.sleep(self.options.counter_pull_rate + 1)
        if self.opt_outfile != None:
            return
        # terminate udp server
        #datafile, datafname = self.queue.get()
        directory = self.outdir
        if not os.path.exists(directory):
            os.makedirs(directory)
        os.close(self.datafile)
        shutil.move(self.datafname, directory + "/counters.dat")
        os.chmod(directory + "/counters.dat", 777)
        if self.options.java_counters == True:
            os.close(self.javafile)
            shutil.move(self.javafname, directory + "/java_counters.dat")
            os.chmod(directory + "/java_counters.dat", 777)


class AgentsPidMap:
    def __init__(self, options):
        self.options = options
        self.pid_map = {}
        for n in options.nodes:
            self.pid_map[n] = {}

    def compute_pid_map(self):
        self.pid_map = is_fds_up.get_pid_table(self.options.nodes.keys())
        self.options.logger.debug("pidmap:" + str(self.pid_map))

    def get_map(self):
        return self.pid_map

    def dump(self, outfile):
        of = open(outfile + "/pidmap","w")
        for n, m in self.pid_map.iteritems():
            for k, v in m.iteritems():
                of.write(n + ":" + k + ":" + str(v) + "\n")
        of.close()

class FdsCluster():
    def __init__(self, options):
        self.options = options
        self.fds_directory = options.fds_directory
        self.pidmap = AgentsPidMap(self.options)
        self.local_test = self.options.myip == self.options.test_node or self.options.test_node == None

    def get_pidmap(self):
        return self.pidmap.get_map()

    def restart(self):
        self.options.logger.info("starting FDS")
        if self.options.no_fds_start:
            time.sleep(1)
        else:
            cwd = os.getcwd()
            os.chdir(self.fds_directory + "/ansible")
            output = utils.loc_exec("./scripts/deploy_fds.sh " + self.options.main_node + " local")
            self.options.logger.debug(output)
            os.chdir(cwd)
            time.sleep(60)
        self.pidmap.compute_pid_map()

    def init_test_once(self, test_name, nvols):
        self.options.logger.info("initializing test for the first time:" + test_name)
        if test_name == "tgen" or test_name == "amprobe" or test_name == "tgen_java":
            for i in range(nvols):
                requests.put("http://%s:8000/volume%d" % (self.options.nodes[self.options.main_node], i))
        if test_name == "smoke-test":
            pass
        elif test_name == "tgen":
            self._init_tgen_once()
        elif test_name == "amprobe":
            self._init_amprobe_once()
        elif test_name == "tgen_java":
            self._init_tgen_java_once()
        elif test_name == "fio":
            self._init_fio_once()
        else:
            assert False, "Test unknown: " + test_name

    def init_test(self, test_name, outdir, test_args = None):
        self.options.logger.info("initializing test:" + test_name)
        self.outdir = outdir
        self.pidmap.dump(self.outdir)
        if not os.path.exists(self.outdir):
            os.makedirs(self.outdir)
        if test_name == "smoke-test":
            pass
        elif test_name == "tgen":
            self._init_tgen(test_args)
        elif test_name == "amprobe":
            self._init_amprobe(test_args)
        elif test_name == "tgen_java":
            self._init_tgen_java(test_args)
        elif test_name == "fio":
            self._init_fio(test_args)
        else:
            assert False, "Test unknown: " + test_name

    def run_test(self, test_name, test_args = None):
        self.options.logger.info("starting test: " + test_name)
        if test_name == "smoke-test":
            output = self._run_smoke_test()
        elif test_name == "tgen":
            output = self._run_tgen(test_args)
        elif test_name == "amprobe":
            output = self._run_amprobe(test_args)
        elif test_name == "tgen_java":
            output = self._run_tgen_java(test_args)
        elif test_name == "fio":
            output = self._run_fio(test_args)
        else:
            assert False, "Test unknown: " + test_name    
        outfile = open(self.outdir + "/test.out", "w")
        outfile.write(output)
        outfile.close()

    def _run_smoke_test(self):
        assert False, "Obsolete"
        return ""

    def _init_tgen_once(self):
        if self.local_test == True:
            shutil.copyfile(self.fds_directory + "/source/test/traffic_gen.py", "/root/traffic_gen.py")
            utils.loc_exec("rm -f /tmp/fdstrgen*")
        else:
            utils.transfer_file(self.options.test_node, self.fds_directory + "/source/test/traffic_gen.py", "/root/traffic_gen.py", "put")
            utils.ssh_exec(self.options.test_node, "rm -f /tmp/fdstrgen*")

    def _init_tgen(self, args):
        pass

    def _run_tgen(self, test_args):
        cmd = "python3 /root/traffic_gen.py -t %d -n %d -T %s -s %d -F %d -v %d -u -N %s" % (
                                                                            test_args["threads"],
                                                                            test_args["nreqs"],
                                                                            test_args["type"],
                                                                            test_args["fsize"],
                                                                            test_args["nfiles"],
                                                                            test_args["nvols"],
                                                                            self.options.nodes[self.options.main_node])
        self.options.logger.debug("Test: " + cmd)
        if self.local_test == True:
            output = utils.loc_exec(cmd)
        else:
            self.options.logger.debug("starting test remotely on node " + self.options.test_node)
            output = utils.ssh_exec(self.options.test_node, cmd)
        self.options.logger.debug("--> " + output)
        return output

    def _init_amprobe_once(self):
        pass

    def _init_amprobe(self, test_args):
        self._init_tgen(test_args)
        # FIXME: maybe redundant
        shutil.copyfile(self.fds_directory + "/source/test/traffic_gen.py", "/root/traffic_gen.py")
        utils.loc_exec("rm -f /tmp/fdstrgen*")
        # need to upload objects
        cmd = "python " + self.fds_directory + "/source/test/traffic_gen.py -t 1 -n 10000 -T PUT -s %d -F %d -v %d -u -N %s" % (test_args["fsize"], test_args["nfiles"], test_args["nvols"], self.options.nodes[self.options.main_node])
        output = utils.loc_exec(cmd)
        cmd = "python " + self.fds_directory + "/source/test/perf/gen_json.py -n %d -s %d -v %d > /tmp/.am-get.json" % (test_args["nreqs"], test_args["fsize"], test_args["nvols"])
        output = utils.loc_exec(cmd)
        of = open("/tmp/.am-get.json", "w")
        of.write(output)
        of.close()

    def _init_tgen_java_once(self):
        curr = os.getcwd()
        os.chdir("%s/source/Build/linux-x86_64.release" % self.fds_directory)
        cmd = "tar czvf java_tools.tgz tools lib/java"
        output = utils.loc_exec(cmd)
        shutil.copyfile(self.fds_directory + "/source/Build/linux-x86_64.release/java_tools.tgz", "/root/java_tools.tgz")
        utils.transfer_file(self.options.test_node, self.fds_directory + "/source/Build/linux-x86_64.release/java_tools.tgz" , "/root/java_tools.tgz", "put")
        os.chdir(curr)
        utils.ssh_exec(self.options.test_node, "tar xzvf java_tools.tgz")

    def _init_tgen_java(self, args):
        pass

    def _run_tgen_java(self, test_args):
        # good configurations seems 4 instances and 50 outs/conns
        def task(node, queue):
            cmd = "pushd /root/tools && ./trafficgen --n_reqs %d --n_files %d --outstanding_reqs %d --test_type %s --object_size %d --hostname %s --n_conns %d && popd" % ( test_args["nreqs"], 
                                                        test_args["nfiles"], 
                                                        test_args["outstanding"], 
                                                        test_args["type"], 
                                                        test_args["fsize"], 
                                                        self.options.nodes[self.options.main_node],
                                                        test_args["outstanding"] 
                                                        )
            output = utils.ssh_exec(node, cmd)
            queue.put(output)
        
        # nodes = ["10.1.10.222", "10.1.10.221"]
        nodes = [self.options.test_node]
        outs = test_args["outstanding"]
        threads = []
        queue = Queue.Queue()
        for n in nodes:
            for i in  range(test_args["threads"]):
                t = threading.Thread(target=task, args=(n, queue))
                t.start()
                threads.append(t)
        for t in threads:
            t.join()

        output = ""
        while not queue.empty():
            e = queue.get()
            output += e + "\n"
            queue.task_done()
        return output

    def _run_amprobe(self, test_args):
        cmd = "curl -v -X PUT -T /tmp/.am-get.json http://%s:8080" % (self.options.nodes[self.options.main_node])
        output = utils.loc_exec(cmd)
        time.sleep(30) # AM probe will keep processing stuff for a while
        cmd = "cat /fds/var/logs/am.*|grep CRITICAL"
        output = utils.ssh_exec(self.options.main_node, cmd)
        return output

    def _init_fio_once(self):
        # please use fdscli to create volumes..
        cmd = 'bash -c "pushd /fds/sbin && ./fdsconsole.py volume create volume1 --vol-type block --blk-dev-size 10737418240 --max-obj-size 4096  && popd"'
        output = utils.loc_exec(cmd)
        time.sleep(5)
        if self.local_test == True:
            cmd = "python %s/source/cinder/nbdadm.py attach localhost volume1" % self.options.fds_directory
            output = utils.loc_exec(cmd)
        else:
            shutil.copyfile(self.fds_directory + "/source/cinder/nbdadm.py", "/root/nbdadm.py")
            cmd = "python nbdadm.py attach %s volume1" % self.options.main_node
            output = utils.ssh_exec(self.options.test_node, cmd)
        time.sleep(5)
        self.nbdvolume = output.rstrip("\n")
        self.options.logger.debug("nbd: " + self.nbdvolume)
        cmd = 'bash -c "pushd /fds/sbin && ./fdsconsole.py volume modify volume1 --minimum 0 --maximum 0 --priority 1 && popd"'
        output = utils.loc_exec(cmd)
        time.sleep(5)

    def _init_fio(self, args):
        pass

    def _run_fio(self, test_args):
        disk = self.nbdvolume
        bs = test_args["bs"]
        numjobs = test_args["numjobs"]
        jobname = test_args["fio_jobname"]
        rw = test_args["fio_type"]
        iodepth = test_args["iodepth"]
        runtime = test_args["runtime"]
        options =   "--name=" + jobname + " " + \
                    "--rw=" + rw + " " + \
                    "--filename=" + disk + " " + \
                    "--bs=" + str(bs) + " " + \
                    "--numjobs=" + str(numjobs) + " " + \
                    "--iodepth=" + str(iodepth) + " " + \
                    "--ioengine=libaio --direct=1 --size=16m --time_based --minimal "
        if runtime > 0:            
            options += " --runtime=" + str(runtime) + " "
        cmd = "fio " + options
        if self.local_test == True:
            output = utils.loc_exec(cmd)
        else:
            output = utils.ssh_exec(self.options.test_node, cmd)
        return output

class CommandInjector:
    def __init__(self, options, commands):
        self.options = options
        self.local = self.options.myip == self.options.nodes[self.options.main_node]
        assert self.local == True, "Must be local for CommandInjector"
        self.commands = commands
        self.proc = multiprocessing.Process(target = self._task, args = (self.commands,))

    def start(self):
        self.proc.start()

    def _task(self, commands):
        for cmd in commands:
            m = re.match("^sleep\s+=\s+(\d+)", cmd)
            if m is not None:
                delay = int(m.group(1))
                self.options.logger.info("CmdInj: sleep for " + str(delay) + " time: " + str(time.time()))
                time.sleep(delay)
            else:
                self.options.logger.debug("CmdInj: " + cmd + " time: " + str(time.time()))
                utils.loc_exec(cmd)

    def terminate(self):
        self.proc.join()

