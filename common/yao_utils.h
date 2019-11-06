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

	static void Convert(YaoQuote *dest, const MYShfeMarketData* source)
	{
		dest->feed_type = FeedTypes::ShfeLevel2;
		strcpy(dest->symbol, source->InstrumentID);		/*合约编码*/
		dest->exchange = YaoExchanges::YSHFE;
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

		dest->bp_array[0] = InvalidToZeroD(source->buy_price[29]);     /*买入价格 下标从0开始*/
		dest->bp_array[1] = InvalidToZeroD(source->buy_price[28]);     /*买入价格 下标从0开始*/
		dest->bp_array[2] = InvalidToZeroD(source->buy_price[27]);     /*买入价格 下标从0开始*/
		dest->bp_array[3] = InvalidToZeroD(source->buy_price[26]);     /*买入价格 下标从0开始*/	
		dest->bp_array[4] = InvalidToZeroD(source->buy_price[25]);     /*买入价格 下标从0开始*/

		dest->ap_array[0] = InvalidToZeroD(source->sell_price[29]);     /*卖出价 下标从0开始*/
		dest->ap_array[1] = InvalidToZeroD(source->sell_price[28]);     /*卖出价 下标从0开始*/
		dest->ap_array[2] = InvalidToZeroD(source->sell_price[27]);     /*卖出价 下标从0开始*/
		dest->ap_array[3] = InvalidToZeroD(source->sell_price[26]);     /*卖出价 下标从0开始*/
		dest->ap_array[4] = InvalidToZeroD(source->sell_price[25]);     /*卖出价 下标从0开始*/

		dest->bv_array[0] = source->buy_volume[29];          /*买入数量 下标从0开始*/
		dest->bv_array[1] = source->buy_volume[28];          /*买入数量 下标从0开始*/
		dest->bv_array[2] = source->buy_volume[27];          /*买入数量 下标从0开始*/
		dest->bv_array[3] = source->buy_volume[26];          /*买入数量 下标从0开始*/
		dest->bv_array[4] = source->buy_volume[25];          /*买入数量 下标从0开始*/

		dest->av_array[0] = source->sell_volume[29];          /*卖出数量 下标从0开始*/
		dest->av_array[1] = source->sell_volume[28];          /*卖出数量 下标从0开始*/
		dest->av_array[2] = source->sell_volume[27];          /*卖出数量 下标从0开始*/
		dest->av_array[3] = source->sell_volume[26];          /*卖出数量 下标从0开始*/
		dest->av_array[4] = source->sell_volume[25];          /*卖出数量 下标从0开始*/

		dest->total_vol =				source->Volume; //	总成交量
		dest->total_notional =	InvalidToZeroD(source->Turnover); // 存储成交金额(StdQuote5.turnover)
		dest->upper_limit_px =	InvalidToZeroD(source->UpperLimitPrice);	/*涨停板*/
		dest->lower_limit_px =	InvalidToZeroD(source->LowerLimitPrice);	/*跌停板*/
		dest->close_px =			InvalidToZeroD(source->ClosePrice);	    /*收盘价*/
		dest->settle_px =		InvalidToZeroD(source->SettlementPrice);	/*结算价*/


		dest->total_buy_ordsize =  (int)source->buy_total_volume;	/*委买总量*/
		dest->total_sell_ordsize = (int)source->sell_total_volume;	/*委卖总量*/

		dest->weighted_buy_px =	InvalidToZeroD(source->buy_weighted_avg_price);	    /*平均买价*/
		dest->weighted_sell_px =	InvalidToZeroD(source->sell_weighted_avg_price);	/*平均卖价*/

		// d->st.implied_bid_size // yao 不需要
		//dest.implied_ask_size  // yao 不需要
	}
};
