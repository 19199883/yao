
#!/bin/bash
export LD_LIBRARY_PATH=./:/home/u910019/tools/::$LD_LIBRARY_PATH

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

scp  -P 44163  "u910019@101.231.3.117:/home/u910019/yao/trade/ytrader/trading-day.txt" ./

###########################
# download zce market data from production server.
#
#
################################
cd $this_dir
ZCE_TICK_DATA_DIR="$(cat ./trading-day.txt)/1/207/0/"
cd tick-data
#rm -r ${ZCE_TICK_DATA_DIR}
mkdir -p ${ZCE_TICK_DATA_DIR}

TICK_DATA="/home/u910019/yao/market-data/night/backup/yao_zce_quote_`date +%y%m%d`.tar.gz"
TICK_DATA_GZ="yao_zce_quote_`date +%y%m%d`.tar.gz"
scp  -P 8012  "u910019@1.193.38.91:${TICK_DATA}" ${ZCE_TICK_DATA_DIR}
cd ${ZCE_TICK_DATA_DIR}

if [ -s $TICK_DATA_GZ ];then
	tar -xvzf $TICK_DATA_GZ
	cp -a ./backup/yao_zce_quote_*/*.dat ./
	rm $TICK_DATA_GZ
	rm -r ./backup

	for contract_file in $(ls -S *.dat) 
	do
		/home/u910019/tools/dat2csvd ${contract_file}
	done

	rm *.dat
	rm *-*.csv
else
	rm *.dat
fi



###########################
# download dce market data from production server.
#
#
################################
cd $this_dir
DCE_TICK_DATA_DIR="$(cat ./trading-day.txt)/1/227/0/"
cd tick-data

rm -r ${DCE_TICK_DATA_DIR}
mkdir -p ${DCE_TICK_DATA_DIR}

TICK_DATA="/home/u910019/yao/market-data/night/backup/y-dcequote_`date +%y%m%d`.tar.gz"
TICK_DATA_GZ="y-dcequote_`date +%y%m%d`.tar.gz"
scp  -P 44153  "u910019@101.231.3.117:${TICK_DATA}" ${DCE_TICK_DATA_DIR}
cd ${DCE_TICK_DATA_DIR}

if [ -s $TICK_DATA_GZ ];then
	tar -xvzf $TICK_DATA_GZ
	cp -a ./backup/y-dcequote_*/*.dat ./
	rm $TICK_DATA_GZ
	rm -r ./backup

	for contract_file in $(ls -S *.dat) 
	do
		/home/u910019/tools/dat2csvd ${contract_file}
	done

	rm *.dat
	rm *-*.csv
else
	rm *.dat
fi

cd $this_dir


###########################
# download shfe market data from production server.
#
#
################################
cd $this_dir
SHFE_TICK_DATA_DIR="$(cat ./trading-day.txt)/1/206/0/"
cd tick-data

rm -r ${SHFE_TICK_DATA_DIR}
mkdir -p ${SHFE_TICK_DATA_DIR}

SHFE_TICK_DATA="/home/u910019/yao/market-data/night/backup/y-shfequote_`date +%y%m%d`.tar.gz"
SHFE_TICK_DATA_GZ="y-shfequote_`date +%y%m%d`.tar.gz"
scp  -P 44163  "u910019@101.231.3.117:${SHFE_TICK_DATA}" ${SHFE_TICK_DATA_DIR}

INE_TICK_DATA="/home/u910019/yao/market-data/night/backup/y-inequote_`date +%y%m%d`.tar.gz"
INE_TICK_DATA_GZ="y-inequote_`date +%y%m%d`.tar.gz"
scp  -P 44163  "u910019@101.231.3.117:${INE_TICK_DATA}" ${SHFE_TICK_DATA_DIR}

cd ${SHFE_TICK_DATA_DIR}

if [ -s $SHFE_TICK_DATA_GZ ];then
	tar -xvzf $SHFE_TICK_DATA_GZ
	cp -a ./backup/y-shfequote_*/*.dat ./
	rm $SHFE_TICK_DATA_GZ
	rm -r ./backup

	if [ -s $INE_TICK_DATA_GZ ];then
		tar -xvzf $INE_TICK_DATA_GZ
		cp -a ./backup/y-inequote_*/*.dat ./
		rm $INE_TICK_DATA_GZ
		rm -r ./backup
	fi

	for contract_file in $(ls -S *.dat) 
	do
		/home/u910019/tools/dat2csvd ${contract_file}
	done

	rm *.dat
	rm *-*.csv
else
	rm *.dat
fi

cd $this_dir

###########################
# download strategy log from production server.
#
#
################################
cd $this_dir
TRADING_DAY="$(cat ./trading-day.txt)"
cd "strategy-log"

STRATEGY_LOG="/home/u910019/yao/trade/backup/${TRADING_DAY}_*_night.log"
scp  -P 44163  "u910019@101.231.3.117:${STRATEGY_LOG}" ./

sh ../tools/mc-mgr.sh 1
