#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <boost/atomic.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/progress.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include <cstdint>
#include <iostream>
#include <chrono>
#include "yao_quote.h"

char *header = ",feed_type,symbol,exchange,int_time,pre_close_px,pre_settle_px,pre_open_interest,open_interest,open_px,high_px,low_px,avg_px,last_px,bp1,bp2,bp3,bp4,bp5,ap1,ap2,ap3,ap4,ap5,bv1,bv2,bv3,bv4,bv5,av1,av2,av3,av4,av5,total_vol,total_notional,upper_limit_px,lower_limit_px,close_px,settle_px,implied_bid_size1,implied_bid_size2,implied_bid_size3,implied_bid_size4,implied_bid_size5,implied_ask_size1,implied_ask_size2,implied_ask_size3,implied_ask_size4,implied_ask_size5,total_buy_ordsize,total_sell_ordsize,weighted_buy_px,weighted_sell_px";

using namespace std;

int main(int argc, const char **argv)
{	
	std::cout << "Usage: Dat1Csv BinaryQuoteDataFile" << std::endl;
	if (argc < 2)
	{
		std::cout << "miss parameters" << std::endl;
	}

	std::string file_name = argv[1];
	std::size_t real_count = 0;
	std::size_t calc_count = 0;

	struct stat64 info;
	stat64(file_name.c_str(), &info);
	long long file_size = info.st_size;
	long long handled_size = 0;

	ifstream f_in(file_name.c_str(), std::ios_base::binary);
	SaveFileHeaderStruct f_header;
	f_header.data_count = 0;
	f_header.data_length = 0;
	f_header.data_type = 0;

	f_in.read((char *)&f_header, sizeof(f_header));

	// 错误处理
	if (!f_in)
	{
		std::cout << file_name << " open failed. " << std::endl;
		return 1;
	}

	// 错误处理
	if (f_header.data_length == 0)
	{
		std::cout << file_name << " read header failed. " << std::endl;
		return 1;
	}

	calc_count = (std::size_t)((file_size - sizeof(f_header))/f_header.data_length);
	
	// 错误处理
	if (file_size != (calc_count * f_header.data_length + sizeof(f_header))
		&& f_header.data_count == 0)
	{
		std::cout << "WARNING: there are incomplete data item in " << file_name  << std::endl;
	}

	if (f_header.data_count == 0 || f_header.data_count == calc_count)
	{
		std::cout << file_name << " total count is: " << calc_count << std::endl;
	}
	else
	{
		std::cout << file_name << " f_header.data_count is: " << f_header.data_count 
			<< "; calc_count is: " << calc_count << std::endl;
	}

	// output file
	std::string quote_str_file = file_name;
	quote_str_file = quote_str_file.substr(quote_str_file.rfind("_") + 1);
	boost::replace_last(quote_str_file, ".dat", ".csv");
	ofstream f_out(quote_str_file.c_str(), std::ios_base::out | std::ios_base::app);

	// 修改头中的数据条数field
	if (f_header.data_count == 0)
	{
		FILE *f_mod;
		f_mod = fopen(file_name.c_str(), "rb+");
		if (f_mod)
		{
			std::cout <<  ftell(f_mod) << std::endl;
			fseek(f_mod, 0, SEEK_SET);
			std::cout <<  ftell(f_mod) << std::endl;
			rewind(f_mod);
			std::cout <<  ftell(f_mod) << std::endl;
			fwrite(&calc_count, sizeof(calc_count), 1, f_mod);
			fclose(f_mod);

			std::cout << "WARNING: data count field correct success. " << std::endl;
		}
		else
		{
			std::cout << "WARNING: data count field correct failed. " << std::endl;
		}

		f_header.data_count = calc_count;
	}
	
	// 进度显示
	std::cout << "begin convert, waiting..." << std::endl;

	boost::progress_display pd(f_header.data_count);

	f_out << header << std::endl;

	//pd += sizeof(f_header);
	while (true)
	{
		for (int i = 0; i < f_header.data_count; ++i)
		{
			switch (f_header.data_type)
			{
				case YAO_QUOTE_TYPE:
				{
					SaveData_YaoQuote t;
					f_in.read((char *)&t, sizeof(t));
					f_out << YaoQuoteToString(i, &t) << std::endl;
					break;
				}
			default:
				{
					std::cout << "can't handle quote data of type: " 
						<< f_header.data_type << std::endl;
					break;
				}
			}

			pd += 1;
			++real_count;
		}
		handled_size += ((long long)f_header.data_count * f_header.data_length + sizeof(f_header));

		if (handled_size + sizeof(f_header) < file_size)
		{
			f_in.read((char *)&f_header, sizeof(f_header));
			//pd += sizeof(f_header);
		}
		else
		{
			break;
		}

		if (f_header.data_count == 0) break;
	}

	f_in.close();
	f_out.close();
	std::cout << std::endl << file_name 
		<< " process success, convert count is: " 
		<< real_count << std::endl;
	return 0;
}
