#!/usr/bin/python
import subprocess
import os
import shutil

def create_files(unit, files, path):
  cmd = "fallocate -l %s %s"
  fname = "test_sample_%s"
  os.chdir(path)

  for s in files:
     size = ("%s%s" % (s, unit))
     current = fname % size
     subprocess.call([cmd % (size, current)], shell=True)

def create_tar_file():
   current = os.path.dirname(os.path.realpath(__file__))
   test_files = "test_files"
   path = os.path.join(current, "test_files")
   if os.path.exists(path):
      shutil.rmtree(path)
   os.mkdir(path)

   mb_size_files = [1,2,5,10,15,20,30,50,80,100,125,150,200,500]
   gb_size_files = [1,2,3]
   
   create_files("M", mb_size_files, path)
   create_files("G", gb_size_files, path)

   os.chdir(current)   
   cmd = "tar -cvzf sample.tar.gz %s" % test_files
   subprocess.call([cmd], shell=True)
 
if __name__ == "__main__":
   create_tar_file()
