FROM registry.formationds.com:5000/fds_base
MAINTAINER Aaron Nichols <aaron@formationds.com>
#
USER pairing
RUN mkdir -p /home/pairing/.ssh /home/pairing/projects
ADD resources/ssh/id_rsa-fds-src /home/pairing/.ssh/id_rsa-fds-src
ADD resources/ssh/github-config /home/pairing/.ssh/config
USER root
RUN chown pairing /home/pairing/.ssh/id_rsa-fds-src
RUN chmod 600 /home/pairing/.ssh/id_rsa-fds-src
USER pairing
RUN git clone git@fds-src.github.com:FDS-Dev/fds-src.git /home/pairing/projects/fds-src

WORKDIR /home/pairing/projects/fds-src/source
RUN touch ~/.ihaveaswapfile
RUN make devsetup
RUN git remote set-url origin https://github.com/fds-dev/fds-src
RUN rm -f ~/.ssh/*

USER root
