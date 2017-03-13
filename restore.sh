#!/bin/sh
rm -f /tmp/log-*
rman target / <<END
shutdown abort;
END

./sbt-restore.sh > sbt-restore.txt
rman cmdfile=sbt-restore.txt
rman target / <<END
alter database open resetlogs;
END
