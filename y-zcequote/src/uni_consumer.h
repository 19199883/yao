// done
#ifndef __UNI_CONSUMER_H__
#define __UNI_CONSUMER_H__


#include <functional>
#include <array>
#include <string>
#include <list>
#include <unordered_map>
#include "vrt_value_obj.h"
#include "lev1_producer.h"
#include "l2md_producer.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "moduleloadlibrarylinux.h"
#include <utility>
#include "loadlibraryproxy.h"
#include <boost/asio.hpp>
#include <mutex>          // std::mutex, std::lock_guard
#include <vector>
#include "YaoQuote.h"

struct Uniconfig
{
	// disruptor yield strategy
	char yield[20];
	char MarketDataReceiverIp[24];
	int MarketDataReceiverPort;
};

class UniConsumer
{
	public:
		UniConsumer(struct vrt_queue  *queue, 
					Lev1Producer *l1md_producer, 
					L2MDProducer *l2md_producer);
		~UniConsumer();

		void Start();
		void Stop();
		

	private:
		void InitMarketDataServer();
		void CloseMarketDataServer();

		bool running_;
		const char* module_name_;  
		struct vrt_consumer *consumer_;
		L2MDProducer *l2_md_producer_;
		Lev1Producer *l1_md_producer_;
		void ParseConfig();

		// business logic
		void ProcYaoQuote(YaoQuote* md);
		void FeedL2QuoteSnapshot(int32_t straidx);
		Uniconfig config_;
		
		int local_sev_socket_;
		struct sockaddr_in marketdata_rev_socket_addr_;
		char send_buf_[5120];
};

#endif

