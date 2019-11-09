#include <thread>         
#include <chrono>        
#include <ctime>
#include <ratio>
#include <chrono>
#include <algorithm>    
#include "uni_consumer.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "md_helper.h"

using namespace std::placeholders; 
using namespace std::chrono;

UniConsumer::UniConsumer(struct vrt_queue  *queue, 
			TapMDProducer *l1md_producer, 
			L2MDProducer *l2md_producer)
:	 module_name_("uni_consumer"),
	running_(true), 
	l1_md_producer_(l1md_producer),
    l2_md_producer_(l2md_producer)
{
	ParseConfig();

	(this->consumer_ = vrt_consumer_new(module_name_, queue));
	clog_warning("[%s] yield:%s", module_name_, config_.yield); 
	if(strcmp(config_.yield, "threaded") == 0)
	{
		this->consumer_->yield = vrt_yield_strategy_threaded();
	}
	else if(strcmp(config_.yield, "spin") == 0)
	{
		this->consumer_->yield = vrt_yield_strategy_spin_wait();
	}
	else if(strcmp(config_.yield, "hybrid") == 0)
	{
		this->consumer_->yield = vrt_yield_strategy_hybrid();
	}
}

UniConsumer::~UniConsumer()
{
}

void UniConsumer::ParseConfig()
{
	std::string config_file = "x-trader.config";
	TiXmlDocument doc = TiXmlDocument(config_file.c_str());
    doc.LoadFile();
    TiXmlElement *root = doc.RootElement();    
	
	// yield strategy
    TiXmlElement *dis_node = root->FirstChildElement("Disruptor");
	if (dis_node != NULL)
	{
		strcpy(config_.yield, dis_node->Attribute("yield"));
	} 
	else 
	{ 
		clog_error("[%s] x-trader.config error: Disruptor node missing.", module_name_); 
	}

    TiXmlElement *marketDataServerNode = root->FirstChildElement("MarketDataReceiver");
	strcpy(config_.MarketDataReceiverIp, marketDataServerNode->Attribute("ip"));
	this->config_.MarketDataReceiverPort = atoi(marketDataServerNode->Attribute("port"));
}

void UniConsumer::InitMarketDataServer()
{
	local_sev_socket_= socket(AF_INET, SOCK_DGRAM, 0);
	if(-1 == local_sev_socket_)
	{
		clog_error("[%s] MarketDataServer: create socket error, ip:%s; port:%d", 
					module_name_,
					config_.MarketDataReceiverIp,
					config_.MarketDataReceiverPort); 
	}
	else
	{
		clog_error("[%s] MarketDataServer: create socket succeed, ip:%s; port:%d", 
					module_name_,
					config_.MarketDataReceiverIp,
					config_.MarketDataReceiverPort); 
	}

    int sndbufsize = 5120;
    int ret = setsockopt(local_sev_socket_, 
				SOL_SOCKET, 
				SO_SNDBUF , 
				(const void *) &sndbufsize, 
				sizeof(sndbufsize));
    if (ret != 0)
	{
        clog_error("UDP - set SO_SNDBUF failed.");
    }

	bzero(&marketdata_rev_socket_addr_, sizeof(marketdata_rev_socket_addr_));
	marketdata_rev_socket_addr_.sin_family = AF_INET;   
	marketdata_rev_socket_addr_.sin_addr.s_addr = inet_addr(config_.MarketDataReceiverIp); 
	marketdata_rev_socket_addr_.sin_port = htons(config_.MarketDataReceiverPort);  //网络字节序
}

void UniConsumer::CloseMarketDataServer()
{
	close(local_sev_socket_);
}


void UniConsumer::Start()
{
	InitMarketDataServer();
	running_  = true;

	MdHelper myquotedata(l2_md_producer_, l1_md_producer_);
	auto f_md = std::bind(&UniConsumer::ProcYaoQuote, this,_1);
	myquotedata.SetQuoteDataHandler(f_md);

	int rc = 0;
	struct vrt_value  *vvalue;
	while (running_ &&
		   (rc = vrt_consumer_next(consumer_, &vvalue)) != VRT_QUEUE_EOF) 
	{
		if (rc == 0) 
		{
			struct vrt_hybrid_value *ivalue = 
				cork_container_of(vvalue, 
							struct vrt_hybrid_value, 
							parent);

			switch (ivalue->data){
				case ZCE_L1_MD:
					myquotedata.ProcL1MdData(ivalue->index);
					break;
				case ZCE_L2_MD:
					myquotedata.ProcL2Data(ivalue->index);
					break;
				default:
					clog_error("[%s] [start] unexpected index: %d", module_name_, ivalue->index);
					break;
			}
		}
	} // end while (running_ &&

	if (rc == VRT_QUEUE_EOF) 
	{
		clog_warning("[%s] [start] rev EOF.", module_name_);
	}
	clog_warning("[%s] [start] exit.", module_name_);
}

void UniConsumer::Stop()
{
	if(running_)
	{
		l1_md_producer_->End();
		l2_md_producer_->End();		
		CloseMarketDataServer();
		running_ = false;
		clog_warning("[%s] Stop exit.", module_name_);
	}

	fflush (Log::fp);
}

void UniConsumer::ProcYaoQuote(YaoQuote* md)
{

#ifdef LATENCY_MEASURE
		high_resolution_clock::time_point t0 = high_resolution_clock::now();
#endif
	clog_info("[%s] send YaoQuote: %s", 
				module_name_,
				YaoQuote::ToString(md).c_str());

	memcpy(send_buf_, md, sizeof(YaoQuote));
	int result = sendto(local_sev_socket_, 
				send_buf_, 
				sizeof(YaoQuote), 
				0, 
				(struct sockaddr *)&marketdata_rev_socket_addr_,
				sizeof(marketdata_rev_socket_addr_));
	if(-1 == result)
	{
		clog_error("[%s] socket sendto error.", module_name_);
	}

#ifdef LATENCY_MEASURE
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		int latency = (t1.time_since_epoch().count() - t0.time_since_epoch().count()) / 1000;
		clog_warning("[%s] ProcYaoQuote latency:%d us", module_name_, latency); 
#endif
}

