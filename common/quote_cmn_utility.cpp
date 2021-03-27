﻿#include <fstream>
#include <string.h>
#include <stdio.h>
#include "quote_cmn_utility.h"

using namespace std;

IPAndPortNum ParseIPAndPortNum(const std::string &addr_cfg)
{
    //format: udp://192.168.60.23:7120   or  tcp://192.168.60.23:7120
    std::string ip_port = addr_cfg.substr(6);
    std::size_t split_pos = ip_port.find(":");
    if ((split_pos == std::string::npos) || (split_pos + 1 >= ip_port.length())){
        return std::make_pair("", (unsigned short) 0);
    }

    std::string addr_ip = ip_port.substr(0, split_pos);
    std::string addr_port = ip_port.substr(split_pos + 1);
    int port_tmp = atoi(addr_port.c_str());
    if (port_tmp <= 0 || port_tmp > 0xFFFF){
        return std::make_pair("", 0);
    }

    return std::make_pair(addr_ip, (unsigned short) port_tmp);
}

IPAndPortStr ParseIPAndPortStr(const std::string &addr_cfg)
{
    //format: udp://192.168.60.23:7120   or  tcp://192.168.60.23:7120
    std::string ip_port = addr_cfg.substr(6);
    std::size_t split_pos = ip_port.find(":");
    if ((split_pos == std::string::npos) || (split_pos + 1 >= ip_port.length())){
        return std::make_pair("", "");
    }

    std::string addr_ip = ip_port.substr(0, split_pos);
    std::string addr_port = ip_port.substr(split_pos + 1);
    int port_tmp = atoi(addr_port.c_str());
    if (port_tmp <= 0 || port_tmp > 0xFFFF){
    }

    return std::make_pair(addr_ip, addr_port);
}


char* get_curtime(char buffer[],int size)
{
	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime (buffer,size,"%H:%M:%S",timeinfo);

	return buffer;
}

int32_t LoadDominantContracts(string file, char buffer[][10])
{
	int32_t count = 0;

	std::ifstream is;
	is.open (file);
	string contrs = "";
	if (is) {
		getline(is, contrs);
		contrs += " ";
		size_t start_pos = 0;
		size_t end_pos = 0;
		string contr = "";
		while ((end_pos=contrs.find(" ",start_pos)) != string::npos){
			contr = contrs.substr (start_pos, end_pos-start_pos);
			strcpy(buffer[count], contr.c_str());
			
			printf("LoadDominantContracts:dominant contract:%s",contr.c_str());

			start_pos = end_pos + 1;
			count++;
		}
	}
	else 
	{ 
		printf("LoadDominantContracts: can't open: %s", file.c_str()); 
	}

	return count;
}

bool IsEmptyString(char *str)
{
	if(0 == str[0]){
		return true;
	}else{
		return false;
	}
}

bool IsEqualContract(char *contract1, char* contract2)
{
	if (strcmp(contract1, contract2) == 0)
	{
		return true;
	}else{
		return false;
	}
}

/////////////////////the following is for zce/////////////

/*
 *contract: SR801
 *buffer[0].contract:SR801
 *
 */
bool IsDominantSize3Imp(const char* contract, char buffer[][10], 
	int32_t buffer_size)
{
	bool is_dominant = false;

	for(int i = 0; i < buffer_size; i++)
	{
		if(strcmp(buffer[i], contract) == 0)
		{
			is_dominant = true;
			break;
		}
	}

	return is_dominant;
}


/*
 *contract: SR1801
 *buffer[0].contract:SR801
 *
 */
bool IsDominantSize4Imp(const char* contract, char buffer[][10], 
	int32_t buffer_size)
{
	bool is_dominant = false;

	for(int i = 0; i < buffer_size; i++)
	{
		if(IsSize3EqualSize4(buffer[i], contract))
		{
			is_dominant = true;
			break;
		}
	}

	return is_dominant;
}


bool IsSize3EqualSize4(const char *contract_size3, 
			const char* contract_size4)
{
	// contract:e.g. SR1801
	if(strncmp(contract_size3, contract_size4, 2) == 0 && 
		strncmp(contract_size3 + 2, contract_size4 + 3, 3) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

