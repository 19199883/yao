#!/bin/bash

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

function mail_log()
{
	sh 3rd-get_live_log.sh

	last_log=""
	 log_dir="./strategy-log"
	 OLD_IFS=$IFS
	 IFS=$'\n'
	 for ln in `ls -lt "${log_dir}" | grep ".*\.log" `
	 do  
		col_cnt=0
		IFS=' '
		for col in $ln
		do
			if [ $col_cnt -eq 8 ]
			then
				last_log="${log_dir}/${col}"
				break
			fi
			col_cnt=$[$col_cnt+1]   
		done
		break
	 done
	 IFS=$IFS.OLD

	echo "find the file:${last_log}"
	cat "${last_log}" | mail -s "strategy log" 3580771905@qq.com

}	 

enter_cur_dir
mail_log
