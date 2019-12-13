#!/bin/bash

export LD_LIBRARY_PATH=./:/home/u910019/tools/::$LD_LIBRARY_PATH

function upload_files
{
	echo "--------------uploading begins -----------------------"

	ssh -p 44163 u910019@101.231.3.117 'cp -a /home/u910019/yao/trade/ytrader/x-trader.config /home/u910019/yao/trade/ytrader/x-trader_`date +%y%m%d`.config'
	scp -Cp -P 44163 ./trader/x-trader.config	"u910019@101.231.3.117:/home/u910019/yao/trade/ytrader/"
	scp -Cp -P 44163 ./trader/ev_from_cache	"u910019@101.231.3.117:/home/u910019/yao/trade/ytrader/ev/"

	scp -Cp -P 44163 /home/u910019/tick-data/mc/yao-shfe-subcribed-mc.csv	"u910019@101.231.3.117:/home/u910019/yao/mc/"
	ssh -p 44163 u910019@101.231.3.117 'mv -v /home/u910019/yao/mc/yao-shfe-subcribed-mc.csv /home/u910019/yao/mc/contracts.txt'

	scp -Cp -P 44153 /home/u910019/tick-data/mc/yao-dce-subcribed-mc.csv	"u910019@101.231.3.117:/home/u910019/yao/mc/"
	ssh -p 44153 u910019@101.231.3.117 'mv -v /home/u910019/yao/mc/yao-dce-subcribed-mc.csv /home/u910019/yao/mc/contracts.txt'

	scp -Cp -P 8012 /home/u910019/tick-data/mc/yao-zce-subcribed-mc.csv	"u910019@1.193.38.91:/home/u910019/yao/mc/"
	ssh -p 8012 u910019@1.193.38.91 'mv -v /home/u910019/yao/mc/yao-zce-subcribed-mc.csv /home/u910019/yao/mc/contracts.txt'

	echo "---------------uploading finishs----------------------"

}

###################
# enter working directory
###########################
function  enter_wd
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

enter_wd
upload_files
