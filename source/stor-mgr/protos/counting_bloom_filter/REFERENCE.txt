This directory contains an initial implmentation of multi-thread safe counting bloom filter.S

This isn't integrated with the FDS source base.  This was authored mainly to experiment
with time-decay bloom filter, but the initial implmentation is the multi-thread safe counting
bloom filter.

To compile, use following command:
    g++ -Wall -std=c++11 -g RandomHashGenerator.cpp -o RandomHashGenerator -lpthread


Following set of literature was consulted on this implementation and how SM is going
to use it for the hybrid volume object ranking.

fast hashing on bloom filter
============================
http://www.eecs.harvard.edu/~michaelm/postscripts/rsa2008.pdf

time decay bloom filter
=======================
http://ce.sharif.edu/~arambam/papers/Time-Decaying%20Bloom%20Filters%20for%20Data%20Streams%20with%20Skewed%20Distributions.pdf

inferential time decay bloom filter
===================================
http://www.edbt.org/Proceedings/2013-Genova/papers/edbt/a23-dautrich.pdf


Adapting the Bloom Filter to Multithreaded Environments
=======================================================
https://bib.irb.hr/datoteka/469831.mtbloom.pdf


Maintaining Time-Decaying Stream Aggregates
===========================================
http://www.cs.cmu.edu/~srini/15-829/readings/CS03.pdf

Time decay
==========
http://dimacs.rutgers.edu/~graham/pubs/papers/fwddecay.pdf

