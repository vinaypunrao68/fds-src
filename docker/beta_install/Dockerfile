FROM registry.formationds.com:5000/fds_base
MAINTAINER Aaron Nichols <aaron@formationds.com>
#
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
ENV CHANNEL "beta"
ENV VERSION "2015.06.19"

# Download our packages

RUN echo "Channel is ${CHANNEL}"
RUN echo "Version is ${VERSION}"

ENV FDS_DEPS fds-deps_2015.06.19-1_amd64.deb
ENV FDS_PLAT fds-platform-rel_2015.06.19-29_amd64.deb


ADD install /home/pairing/install
RUN chown -R pairing /home/pairing/install

RUN wget http://bld-apt.formationds.com/formation-apt/pool/beta/${FDS_DEPS}
RUN wget http://bld-apt.formationds.com/formation-apt/pool/beta/${FDS_PLAT}

RUN dpkg -i ${FDS_DEPS}

RUN mv ${FDS_DEPS} /home/pairing/install/omnibus/omnibus-fds-deps/pkg/
RUN mv ${FDS_PLAT} /home/pairing/install/omnibus/omnibus-fds-platform/pkg/

RUN sudo rm -rf /home/pairing/.ansible
