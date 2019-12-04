
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

MC_DIR="$(cat ./trading-day.txt)/1/mc/"
MC_FILE="contracts.csv"
cd tick-data
mkdir -p ${MC_DIR}
cd ${MC_DIR}
echo "date,datenext,product,r1,r2,r3,r4" > ${MC_FILE} 

ZCE_SRC_MC_FILE="/home/u910019/domi_contr_check/yao_contracts.txt"
scp  -P 8012  "u910019@1.193.38.91:${ZCE_SRC_MC_FILE}" ./
for src_line in $(cat ${ZCE_SRC_MC_FILE}) 
do
	dest_line="$(cat ./trading-day.txt),,${src_line}"
	echo "${dest_line}" >>  ${MC_FILE} 
done

DCE_SRC_MC_FILE="/home/u910019/domi_contr_check/yao_contracts.txt"
scp -P 44153  "u910019@101.231.3.117:${DCE_SRC_MC_FILE}" ./
for src_line in $(cat ${DCE_SRC_MC_FILE}) 
do
	dest_line="$(cat ./trading-day.txt),,${src_line}"
	echo "${dest_line}" >>  ${MC_FILE} 
done

SHFE_SRC_MC_FILE="/home/u910019/domi_contr_check/yao_contracts.txt"
scp -P 44163  "u910019@101.231.3.117:${SHFE_SRC_MC_FILE}" ./
for src_line in $(cat ${SHFE_SRC_MC_FILE}) 
do
	dest_line="$(cat ./trading-day.txt),,${src_line}"
	echo "${dest_line}" >>  ${MC_FILE} 
done


