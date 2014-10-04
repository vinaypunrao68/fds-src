FROM ubuntu:14.04
MAINTAINER Aaron Nichols <aaron@formationds.com>
#
# Formation Dockerfile
#
# This is a general purpose dev environment for building & testing Formation
# software in Docker. This can be used either as a Jenkins slave or as an
# on-demand dev environment using docker-spoon. The image includes a lot of
# different things to allow for use in different areas at the cost of taking
# a long time to rebuild.

RUN locale-gen en_US.UTF-8

# If you run into 404's during package installs, update the date here to todays date
RUN apt-get update #20140926

RUN apt-get -y purge openjdk*
RUN apt-get install -y python-software-properties software-properties-common python-setuptools
RUN add-apt-repository -y ppa:webupd8team/java
RUN add-apt-repository -y ppa:chris-lea/redis-server
RUN apt-get update
RUN apt-get install -y zip openssl git ack-grep python wget xsel python-pip zsh \
  curl build-essential runit openssh-server ruby tree vim openssh-server sudo \
  lshw

# If you are adding packages, add to the end so we don't re-install all of these
# They are broken up to take advantage of the Docker cache
RUN apt-get -y install git python3-distupgrade gdb build-essential redis-server
RUN apt-get -y install redis-tools maven libudev-dev libparted0-dev
RUN apt-get -y install python2.7 python-dev python-pip python-boto
RUN apt-get -y install libpcre3-dev libpcre3 libssl-dev libfuse-dev libevent-dev
RUN apt-get -y install bison flex ragel ccache
RUN apt-get -y install libboost-log1.54-dev libboost-program-options1.54-dev
RUN apt-get -y install libboost-timer1.54-dev libboost-thread1.54-dev libboost-regex1.54-dev
RUN apt-get -y install libical-dev libical1 libleveldb-dev libncurses-dev
RUN apt-get -y install python-yaml libyaml-dev libconfig-dev libconfig++-dev libfiu-dev fiu-utils
RUN apt-get -y install host npm nodejs ruby ruby-sass rake

# Install the Docker client
RUN curl -sSL https://get.docker.io/ubuntu/ | sudo sh

# We symlink in support of tools expecting a node binary
RUN ln -s /usr/bin/nodejs /usr/bin/node

# FDS Packages
RUN wget -O - http://coke.formationds.com:8000/fds.repo.gpg.key 2>/dev/null | apt-key add -
RUN echo 'deb http://coke.formationds.com:8000 fds test' > /etc/apt/sources.list.d/fds.list
RUN apt-get update
RUN apt-get -y install fds-pkghelper fds-pkg fds-pkgtools fds-systemdir fds-coredump

# Httpie
RUN easy_install httpie
RUN pip install https://github.com/Lokaltog/powerline/tarball/develop

# Java
RUN echo oracle-java8-installer shared/accepted-oracle-license-v1-1 select true | /usr/bin/debconf-set-selections
RUN apt-get -y install oracle-java8-installer
ENV JAVA_HOME /usr/lib/jvm/java-8-oracle/

# Test deps
RUN pip install unittest-xml-reporting
RUN pip install paramiko

# FDS Console deps
RUN pip install thrift

# Tmux
RUN wget http://downloads.sourceforge.net/tmux/tmux-1.9a.tar.gz
RUN tar -zxf tmux-1.9a.tar.gz
RUN cd tmux-1.9a && ./configure && make install
RUN rm -r /tmux-1.9a*

# Setup our pairing user
RUN useradd -s /bin/zsh -m -d /home/pairing -g root pairing
RUN echo "pairing:pairing" | chpasswd
RUN echo "pairing        ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER pairing
RUN git clone https://github.com/robbyrussell/oh-my-zsh.git /home/pairing/.oh-my-zsh
RUN chown -R pairing /home/pairing/.oh-my-zsh

RUN pip install --upgrade --user git+git://github.com/Lokaltog/powerline
RUN git clone https://github.com/adnichols/tmux-and-vim.git ~/.janus
RUN bash -l -c ~/.janus/setup/setup.sh
ADD resources/zshrc.default /home/pairing/.zshrc
ADD resources/tmux.conf /home/pairing/.tmux.conf
ADD resources/clone-fds-src.sh /home/pairing/clone-fds-src.sh

USER root

# Runit setup
RUN mkdir -p /etc/service
ADD runit-ssh /etc/service/ssh
ADD runit /etc/runit

RUN mkdir -p /var/run/sshd
RUN mkdir /root/.ssh
RUN chmod 700 /root/.ssh
ADD resources/ssh/id_rsa.pub /root/.ssh/authorized_keys
ADD resources/ssh/config /root/.ssh/config
ADD resources/profile.d/java.sh /etc/profile.d/java.sh

RUN mkdir /home/pairing/projects
RUN chown pairing /home/pairing/projects

EXPOSE 22
EXPOSE 8000
EXPOSE 8443
EXPOSE 7000
EXPOSE 7443
EXPOSE 7777

CMD /usr/sbin/sshd -D -o UsePAM=no
