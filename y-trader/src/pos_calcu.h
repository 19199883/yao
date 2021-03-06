// done
#ifndef POS_CALC_H 
#define POS_CALC_H

#include <tinyxml.h>
#include <tinystr.h>
#include <tinyxml.h>
#include <tinystr.h>
#include <map>
#include <list>
#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::lock_guard
#include <iostream>     // std::cin, std::cout
#include <string>
#include <vector>
#include <fstream>      // std::ifstream
#include "vrt_value_obj.h"

using namespace std;

class pos_calc
{
	public:

		/*
		 * 获取指定合约的仓位
		 * stra: strategy name
		 * cont: contract
		 * yLong: yesterday long position
		 * yShort: yesterday short position
		 * tLong: today long position
		 * tShort: today short position
		 *
		 */
		static void get_pos(string &strategy, const char* contract, int &yLong, int &yShort, 
					int &tLong, int &tShort)
		{
			yLong = 0;
			yShort = 0; 
			tLong = 0;
			tShort = 0; 

			string pos_file = POSITION_FILE;
			char buf[1024];
			string line = "";
			int cur_pos = 0;
			int next_pos = 0;

			std::ifstream is;
			is.open (pos_file);
			while(is.good()) 
			{
				is.getline(buf, sizeof(buf));
				line = buf;
				cur_pos = 0;
				next_pos = 0;
				// contract
				next_pos = line.find(';', cur_pos);
				char *cur_contract = (char*)line.substr(cur_pos, next_pos-cur_pos).c_str();
				if(strcmp(contract, cur_contract)==0)
				{
					// yesterday long position
					cur_pos = next_pos + 1;
					next_pos = line.find(';', cur_pos);
					yLong = stoi(line.substr(cur_pos, next_pos-cur_pos));
					// yesterday short position
					cur_pos = next_pos + 1;
					next_pos = line.find(';', cur_pos);
					yShort = stoi(line.substr(cur_pos, next_pos - cur_pos));

					// today long position
					cur_pos = next_pos + 1;
					next_pos = line.find(';', cur_pos);
					tLong = stoi(line.substr(cur_pos, next_pos-cur_pos));

					// today short position
					cur_pos = next_pos + 1;
					next_pos = line.find(';', cur_pos);
					tShort = stoi(line.substr(cur_pos, next_pos - cur_pos));
					break;
				}
			}
			is.close();
		}
};

#endif /* POS_CALC_H */
