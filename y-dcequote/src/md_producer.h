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
#include "DFITCL2ApiDataType.h"
#include "quote_cmn_save.h"
#include "quote_cmn_utility.h"
#include "YaoQuote.h"

using namespace DFITC_L2;

/*
 *  * 10 power of 2
 *   */
#define MD_BUFFER_SIZE 1000 

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

		YaoQuote* GetData(int32_t index);

		void End();
		/*
		 * check whether the given contract is dominant.
		 */
		bool IsDominant(const char *contract);

	private:
		int32_t Push(const YaoQuote& md);
		/*
		 * 用于vrt队列
		 */
		YaoQuote yaoQuote_buffer_[MD_BUFFER_SIZE];

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
		 * 在缓冲区里获取一个指定合约的对象
		 */
		YaoQuote* GetDepthData(const char* contract);

		/*
		 * 缓冲区里获取一个新的对象
		 */
		YaoQuote* ProcessDepthData(MDBestAndDeep* depthdata);
		/*
		 * 缓存level2数据，每个合约一个位置
		 *
		 */
		YaoQuote  depth_buffer_[MAX_CONTRACT_COUNT];

		/*
		 * 缓冲区里获取一个新的对象
		 */
		MDOrderStatistic* GetNewOrderStatData();

		/*
		 * 在缓冲区里获取一个指定合约的对象
		 */
		MDOrderStatistic* GetOrderStatData(const char* contract);
		YaoQuote* ProcessOrderStatData(MDOrderStatistic* orderStat);
		/*
		 * 缓存OrderStatis数据，每个合约一个位置
		 */
		MDOrderStatistic orderstat_buffer_[MAX_CONTRACT_COUNT];

		void ToString(MDBestAndDeep* quote);

#ifdef PERSISTENCE_ENABLED 
	QuoteDataSave<YaoQuote> *p_save_quote_;

#endif
};

#endif
