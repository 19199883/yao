#ifndef __MD_PRODUCER_H__
#define __MD_PRODUCER_H__

#include <functional>
#include <array>
#include <thread>         
#include <chrono>        
#include <string>        
#include "vrt_value_obj.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "quote_datatype_dce_level2.h"
#include "quote_cmn_save.h"
#include "quote_cmn_utility.h"
#include "YaoQuote.h"

using namespace std;

struct Mdconfig
{
	string addr;
	char ip[30];
	int port;
	char contracts_file[500];
	char yield[20]; // disruptor yield strategy
};

enum EDataType
{
    eMDBestAndDeep = 0,
    eArbiBestAndDeep,
    eMDTenEntrust,
    eMDRealTimePrice,
    eMDOrderStatistic,
    eMDMarchPriceQty,
};

class MDProducer
{
	public:
		MDProducer(struct vrt_queue  *queue);
		~MDProducer();

		YaoQuote* GetBestAnddeep(int32_t index);

		void End();
		/*
		 * check whether the given contract is dominant.
		 */
		bool IsDominant(const char *contract);

	private:
		int32_t Push(const YaoQuote& md);

		const char *module_name_;  
		int udp_fd_;

		bool ended_;
		Mdconfig config_;

		struct vrt_producer  *producer_;
		void ParseConfig();
		int InitMDApi();
		void RevData();
		std::thread *thread_rev_;
		int32_t dominant_contract_count_;
		char dominant_contracts_[MAX_CONTRACT_COUNT][10];

		/*
		 * 缓冲区里获取一个新的对象
		 */
		YaoQuote* GetNewDepthData();

		/*
		 * 缓冲区里获取一个新的对象
		 */
		MDOrderStatistic* GetNewOrderStatData();

		/*
		 * 在缓冲区里获取一个指定合约的对象
		 */
		YaoQuote* GetDepthData(const char* contract);

		/*
		 * 在缓冲区里获取一个指定合约的对象
		 */
		MDOrderStatistic* GetOrderStatData(const char* contract);
		YaoQuote* ProcessOrderStatData(MDOrderStatistic* orderStat);
		YaoQuote* ProcessDepthData(MDBestAndDeep* depthdata);

		MDOrderStatistic orderstat_buffer_[MAX_CONTRACT_COUNT];
		YaoQuote  depth_buffer_[MAX_CONTRACT_COUNT];

#ifdef PERSISTENCE_ENABLED 
	QuoteDataSave<YaoQuote> *p_save_quote_;

#endif
};

#endif
