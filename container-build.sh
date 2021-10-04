#! /bin/bash

export DOCKER_BUILDKIT=1

kvuser=kvalobs
kvuserid=5010
mode=test
targets=norcom2kv
tag=latest
os=focal
#os=bionic
registry="registry.met.no/obs/kvalobs/kvbuild"
nocache=
only_build=

gitref=$(git rev-parse --show-toplevel)/gitref.sh

use() {

  usage="\
Usage: $0 [--help] [--staging|--prod|--test] [--tag tag] [--no-cache] [--only-build] target-list

This script build a norcom2kv container. 
Stop at build stage 'stage'. Default $targets.

If --staging or --prod is given it is copied to the 
container registry at $registry. 
If --test, the default, is used it will not be copied 
to the registry.

Options:
  --help        display this help and exit.
  --tag tagname tag the image with the name tagname, default $tag.
  --staging     build and push to staging.
  --prod        build and push to prod.
  --test        only build. Default.
  --stage stage stop at build stage. Default $targets.
  --no-cache    Do not use the docker build cache.
  --only-build  Stop after building.
  
"
echo -e "$usage\n\n"
}


while test $# -ne 0; do
  case $1 in
    --tag) tag=$2; shift;;
    --help) 
        use
        exit 0;;
    --staging) mode=staging;;
    --prod) mode=prod;;
    --test) mode=test;;
    --no-cache) nocache="--no-cache";;
    --only-build)
        only_build="--target build";;
    -*) use
      echo "Invalid option $1"
      exit 1;;  
    *) targets="$targets $1";;
  esac
  shift
done

echo "tag: $tag"
echo "mode: $mode"
echo "os: $os"
echo "Build mode: $mode"
echo "targets: $targets"

if [ "$mode" = "test" ]; then 
  kvuserid=$(id -u)
  registry=""
else 
  registry="$registry/$mode/"
fi

echo "registry: $registry"
echo "tag: $tag"


$gitref 

for target in $targets ; do
  docker build $nocache --target $target --build-arg "kvuser=$kvuser" --build-arg "kvuserid=$kvuserid" \
    -f docker/${os}/${target}.dockerfile ${only_build} --tag ${registry}${target}:$tag .
  
  if [ $mode != test ]; then 
    docker push ${registry}${target}:$tag
  fi
done


