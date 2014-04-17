Packaging jdk 8
===============
- fetch the jdk tar from oracle's website
```
wget -O jdk-8u5-linux-x64.tar.gz http://download.oracle.com/otn-pub/java/jdk/8u5-b13/jdk-8u5-linux-x64.tar.gz?AuthParam=1397675350_f7010ad6a4147a6f67cba0ef705d341d
copy it to http://coke.formationds.com:8000/external/

or
wget http://coke.formationds.com:8000/external/jdk-8u5-linux-x64.tar.gz
```
- do `apt-get install java-package`
- do `make-jpg jdk-8u5-linux-x64.tar.gz`
- copy the created deb to `http://coke.formationds.com:8000/external/`