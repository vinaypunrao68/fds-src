#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import json

def dependency_resolve(graph, start, resolved, seen):
    seen.append(start)
    for edge in graph[start]:
        if edge not in resolved:
            if edge in seen:
                raise Exception('Circular reference detected: %s -&gt; %s' % 
                                (start, edge))
            dependency_resolve(graph, edge, resolved, seen)
    resolved.append(start)

if __name__ == "__main__":
    
    test_file = "test_list.json"
    with open(test_file, 'r') as data_file:
        data = json.load(data_file)
    test_set_list = data['test_sets']
    graph = {}
    lookup_table = {}
    test_executed = []
    for test_set in test_set_list:
        current = test_set_list[test_set]
        for testcase in current:
            name = testcase['name']
            if testcase['depends'] == []:
                print "Executing ", name
                test_executed.append(name)
            else:
                lookup_table[name] = testcase
            graph[name] = testcase['depends']

    for k, v in graph.iteritems():
        if k not in test_executed and v != []:
            resolved = []
            dependency_resolve(graph, k, resolved, [])
            for node in resolved:
                if node not in test_executed:
                    print "Executing ", node
                    test_executed.append(node)
    print test_executed