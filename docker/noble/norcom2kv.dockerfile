FROM ubuntu:noble AS build

RUN apt-get update && apt-get install -y libgmock-dev language-pack-nb-base\
  gnupg2 software-properties-common apt-utils

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/noble noble main contrib'

RUN apt-get update && apt-get install --yes \
  debhelper autotools-dev autoconf-archive libboost-thread-dev lsb-base g++ devscripts debconf automake libtool fakeroot \
  libboost-thread-dev \
  omniidl metlibs-putools-dev libkvcpp-dev 

VOLUME /src
VOLUME /build
WORKDIR /build

COPY . /src

# RUN --mount=type=cache,target=/build cd /src/ && autoreconf -if && cd /build && \
#   /src/configure --prefix=/usr --mandir=/usr/share/man --infodir=/usr/share/info  \
#   --localstatedir=/var --sysconfdir=/etc  \
#   CFLAGS=-g && make && make install


RUN cd /src/ && autoreconf -if && cd /build && \
  /src/configure --prefix=/usr --mandir=/usr/share/man --infodir=/usr/share/info  \
  --localstatedir=/var --sysconfdir=/etc  \
  CFLAGS=-g && make && make install

ENTRYPOINT [ "/bin/bash"]


FROM ubuntu:noble AS norcom2kv
ARG DEBIAN_FRONTEND='noninteractive'
ARG kvuser=kvalobs
ARG kvuserid=5010

RUN apt update && apt install -y language-pack-nb-base\
  gnupg2 software-properties-common apt-utils

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/noble noble main contrib'

RUN apt update && apt-get install --yes \
  libkvcpp10 libmetlibs-putools8 libboost-regex1.83.0

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