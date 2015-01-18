import argh
import os
import subprocess

base_path = '../..'

# src directories to index
src_index_dirs = [
    'thrift-0.9.0/lib/cpp/src/thrift',
    'leveldb-1.15.0',
    'gmock-1.7.0',
    'source']

# include directories to index
include_index_dirs = [
    'thrift-0.9.0',
    'leveldb-1.15.0',
    'gmock-1.7.0',
    'source']

# list of src files with respect to base_path separated by new lines.
# These will be populate by the script.  Add your custom ones here
src_list = []

# list of include directories with respect to base_path separated by new lines.
# These will be populate by the script.  Add your custom ones here
include_list = [
    'thrift-0.9.0/lib/cpp/src/thrift',
    'source']

def main():
    global src_list
    global include_list
    # change to fds-src directory
    os.chdir(base_path)
    print 'current working directory: {}'.format(os.getcwd())

    # find all sources
    for d in src_index_dirs:
        print 'Indexing src dir: {}'.format(d)
        result = subprocess.check_output(['find', d , '-name', '*.c', '-or', '-name', '*.cpp'])
        src_list += result.split()

    # get all the includes
    for i in include_index_dirs:
        print 'Indexing include dir: {}'.format(d)
        result = subprocess.check_output(['find', i, '-type', 'd', '-name', 'include'])
        include_list += result.split()
        
    # generate the CMakeLists.txt
    with open('CMakeLists.txt', 'w') as f:
        f.write('cmake_minimum_required(VERSION 2.8.4)\n')
        f.write('project(dev)\n\n')
        f.write('set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")\n\n')

        f.write('set(SOURCE_FILES')
        for l in src_list:
            f.write('\n' + l)
        f.write(')\n\n')

        for l in include_list:
            f.write('include_directories({})\n'.format(l))
        f.write('\n')

        f.write('add_executable(dev ${SOURCE_FILES})')

    print 'Completed.  Src files cnt: {}.  Include dirs cnt: {}'.format(len(src_list), len(include_list))
    return 0

argh.dispatch_command(main)
