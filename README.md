# fds-src
 FormationOne Dynamic Storage Platform
 
FormationOne is a hyper-scale software-defi ned storage platform, built from
the ground up, for the modern enterprise data center. FormationOne virtualizes all elements of storing and managing data. The system dynamically supports
flash, disk, and even IaaS storage as well as any new form of non-volatile storage
media. It supports dynamic capacity changes and can dynamically scale-out to
Exabyte-capacities. FormationOne also virtualizes storage resources across
industry standard hardware elements as long as those elements meet certain
minimum standards. We architected FormationOne to easily accommodate
new hardware and storage media.

Through an open universal “Data API,” FormationOne dynamically supports all
data types including block, fi le, Hadoop, and object data. FormationOne supports
data management features like snapshot and cloning across every data type.
From capacity to service level, every element within FormationOne is dynamically
adjustable.

FormationOne allows enterprises to easily increase their storage capacity as
needed and improves operational effi ciency by collapsing existing specialty
storage and technology silos, such as backup platforms, Network Attached
Storage (NAS), and Hadoop/HDFS, into a single platform.
The system provides a single platform for data, no matter what its business value,
simplifi ed storage management through a single pane of glass, and API-driven
provisioning for quicker deployment and monitoring of business processes and
effi ciency. The platform also has a built-in self-healing mechanism that prevents
disruption during service, node and disk failures.
We built FormationOne with three fundamental software layers

The following table summarizes these functions:

ACCESS MANAGER
Entry point for all data into the Formation domain. AM chunks
and secure hashes the DOs. Access Manager is the coordinator
in a distributed transaction that produces DOs and catalog objects
and sends out the simulcast of these objects into the storage and
data managers.

STORAGE MANAGER
Storage management layer that manages the data storage on the
node. It keeps object-to-disk mapping, provides tiering between
flash and disk, collects garbage, and de-duplicates data.

DATA MANAGER
Metadata manager for the volumes. It participates in the replication of data, time lining and cloning of volumes, and journaling
of operations on a per-volume basis.

ORCHESTRATION MANAGER
The brain of the domain; it holds the map of active nodes that
form the domain. It coordinates control path requests, such as
creation and deletion of volumes or snapshots and clones, and
starting and stopping rebalance on node reboot. It also provides
an XDI interface that is available to the User Interface (UI) and
applications.

PLATFORM MANAGER
This component provides the common interface to the hardware
in a node. For instance, it discovers disks and SSDs, and network
interfaces. It participates in building the domain node map, and
provides a messaging bus for all the other components above.
