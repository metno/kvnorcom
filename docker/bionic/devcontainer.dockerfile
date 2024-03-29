#FROM registry.met.no/obs/kvalobs/kvbuild/staging/builddep:latest
FROM builddep:bionic
ARG DEBIAN_FRONTEND='noninteractive'

RUN apt-get update && apt-get install -y  google-mock language-pack-nb-base 

#Add intern repos
COPY docker/internrepo-4E8A0C14.asc /tmp/
RUN apt-key add /tmp/internrepo-4E8A0C14.asc && rm /tmp/internrepo-4E8A0C14.asc && \
  add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/bionic bionic main contrib'

# RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 4e8a0c1418494cf45d1b8533799e9fe74bb0156c &&\
#   add-apt-repository 'deb [arch=amd64] http://internrepo.met.no/focal focal main contrib'


RUN apt-get update && apt-get --yes install \
   libperl5.26  libkvcpp-dev mg nano
  
#metlibs-putools-dev

RUN locale-gen en_US.UTF-8

RUN useradd -ms /bin/bash vscode
