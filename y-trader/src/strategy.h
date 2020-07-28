// done
#pragma once

#include <list>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <atomic>         // std::atomic_flag
#include <memory>
#include "signal.h"
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"
#include "tunn_rpt_producer.h"
#include "YaoQuote.h"

using namespace std;

#define STRATEGY_METHOD_INIT "st_init_"
#define STRATEGY_METHOD_FEED_MD_YAO "st_feed_future_quote_"
#define STRATEGY_METHOD_FEED_SIG_RESP "st_feed_sig_resp_"
#define STRATEGY_METHOD_FEED_DESTROY "st_destroy_"
#define STRATEGY_METHOD_FEED_INIT_POSITION  "st_feed_init_position_"

// 假设一个策略最多产生3000个信号
#define SIGANDRPT_TABLE_SIZE 15000

// 一个x-trader最多支持100个策略
#define MAX_STRATEGY_COUNT 20

#define MAX_LINES_FOR_LOG 20000

struct StrategySetting
{
public:
	st_config_t config;
	string file;
};

class Strategy	
{
public:
	typedef void (* Init_ptr)(st_config_t *config, int *ret_code, struct strat_out_log *log);

	typedef void ( *FeedYaoMarketData_ptr)(YaoQuote* md, 
				int *sig_cnt, 
				signal_t* signals, 
				struct strat_out_log *log);	

	typedef void ( *FeedSignalResponse_ptr)(signal_resp_t* rpt, 
				symbol_pos_t *pos, 
				int *sig_cnt, 
				signal_t* sigs, 
				struct strat_out_log *log);

	typedef void (*Destroy_ptr)();

	typedef void (*FeedInitPosition_ptr)(strategy_init_pos_t *data, 
				struct strat_out_log *log);

public:
	Strategy();
	virtual ~Strategy(void);

	// things relating to strategy interface
	void Init(StrategySetting &setting, CLoadLibraryProxy *pproxy);
	void FeedInitPosition();
	void FeedMd(YaoQuote* md, int *sig_cnt, signal_t* signals);
	void feed_sig_response(signal_resp_t* rpt, 
				symbol_pos_t *pos, 
				int *sig_cnt, 
				signal_t* sigs);

	// things relating to x-trader internal logic
	void finalize(void);
	int32_t GetId();
	const char* GetContract();
	exchange_names GetExchange(const char* contract);
	int32_t GetMaxPosition(const char* contract);
	const char* GetSoFile();
	int GetLocalOrderID(int32_t sig_id);
	bool Deferred(int sig_id, 
				unsigned short sig_openclose, 
				unsigned short int sig_act);

	void PrepareForExecutingSig(int localorderid, 
				const signal_t &sig, 
				int32_t actual_vol);
	void FeedTunnRpt(int32_t sigidx, 
				const TunnRpt &rpt, 
				int *sig_cnt, 
				signal_t* sigs);

	bool HasFrozenPosition(const char *contract);
	int32_t GetCounterByLocalOrderID(int local_ord_id);
	signal_t* GetSignalBySigID(int32_t sig_id);
	void Push(const signal_t &sig);
	int GetVol(const signal_t &sig);
	void End(void);
	int32_t GetSignalIdxBySigId(long sigid);
	int32_t GetSignalIdxByLocalOrdId(int localordid);
	char* GetSysOrderIdBySigID(int32_t sig_id);
	if_sig_state_t GetStatusBySigIdx(int32_t sig_idx);
	const char *GetContractBySigId(int32_t sig_id);
	const char* GetContractBySigIdx(int32_t sig_idx);

	/*
	 * 判断是否策略订阅了指定的合约。
	 * true：订阅了
	 * false：未定阅读
	 *
	 */
	bool Subscribed(const char* contract);

	StrategySetting setting_;
private:
	if_sig_state_t ConvertStatusFromCtp(TThostFtdcOrderStatusType ctp_state);
	string generate_log_name(char * log_path);

	// things relating to strategy interface
	Init_ptr pfn_init_;
	FeedYaoMarketData_ptr pfn_feedyaomarketdata_;
	FeedSignalResponse_ptr pfn_feedsignalresponse_;
	Destroy_ptr pfn_destroy_;
	FeedInitPosition_ptr pfn_feedinitposition_;

	int GetLocalOrderIDByCounter(int counter);

	int cursor_;
	signal_t sig_table_[SIGANDRPT_TABLE_SIZE];
	signal_resp_t sigrpt_table_[SIGANDRPT_TABLE_SIZE];
	// 数组下标：信号id；值：信号索引(cursor)
	TThostFtdcOrderSysIDType sys_order_id_[SIGANDRPT_TABLE_SIZE];

	// key: LocalOrderID的counter部分; value:信号和报告所在数组的索引。
	// counter是全局累加的，所以数组长度：一个策略最大信号苏沪 * 支持的最大策略数 
	int localorderid_sigandrptidx_map_table_[SIGANDRPT_TABLE_SIZE * MAX_STRATEGY_COUNT]; 

	// key: signal id; value: LocalOrderID
	int sigid_localorderid_map_table_[SIGANDRPT_TABLE_SIZE];
	
	// key: signal id; value: 号所存数组的位置
	long sigid_sigidx_map_table_[SIGANDRPT_TABLE_SIZE];

	// be used to check whether the stategy is valid
	bool valid_;
	int id_;


	CLoadLibraryProxy *pproxy_;
	const char *module_name_;  

	/*
	 *从仓位文件中加载仓位，init_pos_
	 *
	 */
	void LoadPosition();

	/*
	 * 存储该策略测初始仓位
	 */
	strategy_init_pos_t init_pos_;

	/*
	 * 
	 */
	void UpdateSigrptByTunnrpt(int32_t lastqty,
				double last_price, 
				signal_resp_t& sigrpt, 
				if_sig_state_t &status, 
				int err);
};

