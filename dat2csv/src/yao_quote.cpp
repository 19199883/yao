#include "yao_quote.h"
#include <string>

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

					",feed_type,symbol,exchange,int_time,pre_close_px,pre_settle_px,pre_open_interest,open_interest,open_px,high_px,low_px,avg_px,last_px,bp1,bp2,bp3,bp4,bp5,ap1,ap2,ap3,ap4,ap5,bv1,bv2,bv3,bv4,bv5,av1,av2,av3,av4,av5,total_vol,total_notional,upper_limit_px,lower_limit_px,close_px,settle_px,implied_bid_size1,implied_bid_size2,implied_bid_size3,implied_bid_size4,implied_bid_size5,implied_ask_size1,implied_ask_size2,implied_ask_size3,implied_ask_size4,implied_ask_size5,total_buy_ordsize,total_sell_ordsize,weighted_buy_px,weighted_sell_px",
std::string YaoQuoteToString(const SaveData_YaoQuote* const p_data)
{
	if (!p_data)
	{
		return "";
	}

	const YaoQuote * const p = &(p_data->data_);	
	char buf[5120];
	if (p) 
	{
		sprintf(buf, 
					"%d,%d,%s,%d,%d,"
					"%.4f,%.4f,%.4f,%.4f,"
					"%.4f,%.4f,"
					,
					serial_number, (int)p->feed_type, p->symbol, p->exchange, p->int_time,         
					p->pre_close_px, p->pre_settle_px, p->pre_open_interest, p->open_interest,         
					p->open_px, p->high_px,      
			p->low_px,     
			p->avg_px,       
			p->last_px,  

			p->bp_array[0],                
			p->bp_array[1],                
			p->bp_array[2],                
			p->bp_array[3],                
			p->bp_array[4],                

			p->ap_array[0],                
			p->ap_array[1],                
			p->ap_array[2],                
			p->ap_array[3],                
			p->ap_array[4],                

			p->bv_array[0],              
			p->bv_array[1],              
			p->bv_array[2],              
			p->bv_array[3],              
			p->bv_array[4],              

			p->av_array[0],              
			p->av_array[1],              
			p->av_array[2],              
			p->av_array[3],              
			p->av_array[4], 

			p->total_vol,	
			p->total_notional,     
			p->upper_limit_px,         
			p->lower_limit_px,          
			p->close_px,        
			p->settle_px,         
			p->total_buy_ordsize,      
			p->total_sell_ordsize,      





		);
	}
	
	
	return buf;
}

