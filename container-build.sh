#! /bin/bash

export DOCKER_BUILDKIT=1

kvuser=kvalobs
kvuserid=5010
mode=test
targets=norcom2kv
tag=latest
tag_and_latest="false"
os=focal
registry="registry.met.no/met/obsklim/kvalobs/kvbuild"
nocache=
only_build=
VERSION="$(./version.sh)"

gitref=$(git rev-parse --show-toplevel)/gitref.sh

use() {

  usage="\
Usage: $0 [--help] [--os os] [--staging|--prod|--test] [--tag tag] [--no-cache] [--only-build] 
       [--tag-and-latest tag] [--tag-version] target-list

This script build a norcom2kv container. 
Stop at build stage 'stage'. Default $targets.

If --staging or --prod is given it is copied to the 
container registry at $registry. 
If --test, the default, is used it will not be copied 
to the registry.

Options:
  --help        display this help and exit.
  --os os       The os to build for, default $os. 
  --tag tagname tag the image with the name tagname, default $tag.
  --tag-and-latest tagname tag the image with the name tagname  and also create latest tag.
  --tag-version Use version from configure.ac as tag. Also tag latest.
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
    --tag-and-latest) 
        tag="$2"
        tag_and_latest=true
        shift;;
    --tag-version) 
        tag="$VERSION"
        tag_and_latest=true;;
    --help) 
        use
        exit 0;;
    --os)
        os=$2
        shift;;
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

echo "VERSION: $VERSION"
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
  
  if [ "$tag_and_latest" = "true" ]; then
      docker tag "${registry}${target}:$tag" "${registry}${target}:latest"
  fi
  
  if [ $mode != test ]; then 
    docker push ${registry}${target}:$tag
    if [ "$tag_and_latest" = "true" ]; then
      docker push "${registry}${target}:latest"
    fi
  fi
done


