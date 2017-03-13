#!/bin/sh

SBT=`pwd`/libsbt.so

# Get the catalogs
for i in `ls /home/swei/libobk/data/ | grep -v backup_file`; do
    echo "CATALOG DEVICE TYPE SBT_TAPE BACKUPPIECE '$i';"  >> tmp
done
CATALOGS=`cat tmp`
rm -f tmp

cat <<END
connect target /
startup nomount;
set DBID 1342594059

run {
allocate channel t1 device type SBT
    PARMS='SBT_LIBRARY=$SBT
           ENV=(var1=value1,var2=value2)';

restore controlfile from AUTOBACKUP;
alter database mount;
}

CONFIGURE DEFAULT DEVICE TYPE TO 'SBT_TAPE';
CONFIGURE CONTROLFILE AUTOBACKUP FORMAT FOR DEVICE type SBT_TAPE TO '%F';
CONFIGURE DEVICE type SBT_TAPE PARALLELISM 3 BACKUP TYPE TO BACKUPSET;
CONFIGURE CHANNEL DEVICE TYPE sbt
    PARMS='SBT_LIBRARY=$SBT
           ENV=(var1=value1,var2=value2)';

$CATALOGS

restore database;
recover database;
END

