﻿#include <thread>         // std::thread
#include "quote_interface_ine_my.h"
#include "yao_utils.h"

std::string MYIneQuoteData::ToString(const MYShfeMarketData &d) {
//	clog_debug("MYShfeMarketData: instrument:%s, "
//				"data_flag:%d,buy_total_volume:%d; "
//				"sell_total_volume:%d; buy_weighted_avg_price:%lf; "
//				"sell_weighted_avg_price:%lf",
//				d.InstrumentID, 
//				d.data_flag, 
//				d.buy_total_volume,
//				d.sell_total_volume,
//				d.buy_weighted_avg_price,
//				d.sell_weighted_avg_price);
//
	//clog_debug("dir:buy; price, volume");
	for(int i = 0; i < 30; i++) 
	{
		// clog_debug("price%d: %lf, volume%d: %d\n", 
		//			 i, 
		//			 d.buy_price[i], 
		//			 i, 
		//			 d.buy_volume[i]);
	}

	//clog_debug("dir:sell; price, volume");
	for(int i = 0; i < 30; i++) 
	{
		// clog_debug("price%d: %lf, volume%d: %d\n", 
		//			 i, 
		//			 d.sell_price[i], 
		//			 i, 
		//			 d.sell_volume[i]);
	}
  
  return "";
}

MYIneQuoteData::MYIneQuoteData(ShfeFullDepthMDProducer *fulldepth_md_producer, 
			ShfeL1MDProducer *l1_md_producer)
	: fulldepth_md_producer_(fulldepth_md_producer), 
	l1_md_producer_(l1_md_producer),
	module_name_("MYIneQuoteData"), 
	seq_no_(0), 
	server_(0)
{
	l1_md_last_index_ = L1MD_NPOS;

	for (int i=0; i<10; i++)
	{
		repairers_[i] = new repairer(fulldepth_md_producer_ );
		repairers_[i]->server_ = i;
	}
}

MYIneQuoteData::~MYIneQuoteData()
{
	for (int i=0; i<10; i++)
	{
		delete repairers_[i];
	}

	clog_warning("[%s] ~MYIneQuoteData invoked.", module_name_);
}

void MYIneQuoteData::ProcIneFullDepthData(int32_t index)
{
	MDPackEx* md = fulldepth_md_producer_->GetData(index);
	int new_svr = md->content.seqno % 10;
	if (new_svr != server_) 
	{ 
		//clog_debug("[%s] server from %d to %d",module_name_, server_, new_svr); 
	}

	repairers_[new_svr]->rev(index);

	//clog_info("[%s] proc sn:%d",module_name_, md->content.seqno); 

	bool empty = true;
	char cur_contract[10];
	strcpy(cur_contract,"");
	char new_contract[10];
	strcpy(new_contract,"");
	Reset();
	MDPackEx* data = repairers_[new_svr]->next(empty);
	while (!empty) 
	{ 
		// empty contract
		if(IsEmptyString(cur_contract))
		{ // 为空，表示第一次进入循环
			strcpy(cur_contract,data->content.instrument);
		}
		strcpy(new_contract,data->content.instrument);

		if(!IsEqualContract(cur_contract,new_contract))
		{
			FillFullDepthInfo();
			Send(cur_contract);
			Reset();
		}
		
		// 别放到买卖对用缓冲，待该合约的数据都接受完后，统一处理
		if(data->content.direction == SHFE_FTDC_D_Buy)
		{
			buy_data_cursor_ = buy_data_cursor_ + 1;
			buy_data_buffer_[buy_data_cursor_] = data;
		}
		else if(data->content.direction == SHFE_FTDC_D_Sell)
		{
			sell_data_cursor_ = sell_data_cursor_ + 1;
			sell_data_buffer_[sell_data_cursor_] = data;
		}
		strcpy(cur_contract,data->content.instrument);

		data = repairers_[new_svr]->next(empty);
		if(empty)
		{ 
			FillFullDepthInfo();
			Send(cur_contract); 
			Reset();
		}
	}

	server_ = new_svr;
}

void MYIneQuoteData::FillBuyFullDepthInfo()
{
    target_data_.buy_total_volume = 0;
    target_data_.buy_weighted_avg_price = 0;
    double amount = 0;

	bool damaged = false;
	// VPair数据计数器，用于计数从尾部开始最多30笔数据，用于复制盘口30档数据用
	int price30_count = MY_SHFE_QUOTE_PRICE_POS_COUNT - 1; 
	double price30[MY_SHFE_QUOTE_PRICE_POS_COUNT] = {0};
	int vol30[MY_SHFE_QUOTE_PRICE_POS_COUNT] = {0};
	for(int i=buy_data_cursor_; i>=0; i--)
	{ // 从尾部向前遍历MDPackEx数据 
		MDPackEx *src_mdpackex = buy_data_buffer_[i];
		if (src_mdpackex->damaged) damaged = true;

		for(int j=src_mdpackex->content.count-1; j>=0; j--)
		{ //从尾部向前遍历PVPair数据 
			PVPair &src_pvpaire = src_mdpackex->content.data[j];

			// 处理30档买方向数据
			if(price30_count >= 0)
			{
				price30[price30_count] = src_pvpaire.price;  
				vol30[price30_count] = src_pvpaire.volume;  
				price30_count = price30_count - 1;
			}

			// 计算总委买量
			target_data_.buy_total_volume += src_pvpaire.volume;
			amount += src_pvpaire.price * src_pvpaire.volume;
		} // for(int j=buy_data_buffer_[i].content.count-1; j>=0; j--)//从尾部向前遍历PVPair数据 
	} // for(int i=buy_data_cursor_; i>=0; i--) // 从尾部向前遍历MDPackEx数据 
	
	// 计算均价
	if(damaged) target_data_.buy_total_volume = 0;
	if (target_data_.buy_total_volume > 0)
	{
		target_data_.buy_weighted_avg_price = amount / target_data_.buy_total_volume;
	}
	// 拷贝盘口30档买方向数据
	memcpy(target_data_.buy_volume, vol30, sizeof(vol30));
	memcpy(target_data_.buy_price, price30, sizeof(price30));
}

void MYIneQuoteData::FillSellFullDepthInfo()
{
    target_data_.sell_total_volume = 0;
    target_data_.sell_weighted_avg_price = 0;
    double amount = 0;

	bool damaged = false;
	// VPair数据计数器，用于计数从尾部开始最多30笔数据，用于复制盘口30档数据用
	int price30_count = MY_SHFE_QUOTE_PRICE_POS_COUNT - 1; 
	double price30[MY_SHFE_QUOTE_PRICE_POS_COUNT] = {0};
	int vol30[MY_SHFE_QUOTE_PRICE_POS_COUNT] = {0};
	for(int i=sell_data_cursor_; i>=0; i--)
	{ // 从尾部向前遍历MDPackEx数据 
		MDPackEx *src_mdpackex = sell_data_buffer_[i];
		if (src_mdpackex->damaged) damaged = true;

		for(int j=src_mdpackex->content.count-1; j>=0; j--)
		{ //从尾部向前遍历PVPair数据 
			PVPair &src_pvpaire = src_mdpackex->content.data[j];

			// 处理30档卖方向数据
			if(price30_count >= 0)
			{
				price30[price30_count] = src_pvpaire.price;  
				vol30[price30_count] = src_pvpaire.volume;  
				price30_count = price30_count - 1;
			}

			// 计算总委卖量
			target_data_.sell_total_volume += src_pvpaire.volume;
			amount += src_pvpaire.price * src_pvpaire.volume;
		} // for(int j=sell_data_buffer_[i].content.count-1; j>=0; j--)//从尾部向前遍历PVPair数据 
	} // for(int i=sell_data_cursor_; i>=0; i--) // 从尾部向前遍历MDPackEx数据 
	
	// 计算均价
	if(damaged) target_data_.sell_total_volume = 0;
	if (target_data_.sell_total_volume > 0)
	{
		target_data_.sell_weighted_avg_price = amount / target_data_.sell_total_volume;
	}
	// 拷贝盘口30档卖方向数据
	memcpy(target_data_.sell_volume, vol30, sizeof(vol30));
	memcpy(target_data_.sell_price, price30, sizeof(price30));
}

// done
void MYIneQuoteData::FillFullDepthInfo()
{
	FillBuyFullDepthInfo();
	FillSellFullDepthInfo();
}

void MYIneQuoteData::Send(const char* contract)
{
	CDepthMarketDataField* l1_md = NULL;

	//clog_info("[%s] INE send %s.", module_name_,contract);

	// 合并一档行情
	if(l1_md_last_index_ != L1MD_NPOS)
	{
		 l1_md =  l1_md_producer_->GetLastData(contract, l1_md_last_index_);
	}
	if(NULL != l1_md)
	{
		target_data_.data_flag = 6; 
		memcpy(&target_data_, l1_md, sizeof(CDepthMarketDataField));

		YaoQuoteHelper::Convert(&yaoquote_, &target_data_);

		if (yaoquote_handler_ != NULL) 
		{ 
			yaoquote_handler_(&yaoquote_); 
		}
	} 
}

void MYIneQuoteData::SetQuoteDataHandler(std::function<void(YaoQuote*)> quote_handler)
{
	clog_warning("[%s] SetQuoteDataHandler invoked.", module_name_);
	yaoquote_handler_= quote_handler;
}

void MYIneQuoteData::ProcIneL1MdData(int32_t index)
{
	l1_md_last_index_ = index;
	CDepthMarketDataField* md = l1_md_producer_->GetData(index);

	//clog_info("[%s] ProcL1MdData:constract:%s;index:%d", module_name_, md->InstrumentID, l1_md_last_index_); 
}

void MYIneQuoteData::Reset()
{
	memset(target_data_.buy_price, 0, sizeof(target_data_.buy_price));
	memset(target_data_.buy_volume, 0, sizeof(target_data_.buy_volume));
	memset(target_data_.sell_price, 0, sizeof(target_data_.sell_price));
	memset(target_data_.sell_volume, 0, sizeof(target_data_.sell_volume));

	buy_data_cursor_ = -1;
	sell_data_cursor_ = -1;
}
