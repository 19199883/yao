#!/usr/bin/python
# -*- coding: UTF-8 -*-

########################################
#  候选主力合约: /home/u910019/tick-data/trading-day/day-night/mc/contracts.csv
#  实盘主力合约：/home/u910019/tick-data/mc/contracts.csv
# 该脚本放在yao_sky的/home/u910019/tools目录下。
# (1) 生成候选主力合约
#       当云服务器的各个交易所当前行情数据(当前交易日的日盘或夜盘)生成完后，
#	   则可以开始生成候选主力合约。
#	   工具访问每个交易所的每个品种的行情文件，读取每个品种的每个合约文件最
#	   后一笔行情的累计成交量，并对其排序，取前四个合约，并按规定格式，写到
#	   候选主力合约文件中。
#	   
#   (2) 主力合约换月
#	    每当生产完候选主力合约，都要比较候选主力合约 和实盘主力合约的每个品种
#	    的主力合约，如果某个品种的主力合约不同，
#	    则将该品种的新的备选主力合约写到文件mc-warn.csv中，并提醒用户换月。
#       如果需要换月，则用户手动修改mc-warm.csv，只保留需要换月的主力合约。
#	   
#	   提醒方式：进入云服务器提醒和发邮件2种方式。
#	   注意：换月过度阶段，要换月的品种需要配置新旧2个合约，等待旧的合约仓位
#	   都平了后，实盘主力合约则完全使用新的合约。
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
from string import digits

shfeContractsFile = "shfe-contracts.csv"
dceContractsFile = "dce-contracts.csv"
zceContractsFile = "zce-contracts.csv"
yaoContractsFile = "contracts.csv"

# 存储实盘正在使用的主力合约的文件
mcFile = "/home/u910019/tick-data/mc/contracts.csv"

mcWarnFile = "/home/u910019/tick-data/mc/mc-warm.csv"
mc2ndWarnFile = "/home/u910019/tick-data/mc/mc-2nd-warm.csv"
deliveryDayWarnFile = "/home/u910019/tick-data/mc/deliveryday-warm.csv"
deliveryDayWarnFile2nd = "/home/u910019/tick-data/mc/deliveryday-warm-2nd.csv"
mediShfeSubcribeMcFile = "/home/u910019/tick-data/mc/medi-shfe-subcribed-mc.csv"
mediDceSubcribeMcFile = "/home/u910019/tick-data/mc/medi-dce-subcribed-mc.csv"
mediZceSubcribeMcFile = "/home/u910019/tick-data/mc/medi-zce-subcribed-mc.csv"
yaoShfeSubcribeMcFile = "/home/u910019/tick-data/mc/yao-shfe-subcribed-mc.csv"
yaoDceSubcribeMcFile = "/home/u910019/tick-data/mc/yao-dce-subcribed-mc.csv"
yaoZceSubcribeMcFile = "/home/u910019/tick-data/mc/yao-zce-subcribed-mc.csv"

#########################
# 参数1：脚本名
# 参数2：是否是夜盘（0：日盘；1：夜盘）
# 参数3：0：是根据行情数据产生中间数据信息
#		 1：根据中间数据信息生成行情订阅数据
#
#########################
def main():
	logging.basicConfig(filename='mc-updater.log',level=logging.DEBUG)

	os.chdir(sys.path[0])
	os.chdir('../')
	logging.info("current working directory: {0}".format(os.getcwd()))
		
	argv_len = len(sys.argv)
	logging.info("argv len: {0}".format(argv_len))
	isNight = sys.argv[1]
	option = sys.argv[2]
	logging.info("isNight:{0}; option: {1}".format(isNight, option))
	
	targetDir = GetTargetDir(isNight)	
	logging.info("target directory: {0}".format(targetDir))
	
	if option == "0":	# 根据行情数据生成中间文件
		WriteDceMcFile(isNight)
		WriteZceMcFile(isNight)
		WriteShfeMcFile(isNight)
		WriteYaoMcFile(isNight)
		Warn1stMcChaningMonth(isNight)
		Warn2ndMcChaningMonth(isNight)
	
	if option == "1":	# 根据中间文件生成订阅文件
		UpdateSubscribedMcForMedi(isNight)	
		UpdateSubscribedMcForYao()

#####################################
# 从trading-day.txt中获取当前交易日。
#
###################################
def GetTradingDay():
	with open("trading-day.txt", mode='r') as f:
		tradingDay = f.readline().rstrip("\n")		
		return tradingDay

###########################
# 获取用于存储获选主力合约的目标目录
#
##############################		
def GetTargetDir(isNight):
	targetDir = "tick-data"
	targetDir += "/"
	targetDir += GetTradingDay()	
	targetDir += "/"
	targetDir += isNight
	targetDir += "/mc/"
		
	if not os.path.exists(targetDir):
		os.makedirs(targetDir)
		
	return targetDir
	
######################
# 获取存储上期的行情数据文件
#
#########################
def GetShfeMdDir(isNight):
	targetDir = "tick-data"
	targetDir += "/"
	targetDir += GetTradingDay()	
	targetDir += "/"
	targetDir += isNight
	targetDir += "/206/0"
	return targetDir
	
######################
# 获取存储大连的行情数据文件
#
#########################
def GetDceMdDir(isNight):
	targetDir = "tick-data"
	targetDir += "/"
	targetDir += GetTradingDay()	
	targetDir += "/"
	targetDir += isNight
	targetDir += "/227/0"
	return targetDir

######################
# 获取存储郑州的行情数据文件
#
#########################
def GetZceMdDir(isNight):
	targetDir = "tick-data"
	targetDir += "/"
	targetDir += GetTradingDay()	
	targetDir += "/"
	targetDir += isNight
	targetDir += "/207/0"
	return targetDir
	
	
########################
# 从指定的行情数据文件中获取最后一笔行情。
# 将最后一笔行情的累计成交量作为键，合约作为值
# 存储到totalVolContractDict字典中。
############################
def GetLastQuote(md_file, totalVolContractDict):
	lastTotal_vol = 0	
	contract = ""
	with open(md_file) as f:
		reader = csv.DictReader(f)		
		for row in reader:					
			#logging.info("total_vol: {0}".format(row["total_vol"]))
			lastTotal_vol = int(row["total_vol"])
			contract = row["symbol"]
	
	totalVolContractDict[lastTotal_vol] = contract	
		
###################
# write first four lively contracts
# 写指定品种的前四个最活跃的合约(按活跃程度降序排列)到
# 指定的文件中。
#
#######################
def WriteFFLC(varity, mc_file, totalVolContractDict):	
	logging.info("mc_file: {0}, varity:{1}".format(mc_file, varity))
	with open(mc_file, 'a') as mcfile:
		fieldnames = ["date", "datenext", "product", "r1", "r2", "r3", "r4"]
		writer = csv.DictWriter(mcfile, fieldnames=fieldnames)
		keys = totalVolContractDict.keys()
		keys.sort(reverse=True)

		contract1 = ""
		if len(keys) >= 1:
			contract1 = totalVolContractDict[keys[0]]
		contract2 = ""
		if len(keys) >= 2:
			contract2 = totalVolContractDict[keys[1]]
		contract3 = ""
		if len(keys) >= 3:
			contract3 = totalVolContractDict[keys[2]]
		contract4 = ""
		if len(keys) >= 4:
			contract4 = totalVolContractDict[keys[3]]

		writer.writerow({
							"date": GetTradingDay(), 
							"datenext": "",
							"product": varity,
							"r1" : contract1, 
							"r2" : contract2, 
							"r3" : contract3, 
							"r4" : contract4
						})


##################
# 根据品种文件的品种，指定的行情数据目录中的行情文件，
# 查找每个品种的前四个最活跃的合约，并将这些合约按指定的
# 格式,写到指定的mc文件中。
# varities_file：存储品种的文件
# md_dir：存档行情数据的目录
# mc_file：存储主力合约的目标文件。
##################
def WriteMcFile(varities_file, md_dir, mc_file):
	with open(mc_file, 'w') as mcfile:
		fieldnames = ["date", "datenext", "product", "r1", "r2", "r3", "r4"]
		writer = csv.DictWriter(mcfile, fieldnames=fieldnames)
		writer.writeheader()		
	
	varities = ""
	totalVolContractDict = {}
	with open(varities_file) as f:
		reader = csv.reader(f)
		for row in reader:
			varities = row
			break
	logging.info("varities: {0}".format(varities[0]))
	for varity in varities[0].split(' '):
		totalVolContractDict.clear()
		#logging.info("process: {0} ...".format(varity))
		md_file = os.path.join(md_dir, varity + '[0-9]*.csv')
		logging.info("md_file: {0} ...".format(md_file))
		for file in glob.glob(md_file):
			#logging.info("process : {0}".format(file))
			GetLastQuote(file, totalVolContractDict)
		if len(totalVolContractDict.keys()) > 0:
			WriteFFLC(varity, mc_file, totalVolContractDict)
	print("WriteMcFile")

#######################
# 根据上期的品种文件，指定路径的上期的行情文件，
# 找到每个品种的前四个合约，按指定格式存储到指
# 定的主力合约文件中。
#
#######################
def WriteShfeMcFile(isNight):
	print("WriteShfeMcFile")
	varities_file = "tools/shfe-varieties.txt"
	md_dir = GetShfeMdDir(isNight)
	mc_dir = GetTargetDir(isNight)	
	mc_file = os.path.join(mc_dir, shfeContractsFile)
	WriteMcFile(varities_file, md_dir, mc_file)

#######################
# 根据大商所的品种文件，指定路径的上期的行情文件，
# 找到每个品种的前四个合约，按指定格式存储到指
# 定的主力合约文件中。
#
#######################
def WriteDceMcFile(isNight):
	varities_file = "tools/dce-varieties.txt"
	md_dir = GetDceMdDir(isNight)
	mc_file = os.path.join(GetTargetDir(isNight), dceContractsFile)
	WriteMcFile(varities_file, md_dir, mc_file)

#######################
# 根据大商所的品种文件，指定路径的上期的行情文件，
# 找到每个品种的前四个合约，按指定格式存储到指
# 定的主力合约文件中。
#
#######################
def WriteZceMcFile(isNight):
	varities_file = "tools/zce-varieties.txt"
	md_dir = GetZceMdDir(isNight)
	mc_file = os.path.join(GetTargetDir(isNight), zceContractsFile)
	WriteMcFile(varities_file, md_dir, mc_file)

def WriteYaoMcFileInner(dictReader, yao_mc_filename):
	logging.info("write yao_mc_filename: {0}".format(yao_mc_filename))
	with open(yao_mc_filename, 'a') as yao_mcfile:
		fieldnames = ["date", "datenext", "product", "r1", "r2", "r3", "r4"]
		writer = csv.DictWriter(yao_mcfile, fieldnames=fieldnames)		
		for row in dictReader:
			#logging.info("write yao_mc_filename: {0}".format(row))
			writer.writerow({
							"date": row["date"], 
							"datenext": row["datenext"],
							"product": row["product"],
							"r1" : row["r1"], 
							"r2" : row["r2"], 
							"r3" : row["r3"], 
							"r4" : row["r4"]
						})								
	
#######################
# 将三个交易所的每个品种前四个最活跃合约写到contracts.csv(给yao)
#
#######################
def WriteYaoMcFile(isNight):
	yao_mc_filename = os.path.join(GetTargetDir(isNight), yaoContractsFile)
	with open(yao_mc_filename, 'w') as yao_mcfile:
		fieldnames = ["date", "datenext", "product", "r1", "r2", "r3", "r4"]
		writer = csv.DictWriter(yao_mcfile, fieldnames=fieldnames)
		writer.writeheader()

	shfe_mc_filename = os.path.join(GetTargetDir(isNight), shfeContractsFile)
	with open(shfe_mc_filename) as f:
		reader = csv.DictReader(f)		
		WriteYaoMcFileInner(reader, yao_mc_filename)
		
	dce_mc_filename = os.path.join(GetTargetDir(isNight), dceContractsFile)
	with open(dce_mc_filename) as f:
		reader = csv.DictReader(f)		
		WriteYaoMcFileInner(reader, yao_mc_filename)
		
	zce_mc_filename = os.path.join(GetTargetDir(isNight), zceContractsFile)
	with open(zce_mc_filename) as f:
		reader = csv.DictReader(f)		
		WriteYaoMcFileInner(reader, yao_mc_filename)
	
######################
# 通过比较候选主力合约与实盘主力合约，找到需要换月的合约，
# 并存储到warnContracts
# 
#
#######################	
def WarnChaningMonthForTotalVolFor1stMc(warnContracts, isNight):
	usingContracts = []
	# 实盘主力合约
	if os.path.exists(mcFile):
		with open(mcFile) as f:
			reader = csv.DictReader(f)		
			for row in reader:								
				usingContracts.append(row["r1"])	
			
	# 备选主力合约
	canditateContracts = []
	yao_mc_filename = os.path.join(GetTargetDir(isNight), yaoContractsFile)
	with open(yao_mc_filename) as f:
		reader = csv.DictReader(f)		
		for row in reader:
			contract = row["r1"]
			if len(contract) > 0:
				canditateContracts.append(contract)
	
	for contract in canditateContracts:
		if contract not in usingContracts:
			warnContracts.append(contract)
				
######################
# 通过比较候选主力合约与实盘主力合约，找到需要换月的合约，
# 并存储到warnContracts
# 
#
#######################	
def WarnChaningMonthForTotalVolFor2ndMc(warnContracts, isNight):
	usingContracts = []
	# 实盘主力合约
	if os.path.exists(mcFile):
		with open(mcFile) as f:
			reader = csv.DictReader(f)		
			for row in reader:								
				usingContracts.append(row["r2"])	
			
	# 备选主力合约
	canditateContracts = []
	yao_mc_filename = os.path.join(GetTargetDir(isNight), yaoContractsFile)
	with open(yao_mc_filename) as f:
		reader = csv.DictReader(f)		
		for row in reader:
			contract = row["r2"]
			if len(contract) > 0:
				canditateContracts.append(contract)
	
	for contract in canditateContracts:
		if contract not in usingContracts:
			warnContracts.append(contract)

def IsNullMd(warnContracts, isNight):
	# 备选主力合约
	canditateContracts = []
	yao_mc_filename = os.path.join(GetTargetDir(isNight), yaoContractsFile)
	if os.path.exists(yao_mc_filename):
		with open(yao_mc_filename) as f:
			reader = csv.DictReader(f)		
			for row in reader:								
				canditateContracts.append(row["r1"])
	
	return len(canditateContracts)==0
	
######################
# 通过遍历实盘主力合约，找到需要接近交割日提醒的合约，
# 并存储到warnContracts
# 
# 假设: 假设所有月份的最后一天都是30
#
#######################
def WarnChaningMonthForDeliveryDay(warnContracts):
	# 实盘主力合约
	usingContracts = []
	if os.path.exists(mcFile):
		with open(mcFile) as f:
			reader = csv.DictReader(f)		
			for row in reader:								
				usingContracts.append(row["r1"])
	
	for contract in usingContracts:
		contractMonth = int(contract[-3:])
		#logging.info("WarnChaningMonthForDeliveryDay contractMonth: {0} {1}".format(contract, contractMonth))
		curMonth = int(date.today().strftime("%y%m")[-3:])
		#logging.info("WarnChaningMonthForDeliveryDay curMonth:{0}".format(curMonth))
		if contractMonth == curMonth :
			if (30 - date.today().day) <= 7 :  # TODO: to here
				#logging.info("WarnChaningMonthForDeliveryDay warn contract:{0}".format(contract))
				warnContracts.append(contract)
		

#################################
# 换月提醒。
# 1. 当某个合约的当日累计成交量超过实盘当前的主力合约的当日累计成交量时，
#    则提醒改品种合约需要换月
# 2. 如果并没有满足(1)中的换月条件，但是当前某主力合约的月份是当前月份，
#    而且当前日期距离该月最后一天小于等于7天，则该合约也需要提醒换月(投机
#    账户是不允许进入交割日的)
#
###########################		
# TODO: here		
def Warn1stMcChaningMonth(isNight):	
	warnContracts = []
	warnContractsForTotalVol = []
	warnContractsForDeliveryDay = []

	if not IsNullMd(warnContractsForTotalVol, isNight):
		WarnChaningMonthForTotalVolFor1stMc(warnContractsForTotalVol, isNight)
		warnContracts = warnContractsForTotalVol
		with open(mcWarnFile, 'w') as f:
			if len(warnContracts) > 0:
				f.write(" ".join(warnContracts))
			else:
				f.write("")
	
	WarnChaningMonthForDeliveryDay(warnContractsForDeliveryDay)
	with open(deliveryDayWarnFile, 'w') as f:
		f.write(" ".join(warnContractsForDeliveryDay))
	

# TODO: here		
def Warn2ndMcChaningMonth(isNight):	
	warnContracts = []
	warnContractsForTotalVol = []
	warnContractsForDeliveryDay = []

	if not IsNullMd(warnContractsForTotalVol, isNight):
		WarnChaningMonthForTotalVolFor2ndMc(warnContractsForTotalVol, isNight)
		warnContracts = warnContractsForTotalVol
		with open(mc2ndWarnFile, 'w') as f:
			if len(warnContracts) > 0:
				f.write(" ".join(warnContracts))
			else:
				f.write("")
	
	WarnChaningMonthForDeliveryDayFor2ndMc(warnContractsForDeliveryDay)
	with open(deliveryDayWarnFile2nd, 'w') as f:
		f.write(" ".join(warnContractsForDeliveryDay))
	
########################
# 为medi更新3个交易所的订阅主力合约文件。
# 因为夜盘有些品种不交易，因此夜盘不生成行情订阅文件
#
########################
def UpdateSubscribedMcForMedi(isNight):	
	if isNight=="1":	# 因为夜盘有些品种不交易，因此夜盘不生成行情订阅文件
		return
		
	shfeCanditateContracts = []
	shfe_mc_filename = os.path.join(GetTargetDir(isNight), shfeContractsFile)
	with open(shfe_mc_filename) as f:
		reader = csv.DictReader(f)		
		for row in reader:								
			shfeCanditateContracts.append(row["r1"])
	if len(shfeCanditateContracts) > 0:
		with open(mediShfeSubcribeMcFile, 'w') as f:
			f.write(" ".join(shfeCanditateContracts))
		
	dce_mc_filename = os.path.join(GetTargetDir(isNight), dceContractsFile)
	dceCanditateContracts = []
	with open(dce_mc_filename) as f:
		reader = csv.DictReader(f)		
		for row in reader:								
			dceCanditateContracts.append(row["r1"])
	if len(dceCanditateContracts) > 0:
		with open(mediDceSubcribeMcFile, 'w') as f:
			f.write(" ".join(dceCanditateContracts))
		
	zce_mc_filename = os.path.join(GetTargetDir(isNight), zceContractsFile)
	zceCanditateContracts = []
	with open(zce_mc_filename) as f:
		reader = csv.DictReader(f)		
		for row in reader:								
			zceCanditateContracts.append(row["r1"])
	if len(zceCanditateContracts) > 0:
		with open(mediZceSubcribeMcFile, 'w') as f:
			f.write(" ".join(zceCanditateContracts))

##################
#	判断指定的合约是否是订阅的品种。
#	如果是订阅的品种，返回true; 否则，返回false
##################
def IsSubscribedVariety(contract, varietyFile):	
	varietyOfContract = contract.translate(None, digits)	
	subscribedVarieties = []
	with open(varietyFile) as f:
		line = f.readline().rstrip("\n")
		subscribedVarieties = line.split(" ")
		#logging.info("subscribedVarieties:{0}".format(subscribedVarieties))
	
	return varietyOfContract in subscribedVarieties
	
def UpdateSubscribedMcForYaoImp(varietiesFile, subcribedContractFile):		
	mclist = []
		
	with open(mcFile) as f:
		reader = csv.DictReader(f)		
		for row in reader:								
			contract = row["r1"]
			if IsSubscribedVariety(contract, varietiesFile):			
				mclist.append(contract)			
			# TODO: here				
			r2contract = row["r2"]
			if IsSubscribedVariety(r2contract, varietiesFile):			
				mclist.append(r2contract)

			close1 = row["close1"]
			if len(close1) > 0 and IsSubscribedVariety(close1, varietiesFile):			
				mclist.append(close1)

			close2 = row["close2"]
			if len(close2) > 0 and IsSubscribedVariety(close2, varietiesFile):			
				mclist.append(close2)
				
	with open(subcribedContractFile, 'w') as f:
		f.write(" ".join(mclist))
			
########################
# 	为yao更新3个交易所订阅的主力合约文件:	
#	yao-zce-subcribed-mc.csv
#	yao-shfe-subcribed-mc.csv
#	yao-dce-subcribed-mc.csv
#	是根据yao实盘使用的主力合约(contracts.csv)分解出的每个交易
#	所的主力合约，再加上mc-warm.csv的新主力合约，这些主力
#	合约会上传到各自，这些主力合约会上传到各自交易所的服务
#	器，替换原来的主力合约文件。
#
########################
def UpdateSubscribedMcForYao():	
	UpdateSubscribedMcForYaoImp("tools/shfe-varieties.txt", yaoShfeSubcribeMcFile)
	UpdateSubscribedMcForYaoImp("tools/dce-varieties.txt", yaoDceSubcribeMcFile)
	UpdateSubscribedMcForYaoImp("tools/zce-varieties.txt", yaoZceSubcribeMcFile)
	
		
		
if __name__=="__main__":   
	main()

#country_str = ET.tostring(countrys[0])
#new_country = ET.XML(country_str)
#root.append(new_country)
#
