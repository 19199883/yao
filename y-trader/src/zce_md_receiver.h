#pragma once

#include <functional>
#include <array>
#include <thread>         
#include <chrono>        
#include <string>        
#include "vrt_value_obj.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "quote_cmn_save.h"
#include "quote_cmn_utility.h"
#include "YaoQuote.h"


/*
 *  * 10 power of 2
 *   */
#define ZCE_MD_BUFFER_SIZE 2048 

using namespace std;

struct ZceMdconfig
{
	char LocalIp[30];
	int Port;
};

class ZceQuote
{
	public:
		ZceQuote(struct vrt_queue  *queue);
		~ZceQuote();

		YaoQuote* GetData(int32_t index);

		void End();

	private:
		int32_t Push(const YaoQuote& md);
		/*
		 * 用于vrt队列
		 */
		YaoQuote yaoQuote_buffer_[ZCE_MD_BUFFER_SIZE];

		const char *module_name_;  
		int udp_fd_;

		bool ended_;
		ZceMdconfig config_;

		struct vrt_producer  *producer_;
		void ParseConfig();
		int InitMDApi();
		void RevData();
		std::thread *thread_rev_;

#ifdef PERSISTENCE_ENABLED 
	QuoteDataSave<YaoQuote> *p_save_quote_;

#endif
};

