
#ifndef __UNI_CONSUMER_H__
#define __UNI_CONSUMER_H__

#include <functional>
#include <array>
#include <string>
#include <list>
#include <unordered_map>
#include "vrt_value_obj.h"
#include "md_producer.h"
#include <tinyxml.h>
#include <tinystr.h>
#include <boost/asio.hpp>
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"
#include <mutex>          // std::mutex, std::lock_guard

using boost::asio::ip::tcp;

// 允许的最大客户端连接数
#define MAX_CONN_COUNT 10

struct Uniconfig
{
	// disruptor yield strategy
	char yield[20];
};

class UniConsumer
{
	public:
		UniConsumer(struct vrt_queue  *queue, MDProducer *md_producer);
		~UniConsumer();

		void Start();
		void Stop();

	private:
		void Server();

		bool running_;
		const char* module_name_;  
		struct vrt_consumer *consumer_;
		MDProducer *md_producer_;
		CLoadLibraryProxy *pproxy_;

		void ParseConfig();
		int32_t GetEmptyNode();

		// business logic
		void ProcYaoQuote(int32_t index);
		// yao quote
		boost::asio::io_service io_service_;
		int port_;
		std::vector<tcp::socket> socks_;
		// 记录连接是否有效。
		// 位置与socks一一对应，1-有效；0-无效：
		int valid_conn_[MAX_CONN_COUNT];
		std::mutex mtx_;

		Uniconfig config_;

};

#endif

