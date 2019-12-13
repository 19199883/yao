#!/usr/bin/python
# -*- coding: UTF-8 -*-

########################################
#  	1. 更新生产环境的配置文件的主力合约以及合约订阅文件
#	每天日夜盘上传ev文件时，根据主力合约contracts.csv更新配置文
#	件x-trader.config。如果mc-warm.csv文件有内容，则将其中的合
#	约条件到x-trader.config的合约列表中。  
#
############################################
import datetime
import calendar
import datetime
import xml.etree.ElementTree as ET
from datetime import date
import os
import shutil
import csv
import logging
import os
import sys
import csv
import glob
import fileinput
from string import digits

varietiesFile = "/home/u910019/trader/varieties.txt"

# 存储实盘正在使用的主力合约的文件
mcFile = "/home/u910019/tick-data/mc/contracts.csv"
mcWarnFile = "/home/u910019/tick-data/mc/mc-warm.csv"
varietyFile = "/home/u910019/trader/varieties.txt"

#########################
# 参数1：脚本名
#
#########################
def main():
	configFile = 'x-trader.config'
	
	os.chdir(sys.path[0])
	os.chdir('../trader')
	print("current working directory:" + os.getcwd())
	
	logging.basicConfig(filename='mc2config.log',level=logging.DEBUG)		
	backup(configFile)

	tree = ET.parse(configFile)
	root = tree.getroot()
	UpdateConfig(root)
	tree.write(configFile, encoding="utf-8") #, xml_declaration=True) 

	# TODO：生成yao的三个交易所的合约订阅文件

#############
#	在更新配置之前，需要对原配置进行备份。
#	以备当更新出问题时还原。
#
#################
def backup(configFile):
	count = 1
	today = date.today()
	today_str = today.strftime("%Y%m%d")
	configBackup = 'x-trader_{0}_{1}.config'.format(today_str,count)
	while(os.path.exists(configBackup)):
		count = count + 1
		configBackup = 'x-trader_{0}_{1}.config'.format(today_str,count)
	shutil.copyfile(configFile, configBackup)

##################
#	判断指定的合约是否是订阅的品种。
#	如果是订阅的品种，返回true; 否则，返回false
##################
def IsSubscribedVariety(contract):
	print("IsSubscribedVariety contract " + contract)
	varietyOfContract = contract.translate(None, digits)
	print("IsSubscribedVariety varietyOfContract " + varietyOfContract)
	subscribedVarieties = []
	f = fileinput.input(files=(varietyFile))
	for line in f:
		subscribedVarieties = line.split(" ")
		break
	f.close()
	print("subscribedVarieties ", subscribedVarieties)
	
	return varietyOfContract in subscribedVarieties
	
def UpdateConfig(root):
	ClearSymbols(root)

	mcDict = {}
	with open(mcFile) as f:
		reader = csv.DictReader(f)		
		for row in reader:			
			mcDict[row["r1"]] = row["r1"]			
		
	f = fileinput.input(files=mcWarnFile)
	for line in f:
		for contract in line.split(" "):
			mcDict[contract] = contract
	f.close()

	strategyElement = root.find("./models/strategy")
	# find a symbol element as template
	symbolElementTemplate = strategyElement.find("./symbol")
	for contract in list(mcDict.keys()):
		if IsSubscribedVariety(contract):
			print("subscribed to " + contract)
			AddSymbol(strategyElement, symbolElementTemplate, contract)
		
	strategyElement.remove(symbolElementTemplate)

def AddSymbol(strategyElement, symbolElementTemplate, contract):
	symbolStr = ET.tostring(symbolElementTemplate, encoding="utf-8")
	newSymbol = ET.fromstring(symbolStr)
	strategyElement.append(newSymbol);
	newSymbol.set('name', contract) 
	

#############################
#	1. 假设：假设当前交易程序只支持单个策略。
#	2. 该方法会清除唯一的strategy下的symbol
#	元素(保留一个symbol元素用于复制)
#	3. 如果strategy元素下没有symbol元素，则报异常。
###########################
def ClearSymbols(root):
	'''
	removes strategy elements from trasev.config until there is 
	a strategy element left.
	'''
	symbolElements = root.findall("./models/strategy/symbol")	
	if len(symbolElements) == 0:		
		raise Exception('There is NOT one symbol element!')

	strategyElement = root.find("./models/strategy")
	for symbolElement in symbolElements[1:]:
		strategyElement.remove(symbolElement)


if __name__=="__main__":   
	main()

#country_str = ET.tostring(countrys[0])
#new_country = ET.XML(country_str)
#root.append(new_country)
#
