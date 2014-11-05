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
import datetime
import requests
import counter_server
import subprocess
import shlex
sys.path.append('../fdslib')
sys.path.append('../fdslib/pyfdsp')
sys.path.append('../fdslib/pyfdsp/fds_service')
sys.path.append('../../tools/fdsconsole/contexts')
sys.path.append('../../tools/fdsconsole')
sys.path.append('../../tools')
from SvcHandle import SvcMap

def get_myip():
    cmd = "ifconfig| grep '10\.1' | awk -F '[: ]+' '{print $4}'"
    #return subprocess.check_output(shlex.split(cmd))
    return subprocess.check_output(cmd, shell = True).rstrip("\n")

# FIXME: need a class for this
def get_node_from_ip(nodes, node_ip):
    for k, v in nodes.iteritems():
        if v == node_ip:
            return k
    return None

def test_to_str(test):
    test_str = ""
    for k, v in test.iteritems():
        if k != "injector":
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

# TODO: change all execute_simple in ssh_exec... should work the same!
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

# FIXME: cmd-server autostart does not really work
# FIXME: fix the local thing
# FIXME: What if monitor on the local node
class RemoteExecutor:
    def __init__(self, node, options):
        print "Starting remote executor on ", node
        self.options = options
        self.local = self.options.myip == self.options.nodes[self.options.main_node]
        # assert self.local == False, "RemoteExecutor must be remote. Please, fix the local case"
        if not self.local == False:
            print "RemoteExecutor must be remote. Please, fix the local case"
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
            shutil.move(fnamestdout, directory + "/stdout")
            shutil.move(fnamestderr, directory + "/stderr")
            os.chmod(directory + "/stdout", 755)
            os.chmod(directory + "/stderr", 755)
    #FIXME: refactor this!
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
                        "iostat -kxdp -d 1 -k"]
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
        assert not self.queue.empty() and self.queue.qsize() == 1
        datafile, datafname = self.queue.get()
        #os.close(datafile)  # FIXME: cannot really close file, maybe send a signal to the process? Use threads?
        directory = self.outdir
        if not os.path.exists(directory):
            os.makedirs(directory)
        shutil.copyfile(datafname, directory + "/counters.dat")
        self.proc.terminate()


class CounterServerPull:
    def __init__(self, outdir, options):
        self.options = options
        self.outdir = outdir
        self.stop = threading.Event()
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
            svc_map = SvcMap(ip, port)
            svclist = svc_map.list()
            for e in svclist:
                nodeid, svc, ip = e[0], e[1], e[2]
                nodeid = long(nodeid)
                if svc != "om" and svc != "None" and re.match("10\.1\.10\.\d+", ip):
                    try:
                        cntrs = svc_map.client(nodeid, svc).getCounters('*')
                    except TApplicationException:
                        print "Counters failed for", svc
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
        print "starting counter task ", name
        #queue.put((datafile, datafname))
        #############################
        # reset counters
        # subprocess.call(shlex.split("python cli.py -c reset-counters"))
        # pull counters
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
        # terminate udp server
        #datafile, datafname = self.queue.get()
        directory = self.outdir
        if not os.path.exists(directory):
            os.makedirs(directory)
        self.stop.set()
        time.sleep(self.options.counter_pull_rate + 1)
        print "copying counter file", self.datafname
        os.close(self.datafile)
        shutil.move(self.datafname, directory + "/counters.dat")
        os.chmod(directory + "/counters.dat", 755)
        if self.options.java_counters == True:
            print "copying counter file", self.javafname
            os.close(self.javafile)
            shutil.move(self.javafname, directory + "/java_counters.dat")
            os.chmod(directory + "/java_counters.dat", 755)


class AgentsPidMap:
    def __init__(self, options):
        self.options = options
        self.pid_map = {}
        for n in options.nodes:
            self.pid_map[n] = {}

    def compute_pid_map(self):
        if self.options.local == True:
            output = subprocess.check_output(self.options.remote_fds_root + "/source/tools/fds status  | egrep '(pm|om|dm|sm|am|xdi)'| awk '{print $1, $4}'", shell = True)  # FIXME: path
            for l in output.split('\n'):
                if len(l.split()) > 0:
                    t1, t2 = l.split()
                    m = re.match(".*(pm|om|dm|sm|am|xdi).*", t1)
                    if m is not None:
                        agent = m.group(1)
                        pid = int(t2)
                        self.pid_map[self.options.main_node][agent] = pid
        else:
            cmd = self.options.local_fds_root + "/source/test/fds-tool.py -f " + self.options.local_fds_root + "/source/fdstool.cfg -s"
            output = subprocess.check_output(shlex.split(cmd))
            agents_strings ={ "om" : "com\.formationds\.om\.Main",
                "xdi" : "com\.formationds\.am\.Main",
                "am" : "bare_am",
                "pm" : "plat",
                "sm" : "StorMgr",
                "dm" : "DataMgr"}
            for a, s in agents_strings.iteritems():
                match_string = "\[(\d+\.\d+\.\d+\.\d+)\]\s+\w+\s+(\d+).*" + s + ".*"
                for l in output.split("\n"):
                    m = re.match(match_string, l)
                    if m is not None:
                        node_ip = m.group(1)
                        pid = int(m.group(2))
                        self.pid_map[get_node_from_ip(self.options.nodes, node_ip)][a] = pid

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
        self.remote_fds_root = options.remote_fds_root
        self.local_fds_root = options.local_fds_root
        self.pidmap = AgentsPidMap(self.options)
        self.local = self.options.myip == self.options.nodes[self.options.main_node]
        self.local_test = self.options.myip == self.options.test_node or self.options.test_node == None
    def get_pidmap(self):
        return self.pidmap.get_map()

    def restart(self):
        print "starting FDS"
        if self.options.no_fds_start:
            self.pidmap.compute_pid_map()
            time.sleep(1)
        elif self.options.single_node == True:
            if self.local == True:
                output = self._loc_exec(self.remote_fds_root + "/source/tools/fds cleanstart")
            else:
                output = self._rem_exec(self.remote_fds_root + "/source/tools/fds cleanstart")
            print output
            self.pidmap.compute_pid_map()
            time.sleep(10)
        else:
            cmd = self.local_fds_root + "/source/test/fds-tool.py -f " + self.local_fds_root + "/source/fdstool.cfg -c -d -u"
            output = self._loc_exec(cmd)
            print output
            self.pidmap.compute_pid_map()
            time.sleep(10)

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
        elif test_name == "amprobe":
            self._init_amprobe(test_args)
        elif test_name == "tgen_java":
            self._init_tgen_java(test_args)
        elif test_name == "fio":
            self._init_fio(test_args)
        else:
            assert False, "Test unknown: " + test_name

    def run_test(self, test_name, test_args = None):
        print "starting test:", test_name
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
        output = self._rem_exec(self.remote_fds_root + "/source/test/fds-primitive-smoke.py --up=false --down=false")
        print output
        return output

    def _init_tgen(self, args):
        if self.local_test == True:
            shutil.copyfile(self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py")
            self._loc_exec("rm -f /tmp/fdstrgen*")
        else:
            transfer_file(self.options.test_node, self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py", "put")
            ssh_exec(self.options.test_node, "rm -f /tmp/fdstrgen*")
        for i in range(args["nvols"]):
            requests.put("http://%s:8000/volume%d" % (self.options.nodes[self.options.main_node], i))

    def _run_tgen(self, test_args):
        cmd = "python3 /root/traffic_gen.py -t %d -n %d -T %s -s %d -F %d -v %d -u -N %s" % (
                                                                            test_args["threads"],
                                                                            test_args["nreqs"],
                                                                            test_args["type"],
                                                                            test_args["fsize"],
                                                                            test_args["nfiles"],
                                                                            test_args["nvols"],
                                                                            self.options.nodes[self.options.main_node])
        print "Test:", cmd
        if self.local_test == True:
            output = self._loc_exec(cmd)
        else:
            print "starting test remotely on node", self.options.test_node
            output = ssh_exec(self.options.test_node, cmd)
        print "-->", output
        return output

    def _init_amprobe(self, test_args):
        self._init_tgen(test_args)
        # FIXME: maybe redundant
        shutil.copyfile(self.local_fds_root + "/source/test/traffic_gen.py", "/root/traffic_gen.py")
        self._loc_exec("rm -f /tmp/fdstrgen*")
        # need to upload objects
        cmd = "python " + self.local_fds_root + "/source/test/traffic_gen.py -t 1 -n 10000 -T PUT -s %d -F %d -v %d -u -N %s" % (test_args["fsize"], test_args["nfiles"], test_args["nvols"], self.options.nodes[self.options.main_node])
        output = self._loc_exec(cmd)
        cmd = "python " + self.local_fds_root + "/source/test/perf/gen_json.py -n %d -s %d -v %d > /tmp/.am-get.json" % (test_args["nreqs"], test_args["fsize"], test_args["nvols"])
        output = self._loc_exec(cmd)
        of = open("/tmp/.am-get.json", "w")
        of.write(output)
        of.close()

    def _init_tgen_java(self, args):
        for i in range(args["nvols"]):
            requests.put("http://%s:8000/volume%d" % (self.options.nodes[self.options.main_node], i))

    def _run_tgen_java(self, test_args):
        def task(node, outs, queue):
            cmd = "pushd /home/monchier/linux-x86_64.debug/bin && ./trafficgen --n_reqs 100000 --n_files 1000 --outstanding_reqs %d --test_type GET --object_size 4096 --hostname 10.1.10.139 --n_conns %d && popd" % (outs, outs)
            output = ssh_exec(node, cmd)
            queue.put(output)
        
        # nodes = ["10.1.10.222", "10.1.10.221"]
        nodes = ["10.1.10.222"]
        N = test_args["threads"]
        outs = test_args["outstanding"]
        threads = []
        queue = Queue.Queue()
        for n in nodes:
            for i in  range(N):
                t = threading.Thread(target=task, args=(n, outs, queue))
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
        output = self._loc_exec(cmd)
        time.sleep(30) # AM probe will keep processing stuff for a while
        cmd = "cat /fds/var/logs/am.*|grep CRITICAL"
        output = ssh_exec(self.options.main_node, cmd)
        return output

    def _init_fio(self, args):
        cmd = "/fds/bin/fdscli --volume-create volume0 -i 1 -s 10240 -p 50 -y blk"
        output = self._loc_exec(cmd)
        time.sleep(5)
        cmd = "../cinder/nbdadm.py attach localhost volume0"
        output = self._loc_exec(cmd)
        time.sleep(5)
        nbdvolume = output.rstrip("\n")
        args["disk"] = nbdvolume 
        cmd = "/fds/bin/fdscli --volume-modify \"volume0\" -s 10240 -g 0 -m 0 -r 10"
        output = self._loc_exec(cmd)
        time.sleep(5)

    def _run_fio(self, test_args):
        filename = test_args["disk"]
        bs = test_args["bs"]
        numjobs = test_args["numjobs"]
        jobname = test_args["fio_jobname"]
        rw = test_args["fio_type"]
        options =   "--name=" + jobname + " " +
                    "--rw=" + fio_type + " " +
                    "--filename=" + disk + " " +
                    "--bs=" + bs + " " +
                    "--numjobs=" + numjobs + " " +
                    "--runtime=30 --ioengine=libaio --iodepth=16 --direct=$iodirect --size=10g --minimal "
        cmd = "fio " + options
        output = self._loc_exec(cmd)
        return output

    # FIXME: move to global
    def _rem_exec(self, cmd):
        print cmd
        output = ssh_exec(self.options.main_node, cmd)
        return output

    def _loc_exec(self, cmd):
        print cmd
        output = subprocess.check_output(shlex.split(cmd))
        return output

class CommandInjector:
    def __init__(self, options, commands):
        self.options = options
        print self.options.myip
        print self.options.nodes
        print self.options.main_node
        print self.options.nodes[self.options.main_node]
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
                print "CmdInj: sleep for", delay, "time:", time.time()
                time.sleep(delay)
            else:
                print "CmdInj:", cmd, "time:", time.time()
                self._loc_exec(cmd)

    def terminate(self):
        self.proc.join()

    # FIXME: move to global
    def _loc_exec(self, cmd):
        print cmd
        output = subprocess.check_output(shlex.split(cmd))
        return output
