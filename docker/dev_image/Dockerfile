FROM registry.formationds.com:5000/fds_base
MAINTAINER Aaron Nichols <aaron@formationds.com>
#
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
# Download cosbench
WORKDIR /opt
RUN wget https://github.com/intel-cloud/cosbench/releases/download/v0.4.0.1/0.4.0.1.zip
RUN unzip 0.4.0.1.zip
RUN rm 0.4.0.1.zip

ENV JAVA_HOME /usr/lib/jvm/java-8-oracle
# Setup pairing ssh & repo
USER pairing
RUN mkdir -p /home/pairing/.ssh /home/pairing/projects
ADD resources/ssh/id_rsa-fds-src /home/pairing/.ssh/id_rsa-fds-src
ADD resources/ssh/github-config /home/pairing/.ssh/config
USER root
RUN chown pairing /home/pairing/.ssh/id_rsa-fds-src
RUN chmod 600 /home/pairing/.ssh/id_rsa-fds-src
USER pairing
RUN git clone git@fds-src.github.com:FDS-Dev/fds-src.git /home/pairing/projects/fds-src

# Run devsetup
WORKDIR /home/pairing/projects/fds-src/source
RUN make devsetup
RUN git remote set-url origin https://github.com/fds-dev/fds-src
RUN rm -f ~/.ssh/*
RUN ccache -M 2G

RUN git config --global user.name "Deploy"
RUN git config --global user.email "deploy@formationds.com"
RUN git config --global --unset user.name
RUN git config --global --unset user.email

USER root
RUN rm -rf /home/pairing/.ansible
