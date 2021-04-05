#include <thread>         
#include <chrono>        
#include <algorithm>    
#include "perfctx.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "uni_consumer.h"

UniConsumer::UniConsumer(struct vrt_queue  *queue, MDProducer *md_producer)
:	module_name_("uni_consumer"),
	running_(true), 
	md_producer_(md_producer)
{

#ifdef  DCE_TCP_SEND_DATA
	for(int i=0; i<MAX_CONNECT_COUNT; i++)
	{
		connfd_[i] = -1;
	}
#endif	

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
//	if (this->consumer_ != NULL){
//		vrt_consumer_free(this->consumer_);
//		this->consumer_ = NULL;
//		clog_info("[%s] release uni_consumer.", module_name_);
//	}
}

void UniConsumer::ParseConfig()
{
	std::string config_file = "x-trader.config";
	TiXmlDocument doc = TiXmlDocument(config_file.c_str());
    doc.LoadFile();
    TiXmlElement *root = doc.RootElement();    
	
	// yield strategy
    TiXmlElement *dist_node = root->FirstChildElement("Disruptor");
	if (dist_node != NULL)
	{
		strcpy(config_.yield, dist_node->Attribute("yield"));
	} 
	else
	{ 
		clog_error("[%s] y-quote.config error: Disruptor node missing.", module_name_); 
	}

    TiXmlElement *marketDataServerNode = root->FirstChildElement("MarketDataReceiver");
	strcpy(config_.MarketDataReceiverIp, marketDataServerNode->Attribute("ip"));
	this->config_.MarketDataReceiverPort = atoi(marketDataServerNode->Attribute("port"));
}

void UniConsumer::Start()
{
	running_  = true;

#ifdef  DCE_UDP_SEND_DATA
	InitMarketDataServer();
#endif	

#ifdef  DCE_TCP_SEND_DATA
	thread_lisnten_ = new std::thread(&UniConsumer::InitMarketDataServer, this);
#endif	

	int rc = 0;
	struct vrt_value  *vvalue;
	while (running_ &&
		   (rc = vrt_consumer_next(consumer_, &vvalue)) != VRT_QUEUE_EOF) 
	{
		if (rc == 0) 
		{
			struct vrt_hybrid_value *ivalue = cork_container_of(vvalue, struct vrt_hybrid_value, parent);
			switch (ivalue->data)
			{
				case DCE_YAO_DATA:
					ProcYaoQuote(ivalue->index);
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
	clog_warning("[%s] [start] start exit.", module_name_);
}

void UniConsumer::Stop()
{
	if(running_)
	{
		md_producer_->End();
		running_ = false;
		CloseMarketDataServer();
		clog_warning("[%s] End exit", module_name_);
	}
	fflush (Log::fp);
}

void UniConsumer::ProcYaoQuote(int32_t index)
{
	YaoQuote* md = md_producer_->GetData(index);

	clog_info("[%s] send YaoQuote: %s", 
				module_name_,
				YaoQuote::ToString(md).c_str());

	memcpy(send_buf_, md, sizeof(YaoQuote));

 #ifdef  DCE_UDP_SEND_DATA
	clog_info("[%s] ProcYaoQuote use UDP", module_name_);

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
#endif

#ifdef DCE_TCP_SEND_DATA
	clog_info("[%s] ProcYaoQuote use TCP", module_name_);

	for(int i=0; i<MAX_CONNECT_COUNT; i++)
	{
		if( connfd_[i] < 0) break;
		send(connfd_[i], send_buf_, sizeof(YaoQuote), 0);
	}
#endif
}

#ifdef  DCE_UDP_SEND_DATA
void UniConsumer::InitMarketDataServer()
{
	clog_info("[%s] InitMarketDataServer use UDP", module_name_);

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

    int sndbufsize = UDP_RCV_BUF_SIZE;
    int ret = setsockopt(local_sev_socket_, 
				SOL_SOCKET, 
				SO_SNDBUF , 
				(const void *) &sndbufsize, 
				sizeof(sndbufsize));
    if (ret != 0)
	{
        clog_error("UDP - set SO_SNDBUF failed.");
    }


	int opt_val = 0;
	int opt_len = sizeof(opt_val);
	getsockopt(local_sev_socket_, SOL_SOCKET, SO_RCVBUF, &opt_val, &opt_len);
	clog_warning("[%s] get SO_RCVBUF option: %d.", module_name_, opt_val);

	bzero(&marketdata_rev_socket_addr_, sizeof(marketdata_rev_socket_addr_));
	marketdata_rev_socket_addr_.sin_family = AF_INET;   
	marketdata_rev_socket_addr_.sin_addr.s_addr = inet_addr(config_.MarketDataReceiverIp); 
	marketdata_rev_socket_addr_.sin_port = htons(config_.MarketDataReceiverPort);  //网络字节序
}

void UniConsumer::CloseMarketDataServer()
{
	clog_info("[%s] CloseMarketDataServer  use UDP", module_name_);

	close(local_sev_socket_);
}

#endif

#ifdef  DCE_TCP_SEND_DATA
void UniConsumer::CloseMarketDataServer()
{
	clog_info("[%s] CloseMarketDataServer  use TCP", module_name_);

	for(int i=0; i<MAX_CONNECT_COUNT; i++)
	{
		if(connfd_[i] >= 0)
		{
			 close(connfd_[i]);
		}
	}

     close(listenfd_);
}

void UniConsumer::InitMarketDataServer()
{
	clog_info("[%s] InitMarketDataServer use TCP", module_name_);

     struct sockaddr_in  servaddr;
     char  buff[4096];
     int  n;
 
     if( (listenfd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	 {
          clog_error("[%s] create socket error: %s(errno: %d)\n",
					  module_name_,
					  strerror(errno),
					  errno);
     }
 
     memset(&servaddr, 0, sizeof(servaddr));
     servaddr.sin_family = AF_INET;
     servaddr.sin_addr.s_addr = inet_addr(config_.MarketDataReceiverIp);
     servaddr.sin_port = htons(config_.MarketDataReceiverPort);
 
     if( bind(listenfd_, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
	 {
         clog_error("[%s] bind socket error: %s(errno: %d)\n",
					 module_name_,
					 strerror(errno),
					 errno);
     }
 
     if( listen(listenfd_, MAX_CONNECT_COUNT) == -1)
	 {
         clog_error("[%s] listen socket error: %s(errno: %d)\n",
					 module_name_,
					 strerror(errno),
					 errno);
     }
 
	 int i = 0;
     while(running_)
	 {
         if( (connfd_[i] = accept(listenfd_, (struct sockaddr*)NULL, NULL)) == -1)
		 {
             clog_error("[%s] accept socket error: %s(errno: %d)",
						 module_name_,
						 strerror(errno),
						 errno);
             continue;
         }
		 i++;
     }
}

#endif
