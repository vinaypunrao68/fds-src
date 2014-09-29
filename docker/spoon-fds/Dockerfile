FROM ubuntu:14.04

RUN locale-gen en_US.UTF-8
RUN apt-get update #20140926
RUN apt-get -y purge openjdk*
RUN apt-get install -y python-software-properties software-properties-common python-setuptools
RUN add-apt-repository -y ppa:webupd8team/java
RUN add-apt-repository -y ppa:chris-lea/redis-server
RUN apt-get update
RUN apt-get install -y zip openssl git ack-grep python wget xsel python-pip zsh curl \
  build-essential runit openssh-server ruby tree vim openssh-server sudo lshw

RUN apt-get -y install git python3-distupgrade gdb build-essential \
  redis-server redis-tools maven libudev-dev libparted0-dev python2.7 \
  python-dev python-pip python-boto libpcre3-dev libpcre3 libssl-dev libfuse-dev \
  libevent-dev bison flex ragel ccache libboost-log1.54-dev libboost-program-options1.54-dev \
  libboost-timer1.54-dev libboost-thread1.54-dev libboost-regex1.54-dev libical-dev libical1 \
  libleveldb-dev python-yaml libyaml-dev host libncurses-dev npm nodejs ruby ruby-sass

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

RUN mkdir -p /etc/service
ADD runit-ssh /etc/service/ssh
ADD runit /etc/runit

RUN mkdir -p /var/run/sshd

RUN useradd -s /bin/zsh -m -d /home/pairing -g root pairing
RUN echo "pairing:pairing" | chpasswd
RUN echo "pairing        ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

ADD resources/zshrc.default /home/pairing/.zshrc
RUN git clone https://github.com/robbyrussell/oh-my-zsh.git /home/pairing/.oh-my-zsh
RUN chown -R pairing /home/pairing/.oh-my-zsh
ADD resources/tmux.conf /home/pairing/.tmux.conf

USER pairing
RUN pip install --upgrade --user git+git://github.com/Lokaltog/powerline

USER root

RUN touch /tmp/.devsetup.chk
RUN chmod 777 /tmp/.devsetup.chk
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
