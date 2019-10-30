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
	memset(valid_conn_, 0, sizeof(valid_conn_));
	for(int i=0; i<MAX_CONN_COUNT; i++){
		socks_.push_back(tcp::socket(io_service_));
	}

	ParseConfig();
	(this->consumer_ = vrt_consumer_new(module_name_, queue));

	clog_warning("[%s] yield:%s", module_name_, config_.yield); 
	if(strcmp(config_.yield, "threaded") == 0){
		this->consumer_->yield = vrt_yield_strategy_threaded();
	}
	else if(strcmp(config_.yield, "spin") == 0){
		this->consumer_->yield = vrt_yield_strategy_spin_wait();
	}
	else if(strcmp(config_.yield, "hybrid") == 0){
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
	std::string config_file = "y-quote.config";
	TiXmlDocument doc = TiXmlDocument(config_file.c_str());
    doc.LoadFile();
    TiXmlElement *root = doc.RootElement();    
	
	// yield strategy
    TiXmlElement *dist_node = root->FirstChildElement("Disruptor");
	if (dist_node != NULL){
		strcpy(config_.yield, dist_node->Attribute("yield"));
		this->port_ = atoi(dist_node->Attribute("port"));
	} 
	else { 
		clog_error("[%s] y-quote.config error: Disruptor node missing.", module_name_); 
	}
}

void UniConsumer::Start()
{
	running_  = true;

	int rc = 0;
	struct vrt_value  *vvalue;
	while (running_ &&
		   (rc = vrt_consumer_next(consumer_, &vvalue)) != VRT_QUEUE_EOF) {
		if (rc == 0) {
			struct vrt_hybrid_value *ivalue = cork_container_of(vvalue, struct vrt_hybrid_value, parent);
			switch (ivalue->data){
				case YAO_QUOTE:
					ProcYaoQuote(ivalue->index);
					break;
				default:
					clog_info("[%s] [start] unexpected index: %d", module_name_, ivalue->index);
					break;
			}
		}
	} // end while (running_ &&

	if (rc == VRT_QUEUE_EOF) {
		clog_info("[%s] [start] rev EOF.", module_name_);
	}
	clog_info("[%s] [start] start exit.", module_name_);
}

void UniConsumer::Stop()
{
	if(running_){
		md_producer_->End();
		running_ = false;
		clog_warning("[%s] End exit", module_name_);
	}
	fflush (Log::fp);
}

void UniConsumer::ProcYaoQuote(int32_t index)
{
#ifdef LATENCY_MEASURE
		 static int cnt = 0;
		 perf_ctx::insert_t0(cnt);
		 cnt++;
#endif
#ifdef LATENCY_MEASURE
		high_resolution_clock::time_point t0 = high_resolution_clock::now();
#endif

	YaoQuote* md = md_producer_->GetData(index);

	clog_info("[%s] [ProcYaoQuote] index: %d; contract: %s",
				module_name_, 
				index, 
				md->symbol);

	for(int i=0; i< MAX_CONN_COUNT; i++){		
		{
			std::lock_guard<std::mutex> lck (mtx_);
			if(0 == valid_conn_[i]) continue;
		}
		
		try{			
			 boost::system::error_code error;		  		
			 boost::asio::write(socks_[i], boost::asio::buffer(md, sizeof(YaoQuote)), error);			  
			 if (error){
				 valid_conn_[i] = 0;
				clog_warning("[%s] write error: %d", module_name_, error); 			
			 }
		}
		catch (std::exception& e){
			valid_conn_[i] = 0;
			clog_warning("[%s] send error:%s", module_name_, e.what()); 			
		}

#ifdef LATENCY_MEASURE
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		int latency = (t1.time_since_epoch().count() - t0.time_since_epoch().count()) / 1000;
		clog_warning("[%s] ProcBestAndDeep latency:%d us", module_name_, latency); 
#endif
	}
}

void UniConsumer::Server()
{
  tcp::acceptor a(io_service_, tcp::endpoint(tcp::v4(), port_));
  for (;;)
  {	
	int i = 0;
	for(; i<MAX_CONN_COUNT; i++){
		if(0 == valid_conn_[i]) break;
	}
	if(i < MAX_CONN_COUNT){
		a.accept(socks_[i]);
		{
			std::lock_guard<std::mutex> lck (mtx_);
			valid_conn_[i] = 1;
		}
	}
	else{
		clog_warning("[%s] socks_ is full.", module_name_);
	}
  }
}

