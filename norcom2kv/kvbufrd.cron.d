#Check that this machine owns the ip-alias for kvalobs or if this is a test machine.
#On a test machine the file $HOME/etc/KVALOBS_TEST exist.

( /usr/local/sbin/alias_list | /bin/grep -q kvalobs ) || (test -f "$HOME/etc/KVALOBS_TEST") || exit 0

25 5 * * * /usr/bin/kvbufrdbadmin %
