
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
scp  -P 44163  "u910019@101.231.3.117:/home/u910019/yao/3rd-trade/ytrader/trading-day.txt" ./




###########################
# download strategy log from production server.
#
#
################################
cd $this_dir
TRADING_DAY="$(cat ./trading-day.txt)"
cd "strategy-log"

STRATEGY_LOG="/home/u910019/yao/3rd-trade/backup/${TRADING_DAY}_*_night.log"
scp  -P 44163  "u910019@101.231.3.117:${STRATEGY_LOG}" ./


