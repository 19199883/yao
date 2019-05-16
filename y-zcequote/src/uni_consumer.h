
#ifndef __UNI_CONSUMER_H__
#define __UNI_CONSUMER_H__

#include <functional>
#include <array>
#include <string>
#include <list>
#include <unordered_map>
#include "vrt_value_obj.h"
#include "tap_md_producer.h"
#include "l2md_producer.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"

#define SIG_BUFFER_SIZE 32 

// key2: stoi(年月)，如1801
#define MAX_STRATEGY_KEY2 3000 
// 品种字符相加：如jd1801，key1: j+d(ascii 值相加
#define MAX_STRATEGY_KEY1 500

// 允许的最大缓存的待处理信号数量
#define MAX_PENDING_SIGNAL_COUNT 10

struct Uniconfig
{
	// disruptor yield strategy
	char yield[20];
};

class UniConsumer
{
	public:
		UniConsumer(struct vrt_queue  *queue, TapMDProducer *l1md_producer, 
			L2MDProducer *l2md_producer);
		~UniConsumer();

		void Start();
		void Stop();

	private:

		bool running_;
		const char* module_name_;  
		struct vrt_consumer *consumer_;
		L2MDProducer *l2_md_producer_;
		TapMDProducer *l1_md_producer_;
		void ParseConfig();

		// business logic
		void ProcL2QuoteSnapshot(ZCEL2QuotSnapshotField_MY* md);
		void FeedL2QuoteSnapshot(int32_t straidx);
		void ProcSigs(Strategy &strategy, int32_t sig_cnt, signal_t *sigs);
		void ProcTunnRpt(int32_t index);
		void CancelOrder(Strategy &strategy,signal_t &sig);
		void PlaceOrder(Strategy &strategy, const signal_t &sig);
		bool CancelPendingSig(Strategy &strategy, int32_t ori_sigid);
		signal_t sig_buffer_[SIG_BUFFER_SIZE];
		Uniconfig config_;
};

#endif

