#!/bin/sh
rm -rf /home/swei/libobk/data/
rm -f /tmp/log-*

SBT=`pwd`/libsbt.so
sed -r "s|SBT_LIBRARY.*\.so|SBT_LIBRARY=$SBT|" -i sbt-backup.txt

cat > sbt-backup.txt << END
connect target /;

CONFIGURE DEFAULT DEVICE TYPE TO 'SBT_TAPE';
CONFIGURE CONTROLFILE AUTOBACKUP ON;
CONFIGURE CONTROLFILE AUTOBACKUP FORMAT FOR DEVICE type SBT_TAPE TO '%F';
CONFIGURE DEVICE type SBT_TAPE PARALLELISM 3 BACKUP TYPE TO BACKUPSET;
CONFIGURE DATAFILE BACKUP COPIES FOR DEVICE type SBT_TAPE TO 1;
CONFIGURE ARCHIVELOG BACKUP COPIES FOR DEVICE type SBT_TAPE TO 1;
CONFIGURE MAXSETSIZE TO UNLIMITED;
CONFIGURE ENCRYPTION FOR DATABASE OFF;

CONFIGURE CHANNEL DEVICE TYPE sbt
  PARMS='SBT_LIBRARY=$SBT
           ENV=(var1=value1,var2=value2)';

backup device type SBT_TAPE current controlfile format 'control_%I_%T_%u';

sql 'ALTER SYSTEM ARCHIVE LOG CURRENT';
backup INCREMENTAL LEVEL 0 device type SBT_TAPE database 
    include current controlfile 
    plus ARCHIVELOG;
END

rman cmdfile=sbt-backup.txt
