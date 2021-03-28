#include <thread>         // std::thread
#include "quote_interface_shfe_my.h"
#include "yao_utils.h"
#include "ctp_data_formater.h"

MYQuoteData::MYQuoteData(EfhLev2Producer *efhLev2_producer, L1MDProducer *l1_md_producer)
	: efhLev2Producer_(efhLev2_producer),
	l1_md_producer_(l1_md_producer),
	module_name_("MYQuoteData")
{
	l1_md_last_index_ = L1MD_NPOS;

#ifdef PERSISTENCE_ENABLED 
    p_shfe_lev2_data_save_ = new QuoteDataSave<YaoQuote>( "y-shfequote", YAO_QUOTE_TYPE);
#endif

}

MYQuoteData::~MYQuoteData()
{

#ifdef PERSISTENCE_ENABLED 
    if (p_shfe_lev2_data_save_) delete p_shfe_lev2_data_save_;
#endif

	clog_warning("[%s] ~MYQuoteData invoked.", module_name_);
}

void MYQuoteData::CopyLev1ToLev2(CThostFtdcDepthMarketDataField* my_data, efh3_lev2* efh_data )
{
		// from level1
		my_data->UpperLimitPrice =	  InvalidToZeroD(my_data->UpperLimitPrice);
		my_data->LowerLimitPrice =	  InvalidToZeroD(my_data->LowerLimitPrice);
		my_data->HighestPrice =		  InvalidToZeroD(my_data->HighestPrice);
		my_data->LowestPrice =		  InvalidToZeroD(my_data->LowestPrice);
		my_data->OpenPrice =		  InvalidToZeroD(my_data->OpenPrice);
		my_data->ClosePrice =		  InvalidToZeroD(my_data->ClosePrice);
		my_data->PreClosePrice =	  InvalidToZeroD(my_data->PreClosePrice);			
		my_data->PreOpenInterest =	  InvalidToZeroD(my_data->PreOpenInterest);
		my_data->SettlementPrice =	  InvalidToZeroD(my_data->SettlementPrice);
		my_data->PreSettlementPrice = InvalidToZeroD(my_data->PreSettlementPrice);			
		//my_data->PreDelta =			  InvalidToZeroD(my_data->PreDelta);
		//my_data->CurrDelta =		  InvalidToZeroD(my_data->CurrDelta);

		// the below is from sfh_lev2
		if(2 ==  efh_data->m_exchange_id)
		{
			my_data->ExchangeID[0] = YaoExchanges::YINE;
		}
		else
		{
			my_data->ExchangeID[0] = YaoExchanges::YSHFE;
		}

		my_data->LastPrice =	InvalidToZeroD(efh_data->m_last_px);															
		my_data->Volume =					   efh_data->m_last_share;
		my_data->Turnover =     InvalidToZeroD(efh_data->m_turnover);				
		// TODO: 
		memset(my_data->UpdateTime, 0, sizeof(my_data->UpdateTime));
		sprintf (my_data->UpdateTime, 
					"%02hhu:%02hhu:%02hhu", 
					efh_data->m_update_time_h,
					efh_data->m_update_time_m,
					efh_data->m_update_time_s);
		my_data->UpdateMillisec = efh_data->m_millisecond;

		my_data->OpenInterest = InvalidToZeroD(efh_data->m_open_interest);	
		my_data->BidPrice1 =    InvalidToZeroD(efh_data->m_bid_1_px);
		my_data->BidPrice2 =    InvalidToZeroD(efh_data->m_bid_2_px);
		my_data->BidPrice3 =    InvalidToZeroD(efh_data->m_bid_3_px);
		my_data->BidPrice4 =    InvalidToZeroD(efh_data->m_bid_4_px);
		my_data->BidPrice5 =    InvalidToZeroD(efh_data->m_bid_5_px);
		
		my_data->BidVolume1 =				   efh_data->m_bid_1_share;
		my_data->BidVolume2 =				   efh_data->m_bid_2_share;
		my_data->BidVolume3 =				   efh_data->m_bid_3_share;
		my_data->BidVolume4 =				   efh_data->m_bid_4_share;
		my_data->BidVolume5 =				   efh_data->m_bid_5_share;
		
		my_data->AskPrice1 =    InvalidToZeroD(efh_data->m_ask_1_px);
		my_data->AskPrice2 =    InvalidToZeroD(efh_data->m_ask_2_px);
		my_data->AskPrice3 =    InvalidToZeroD(efh_data->m_ask_3_px);
		my_data->AskPrice4 =    InvalidToZeroD(efh_data->m_ask_4_px);
		my_data->AskPrice5 =    InvalidToZeroD(efh_data->m_ask_5_px);
		
		my_data->AskVolume1 =                  efh_data->m_ask_1_share;		
		my_data->AskVolume2 =                  efh_data->m_ask_2_share;
		my_data->AskVolume3 =                  efh_data->m_ask_3_share;		
		my_data->AskVolume4 =                  efh_data->m_ask_4_share;
		my_data->AskVolume5 =                  efh_data->m_ask_5_share;			

}

void MYQuoteData::ProcEfhLev2Data(int32_t index)
{
	efh3_lev2* efh_data = efhLev2Producer_->GetData(index);

	// TODO: commented for debug
	char buffer[2048];
	//clog_info("[%s] rev efh3_lev2:%s", 
	//			module_name_,
	//			EfhLev2Producer::Format(*efh_data, buffer));

	// discard option
	if(strlen(efh_data->m_symbol) > 6)
	{
		return;
	}

	if(!efhLev2Producer_->IsDominant(efh_data->m_symbol)) return;


	CThostFtdcDepthMarketDataField* my_data = NULL;
	if(l1_md_last_index_ != L1MD_NPOS)
	{
		 my_data =  l1_md_producer_->GetLastData(efh_data->m_symbol, l1_md_last_index_);
		if(NULL != my_data)
		{	
			CopyLev1ToLev2(my_data, efh_data);
			Convert(&yaoquote_, my_data);

			// TODO: log
			clog_info("[%s] rev ShfeYaoData:%s", 
					module_name_,
					YaoQuote::ToString(&yaoquote_).c_str());

			char buffer[5120];
	//		clog_info("[%s] send data:%s", 
	//					module_name_,
	//					ShfeLev2Formater::Format(*my_data,buffer));

			if (lev2_data_handler_ != NULL) { lev2_data_handler_(&yaoquote_); }

#ifdef PERSISTENCE_ENABLED 
			timeval t;
			gettimeofday(&t, NULL);
			p_shfe_lev2_data_save_->OnQuoteData(t.tv_sec * 1000000 + t.tv_usec, &yaoquote_);
#endif
		}
		else
		{
			CThostFtdcDepthMarketDataField my_data;
			memset(&my_data, 0, sizeof(CThostFtdcDepthMarketDataField));
			strcpy(my_data.InstrumentID, efh_data->m_symbol);

			CopyLev1ToLev2(&my_data, efh_data);
			Convert(&yaoquote_, &my_data);

#ifdef PERSISTENCE_ENABLED 
			timeval t;
			gettimeofday(&t, NULL);
			p_shfe_lev2_data_save_->OnQuoteData(t.tv_sec * 1000000 + t.tv_usec, &yaoquote_);

			// TODO: log
			char buffer[5120];
	//		clog_info("[%s] new contract data:%s", 
	//					module_name_,
	//					ShfeLev2Formater::Format(my_data, buffer));
#endif
			clog_warning("[%s] can not find lev1 for:%s",module_name_, efh_data->m_symbol);
		}
	}
}

	void MYQuoteData::Convert(YaoQuote *dest, const CThostFtdcDepthMarketDataField* source)
	{
		dest->feed_type = FeedTypes::ShfeLevel2;
		strcpy(dest->symbol, source->InstrumentID);		/*合约编码*/
		dest->exchange = source->ExchangeID[0];
		dest->int_time = YaoQuoteHelper::GetIntTime(source->UpdateTime, source-> UpdateMillisec);
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

void MYQuoteData::SetQuoteDataHandler(std::function<void(YaoQuote*)> quote_handler)
{
	clog_warning("[%s] SetQuoteDataHandler invoked.", module_name_);
	lev2_data_handler_ = quote_handler;
}

void MYQuoteData::ProcL1MdData(int32_t index)
{
	l1_md_last_index_ = index;
}

