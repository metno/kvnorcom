#! /bin/bash

grep 'AC_INIT(' configure.ac | sed 's/AC_INIT( *\[norcom2kv\] *, *\[\(.*\)\] *,.*/\1/'
