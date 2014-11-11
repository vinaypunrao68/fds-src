#!/usr/bin/python
import os, sys, re, random, datetime
import tabulate
import operator 
from influxdb import client as influxdb
import numpy
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

class InfluxDbUtils:
    @staticmethod
    def write_to_influxdb(results, series, branch, regr_type, test_id):
        db = influxdb.InfluxDBClient("10.1.10.222", 8086, "root", "root", "perf")
        date = str(datetime.date.today())
        for e in results:
            test_name =  regr_type + ":" + e[0]
            osize = e[1]   
            concurrency = e[2] 
            iops = e[3] 
            latency = e[4]
            failures = e[5]
            data = [
              {
               "name" : series,
                "columns" : ["iops", "latency", "concurrency", "test_name", "date", "failures", "object_size", "branch", "test_id"],
                "points" : [
                  [float(iops), float(latency), int(concurrency), test_name, date, int(failures), int(osize), branch, int(test_id)] 
                ]
              }
            ]
            db.write_points(data)

    @staticmethod
    def plot_latbw(test_structure, branch, test_id):
        images = []
        db = influxdb.InfluxDBClient("10.1.10.222", 8086, "root", "root", "perf")
        series = "regressions"
        for t in test_structure.keys():
            for o in test_structure[t].keys():
                figure = FigureUtils.new_figure()
                query_cmd = "select iops,latency,date,failures from %s where branch='%s' \
                and failures=0 and test_name='%s' and object_size=%d and test_id=%d" % (series, branch, t, o, test_id)
                result = db.query(query_cmd)
                name = result[0]['name']
                columns = result[0]['columns']
                points = result[0]['points']
                iops =  [operator.itemgetter(columns.index('iops'))(x) for x in points]
                lats =  [operator.itemgetter(columns.index('latency'))(x) for x in points]
                print columns, points
                scatter = zip(*[iops, lats])
                
                for e in scatter:
                    FigureUtils.plot_scatter(e, figure)
                #list1 = sorted([iops, lats], key=operator.itemgetter(1, 2))

                name = "test=%s:object_size=%s" % (t, o)
                filename = name + ".png"
                FigureUtils.save_figure(filename, figure, title = name, ylabel = "Time [ms]", xlabel = "IOPs")
                images.append(filename)
        return images 


    @staticmethod   
    def plot_influxdb(test_structure, branch):
        images = []
        db = influxdb.InfluxDBClient("10.1.10.222", 8086, "root", "root", "perf")
        series = "regressions"
        figures = {"iops" : {}, "latencies" : {}, "failures" : {}}
        for t in test_structure.keys():
            for n in figures.keys():
                figures[n][t] = FigureUtils.new_figure()
            for o in test_structure[t].keys():
                for c in test_structure[t][o].keys():
                    query_cmd = "select iops,latency,date,failures from %s where branch='%s' \
                    and failures=0 and test_name='%s' and object_size=%d and \
                    concurrency=%d" % (series, branch, t, o, c)
                    print query_cmd
                    result = db.query(query_cmd)
                    name = result[0]['name']
                    columns = result[0]['columns']
                    print columns
                    points = result[0]['points']
                    # FIXME: order
                    # FIXME: window
                    iops =  [operator.itemgetter(columns.index('iops'))(x) for x in points]
                    lats =  [operator.itemgetter(columns.index('latency'))(x) for x in points]
                    failures =  [operator.itemgetter(columns.index('failures'))(x) for x in points]
                    time =  [operator.itemgetter(columns.index('time'))(x) for x in points]
                    label = "obj_size=%d conc=%d" % (o, c)
                    FigureUtils.plot_series(iops, figures["iops"][t], label)
                    FigureUtils.plot_series(lats, figures["latencies"][t], label)
                    FigureUtils.plot_series(failures, figures["failures"][t], label)
            for n in figures.keys():
                name = "test=%s:%s" % (t, n)
                filename = name + ".png"
                FigureUtils.save_figure(filename, figures[n][t],title = name, ylabel = n, xlabel = "Time")
                images.append(filename)
        return images 

class FigureUtils:
    @staticmethod
    def new_figure():
        return plt.figure()
     
    @staticmethod
    def plot_series(series, fig, _label = ""):
        plt.figure(fig.number)
        data = numpy.asarray(series)
        plt.plot(data, label=_label)

    @staticmethod
    def plot_scatter(point, fig):
        plt.figure(fig.number)
        #data = numpy.asarray(series)
        # plt.scatter(x, y, s=area, c=colors, alpha=0.5)
        plt.scatter(point[0], point[1])
    
    @staticmethod
    def save_figure(filename, fig, title = None, ylabel = None, xlabel = None):
        plt.figure(fig.number)
        if title:
            plt.title(title)
        if ylabel:
            plt.ylabel(ylabel)
        if xlabel:
            plt.xlabel(xlabel)
        plt.ylim(ymin = 0)
        #plt.legend()
        ax = plt.subplot(111)
        box = ax.get_position()
        #ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
        ax.set_position([0.1, 0.1, 0.5, 0.8])
        ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
        ax.set_aspect("auto")
        plt.savefig(filename)

def get_lat(f):
    cmd = "cat %s | grep Summary | awk '{print $17}'" % (f)
    lat = float(os.popen(cmd).read().rstrip('\n'))
    return lat

def get_iops(f):
    cmd = "cat %s | grep Summary |awk '{print $15}'" % (f)    
    iops = float(os.popen(cmd).read().rstrip('\n'))
    return iops

def get_failures(f):
    cmd = "cat %s | grep Summary | awk '{print $19}'" % (f)
    failures = int(os.popen(cmd).read().rstrip('\n'))
    return failures

def get_lat_block(f):
    cmd = "cat %s " % (f)
    #lat (msec): min=16 , max=121 , avg=42.84, stdev=11.35
    for l in os.popen(cmd):
        m = re.match("\s+lat\s+\((\w+)\):.*avg=\s*([\d\.]+),\s+.*", l)
        if m != None:
            unit = m.group(1)
            if unit == "usec":
                return float(m.group(2)) * 1000
            elif unit == "msec":    
                return float(m.group(2))
            else:
                assert False, "fio unit not supperted"
    return -1.0

def get_iops_block(f):
    cmd = "cat %s | grep iops" % (f)    
    line = os.popen(cmd).read().rstrip("\n")
    m = re.match(".*iops=(\w+).*", line)
    assert m != None
    iops = float(m.group(1))
    return iops

def get_failures_block(f):
    assert False, "not implemented"
    failures = 0
    return failures


def check_ok_s3(f):
    cmd = "cat %s | grep Summary | wc -l" % (f)
    n = int(os.popen(cmd).read().rstrip('\n'))
    return n > 0

def check_ok_block(f):
    cmd = "cat %s | grep 'Run status group' | wc -l" % (f)
    n = int(os.popen(cmd).read().rstrip('\n'))
    return n > 0

def get_hostname():
    return os.popen("hostname").read().rstrip("\n")


def get_results_s3(directory):
    files = os.listdir(directory)
    results_table = []
    
    for fname in files:
        m = re.match("out-s3-(\w+)-(\w+)-(\w+)",fname)
        if m is not None:
            ok = check_ok_s3(directory + "/" + fname)
            if not ok:
                continue
            test =  m.group(1)
            concurrency =  int(m.group(2))
            osize =  int(m.group(3))
            iops = get_iops(directory + "/" + fname)
            lat = get_lat(directory + "/" + fname)
            failures = get_failures(directory + "/" + fname)
            #results_table[test + ":" + str(concurrency) + ":" + str(osize)] = (iops, lat, failures)  
            results_table.append([test, osize, concurrency, iops, lat, failures])  
    return results_table

def get_results_block(directory):
    files = os.listdir(directory)
    results_table = []
    
    for fname in files:
        m = re.match("out-block-(\w+)-(\w+)-(\w+)",fname)
        if m is not None:
            ok = check_ok_block(directory + "/" + fname)
            if not ok:
                continue
            test = m.group(1)
            osize =  int(m.group(2).rstrip('k')) * 1024
            concurrency = int(m.group(3))
            iops = get_iops_block(directory + "/" + fname)
            lat = get_lat_block(directory + "/" + fname)
            failures = 0
            #results_table[test + ":" + str(concurrency) + ":" + str(osize)] = (iops, lat, failures)  
            results_table.append([test, osize, concurrency, iops, lat, 0])  
    return results_table

def compute_structure(table, mode):
    structure = {}
    for e in table:
        test = mode + ":" + e[0]
        osize = e[1]
        concurrency = e[2]
        if not test in structure:
            structure[test] = {}
        if not osize in structure[test]:
            structure[test][osize] = {}
        if not concurrency in structure[test][osize]:
            structure[test][osize][concurrency] = True
    return structure

def get_githead(directory):
    if os.path.exists(directory + "/githead"):
        with open(directory + "/githead","r") as _f:
            text = _f.readlines()[0]
    else:
        text = "n/a"
    return text

def get_gitbranch(directory):
    if os.path.exists(directory + "/gitbranch"):
        with open(directory + "/gitbranch","r") as _f:
            text = _f.readlines()[0]
    else:
        text = "n/a"
    return text


def mail_success(text, directory, branch, regr_type, images = []):
    githead = get_githead(directory)
    # gitbranch = get_gitbranch(directory)
    hostname = get_hostname()
    preamble = "\
Performance regression\n\
Regression type: %s\n\
Number of nodes: 1\n\
Branch: %s\n\
hostname: %s\n\
git version: %s\
" % (regr_type, branch, hostname, githead)
    epilogue = "\
Data uploaded to Influxdb @ 10.1.10.222:8083 - login guest - guest - database: perf\n\
Confluence: https://formationds.atlassian.net/wiki/display/ENG/InfluxDB+for+Performance"
    with open(".mail", "w") as _f:
        _f.write(preamble + "\n")
        _f.write("\n")
        _f.write(text + "\n")
        _f.write("\n")
        _f.write(epilogue + "\n")
    # mail_address = "engineering@formationds.com"
    
    mail_address = "matteo@formationds.com"
    attachments = ""
    for e in images:
        attachments += "-a %s " % e
    cmd = "cat .mail | mail -s 'performance regression %s' %s %s" % (regr_type, attachments, mail_address)
    print cmd
    os.system(cmd)

def pretty_dump(results_table):
    header = ["Test", "Object size [byte]", "Concurrency", "IOPs", "Latency [ms]", "Failures"]
    sorted_table = sorted(results_table, key=operator.itemgetter(0, 1, 2))
    return tabulate.tabulate(sorted_table, header)

# def gen_latbw(results_table, test):
#     table = filter(lambda x : x[0] == test,results_table)
#     sizes = set()
#     [sizes.add(e[1]) for e in table]
#     for s in sizes:
#         table_size = filter(lambda x : x[0] == test and x[1] == s, results_table)
#         bw = table_size[:][3]
#         lat = table_size[:][4]
#         print bw, lat
    
if __name__ == "__main__":
    directory = sys.argv[1]
    test_id = int(sys.argv[2])
    mode = sys.argv[3]
    branch = sys.argv[4]
    
    if mode == "s3":
        results_table = get_results_s3(directory)
    elif mode == "block":
        results_table = get_results_block(directory)
    else:
        assert False, "mode not known"
   
    text = pretty_dump(results_table)
    print "Results"
    print text 
    
    InfluxDbUtils.write_to_influxdb(results_table, "regressions", branch, mode, test_id)
    exp_structure = compute_structure(results_table, mode)    
    print exp_structure
    #images = InfluxDbUtils.plot_influxdb(exp_structure, branch)
    images = InfluxDbUtils.plot_latbw(exp_structure, branch, test_id)
    mail_success(text, directory, branch, mode, images)
