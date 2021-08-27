#syntax=docker/dockerfile:1.2

#Build from the top source directory as
# DOCKER_BUILDKIT=1 docker build -t norcom2kv -f docker/norcom2kv.dockerfile .

FROM ubuntu:focal AS build

RUN apt-get update && apt-get install -y libgmock-dev language-pack-nb-base\
  gnupg2 software-properties-common apt-utils

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/focal focal main contrib'


# RUN apt-get update && apt-get --yes \
#   debhelper autotools-dev lsb-base g++  debconf automake libtool \
#    libboost-dev libboost-thread-dev libboost-regex-dev libboost-filesystem-dev \
#   libboost-filesystem-dev libboost-program-options-dev libomniorb4-dev \
#   omniidl metlibs-putools-dev libkvcpp-dev 


RUN apt-get update && apt-get install --yes \
  debhelper autotools-dev autoconf-archive libboost-thread-dev lsb-base g++ devscripts debconf automake libtool fakeroot \
  libboost-thread-dev \
  omniidl metlibs-putools-dev libkvcpp-dev 


VOLUME /src
VOLUME /build
WORKDIR /build

COPY . /src

RUN --mount=type=cache,target=/build cd /src/ && autoreconf -if && cd /build && \
      /src/configure --prefix=/usr --mandir=/usr/share/man --infodir=/usr/share/info  \
	    --localstatedir=/var --sysconfdir=/etc  \
      CFLAGS=-g && make && make install

ENTRYPOINT [ "/bin/bash"]


FROM ubuntu:focal
RUN apt-get update && apt-get install -y language-pack-nb-base\
  gnupg2 software-properties-common apt-utils

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/focal focal main contrib'

RUN apt-get update && apt-get install --yes \
  libkvcpp9 libmetlibs-putools8 libboost-regex1.71.0

COPY --from=build /usr/bin/norcom2kv /usr/bin/

ENTRYPOINT ["/usr/bin/norcom2kv"]