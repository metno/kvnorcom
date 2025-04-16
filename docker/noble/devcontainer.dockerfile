ARG REGISTRY="registry.met.no/met/obsklim/bakkeobservasjoner/data-og-kvalitet/kvalobs/kvbuild/staging"

FROM ${REGISTRY}/kvbuilddep:latest
ARG DEBIAN_FRONTEND='noninteractive'
ARG USER=vscode

RUN apt-get update && apt-get install -y libgmock-dev language-pack-nb-base 

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/noble noble main contrib'


RUN apt update && apt --yes install language-pack-nb-base \
  libperl5.38t64  libkvcpp-dev nano less 

#metlibs-putools-dev

RUN locale-gen en_US.UTF-8 nb_NO.UTF-8
RUN useradd -ms /bin/bash ${USER}

ENV USER=${USER}
