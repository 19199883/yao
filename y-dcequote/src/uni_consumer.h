
#ifndef __UNI_CONSUMER_H__
#define __UNI_CONSUMER_H__

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <functional>
#include <array>
#include <string>
#include <list>
#include <unordered_map>
#include "vrt_value_obj.h"
#include "md_producer.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"
#include <mutex>          // std::mutex, std::lock_guard


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
		UniConsumer(struct vrt_queue  *queue, MDProducer *md_producer);
		~UniConsumer();

		void Start();
		void Stop();

	private:
		void InitMarketDataServer();
		void CloseMarketDataServer();

		bool running_;
		const char* module_name_;  
		struct vrt_consumer *consumer_;
		MDProducer *md_producer_;
		CLoadLibraryProxy *pproxy_;

		void ParseConfig();
		int32_t GetEmptyNode();

		// business logic
		void ProcYaoQuote(int32_t index);
		Uniconfig config_;

		int local_sev_socket_;
		struct sockaddr_in marketdata_rev_socket_addr_;
		char send_buf_[5120];
};

#endif

