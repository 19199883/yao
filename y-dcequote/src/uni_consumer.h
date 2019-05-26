
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
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"

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
		// lic
		bool check_lic();
		bool legal_;

		bool running_;
		const char* module_name_;  
		struct vrt_consumer *consumer_;
		MDProducer *md_producer_;
		CLoadLibraryProxy *pproxy_;

		void ParseConfig();
		int32_t GetEmptyNode();

		// business logic
		void ProcBestAndDeep(int32_t index);
		void FeedBestAndDeep(int32_t straidx);
		Uniconfig config_;

};

#endif

