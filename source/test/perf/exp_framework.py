#!/usr/bin/python
import os, sys, re
import time
import Pyro4
import paramiko
from optparse import OptionParser
import SocketServer
import multiprocessing
import tempfile
import shutil
import datetime
import requests
import counter_server
import subprocess
import shlex
#import threading as th
#from Queue import *

def get_node_from_ip(nodes, node_ip):
    for k, v in nodes.iteritems():
        if v == node_ip:
            return k
    return None

def test_to_str(test):
    test_str = ""
    for k, v in test.iteritems():
        test_str += str(k) + ":" + str(v) + "."
    test_str += "test"
    return test_str

def get_results_dir(base = "results", test = None):
    directory0 = base + "_" + str(datetime.date.today()) + "_" + test_to_str(test)
    directory = directory0
    cntr = 1
    while os.path.exists(directory):
        directory = directory0  + "_" + str(cntr)
        cntr += 1
    os.makedirs(directory)
    return directory

def transfer_file(node, local_f, remote_f, mode):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(node, username='root', password='passwd')
    ftp = ssh.open_sftp()
    if mode == "get":
        ftp.get(remote_f, local_f)
    elif mode == "put":
        ftp.put(local_f, remote_f)
    else:
        raise "Error: transfer mode can be put or get"
    ftp.close()
    ssh.close()

def ssh_exec(node, cmd):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(node, username='root', password='passwd')
    (stdin, stdout, stderr) = ssh.exec_command(cmd)
    output = stdout.read()
    ssh.close()
    return output

def start_nameserver():
    pass

class RemoteExecutor:
    def __init__(self, node, options):
        print "Starting remote executor on ", node
        self.options = options
        self.node = node
        ns_table = Pyro4.locateNS(host = options.ns_ip, port = options.ns_port).list()
        if options.local == False and (options.force_cmd_server or not node + ".executor" in ns_table):
            print "Forcing start of cmd-server.py on", self.node
            assert options.ns_ip is not None and options.ns_port is not None
            self.start_cmd_server(self.node)
            time.sleep(5)
        if options.local == False:
            self.executor = Pyro4.Proxy(ns_table[node + ".executor"])
            try:
                self.executor.ping()
            except:
                assert options.ns_ip is not None and options.ns_port is not None
                self.start_cmd_server(self.node)
                time.sleep(5)
        else:
            self.proc_map = {}
    def start_cmd_server(self, nodename):
        assert self.options.local is False
        #ssh_exec(node_ip,"ps aux|grep cmd-server.py| grep -v grep|wc -l")
        print "bringing up cmd server on", nodename
        node_ip = self.options.nodes[nodename]
        assert os.path.exists(self.options.local_fds_root + "/source/test/perf/cmd-server.py")
        transfer_file(node_ip, self.options.local_fds_root + "/source/test/perf/cmd-server.py", "/root/cmd-server.py", "put")
        ssh_exec(node_ip, "python cmd-server.py --node-name %s --name-server-ip %s --name-server-port %d --server-ip %s 2>&1 > cmd-server.log &"
                    % (nodename, self.options.ns_ip, self.options.ns_port, node_ip,))
        print "done"

    def execute(self, cmd):
        if self.options.local == False:
            pid = self.executor.exec_cmd(cmd)
            print "Executing - cmd:", cmd, "pid:", pid
            return pid
        else:
            args = shlex.split(cmd)
            fileout, fnameout = tempfile.mkstemp(prefix = "out")
            fileerr, fnameerr = tempfile.mkstemp(prefix = "err")
            p = subprocess.Popen(args, stdout = fileout, stderr = fileerr)
            pid = p.pid
            print fnameout, fnameerr
            self.proc_map[pid] = {"proc" : p, "fileout" : (fileout, fnameout), "fileerr" : (fileerr, fnameerr) }
            return pid

    def terminate(self, pid, directory):    
        print "Terminating pid:", pid
        if self.options.local == False:
            fnamestdout, fnamestderr = self.executor.terminate(pid)
            transfer_file(self.options.nodes[self.node], directory + "/stdout",fnamestdout,"get")
            transfer_file(self.options.nodes[self.node], directory + "/stderr",fnamestderr,"get")

        else:
            info = self.proc_map[pid]
            info["proc"].kill()
            os.close(info["fileout"][0])
            os.close(info["fileerr"][0])
            fnamestdout, fnamestderr = info["fileout"][1], info["fileerr"][1]
            shutil.copyfile(fnamestdout, directory + "/stdout")
            shutil.copyfile(fnamestderr, directory + "/stderr")

    def execute_simple(self, cmd):
        if self.options.local == False:
            return self.executor.exec_cmd_simple(cmd)
        else:
            args = shlex.split(cmd)
            output = subprocess.check_output(args, stderr=subprocess.STDOUT)
            return output

class Monitors():
    def __init__(self, outdir, options):
        self.options = options
        self.outdir = outdir 
        self.cmds = {}
        self.infos = {}
        self.ns_ip = options.ns_ip
        self.ns_port = options.ns_port
        for n in options.nodes:
            self.cmds[n] = []
    def run(self):
        self.infos = self.start()
    def compute_monitors_cmds(self, agent_pid_map):
        print agent_pid_map
        for n, m in agent_pid_map.iteritems():
            monitors = ["collectl -P --all",
                        "iostat -p -d 1 -k"]
            top_cmd = "top -b "
            for a, p in agent_pid_map[n].iteritems():
                top_cmd += "-p" + str(p) + " "
            monitors.append(top_cmd)
            print monitors
            self.cmds[n] = monitors
    def start(self):
        infos = {}
        for s in self.options.nodes:
            rex = RemoteExecutor(s, self.options)
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

class CounterServer:
    def __init__(self, outdir):
        # start udp server
        self.outdir = outdir
        self.queue = multiprocessing.Queue()
        task_args = ("counter_server", self.queue)
        self.proc = multiprocessing.Process(target = self._task, args = task_args)
        self.proc.start()
    def _task(self, name, queue):
        print "starting udp server ", name
        datafile, datafname = tempfile.mkstemp(prefix = "counters")
        queue.put((datafile, datafname))
        HOST, PORT = "10.1.10.102", 2003
        server = SocketServer.UDPServer((HOST, PORT), counter_server.FdsUDPHandler, datafile)
        server.datafile = datafile
        server.serve_forever()
    def terminate(self):
        # terminate udp server
        self.proc.terminate()
        assert not self.queue.empty() and self.queue.qsize() == 1
        datafile, datafname =  self.queue.get()
        directory = self.outdir
        if not os.path.exists(directory):
            os.makedirs(directory)
        shutil.copyfile(datafname, directory + "/counters.dat")    
        #os.close(datafile)

class AgentsPidMap:   # FIXME: this needs to be per node!!!
    def __init__(self, options):
        self.options = options
        self.pid_map = {}
        for n in options.nodes:
            self.pid_map[n] = {}
    def compute_pid_map(self):
        if self.options.local == True:
            output = subprocess.check_output(self.options.remote_fds_root + "/source/tools/fds status  | egrep '(pm|om|dm|sm|am)'| awk '{print $1, $4}'", shell = True)  # FIXME: path
            for l in output.split('\n'):
                if len(l.split()) > 0:
                    t1, t2 = l.split()
                    m = re.match(".*(pm|om|dm|sm|am).*", t1)
                    if m is not None:
                        agent = m.group(1)
                        pid = int(t2)
                        self.pid_map[self.options.main_node][agent] = pid
        else:
            cmd = self.options.local_fds_root + "/source/test/fds-tool.py -f " + self.options.local_fds_root + "/source/fdstool.cfg -s"
            output = subprocess.check_output(shlex.split(cmd))
            print output
            agents_strings ={ "om" : "grep com.formationds.om.Main",
                "am" : "grep com.formationds.am.Main",
                "pm" : "plat",
                "sm" : "StorMgr",
                "dm" : "DataMgr"}
            for a, s in agents_strings.iteritems():
                match_string = "\[(\d+\.\d+\.\d+\.\d+)\]\s+\w+\s+(\d+).*" + s + ".*"
                m = re.match(match_string, output)
                assert m is not None
                node_ip = m.group(1)
                pid = int(m.group(2))
                self.pid_map[get_node_from_ip(node_ip)][a] = pid
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
        # self.remote_fds_root = "/home/monchier/FDS/dev_counters"
        self.remote_fds_root = options.remote_fds_root
        self.local_fds_root = options.local_fds_root
        self.rex = RemoteExecutor(self.options.main_node, self.options)
        self.pidmap = AgentsPidMap(self.options)
    def get_pidmap(self):
        return self.pidmap.get_map()
    def start(self):
        print "starting FDS"
        if self.options.no_fds_start:
            #output = self.rex.execute_simple(self.remote_fds_root + "/source/tools/fds status")
            #print output
            self.pidmap.compute_pid_map()
            time.sleep(1)
        elif self.options.single_node == True:
            output = self.rex.execute_simple(self.remote_fds_root + "/source/tools/fds cleanstart")
            print output
            self.pidmap.compute_pid_map()
            time.sleep(10)
        else:
            # output = self.rex.execute_simple("/fds/sbin/fds-tool.py -f /fds/sbin/fdstool.cfg -c -d -u")
            cmd = self.local_fds_root + "/source/test/fds-tool.py -f " + self.local_fds_root + "/source/fdstool.cfg -c -d -u"
            output = subprocess.check_output(shlex.split(cmd))
            print output
            self.pidmap.compute_pid_map()
            time.sleep(10)
    def stop(self):
        pass
    def init_test(self, test_name, outdir, test_args = None):
        print "initializing test:", test_name
        self.outdir = outdir
        self.pidmap.dump(self.outdir)
        if not os.path.exists(self.outdir):
            os.makedirs(self.outdir)
        if test_name == "smoke-test":
            pass
        elif test_name == "tgen":
            self._init_tgen(test_args)
        else:
            assert False, "Test unknown: " + test_name    
    def run_test(self, test_name, test_args = None):
        print "starting test:", test_name
        if test_name == "smoke-test":
            output = self._run_smoke_test()
        elif test_name == "tgen":
            output = self._run_tgen(test_args)
        else:
            assert False, "Test unknown: " + test_name    
        outfile = open(self.outdir + "/test.out", "w")
        outfile.write(output)
        outfile.close()
    def _run_smoke_test(self):
        output = self.rex.execute_simple(self.remote_fds_root + "/source/test/fds-primitive-smoke.py --up=false --down=false")
        print output
        return output
    def _run_tgen(self, test_args):
        cmd = "python /root/traffic_gen.py -t %d -n %d -T %s -s %d -F %d -v %d -u -N %s" % (test_args["threads"],
                                                                                   test_args["nreqs"],
                                                                                   test_args["type"],
                                                                                   test_args["fsize"],
                                                                                   test_args["nfiles"],
                                                                                   test_args["nvols"],
                                                                                   self.options.local_node)
        # FIXME: hate ssh here
        print cmd
        if self.options.test_node != None:
            output = ssh_exec(self.options.test_node, cmd)#[1].read()
        else:
            output = self.rex.execute_simple(cmd)
        return output
    def _init_tgen(self, args):
        if self.options.test_node != None:
            assert self.options.local == True
            transfer_file(self.options.test_node, self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py", "put")
            ssh_exec(self.options.test_node, "rm -f /tmp/fdstrgen*")
        else:
            if self.options.local == True:
                shutil.copyfile(self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py")
            else:
                transfer_file(self.options.main_node, self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py", "put")
            self.rex.execute_simple("rm -f /tmp/fdstrgen*")
        #self.rex.execute_simple("pkill 'collectl|iostat|top'")
        #output = self.rex.execute_simple("/usr/bin/curl -v -X PUT http://localhost:8000/volume")
        for i in range(args["nvols"]):
            requests.put("http://%s:8000/volume%d" % (self.options.nodes[self.options.main_node], i));
