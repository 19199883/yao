#!/usr/bin/python
# -*- coding: UTF-8 -*-

########################################
# 该脚本放在yao_sky的/home/u910019/tools目录下。
# (1) 生成候选主力合约candidate-contracts.csv
#       当云服务器的各个交易所当前行情数据(当前交易日的日盘或夜盘)生成完后，
#	   则可以开始生成candidate-contracts.csv。
#	   工具访问每个交易所的每个品种的行情文件，读取每个品种的每个合约文件最
#	   后一笔行情的累计成交量，并对其排序，取前四个合约，并按规定格式，写到
#	   candidate-contracts.csv文件中。
#	   
#   (2) 主力合约换月
#	   每当生产完candidate-contracts.csv，都要比较candidate-contracts.csv
#	   和contracts.csv的每个品种的主力合约，如果某个品种的主力合约不同，
#	   则将该品种的新的备选主力合约写到文件mc-warn.csv中，并提醒用户换月。
#       如果需要换月，则用户手动修改mc-warm.csv，只保留需要换月的主力合约。
#	   
#	   提醒方式：进入云服务器提醒和发邮件2种方式。
#	   注意：换月过度阶段，要换月的品种需要配置新旧2个合约，等待旧的合约仓位
#	   都平了后，contracts.csv则完全使用新的合约。
#
############################################

import datetime
import xml.etree.ElementTree as ET
from datetime import date
import os
import shutil
import csv
import logging
import os
import sys

src_config_file = '../x-trader.config'
cur_config_file = 'x-trader.config'
stra_setting = 'dce_day067.csv'

#########################
# 参数1：脚本名
# 参数2：是否是夜盘（0：日盘；1：夜盘）
#
#########################
def main():
	os.chdir(sys.path[0])
	os.chdir('../')
	print("current working directory:")
	print(os.getcwd())
	
	argv_len = len(sys.argv)
	print(argv_len)
	isNight = sys.argv[1]
	print(isNight)
	tradingDay = GetTradingDay()
	targetDir = GetTargetDir()
	targetDir += "/"
	targetDir += isNight
	print(targetDir)
	
	
	logging.basicConfig(filename='configurator.log',level=logging.DEBUG)

#	shutil.copyfile(src_config_file, 'x-trader.config')
#	backup()
#
#	tree = ET.parse(cur_config_file)
#	root = tree.getroot()
#	update(root)
#
#	tree.write(cur_config_file, encoding="utf-8") #, xml_declaration=True) 
#	shutil.copyfile(cur_config_file, src_config_file)

#####################################
# 从trading-day.txt中获取当前交易日。
#
###################################
def GetTradingDay():
	with open("trading-day.txt", mode='r') as f:
		tradingDay = f.readline()		
		return tradingDay

def GetTargetDir():
	targetDir = "tick-data"
	targetDir += "/"
	targetDir += GetTradingDay()	
	return targetDir
	
	
	
	########################
	# 从指定的行情数据文件中获取最后一笔行情。
	#
	############################
def GetLastQuote(md_file):
	count = 1
	today = date.today()
	today_str = today.strftime("%Y%m%d")
	bk = 'x-trader_{0}_{1}.config'.format(today_str,count)
	while(os.path.exists(bk)):
		count = count + 1
		bk = 'x-trader_{0}_{1}.config'.format(today_str,count)
	shutil.copyfile(cur_config_file, bk)

	
	
def update(root):
	clear(root)

	strategies = root.find("./strategies")
	# find a strategy element as template
	strategy_temp = strategies.find("./strategy")

	# skip the first row, title row
	with open(stra_setting) as csvfile:
		reader = csv.reader(csvfile)
		id = 0
		for row in reader:
			if id == 0:
				id = id + 1
				continue
			add_strategy(strategies, strategy_temp, row, id)
			id = id + 1

	# remove template element
	strategies.remove(strategy_temp)

def check_cont(contChk):
	# check whether the specified contract is right dominant contract
	#
	#param:
	#	contChk: a contract to be checked whether it 
	#		is right dominant contract
	#
	domContLst = None

	# read dominant contracts
	domContLn = None
	domContFile = "/home/u910019/domi_contr_check/cur_domi_contrs.txt"
	with open(domContFile) as f:
		for line in f:
			domContLn  = line
			break

	valid = False
	domContLst = domContLn.split(" ")
	for cont in domContLst:
		if cont.find(contChk)==0:
			valid = True
			break

	if not valid:
		logging.warning("incorrect contract:{0}".format(contChk))



def add_strategy(strategies, strategy_temp, row, id):
	strategy_tmp_str = ET.tostring(strategy_temp, encoding="utf-8")
	new_strategy = ET.fromstring(strategy_tmp_str)
	strategies.append(new_strategy);

	new_strategy.set('id', str(id)) 
	new_strategy.set('model_file', row[1]) 

	# respectively copy ev file for every fl50,fl33 or fl51 strategry
	ev_name_value = "ev/" + row[1] + ".txt"
	new_strategy.set('ev_name', ev_name_value) 
	strategy_name = row[1]
	soFile = "../" + strategy_name + ".so" 
	if not os.path.exists(soFile):
		logging.warning("can not find " + soFile)


	ev_file_src = ""
	ev_file_dest = "../" + ev_name_value 

	if 0==strategy_name.find('avgp'):
		ev_file_src = "ev_avgp_day.txt"
		shutil.copyfile(ev_file_src, ev_file_dest)
		shutil.copystat(ev_file_src, ev_file_dest)

	symbol = new_strategy.find("./symbol")
	check_cont(row[2]) 
	symbol.set('max_pos', row[3])
	symbol.set('name', row[2])

def clear(root):
	'''
	removes strategy elements from trasev.config until there is 
	a strategy element left.
	'''
	stras = root.findall("./strategies/strategy")
	stra_cnt = len(stras);
	if stra_cnt > 1:
		del stras[0]
	elif stra_cnt == 0:
		raise Exception('There is NOT one strategy element!')

	strategies_e = root.find("./strategies")
	for stra in stras:
		strategies_e.remove(stra)


if __name__=="__main__":   
	main()

#country_str = ET.tostring(countrys[0])
#new_country = ET.XML(country_str)
#root.append(new_country)
#