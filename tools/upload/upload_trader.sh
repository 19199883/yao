#!/bin/bash

export LD_LIBRARY_PATH=./:/home/u910019/tools/::$LD_LIBRARY_PATH

function upload_files
{
	echo "--------------uploading begins -----------------------"

	ssh -p 44163 u910019@101.231.3.117 'cp -a /home/u910019/yao/trade/ytrader/x-trader.config /home/u910019/yao/trade/ytrader/x-trader_`date +%y%m%d`.config'
	scp -Cp -P 44163 ./trader/x-trader.config	"u910019@101.231.3.117:/home/u910019/yao/trade/ytrader/"
	scp -Cp -P 44163 ./trader/ev_from_cache	"u910019@101.231.3.117:/home/u910019/yao/trade/ytrader/ev/"

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
