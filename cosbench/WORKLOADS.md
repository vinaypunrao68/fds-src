## COSBench Workloads

This file describes the workloads available for COSBench. If you add new
workloads please update this file

All files are jinja2 templates & new files must be created w/ the
correct template vars to allow replacement of auth information when the
template is submitted.

#### s3-0001.xml.j2

Containers: range 10
Objects: range 10 per container
Sizes: uniform distribution 64-1024k

main workload:
Workers: 8
Duration: 12 hours
Read: 20% from u(1,10)
Write: 80% from u(10,20) 64-1024k

#### s3-0002.xml.j2

Containers: range 10
Objects: range 100 per container
Sizes: uniform distribution 64-4096k

main workload:
Workers: 8
Duration: 12 hours
Read: 20% from u(1,100)
Write: 80% from u(100,200) 64-4096k

#### s3-0003.xml.j2

This job simply creates & then deletes 1000 buckets

Containers: range 1000
Objects: none
Sizes: N/A

#### s3-0004.xml.j2

This job creates & then deletes 10,000 buckets

Containers: range 10000
Objects: none
Sizes: N/A

#### s3-0005.xml.j2

This job creates & then deletes 1026 buckets

Containers: range 1026
Objects: none
Sizes: N/A

#### s3-0006.xml.j2

This job performs an 80/20 read:write for 30 minutes across 2 buckets
and 20 objects @ 64k only

Containers: range 2
Objects: uniform distribution across 20 objects
Sizes: 64k

main workload:
Workers: 8
Duration: 30 seconds
Read: 80% from u(1,10)
Write: 20% from u(11,20) 64k
