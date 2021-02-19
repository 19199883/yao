#pragma once

#include <stdio.h>
#include "quote_datatype_shfe_my.h"

class YaoQuoteHelper
{
	public:
	/*
	 * 将给定的由字符串表示的时间和毫秒所表示的时间转换成整形.
	 * UpdateTime: 23:12:01
	 * UpdateMillisec:500
	 * return value:231201500
	 */
	static int GetIntTime(const char *UpdateTime, int UpdateMillisec)
	{
		char buffer[30];
		strncpy(buffer, UpdateTime, 2);	// hour
		strncpy(buffer+2, UpdateTime+3, 2);		// min
		strncpy(buffer+4, UpdateTime+6, 2);		// sec
		sprintf(buffer+6, "%03d", UpdateMillisec);
		buffer[9] = 0;

		int int_time = atoi(buffer);
		if(int_time < 40000000)
		{
			int_time += 240000000;
		}

		return int_time;

	}

	static void Convert(YaoQuote *dest, const CThostFtdcDepthMarketDataField* source)
	{
		dest->feed_type = FeedTypes::ShfeLevel2;
		strcpy(dest->symbol, source->InstrumentID);		/*合约编码*/
		dest->exchange = source->ExchangeID[0];
		dest->int_time = GetIntTime(source->UpdateTime, source-> UpdateMillisec);
		dest->pre_close_px =		InvalidToZeroD(source->PreClosePrice);	/*前收盘价格*/
		dest->pre_settle_px =	InvalidToZeroD(source->PreSettlementPrice);	/*前结算价格*/
		dest->pre_open_interest =	  (int)source->PreOpenInterest;	/*previous days's positions */
		dest->open_interest =		  (int)source->OpenInterest;	/*持仓量*/
		dest->open_px =			InvalidToZeroD(source->OpenPrice);	/*开盘价*/
		dest->high_px =			InvalidToZeroD(source->HighestPrice);	    /*最高价*/
		dest->low_px =			InvalidToZeroD(source->LowestPrice);	        /*最低价*/
		dest->avg_px =			0; // InvalidToZeroD(source->AveragePrice);	/*均价*/
		dest->last_px =		InvalidToZeroD(source->LastPrice);				/*最新价*/

		dest->bp_array[0] = InvalidToZeroD(source->BidPrice1);     /*买入价格 下标从0开始*/
		dest->bp_array[1] = InvalidToZeroD(source->BidPrice2);     /*买入价格 下标从0开始*/
		dest->bp_array[2] = InvalidToZeroD(source->BidPrice3);     /*买入价格 下标从0开始*/
		dest->bp_array[3] = InvalidToZeroD(source->BidPrice4);     /*买入价格 下标从0开始*/	
		dest->bp_array[4] = InvalidToZeroD(source->BidPrice5);     /*买入价格 下标从0开始*/

		dest->ap_array[0] = InvalidToZeroD(source->AskPrice1);     /*卖出价 下标从0开始*/
		dest->ap_array[1] = InvalidToZeroD(source->AskPrice2);     /*卖出价 下标从0开始*/
		dest->ap_array[2] = InvalidToZeroD(source->AskPrice3);     /*卖出价 下标从0开始*/
		dest->ap_array[3] = InvalidToZeroD(source->AskPrice4);     /*卖出价 下标从0开始*/
		dest->ap_array[4] = InvalidToZeroD(source->AskPrice5);     /*卖出价 下标从0开始*/

		dest->bv_array[0] = source->BidVolume1;          /*买入数量 下标从0开始*/
		dest->bv_array[1] = source->BidVolume2;          /*买入数量 下标从0开始*/
		dest->bv_array[2] = source->BidVolume3;          /*买入数量 下标从0开始*/
		dest->bv_array[3] = source->BidVolume4;          /*买入数量 下标从0开始*/
		dest->bv_array[4] = source->BidVolume5;          /*买入数量 下标从0开始*/

		dest->av_array[0] = source->AskVolume1;          /*卖出数量 下标从0开始*/
		dest->av_array[1] = source->AskVolume2;          /*卖出数量 下标从0开始*/
		dest->av_array[2] = source->AskVolume3;          /*卖出数量 下标从0开始*/
		dest->av_array[3] = source->AskVolume4;          /*卖出数量 下标从0开始*/
		dest->av_array[4] = source->AskVolume5;          /*卖出数量 下标从0开始*/

		dest->total_vol =				source->Volume; //	总成交量
		dest->total_notional =	InvalidToZeroD(source->Turnover); // 存储成交金额(StdQuote5.turnover)
		dest->upper_limit_px =	InvalidToZeroD(source->UpperLimitPrice);	/*涨停板*/
		dest->lower_limit_px =	InvalidToZeroD(source->LowerLimitPrice);	/*跌停板*/
		dest->close_px =			InvalidToZeroD(source->ClosePrice);	    /*收盘价*/
		dest->settle_px =		InvalidToZeroD(source->SettlementPrice);	/*结算价*/


		dest->total_buy_ordsize  = 0;	/*委买总量*/ 
		dest->total_sell_ordsize = 0;	/*委卖总量*/ 

		dest->weighted_buy_px =	InvalidToZeroD(0.0);	    /*平均买价*/
		dest->weighted_sell_px =	InvalidToZeroD(0.0);	/*平均卖价*/

		// d->st.implied_bid_size // yao 不需要
		//dest.implied_ask_size  // yao 不需要
	}
};
