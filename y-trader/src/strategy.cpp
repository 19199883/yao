// lic
#include<stdlib.h>
#include <chrono>
#include <thread> 
#include <stdio.h>
#include <ratio>
#include <ctime>
#include <sstream>
#include <stdio.h>
#include <fstream>      // std::ifstream, std::ofstream
#include <stdio.h>
#include "strategy.h"
#include "pos_calcu.h"
#include "quote_cmn_utility.h"

using namespace std;
using namespace std::chrono;

Strategy::Strategy()
: module_name_("Strategy")
{
	valid_ = false;
	cursor_ = 0;

	pfn_init_;
	pfn_feedyaomarketdata_ = NULL;
	pfn_feedsignalresponse_ = NULL;
	pfn_feedinitposition_ = NULL;
	pfn_destroy_ = NULL;

	pproxy_ = NULL;

	//log_ = vector<strat_out_log>(MAX_LINES_FOR_LOG);
	//log_cursor_ = 0;
	//pfDayLogFile_ = NULL;
	//cur_ntick_ = -1;
	//max_log_lines_ = 0;
	id_ = 0;

	memset(localorderid_sigandrptidx_map_table_, 0, sizeof(localorderid_sigandrptidx_map_table_));
	memset(sigid_localorderid_map_table_, 0, sizeof(sigid_localorderid_map_table_));
	memset(sigid_sigidx_map_table_, 0, sizeof(sigid_sigidx_map_table_));
	memset(sys_order_id_, 0, sizeof(sys_order_id_));
}

void Strategy::End(void)
{
	//fclose(pfDayLogFile_);
	clog_warning("[%s] strategy(id:%d) close log file", 
				module_name_, 
				this->setting_.config.st_id);

	if (this->pfn_destroy_ != NULL)
	{
		pfn_destroy_ ();
		clog_warning("[%s] strategy(id:%d) destroyed", 
					module_name_, 
					this->setting_.config.st_id);
	}

	fflush (Log::fp);
}

Strategy::~Strategy(void)
{
	if (pproxy_ != NULL)
	{
		// lic
		char cmd[1024];
		char bar_so[1024];
		sprintf(bar_so, "./lib/%s.so", this->setting_.file.c_str());
		pproxy_->deleteObject(bar_so);
		sprintf(cmd, "rm %s", bar_so);
		system(cmd);
		pproxy_ = NULL;
	}
}

string Strategy::generate_log_name(char* log_path)
 {
	string log_full_path = "";

	// parse model name
	string model_name = "";
	unsigned found = this->setting_.file.find_last_of("/");
	if(found==string::npos)
	{ 
		model_name = this->setting_.file; 
	}
	else
	{
		model_name = this->setting_.file.substr(found+1); 
	}

	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer,80,"%Y%m%d_%H-%M-%S",timeinfo);

	log_full_path = log_path;
	log_full_path += "/";
	log_full_path += buffer;

	if(setting_.config.IsNightTrading)
	{
		log_full_path += "_night";
	}
	else
	{
		log_full_path += "_day";
	}

	log_full_path += ".log";

	return log_full_path;
}

void Strategy::Init(StrategySetting &setting, CLoadLibraryProxy *pproxy)
{
	valid_ = true;
	this->setting_ = setting;
	this->pproxy_ = pproxy;
	id_ = this->setting_.config.st_id;

	//max_log_lines_ = MAX_LINES_FOR_LOG - MAX_STRATEGY_COUNT * 100 + GetId() * 100;
	//clog_warning("[%s] strategy:%d; max_log_lines_ :%d", 
	//			module_name_, 
	//			this->GetId(), 
	//			max_log_lines_ ); 

	// lic
	char cmd[1024];
	sprintf(cmd, "openssl des3 -d -k 617999 -salt -in %s.so | tar -xzf - -C ./lib", 
				this->setting_.file.c_str());
	system(cmd);

	char bar_so[1024];
	sprintf(bar_so, "./lib/%s", this->setting_.file.c_str());

	pfn_init_ = (Init_ptr)pproxy_->findObject(bar_so, STRATEGY_METHOD_INIT);
	if (!pfn_init_){
		clog_warning("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, 
					bar_so, 
					STRATEGY_METHOD_INIT, 
					errno);
	}

	pfn_feedyaomarketdata_ = 
		(FeedYaoMarketData_ptr )pproxy_->findObject( bar_so, STRATEGY_METHOD_FEED_MD_YAO);
	if (!pfn_feedyaomarketdata_ )
	{
		clog_warning("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, 
					bar_so, 
					STRATEGY_METHOD_FEED_MD_YAO, 
					errno);
	}

	pfn_feedinitposition_ = 
		(FeedInitPosition_ptr)pproxy_->findObject(bar_so, STRATEGY_METHOD_FEED_INIT_POSITION);
	if (!pfn_feedinitposition_ )
	{
		clog_warning("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, 
					bar_so, 
					STRATEGY_METHOD_FEED_INIT_POSITION, 
					errno);
	}

	pfn_feedsignalresponse_ = 
		(FeedSignalResponse_ptr)pproxy_->findObject(bar_so, STRATEGY_METHOD_FEED_SIG_RESP);
	if (!pfn_feedsignalresponse_)
	{
		clog_warning("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, 
					bar_so, 
					STRATEGY_METHOD_FEED_SIG_RESP, 
					errno);
	}

	pfn_destroy_ = 
		(Destroy_ptr)pproxy_->findObject(bar_so, STRATEGY_METHOD_FEED_DESTROY );
	if (!pfn_destroy_)
	{
		clog_warning("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, 
					bar_so, 
					STRATEGY_METHOD_FEED_DESTROY, 
					errno);
	}

	string model_log = generate_log_name(setting_.config.log_name);
	strcpy(setting_.config.log_name, model_log.c_str());
	setting_.config.log_id = setting_.config.st_id;

	clog_warning("[%s] open log file:%s", module_name_,setting_.config.log_name);

	LoadPosition();

	string sym_log_name = 
		generate_log_name(setting_.config.symbols[0].symbol_log_name);
	strcpy(setting_.config.symbols[0].symbol_log_name, sym_log_name.c_str());

	//pfDayLogFile_ = fopen (setting_.config.log_name, "w");
	int err = 0;
	this->pfn_init_(&this->setting_.config, &err, NULL /*log_.data()+log_cursor_*/);
//	if((log_.data()+log_cursor_)->exch_time > 0) 
//	{
//		log_cursor_++;
//	}

	this->FeedInitPosition();
}

void Strategy::FeedInitPosition()
{
	signal_t sigs[10];
	int sig_cnt = 0;

	this->pfn_feedinitposition_(&init_pos_, NULL /*log_.data()+log_cursor_*/);
//	if((log_.data()+log_cursor_)->exch_time > 0)
//	{
//		log_cursor_++;
//	}
}

void Strategy::FeedMd(YaoQuote* md, int *sig_cnt, signal_t* sigs)
{
#ifdef LATENCY_MEASURE
	high_resolution_clock::time_point t0 = high_resolution_clock::now();
#endif
	
	*sig_cnt = 0;
	//(log_.data()+log_cursor_)->exch_time = 0;
	this->pfn_feedyaomarketdata_(md, sig_cnt, sigs, NULL /*log_.data()+log_cursor_*/);
//	if((log_.data()+log_cursor_)->exch_time > 0)
//	{
//		log_cursor_++;
//	}

	for (int i = 0; i < *sig_cnt; i++ )
	{
#ifdef LATENCY_MEASURE
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		int latency = (t1.time_since_epoch().count() - t0.time_since_epoch().count()) / 1000;
		clog_warning("[%s] strategy latency:%d us", module_name_, latency); 
#endif
		sigs[i].st_id = this->GetId();

		 clog_info("[%s] FeedMd  signal: "
					 "strategy id:%d; sig_id:%d; exchange:%d; symbol:%s;"
					 "open_volume:%d; buy_price:%f; close_volume:%d; "
					 "sell_price:%f; sig_act:%d; sig_openclose:%d; orig_sig_id:%d",
					module_name_, 
					sigs[i].st_id, 
					sigs[i].sig_id,
					sigs[i].exchange, 
					sigs[i].symbol, 
					sigs[i].open_volume, 
					sigs[i].buy_price,
					sigs[i].close_volume, 
					sigs[i].sell_price, 
					sigs[i].sig_act, 
					sigs[i].sig_openclose, 
					sigs[i].orig_sig_id); 
	}
}

void Strategy::feed_sig_response(signal_resp_t* rpt, 
			symbol_pos_t *pos, 
			int *sig_cnt, 
			signal_t* sigs)
{
	*sig_cnt = 0;
	//(log_.data()+log_cursor_)->exch_time = 0;
	this->pfn_feedsignalresponse_(rpt, pos, sig_cnt, sigs, NULL /*log_.data()+log_cursor_*/);
	//if((log_.data()+log_cursor_)->exch_time > 0) log_cursor_++;

	for (int i = 0; i < *sig_cnt; i++ ){
		sigs[i].st_id = GetId();
		clog_info("[%s] feed_sig_respons esignal: strategy id:%d;sig_id:%d; exchange:%d; symbol:%s;"
					"open_volume:%d; buy_price:%f; close_volume:%d; sell_price:%f; sig_act:%d; "
					"sig_openclose:%d; orig_sig_id:%d",
					module_name_, 
					sigs[i].st_id, 
					sigs[i].sig_id,
					sigs[i].exchange, 
					sigs[i].symbol, 
					sigs[i].open_volume, 
					sigs[i].buy_price,
					sigs[i].close_volume, 
					sigs[i].sell_price, 
					sigs[i].sig_act, 
					sigs[i].sig_openclose, 
					sigs[i].orig_sig_id); 
	}
}


int32_t Strategy::GetId()
{
	return id_;
}

exchange_names Strategy::GetExchange(const char* contract)
{
	for(int i=0; i< this->setting_.config.symbols_cnt; i++)
	{
		if (IsEqualContract((char*)this->setting_.config.symbols[i].name, (char*)contract))
		{
			return this->setting_.config.symbols[i].exchange;
		}
	}
}

bool Strategy::Subscribed(const char* contract)
{
	for(int i=0; i< this->setting_.config.symbols_cnt; i++)
	{
		if (IsEqualContract((char*)this->setting_.config.symbols[i].name, (char*)contract))
		{
			return true;
		}
	}

	return false;
}

int32_t Strategy::GetMaxPosition(const char* contract)
{
	for(int i=0; i< this->setting_.config.symbols_cnt; i++)
	{
		if (IsEqualContract((char*)this->setting_.config.symbols[i].name, (char*)contract))
		{
			return this->setting_.config.symbols[i].max_pos;
		}
	}
}

const char* Strategy::GetSoFile()
{
	return this->setting_.file.c_str();
}

int Strategy::GetLocalOrderID(int32_t sig_id)
{
	return sigid_localorderid_map_table_[sig_id];
}

int Strategy::GetVol(const signal_t &sig)
{
	int32_t vol = 0;
	if (sig.sig_openclose == alloc_position_effect_t::OPEN)
	{
		vol = sig.open_volume;
	} 
	else if ((sig.sig_openclose == alloc_position_effect_t::CLOSE ||  
					sig.sig_openclose == alloc_position_effect_t::CLOSE_TOD || 
					sig.sig_openclose==alloc_position_effect_t::CLOSE_YES))
	{
		vol = sig.close_volume;
	} 
	else
	{ 
		clog_error("[%s] PlaceOrder: do support sig_openclose value:%d;", 
				module_name_,
				sig.sig_openclose); 
	}

	return vol;
}

StrategyPosition* Strategy::GetPosition(const char* contract)
{
	StrategyPosition *cur_pos = NULL;
	for(int i=0; i<this->setting_.config.symbols_cnt; i++)
	{
		if(IsEqualContract((char*)position_[i].contract, (char*)contract))
		{
			cur_pos = &position_[i];
			break;
		}
	}

	return cur_pos;
}

signal_t* Strategy::GetSignalBySigID(int32_t sig_id)
{
	int32_t cursor = sigid_sigidx_map_table_[sig_id];
	return &(sig_table_[cursor]);
}

if_sig_state_t Strategy::GetStatusBySigIdx(int32_t sig_idx)
{
	return (if_sig_state_t )sigrpt_table_[sig_idx].status;
}

char* Strategy::GetSysOrderIdBySigID(int32_t sig_id)
{
	int32_t cursor = sigid_sigidx_map_table_[sig_id];
	return sys_order_id_[cursor];
}

void Strategy::Push(const signal_t &sig)
{
	sig_table_[cursor_] = sig;
	sigid_sigidx_map_table_[sig.sig_id] = cursor_;

	// signal response
	memset(&(sigrpt_table_[cursor_]), 0, sizeof(signal_resp_t));
	sigrpt_table_[cursor_].status = -1; 
	sigrpt_table_[cursor_].sig_id = sig.sig_id;
	sigrpt_table_[cursor_].sig_act = sig.sig_act;
	sigrpt_table_[cursor_].sig_openclose = sig.sig_openclose;
	strcpy(sigrpt_table_[cursor_].symbol, sig.symbol);
	if (sig.sig_act==signal_act_t::buy)
	{
		sigrpt_table_[cursor_].order_price = sig.buy_price;
	}
	else if (sig.sig_act==signal_act_t::sell)
	{
		sigrpt_table_[cursor_].order_price = sig.sell_price;
	}

	// 从pending队列中撤单 done
	if (sig.sig_openclose == alloc_position_effect_t::OPEN)
	{
		sigrpt_table_[cursor_].order_volume = sig.open_volume;
	}
	else if (sig.sig_openclose == alloc_position_effect_t::CLOSE || 
				sig.sig_openclose == alloc_position_effect_t::CLOSE_TOD ||
				sig.sig_openclose==alloc_position_effect_t::CLOSE_YES)
	{
		sigrpt_table_[cursor_].order_volume = sig.close_volume;
	}

	cursor_++;
}

void Strategy::PrepareForExecutingSig(int localorderid, const signal_t &sig, int32_t actual_vol)
{
	int32_t cursor = sigid_sigidx_map_table_[sig.sig_id];
	// mapping table
	// sigid_sigandrptidx_map_table_[sig.sig_id] = cursor;
	int32_t counter = GetCounterByLocalOrderID(localorderid);
	localorderid_sigandrptidx_map_table_[counter] = cursor;
	sigid_localorderid_map_table_[sig.sig_id] = localorderid;

}


int Strategy::GetLocalOrderIDByCounter(int counter)
{
    return GetId() + counter * 1000;		
}

int32_t Strategy::GetCounterByLocalOrderID(int local_ord_id)
{
	return (local_ord_id - GetId()) / 1000;
}

// improve place, cancel
int32_t Strategy::GetSignalIdxBySigId(long sigid)
{
	return sigid_sigidx_map_table_[sigid];
}

// improve place, cancel
int32_t Strategy::GetSignalIdxByLocalOrdId(int localordid)
{
	// get signal report by LocalOrderID
	int32_t counter = GetCounterByLocalOrderID(localordid);
	int32_t index = localorderid_sigandrptidx_map_table_[counter];
	return index;
}

if_sig_state_t Strategy::ConvertStatusFromCtp(TThostFtdcOrderStatusType ctp_state)
{
	if_sig_state_t inner_state;

	if(THOST_FTDC_OST_AllTraded==ctp_state)
	{
		inner_state = SIG_STATUS_SUCCESS;
	}
	else if(THOST_FTDC_OST_PartTradedQueueing==ctp_state)
	{
		inner_state = SIG_STATUS_PARTED;
	}
	else if(THOST_FTDC_OST_PartTradedNotQueueing==ctp_state)
	{
		inner_state = SIG_STATUS_CANCEL;
	}
	else if(THOST_FTDC_OST_NoTradeQueueing==ctp_state)
	{
		inner_state = SIG_STATUS_ENTRUSTED;
	}
	else if(THOST_FTDC_OST_NoTradeNotQueueing==ctp_state)
	{
		inner_state = SIG_STATUS_CANCEL;
	}
	else if(THOST_FTDC_OST_Canceled==ctp_state)
	{
		inner_state = SIG_STATUS_CANCEL;
	}
	else if(THOST_FTDC_OST_Unknown==ctp_state)
	{
		inner_state = SIG_STATUS_ENTRUSTED;
	}
	else
	{
		clog_error("[%s] unkonwn tunnel status: %c; ", module_name_, ctp_state);
	}

	return inner_state;
}


// improve place, cancel
void Strategy::FeedTunnRpt(int32_t sigidx, const TunnRpt &rpt, int *sig_cnt, signal_t* sigs)
{
	signal_resp_t& sigrpt = sigrpt_table_[sigidx];
	signal_t& sig = sig_table_[sigidx];
	// TODO: yao cancel ctp
	strncpy(sys_order_id_[sigidx], rpt.OrderSysID, sizeof(sys_order_id_[sigidx]));

	if_sig_state_t status = ConvertStatusFromCtp(rpt.OrderStatus);
	int lastqty = rpt.MatchedAmount - sigrpt.acc_volume;
	// update signal report
	UpdateSigrptByTunnrpt(lastqty, rpt.MatchedPrice, sigrpt, status, rpt.ErrorID);
	// update strategy's position
	StrategyPosition *cur_pos = GetPosition(sig.symbol);
	UpdatePosition(cur_pos, lastqty, status, sig.sig_openclose, sig.sig_act, rpt.ErrorID);
	if (rpt.MatchedAmount > 0)
	{
		// fill signal position report by tunnel report
		FillPositionRpt();
	}

	for(int i=0; i<this->setting_.config.symbols_cnt; i++)
	{
		if(IsEqualContract(pos_cache_.s_pos[i].symbol, sig.symbol))
		{
			feed_sig_response(&sigrpt, &pos_cache_.s_pos[i], sig_cnt, sigs);

			clog_info("[%s] FeedTunnRpt: strategy id:%d; contract:%s; long:%d; short:%d; "
						"sig_id:%d; symbol:%s; sig_act:%d; order_volume:%d; order_price:%f; "
						"exec_price:%f; exec_volume:%d; acc_volume:%d; status:%d; killed:%d; "
						"rejected:%d",
						module_name_, 
						setting_.config.st_id, 
						pos_cache_.s_pos[i].symbol, 
						pos_cache_.s_pos[i].long_volume, 
						pos_cache_.s_pos[i].short_volume, 
						sigrpt.sig_id, 
						sigrpt.symbol,
						sigrpt.sig_act, 
						sigrpt.order_volume, 
						sigrpt.order_price, 
						sigrpt.exec_price,
						sigrpt.exec_volume, 
						sigrpt.acc_volume,
						sigrpt.status,
						sigrpt.killed,
						sigrpt.rejected);
		}
	}

}

void Strategy::UpdatePosition(StrategyPosition *stra_pos, 
			int32_t lastqty, 
			if_sig_state_t status, 
			unsigned short sig_openclose, 
			unsigned short int sig_act, 
			int err)
{
	if (lastqty > 0)
	{
		if (sig_openclose==alloc_position_effect_t::OPEN && sig_act==signal_act_t::buy)
		{
			stra_pos->cur_long += lastqty;
			stra_pos->frozen_open_long -= lastqty;
		}
		else if (sig_openclose==alloc_position_effect_t::OPEN && sig_act==signal_act_t::sell)
		{
			stra_pos->cur_short += lastqty;
			stra_pos->frozen_open_short -= lastqty;
		}
		else if ((sig_openclose==alloc_position_effect_t::CLOSE || 
						sig_openclose==alloc_position_effect_t::CLOSE_TOD || 
						sig_openclose==alloc_position_effect_t::CLOSE_YES) && 
					sig_act==signal_act_t::buy)
		{
			stra_pos->cur_short -= lastqty;
			stra_pos->frozen_close_short -= lastqty;
		}
		else if ((sig_openclose==alloc_position_effect_t::CLOSE ||
						sig_openclose==alloc_position_effect_t::CLOSE_TOD  || 
						sig_openclose==alloc_position_effect_t::CLOSE_YES)&& 
					sig_act==signal_act_t::sell)
		{
			stra_pos->cur_long -= lastqty;
			stra_pos->frozen_close_long -= lastqty;
		}
	} //end if (rpt.MatchedAmount > 0)

	// 从pending队列中撤单 done
	if (err != CANCELLED_FROM_PENDING)
	{
		if (status==SIG_STATUS_SUCCESS ||
			status==SIG_STATUS_CANCEL||
			status==SIG_STATUS_REJECTED){ // 释放冻结仓位
			if (sig_openclose==alloc_position_effect_t::OPEN && sig_act==signal_act_t::buy)
			{
				stra_pos->frozen_open_long = 0;
			}
			else if (sig_openclose==alloc_position_effect_t::OPEN && sig_act==signal_act_t::sell)
			{
				stra_pos->frozen_open_short = 0;
			}
			else if ((sig_openclose==alloc_position_effect_t::CLOSE || 
							sig_openclose==alloc_position_effect_t::CLOSE_TOD || 
							sig_openclose==alloc_position_effect_t::CLOSE_YES) && 
						sig_act==signal_act_t::buy)
			{
				stra_pos->frozen_close_short = 0;
			}
			else if ((sig_openclose==alloc_position_effect_t::CLOSE || 
							sig_openclose==alloc_position_effect_t::CLOSE_TOD || 
							sig_openclose==alloc_position_effect_t::CLOSE_YES) && 
						sig_act==signal_act_t::sell)
			{
				stra_pos->frozen_close_long = 0;
			}
		}
	}

	clog_info("[%s] UpdatePosition: strategy id:%d; current long:%d; current short:%d; \
				frozen_close_long:%d; frozen_close_short:%d; frozen_open_long:%d; \
				frozen_open_short:%d; ",
				module_name_, 
				setting_.config.st_id, 
				stra_pos->cur_long, 
				stra_pos->cur_short,
				stra_pos->frozen_close_long, 
				stra_pos->frozen_close_short,
				stra_pos->frozen_open_long, 
				stra_pos->frozen_open_short);
}

void Strategy::FillPositionRpt()
{
	position_t &pos = pos_cache_;
	// 注意pos.s_pos与position_以同样的合约顺序存储
	for(int i=0; i<this->setting_.config.symbols_cnt; i++)
	{
		pos.s_pos[i].long_volume  = position_[i].cur_long;
		pos.s_pos[i].short_volume = position_[i].cur_short;
		pos.s_pos[i].changed = 1;
	}
}

void Strategy::UpdateSigrptByTunnrpt(int32_t lastqty, 
			double last_price, 
			signal_resp_t& sigrpt, 
			if_sig_state_t &status, 
			int err)
{
	if (lastqty > 0)
	{
		sigrpt.exec_price = last_price;
		sigrpt.exec_volume = lastqty;
		sigrpt.acc_volume += lastqty;
	}

	if (status == SIG_STATUS_CANCEL)
	{
		sigrpt.killed = sigrpt.order_volume - sigrpt.acc_volume;
	}
	else
	{
		sigrpt.killed = 0; 
	}

	sigrpt.rejected = 0; 
	sigrpt.error_no = err;
	sigrpt.status = status;
}

// done
void Strategy::LoadPosition()
{
	char* contract;
	int yLong;
	int yShort;
	int tLong;
	int tShort;

	// TODO: yao position
	memset(&init_pos_, 0, sizeof(strategy_init_pos_t));
	position_t &today_pos = init_pos_._cur_pos;
	today_pos.symbol_cnt = this->setting_.config.symbols_cnt; 
	position_t &yesterday_pos = init_pos_._yesterday_pos;
	yesterday_pos.symbol_cnt = this->setting_.config.symbols_cnt; 

	memset(&pos_cache_, 0, sizeof(pos_cache_));
	pos_cache_.symbol_cnt = this->setting_.config.symbols_cnt;

	memset(&position_, 0, sizeof(position_));
	string strategy = (char*)GetSoFile();
	
	// 注意pos.s_pos与position_以同样的合约顺序存储
	for(int i=0; i< this->setting_.config.symbols_cnt; i++)
	{
		contract = this->setting_.config.symbols[i].name;
		pos_calc::get_pos(strategy, contract, yLong, yShort, tLong, tShort);

		strcpy(position_[i].contract, contract);
		position_[i].cur_long = yLong + tLong;
		position_[i].cur_short = yShort + tShort;

		strcpy(pos_cache_.s_pos[i].symbol, contract);
		pos_cache_.s_pos[i].long_volume  = yLong + tLong;
		pos_cache_.s_pos[i].short_volume = tLong + tShort;

		symbol_pos_t &yesterday_sym_pos = yesterday_pos.s_pos[i];
		strncpy(yesterday_sym_pos.symbol, contract, sizeof(yesterday_sym_pos.symbol));
		yesterday_sym_pos.long_volume = yLong;
		yesterday_sym_pos.short_volume = yShort;
		yesterday_sym_pos.exchg_code = this->GetExchange(contract); 

		symbol_pos_t &today_sym_pos = today_pos.s_pos[i];
		strncpy(today_sym_pos.symbol, contract, sizeof(today_sym_pos.symbol));
		today_sym_pos.long_volume = tLong;
		today_sym_pos.short_volume = tShort;
		today_sym_pos.exchg_code = this->GetExchange(contract); 

		clog_warning("[%s] FeedInitPosition strategy id:%d; contract:%s; exchange:%d; "
					"ylong:%d; yshort:%d; tlong:%d; tshort:%d; total_long:%d; total_short:%d",
					module_name_, 
					GetId(), 
					yesterday_sym_pos.symbol, 
					yesterday_sym_pos.exchg_code, 
					yesterday_sym_pos.long_volume, 
					yesterday_sym_pos.short_volume,
					today_sym_pos.long_volume, 
					today_sym_pos.short_volume,
					position_[i].cur_long,
					position_[i].cur_short);
	}
}

// strategy log
//FILE * Strategy::get_log_file()
//{
//	return pfDayLogFile_;
//}

/*
 * 因为lock_log_并不能对不同策略间的日志读写缓存进行同步，
 * 所以必须将不同的策略日志时间错开，保证一个策略日志落地后，
 * 再落地另外策略的日志
 *
 */
//int32_t Strategy::FullLineCount()
//{	
//	return max_log_lines_ ;
//}

//bool Strategy::IsLogFull()
//{
//	if(log_cursor_ == FullLineCount()) {
//		return true;
//	}else{
//		return false;
//	}
//}

//void Strategy::get_log(vector<strat_out_log> &log_buffer, int32_t &count)
//{
//	log_buffer.swap(log_);
//	count = log_cursor_;
//	log_cursor_ = 0;
//}


const char* Strategy::GetContractBySigId(int32_t sig_id)
{
	signal_t& sig = sig_table_[sigid_sigidx_map_table_[sig_id]];
	return sig.symbol;
}

const char* Strategy::GetContractBySigIdx(int32_t sig_idx)
{
	signal_t& sig = sig_table_[sig_idx];
	return sig.symbol;
}

