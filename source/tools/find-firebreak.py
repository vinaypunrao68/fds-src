#!/usr/bin/python3

# find . -name '*_min.log' | xargs grep "Sigma" | cut -f 1,2,14,15,17,18 -d , | grep -v SYSTEM_VOLUME | grep -v "Short Term Capacity Sigma: 0, Long Term Capacity Sigma: 0, Short Term Perf Sigma: 0, Long Term Perf Sigma: 0" |
# cut -d : -f 6,8,10,12 |head
import os
import csv
import optparse

#
# f stat file
# type ANY, CAPACITY, FIREBREAK
# threshold 
#
def parseStatFileCsv(f, type="ANY", threshold=0):
    with open(f, 'r') as csvf:
        reader = csv.reader(csvf, delimiter=',')
        for row in reader:
            vol=row[0]
            ts=row[1]
            stc=row[13]
            ltc=row[14]
            stp=row[16]
            ltp=row[17]

            stcv=float(stc.split(':')[1].lstrip(' '))
            ltcv=float(ltc.split(':')[1].lstrip(' '))
            stpv=float(stp.split(':')[1].lstrip(' '))
            ltpv=float(ltp.split(':')[1].lstrip(' '))

            if type == 'ANY' or type == 'CAPACITY':
                cfb = 0;
                if stcv > 0 and ltcv > 0:
                    cfb=stcv / ltcv
                    if cfb > threshold:
                        print("{:<25} {:<20} {:>12}: {} / {} = {}".format(ts, vol, 'Capacity', str(stcv), str(ltcv),str(cfb)))

            if type == 'ANY' or type == 'PERFORMANCE':
                pfb = 0;
                if stpv > 0 and ltpv > 0:
                    pfb=stpv / ltpv
                    if pfb > threshold:
                        print("{:<25} {:<20} {:>12}: {} / {} = {}".format(ts, vol, 'Performance', str(stpv), str(ltpv),str(pfb)))


parser = optparse.OptionParser("usage : %prog [options]")
parser.add_option("-R", "--fds-root", dest='fds_root', default='/fds', help='Specify fds root directory')
parser.add_option("-t", "--threshold", dest='threshold', default=0, help='Specify the firebreak threshold')
parser.add_option("-p", "--performance", dest="perf", action="store_true", help='Select performance FB')
parser.add_option("-c", "--capacity", dest="cap", action="store_true", help="Select capacity FB")
(options, args) = parser.parse_args()

t="ANY"
if options.cap and not options.perf:
    t="CAPACITY"
elif not options.cap and options.perf:
    t="PERFORMANCE"

for dirpath, dirnames, files in os.walk(options.fds_root + '/user-repo/vol-stats'):
    for name in files:
        if name.endswith(".log"):
            spath=os.path.join(dirpath, name)
            parseStatFileCsv(spath,t,float(options.threshold))

