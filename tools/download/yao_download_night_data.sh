#!/bin/bash
SUFFIX="night"
program_name="sh_tra_${SUFFIX}"

STRATEGY_DIR="./stra_log"
TICKDATA_DIR="./tickdata"
LEV2MD_DIR="./lev2-md"

# obtain the directory where this script file locates.
 this_dir=`pwd`
 dirname $0|grep "^/" >/dev/null
 if [ $? -eq 0 ];then
         this_dir=`dirname $0`
 else
         dirname $0|grep "^\." >/dev/null
         retval=$?
         if [ $retval -eq 0 ];then
                 this_dir=`dirname $0|sed "s#^.#$this_dir#"`
         else
                 this_dir=`dirname $0|sed "s#^#$this_dir/#"`
         fi
 fi

cd $this_dir

STRA_LOG="/home/u910019/trade/night/backup/sh_stra_${SUFFIX}_`date +%y%m%d`.tar.gz"
STRA_LOG_GZ="sh_stra_${SUFFIX}_`date +%y%m%d`.tar.gz"
STRA_LOG_BJ2="sh_stra_${SUFFIX}_`date +%y%m%d`.tar.bz2"
scp  -P 44163  "u910019@101.231.3.117:${STRA_LOG}" ${STRATEGY_DIR}
cd ./stra_log
openssl des3 -d -k explorer -salt -in $STRA_LOG_GZ | tar --remove-files  -xvzf -
#tar -xvzf $STRA_LOG_GZ 
rm $STRA_LOG_GZ
tar --remove-files -cvjf $STRA_LOG_BJ2 ./backup
rm -r ./backup
cd ..


TICK_DATA="/home/u910019/md/download/night/backup/sh_md_${SUFFIX}_`date +%y%m%d`.tar.gz"
TICK_DATA_GZ="sh_md_${SUFFIX}_`date +%y%m%d`.tar.gz"
TICK_DATA_BZ2="sh_md_${SUFFIX}_`date +%y%m%d`.tar.bz2"
scp  -P 44163  "u910019@101.231.3.117:${TICK_DATA}" ${TICKDATA_DIR}
cd ./tickdata
tar -xvzf $TICK_DATA_GZ
rm $TICK_DATA_GZ
tar -cvjf $TICK_DATA_BZ2 ./backup
rm -r ./backup
cd ..

LEV2MD="/home/u910019/market-data/shfe-lev2/night/backup/shfe_lev2_data_${SUFFIX}_`date +%y%m%d`.tar.gz"
LEV2MD_GZ="shfe_lev2_data_${SUFFIX}_`date +%y%m%d`.tar.gz"
LEV2MD_BZ2="shfe_lev2_data_${SUFFIX}_`date +%y%m%d`.tar.bz2"
scp  -P 44163  "u910019@101.231.3.117:${LEV2MD}" ${LEV2MD_DIR}
cd ./lev2-md
tar -xvzf $LEV2MD_GZ
rm $LEV2MD_GZ
tar -cvjf $LEV2MD_BZ2 ./backup
rm -r ./backup
cd ..

