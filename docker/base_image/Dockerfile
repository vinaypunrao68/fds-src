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

RUN apt-get install -y zip openssl git ack-grep python wget xsel python-pip zsh \
  curl build-essential runit openssh-server ruby tree vim openssh-server sudo \
  lshw
RUN apt-get -y install libevent-dev libncurses-dev
RUN apt-get -y install host npm nodejs ruby ruby-sass rake

# Install the Docker client
RUN curl -sSL https://get.docker.io/ubuntu/ | sudo sh

# We symlink in support of tools expecting a node binary
RUN ln -s /usr/bin/nodejs /usr/bin/node

# Httpie
RUN easy_install httpie
RUN pip install https://github.com/Lokaltog/powerline/tarball/develop

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
RUN mkdir -p /home/pairing/.ssh /home/pairing/projects
RUN git clone https://github.com/robbyrussell/oh-my-zsh.git /home/pairing/.oh-my-zsh
RUN chown -R pairing /home/pairing/.oh-my-zsh

RUN pip install --upgrade --user git+git://github.com/Lokaltog/powerline
RUN git clone https://github.com/adnichols/tmux-and-vim.git /home/pairing/.janus
RUN bash -l -c /home/pairing/.janus/setup/setup.sh
ADD resources/zshrc.default /home/pairing/.zshrc
ADD resources/tmux.conf /home/pairing/.tmux.conf
RUN mkdir -p /home/pairing/projects

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
RUN chown pairing /home/pairing/projects
RUN chmod u+s /usr/bin/sudo


EXPOSE 22
EXPOSE 8000
EXPOSE 8443
EXPOSE 7000
EXPOSE 7443
EXPOSE 7777

CMD /usr/sbin/sshd -D -o UsePAM=no
