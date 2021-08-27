#!/usr/bin/env bash


#When a process is killed by a signal, it has exit code 128 + signum. (LINUX)
#Killed by SIGTERM (15) => 128 + 15 = 143.

app=norcom2kv
version=$(cat /usr/share/$app/VERSION)

set -e
echo "ENTRYPOINT: NARGS: $# ARGS: '$@'"
echo "getent: $(getent passwd kvalobs)"
echo "id: $(id -u)"
echo "VERSION: $version"


echo "app: '$app'  $#"
echo "$version" > "/var/log/kvalobs/${app}_VERSION"

if [ -f /usr/share/kvalobs/VERSION ]; then
  cat /usr/share/kvalobs/VERSION >> "/var/log/kvalobs/${app}_VERSION"
fi

if [ "$app" != "bash" ]; then
  echo "ENTRYPOINT $app"
  echo "Starting $app."
  exec /usr/bin/${app}  2>&1 
elif [ "$app" = "bash" ]; then
  echo "VERSION: $version"
  echo "ENTRYPOINT starting bash!"
  exec /bin/bash
else
  echo "VERSION: $version"
  echo "ENTRYPOINT sleep forever!"
  while running="true"; do 
    sleep 1; 
  done
fi
