#!/bin/bash
#############################################
# 脚本第一个参数:$1,传递是日盘还是夜盘参数
#
#######################################

# the directory where this script file is.
function enter_cur_dir()
{
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
}

function run()
{	
	isNight=$1
	
}	 

isNight=$1
enter_cur_dir
echo "isNight=${isNight}"
python ./mc-updater.py $isNight 0
python ./mc-updater.py $isNight 1
sh ./deliveryday-warn.sh

# yao 只在日盘结束的时候确定吧,就是同一个交易日的夜盘和日盘不发生合约的改变
if [ $isNight == "0" ];then
	echo "do execute mc2config.py"
	python ./mc2config.py
else
	echo "do not execute mc2config.py"
fi

