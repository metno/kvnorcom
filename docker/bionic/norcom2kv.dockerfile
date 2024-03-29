#syntax=docker/dockerfile:1.2

#Build from the top source directory as
# DOCKER_BUILDKIT=1 docker build -t norcom2kv -f docker/norcom2kv.dockerfile .

FROM ubuntu:bionic AS build
ARG DEBIAN_FRONTEND='noninteractive'

RUN apt-get update && apt-get install -y google-mock language-pack-nb-base\
  gnupg2 software-properties-common apt-utils

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/bionic bionic main contrib'


# RUN apt-get update && apt-get --yes \
#   debhelper autotools-dev lsb-base g++  debconf automake libtool \
#    libboost-dev libboost-thread-dev libboost-regex-dev libboost-filesystem-dev \
#   libboost-filesystem-dev libboost-program-options-dev libomniorb4-dev \
#   omniidl metlibs-putools-dev libkvcpp-dev 


RUN apt-get update && apt-get install --yes \
  debhelper autotools-dev autoconf-archive libboost-thread-dev lsb-base g++ devscripts debconf automake libtool fakeroot \
  libboost-thread-dev \
  omniidl metlibs-putools-dev  libkvcpp-dev 


VOLUME /src
VOLUME /build
WORKDIR /build

COPY . /src

RUN --mount=type=cache,target=/build cd /src/ && autoreconf -if && cd /build && \
      /src/configure --prefix=/usr --mandir=/usr/share/man --infodir=/usr/share/info  \
	    --localstatedir=/var --sysconfdir=/etc  \
      CFLAGS=-g && make && make install

ENTRYPOINT [ "/bin/bash"]


FROM ubuntu:bionic AS norcom2kv
ARG DEBIAN_FRONTEND='noninteractive'
ARG kvuser=kvalobs
ARG kvuserid=5010

RUN apt-get update && apt-get install -y language-pack-nb-base\
  gnupg2 software-properties-common apt-utils

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/bionic bionic main contrib'

RUN apt-get update && apt-get install --yes \
  libkvcpp10 libmetlibs-putools8 libboost-regex1.65.1

COPY --from=build /usr/bin/norcom2kv /usr/bin/
COPY docker/entrypoint.sh /usr/bin/
COPY GITREF /usr/share/norcom2kv/VERSION
RUN useradd -ms /bin/bash --uid ${kvuserid} --user-group  ${kvuser}
RUN mkdir -p /etc/kvalobs && chown ${kvuser}:${kvuser}  /etc/kvalobs
RUN mkdir -p /var/log/kvalobs && chown ${kvuser}:${kvuser}  /var/log/kvalobs

VOLUME /etc/kvalobs
VOLUME /var/log/kvalobs
VOLUME /var/lib/kvalobs

USER ${kvuser}:${kvuser}

ENTRYPOINT ["/usr/bin/entrypoint.sh"]