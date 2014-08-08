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
import simple_experiment
#import threading as th
#from Queue import *

nodes = {
    #"node1" : "10.1.10.16",
    #"node2" : "10.1.10.17",
    #"node3" : "10.1.10.18"
    "tiefighter" : "10.1.10.102"
}

def unescape_string(input):
    ansi_escape = re.compile(r'\x1b[^m]*m')
    return ansi_escape.sub('', input)

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
    ssh.close()
    return (stdin, stdout, stderr)



def start_nameserver():
    pass

class RemoteExecutor:
    def __init__(self, node, options):
        self.options = options
        self.node = node
        self.executor = Pyro4.Proxy("PYRONAME:%s.executor" % node)
        if options.local == False:
            try:
                self.executor.ping()
            except:
                assert options.ns_ip is not None and options.ns_port is not None
                self.start_cmd_server(self.node)
                time.sleep(5)
        else:
            self.proc_map = {}
  # TODO: move into RemoteExecutor
    def start_cmd_server(self, nodename):
        assert self.options.local is False
        #ssh_exec(node_ip,"ps aux|grep cmd-server.py| grep -v grep|wc -l")
        print "bringing up cmd server on", nodename
        node_ip = nodes[nodename]
        assert os.path.exists("cmd-server.py")
        transfer_file(node_ip, "cmd-server.py", "/root/cmd-server.py", "put")
        ssh_exec(node_ip, "python cmd-server.py --node-name %s --name-server-ip %s --name-server-port %d --server-ip %s 2>&1 > cmd-server.log &"
                    % (nodename, self.options.ns_ip, self.options.ns_port, node_ip,))

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
            transfer_file(nodes[self.node], directory + "/stdout",fnamestdout,"get")
            transfer_file(nodes[self.node], directory + "/stderr",fnamestderr,"get")

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
            print output
            return output

class Monitors():
    def __init__(self, cmds, outdir, nodes, options):
        self.options = options
        self.outdir = outdir 
        self.cmds = cmds
        self.infos = {}
        self.nodes = nodes
        self.ns_ip = options.ns_ip
        self.ns_port = options.ns_port
    def run(self):
        self.infos = self.start()
    def compute_monitors_cmds(self, agent_pid_map):
        print agent_pid_map
        monitors = ["collectl -P --all",
                    "iostat -p -d 1 -k"]
        top_cmd = "top -b "
        for a, p in agent_pid_map.iteritems():
            top_cmd += "-p" + str(p) + " "
        monitors.append(top_cmd)
        print monitors
        self.cmds = monitors
    def start(self):
        infos = {}
        for s in self.nodes:
            rex = RemoteExecutor(s, self.options)
            infos[rex] = {}
            for cmd in self.cmds:
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

class AgentsPidMap:
    def __init__(self):
        self.pid_map = None
    def compute_pid_map(self, text):
        self.pid_map = self._get_agents_pid_map(text)
        print self.pid_map
    def _get_agents_pid_map(self, text):
        # print "-->", text
        # text = unescape_string(text)
        # pid_map = {}
        # for l in text:
        #     m = re.match("(pm|om|dm|sm|am)\s*:\s*\w+\s*(\d+).*", l) #FIXME: debug this... maybe need some whitespaces at the beginning
        #     if m is not None:
        #         agent = m.group(1)
        #         pid = int(m.group(2))
        #         print agent, pid
        #         pid_map[agent] = pid
        pid_map = {}
        output = subprocess.check_output("../../tools/fds status  | egrep '(pm|om|dm|sm|am)'| awk '{print $1, $4}'", shell = True)  # FIXME: path
        for l in output.split('\n'):
            if len(l.split()) > 0:
                t1, t2 = l.split()
                m = re.match(".*(pm|om|dm|sm|am).*", t1)
                if m is not None:
                    agent = m.group(1)
                    pid = int(t2)
                    pid_map[agent] = pid
        return pid_map
    def get_map(self):
        return self.pid_map
    def dump(self, outfile):
        of = open(outfile + "/pidmap","w")
        for k, v in self.pid_map.iteritems():
            of.write(k + ":" + str(v) + "\n")
        of.close()

class FdsCluster():
    def __init__(self, main_node, options):
        self.options = options
        self.main_node = main_node
        self.single_node = True
        # self.remote_fds_root = "/home/monchier/FDS/dev_counters"
        self.remote_fds_root = "/home/monchier/FDS/dev"
        self.local_fds_root =  "/home/monchier/FDS/dev"
        self.rex = RemoteExecutor(main_node, self.options)
        self.pidmap = AgentsPidMap()
    def get_pidmap(self):
        return self.pidmap.get_map()
    def start(self):
        print "starting FDS"
        if self.single_node == True:
            output = self.rex.execute_simple(self.remote_fds_root + "/source/tools/fds cleanstart")
            print output
            self.pidmap.compute_pid_map(output)
            time.sleep(10)
        else:
            output = self.rex.execute_simple("/fds/sbin/fds-tool.py -f /fds/sbin/fdstool.cfg -c -d -u")
            time.sleep(10)
            print output
            #self.pidmap.compute_pid_map(output) # FIXME: pidmap not computed!
            time.sleep(5)
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
        cmd = "python /root/traffic_gen.py -t %d -n %d -T %s -s %d -F %d -v %d" % (test_args["threads"],
                                                                                   test_args["nreqs"],
                                                                                   test_args["type"],
                                                                                   test_args["fsize"],
                                                                                   test_args["nfiles"],
                                                                                   test_args["nvols"])
        output = self.rex.execute_simple(cmd)
        print output
        return output
    def _init_tgen(self, args):
        if self.options.local == True:
            shutil.copyfile(self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py")
        else:
            transfer_file(self.main_node, self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py", "put")
        self.rex.execute_simple("rm -f /tmp/fdstrgen*")
        #self.rex.execute_simple("pkill 'collectl|iostat|top'")
        #output = self.rex.execute_simple("/usr/bin/curl -v -X PUT http://localhost:8000/volume")
        for i in range(args["nvols"]):
            requests.put("http://%s:8000/volume%d" % (nodes[self.main_node], i));

def main():
    parser = OptionParser()
    #parser.add_option("-d", "--directory", dest = "directory", help = "Directory")
    #parser.add_option("-d", "--node-name", dest = "node_name", default = "node", help = "Node name")
    parser.add_option("-n", "--name-server-ip", dest = "ns_ip", default = "10.1.10.102", help = "IP of the name server")
    parser.add_option("-p", "--name-server-pprt", dest = "ns_port", type = "int", default = 47672, help = "Port of the name server")
    parser.add_option("-l", "--local", dest = "local", action = "store_true", default = False, help = "Run locally")
    #parser.add_option("-s", "--server-ip", dest = "s_ip", default = "10.1.10.17", help = "IP of the server")
    #parser.add_option("-c", "--column", dest = "column", help = "Column")
    #parser.add_option("-p", "--plot-enable", dest = "plot_enable", action = "store_true", default = False, help = "Plot graphs")
    #parser.add_option("-A", "--ab-enable", dest = "ab_enable", action = "store_true", default = False, help = "AB mode")
    (options, args) = parser.parse_args()

    # start FDS
    results_dir = get_results_dir()
    test = "tgen"
    test_conf = {
        "nvols" : 10,
        "threads" : 10,
        "nreqs" : 1000,
        "type" : "PUT",
        "fsize" : 4096,
        "nfiles" : 1000
        }
    fds = FdsCluster("node2", options)
    fds.start()
    fds.init_test(test, results_dir, test_conf)
    # start stats collection
    counter_server = CounterServer(results_dir)
    cmds = ["iostat -d 1", "collectl"]
    monitors = Monitors(cmds, results_dir, nodes, options)
    monitors.run()

    # execute test 
    # time.sleep(5)
    fds.run_test(test, test_conf)
    
    # terminate stats collection 
    monitors.terminate()
    counter_server.terminate()
    # terminate FDS
    # fds.terminate() 

if __name__ == "__main__":
    main()
