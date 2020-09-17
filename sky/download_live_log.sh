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


###########################
# download strategy log from production server.
#
#
################################
cd $this_dir
cd "strategy-log"

STRATEGY_LOG="/home/u910019/yao/trade/ytrader/log/*.log"
scp  -P 44163  "u910019@101.231.3.117:${STRATEGY_LOG}" ./
if [ $? -ne 0 ]; then
	echo "" | mail -s "异常：没有发现so的日志文件，可能原因：CTP初始化问题，请检查！！！" 3580771905@qq.com
	echo "" | mail -s "异常：没有发现so的日志文件，可能原因：CTP初始化问题，请检查！！！" 17199883@qq.com

	ssh -p 44163  u910019@101.231.3.117 "pkill -SIGUSR2 y-trade"
	cat "" | mail -s "关闭交易程序并重启。。。" 3580771905@qq.com
	cat "" | mail -s "关闭交易程序并重启。。。" 17199883@qq.com

	ssh -p 44163  u910019@101.231.3.117 "nohup sh /home/u910019/yao/trade/ytrader/start_only.sh >/dev/null 2>&1 &" 
	
	echo "交易程序重启完成。"
	echo "" | mail -s "交易程序重启完成。" 3580771905@qq.com
	echo "" | mail -s "交易程序重启完成。" 17199883@qq.com

	echo "交易程序重启完成。"

	 sleep 15


	scp  -P 44163  "u910019@101.231.3.117:${STRATEGY_LOG}" ./

fi


echo "done!!!"
