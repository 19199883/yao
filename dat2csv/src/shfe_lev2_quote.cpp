#include "shfe_lev2_quote.h"
#include <string>

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

std::string ShfeLev2QuoteToString(int serial_number, const SaveData_ShfeLev2Data* const p_data)
{
	if (!p_data)
	{
		return "";
	}

	const CThostFtdcDepthMarketDataField * const p = &(p_data->data_);	
	char buf[5120];
	if (p) 
	{
		sprintf(buf, 
					"%d,%d,%s,%s,%s.%d,"
					"%.4f,%.4f,%.4f,%.4f,"
					"%.4f,%.4f,%.4f,%.4f,%.4f,"
					"%.4f,%.4f,%.4f,%.4f,%.4f,"
					"%.4f,%.4f,%.4f,%.4f,%.4f,"
					"%d,%d,%d,%d,%d,"
					"%d,%d,%d,%d,%d,"
					"%d,%.4f,%.4f,%.4f,"
					"%.4f,%.4f,"
					"0,0,0,0,0," // implied_bid_size
					"0,0,0,0,0," // implied_ask_size
					"%d,%d,%.4f,%.4f"
					,
					serial_number, 6, p->InstrumentID, p->ExchangeID, p->UpdateTime, p->UpdateMillisec,
					p->PreClosePrice, p->PreSettlementPrice, p->PreOpenInterest, p->OpenInterest,         
					p->OpenPrice, p->HighestPrice, p->LowestPrice, p->AveragePrice, p->LastPrice,  
					p->BidPrice1, p->BidPrice2, p->BidPrice3, p->BidPrice4, p->BidPrice5, 
					p->AskPrice1, p->AskPrice2, p->AskPrice3, p->AskPrice4, p->AskPrice5, 
					p->BidVolume1, p->BidVolume2, p->BidVolume3, p->BidVolume4, p->BidVolume5, 
					p->AskVolume1, p->AskVolume2, p->AskVolume3, p->AskVolume4, p->AskVolume5, 
					p->Volume, p->Turnover, p->UpperLimitPrice, p->LowerLimitPrice,          
					p->ClosePrice, p->SettlementPrice,         
					0, 0, 0, 0);
	}
	
	
	return buf;
}

