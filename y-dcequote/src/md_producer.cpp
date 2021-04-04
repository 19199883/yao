#include <functional>   // std::bind
#include <sys/types.h>
#include <sys/time.h>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "md_producer.h"
#include "quote_cmn_utility.h"
#include "perfctx.h"
#include "quote_datatype_dce_level2.h"
#include "yao_utils.h"

using namespace std;
using namespace std::placeholders;

static void Convert(const MDBestAndDeep &other, YaoQuote &data)
{
	//	交易所行情时间(HHMMssmmm), 如：90000306表示09:00:00 306. 0点-3点的数据 +24hrs
	int updateMillisec = atoi(other.GenTime + 9);
	data.int_time = YaoQuoteHelper::GetIntTime(other.GenTime, updateMillisec);	// GenTime=09:43:41.895
	data.pre_close_px = InvalidToZeroF(other.LastClose);           //昨收盘
    data.pre_settle_px = InvalidToZeroF(other.LastClearPrice); //昨结算价
	data.pre_open_interest = other.LastOpenInterest;             //昨持仓量
	data.open_interest = other.OpenInterest;                     //持仓量
	data.open_px = InvalidToZeroF(other.OpenPrice);           //今开盘
	data.high_px = InvalidToZeroF(other.HighPrice);           //最高价
	data.low_px = InvalidToZeroF(other.LowPrice);             //最低价
	data.avg_px = InvalidToZeroF(other.AvgPrice);             //成交均价
	data.last_px = InvalidToZeroF(other.LastPrice);           //最新价
		
	data.bp_array[0] = InvalidToZeroF(other.BuyPriceOne);		//买入价格1 	
	data.bp_array[1] = InvalidToZeroF(other.BuyPriceTwo);	
	data.bp_array[2] = InvalidToZeroF(other.BuyPriceThree);	
	data.bp_array[3] = InvalidToZeroF(other.BuyPriceFour);	
	data.bp_array[4] = InvalidToZeroF(other.BuyPriceFive);

	data.ap_array[0] = InvalidToZeroF(other.SellPriceOne);     //卖出价格1	
	data.ap_array[1] = InvalidToZeroF(other.SellPriceTwo); 	
	data.ap_array[2] = InvalidToZeroF(other.SellPriceThree); 	
	data.ap_array[3] = InvalidToZeroF(other.SellPriceFour);	
	data.ap_array[4] = InvalidToZeroF(other.SellPriceFive);

	data.bv_array[0] = other.BuyQtyOne;		
	data.bv_array[1] = other.BuyQtyTwo; 		
	data.bv_array[2] = other.BuyQtyThree;		
	data.bv_array[3] = other.BuyQtyFour;		
	data.bv_array[4] = other.BuyQtyFive; 

	data.av_array[0] = other.SellQtyOne;  		
	data.av_array[1] = other.SellQtyTwo;   		
	data.av_array[2] = other.SellQtyThree; 		
	data.av_array[3] = other.SellQtyFour; 		
	data.av_array[4] = other.SellQtyFive; 

	data.total_vol = other.MatchTotQty;                       //成交数量
	data.total_notional = InvalidToZeroD(other.Turnover);             //成交金额	 
	data.upper_limit_px = InvalidToZeroF(other.RiseLimit);           //涨停价
	data.lower_limit_px = InvalidToZeroF(other.FallLimit);	//	跌停价
	data.close_px = InvalidToZeroF(other.Close);                   //今收盘
	data.settle_px = InvalidToZeroF(other.ClearPrice);         //今结算价
	
	data.implied_bid_size[0] = other.BuyImplyQtyOne;
	data.implied_bid_size[1] = other.BuyImplyQtyTwo; 
	data.implied_bid_size[2] = other.BuyImplyQtyThree; 
	data.implied_bid_size[3] = other.BuyImplyQtyFour;
	data.implied_bid_size[4] = other.BuyImplyQtyFive; 
	
	data.implied_ask_size[0]	= other.SellImplyQtyOne;
    data.implied_ask_size[1]	= other.SellImplyQtyTwo;
    data.implied_ask_size[2]	= other.SellImplyQtyThree;
    data.implied_ask_size[3]	= other.SellImplyQtyFour;
    data.implied_ask_size[4]	= other.SellImplyQtyFive;
}

MDProducer::MDProducer(struct vrt_queue  *queue)
	:module_name_("MDProducer")
{

	memset(orderstat_buffer_, 0, sizeof(orderstat_buffer_));
	memset(depth_buffer_, 0, sizeof(depth_buffer_));

#ifdef PERSISTENCE_ENABLED 
	 p_save_quote_ = new QuoteDataSave<YaoQuote>( "y-dcequote", YAO_QUOTE_TYPE);
#endif
	udp_fd_ = 0;

	ended_ = false;
	clog_warning("[%s] MAX_DOMINANT_CONTRACT_COUNT: %d;", 
				module_name_, 
				MAX_DOMINANT_CONTRACT_COUNT);

	ParseConfig();

	// init dominant contracts
	memset(dominant_contracts_, 0, sizeof(dominant_contracts_));
	dominant_contract_count_ = 
		LoadDominantContracts(config_.contracts_file, 
					dominant_contracts_);

	this->producer_ = vrt_producer_new("md_producer", 1, queue);
	clog_warning("[%s] yield:%s", module_name_, config_.yield); 
	if(strcmp(config_.yield, "threaded") == 0)
	{
		this->producer_ ->yield = vrt_yield_strategy_threaded();
	}
	else if(strcmp(config_.yield, "spin") == 0)
	{
		this->producer_ ->yield = vrt_yield_strategy_spin_wait();
	}
	else if(strcmp(config_.yield, "hybrid") == 0)
	{
		this->producer_ ->yield = vrt_yield_strategy_hybrid();
	}

    thread_rev_ = new std::thread(&MDProducer::RevData, this);
}

void MDProducer::ParseConfig()
{
	TiXmlDocument config = TiXmlDocument("x-trader.config");
    config.LoadFile();
    TiXmlElement *RootElement = config.RootElement();    

	// yield strategy
    TiXmlElement *comp_node = RootElement->FirstChildElement("Disruptor");
	if (comp_node != NULL)
	{
		strcpy(config_.yield, comp_node->Attribute("yield"));
	} 
	else 
	{ 
		clog_error("[%s] y-dcequote.config error: Disruptor node missing.", module_name_); 
	}

	// addr
    TiXmlElement *fdmd_node = RootElement->FirstChildElement("Md");
	if (fdmd_node != NULL)
	{
		config_.addr = fdmd_node->Attribute("addr");
	} 
	else 
	{ 
		clog_error("[%s] x-shmd.config error: FulldepthMd node missing.", module_name_); 
	}

	// contracts file
    TiXmlElement *contracts_file_node = RootElement->FirstChildElement("Subscription");
	if (contracts_file_node != NULL){
		strcpy(config_.contracts_file, contracts_file_node->Attribute("contracts"));
	}
	else 
	{
		clog_error("[%s] x-shmd.config error: Subscription node missing.", module_name_); 
	}

	size_t ipstr_start = config_.addr.find("//")+2;
	size_t ipstr_end = config_.addr.find(":",ipstr_start);
	strcpy(config_.ip, config_.addr.substr(ipstr_start,ipstr_end-ipstr_start).c_str());
	config_.port = stoi(config_.addr.substr(ipstr_end+1));
}

MDProducer::~MDProducer()
{
#ifdef PERSISTENCE_ENABLED 
    if (p_save_quote_) delete p_save_quote_;
#endif
}

int MDProducer::InitMDApi()
{
    // init udp socket
    int udp_client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    /* set reuse and non block for this socket */
    int son = 1;
    setsockopt(udp_client_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &son, sizeof(son));

    // bind address and port
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; //IPv4协议
    servaddr.sin_addr.s_addr = inet_addr(config_.ip);
    servaddr.sin_port = htons(config_.port);
    if (bind(udp_client_fd, (sockaddr *) &servaddr, sizeof(servaddr)) != 0)
	{
        clog_error("[%s] UDP - bind failed: %s:%d", 
					module_name_, 
					config_.ip,config_.port);
        return -1;
    }

    // set nonblock flag
   // int socket_ctl_flag = fcntl(udp_client_fd, F_GETFL);
   // if (socket_ctl_flag < 0)
   // {
   //     clog_error("UDP - get socket control flag failed.");
   // }
   // if (fcntl(udp_client_fd, F_SETFL, socket_ctl_flag | O_NONBLOCK) < 0)
   // {
   //     clog_error("UDP - set socket control flag with nonblock failed.");
   // }

    // set buffer length
    int rcvbufsize = UDP_RCV_BUF_SIZE;
    int ret = setsockopt(udp_client_fd, SOL_SOCKET, SO_RCVBUF, 
			(const void *) &rcvbufsize, sizeof(rcvbufsize));
    if (ret != 0)
	{
        clog_error("UDP - set SO_RCVBUF failed.");
    }

    int broadcast_on = 1;
    ret = setsockopt(udp_client_fd, SOL_SOCKET, SO_BROADCAST, 
			&broadcast_on, sizeof(broadcast_on));
    if (ret != 0)
	{
        clog_error("UDP - set SO_BROADCAST failed.");
    }

    return udp_client_fd;
}

YaoQuote* MDProducer::ProcessDepthData(MDBestAndDeep* depthdata )
{
	clog_info("[%s] rev MDBestAndDeep: %s", 
				module_name_,
				DceQuoteFormat::ToString(depthdata).c_str());

	YaoQuote* valid_quote = NULL;

	YaoQuote *quote = this->GetDepthData(depthdata->Contract);
	if(NULL == quote)
	{
		quote = this->GetNewDepthData();
		quote->feed_type = FeedTypes::DceCombine;    
		memcpy(quote->symbol, depthdata->Contract, sizeof(quote->symbol)); 
		quote->exchange = YaoExchanges::YDCE; 
	}
	Convert(*depthdata, *quote);

	MDOrderStatistic* orderStat = this->GetOrderStatData(depthdata->Contract);
	if(NULL == orderStat)
	{
		valid_quote = NULL;
    //    clog_info("[%s] can not find MDOrderStatistic: %s", 
	//				module_name_,
	//				depthdata->Contract);
	}
	else
	{
		quote->total_buy_ordsize =  orderStat->TotalBuyOrderNum;	
		quote->total_sell_ordsize = orderStat->TotalSellOrderNum;
		quote->weighted_buy_px =  InvalidToZeroD(orderStat->WeightedAverageBuyOrderPrice);   
		quote->weighted_sell_px = InvalidToZeroD(orderStat->WeightedAverageSellOrderPrice);

		valid_quote = quote;
	}

	return valid_quote;
}

void MDProducer::RevData()
{
	int udp_fd = InitMDApi();
	udp_fd_ = udp_fd; 
    if (udp_fd < 0) 
	{
        clog_error("[%s] MY_SHFE_MD - CreateUdpFD failed.",module_name_);
        return;
    }
	else
	{
        clog_warning("[%s] MY_SHFE_MD - CreateUdpFD succeeded.",module_name_);
	}

    clog_debug("[%s] DCE_UDP-sizeof(MDBestAndDeep):%u", 
				module_name_, 
				sizeof(MDBestAndDeep));
    clog_debug("[%s] DCE_UDP-sizeof(MDOrderStatistic):%u", 
				module_name_, 
				sizeof(MDOrderStatistic));

    char buf[2048];
    int data_len = 0;
    sockaddr_in src_addr;
    unsigned int addr_len = sizeof(sockaddr_in);
    while (!ended_)
	{
        data_len = recvfrom(udp_fd, 
					buf, 
					sizeof(buf), 
					0, 
					(sockaddr *) &src_addr, 
					&addr_len);
        if (data_len > 0)
		{
			YaoQuote *quote = NULL;
            int type = (int) buf[0];
            if (type == EDataType::eMDBestAndDeep)
			{
                MDBestAndDeep * p = (MDBestAndDeep *) (buf + 1);

				// discard option
				if(strlen(p->Contract) > 6)
				{
					continue;
				}

				quote = ProcessDepthData(p);
            }
			else if (type == EDataType::eMDOrderStatistic)
			{
                MDOrderStatistic * p = (MDOrderStatistic *) (buf + 1);

				// discard option
				if(strlen(p->ContractID) > 6)
				{
					continue;
				}

				quote = this->ProcessOrderStatData(p);
            }

			if(NULL != quote)
			{
#ifdef PERSISTENCE_ENABLED 
				timeval t;
				gettimeofday(&t, NULL);
				p_save_quote_->OnQuoteData(t.tv_sec * 1000000 + t.tv_usec, quote);
#endif
				if(!(IsDominant(quote->symbol))) continue; // 抛弃非主力合约

				struct vrt_value  *vvalue;
				struct vrt_hybrid_value  *ivalue;
				vrt_producer_claim(producer_, &vvalue);
				ivalue = cork_container_of (vvalue, struct vrt_hybrid_value, parent);
				ivalue->index = Push(*quote);
				ivalue->data = DCE_YAO_DATA;
				vrt_producer_publish(producer_);
			}
        } // end if (data_len > 0)
    } // while (running_flag_)

	clog_warning("[%s] RevData exit.",module_name_);
}


void MDProducer::End()
{
	if(!ended_)
	{
		ended_ = true;

		shutdown(udp_fd_, SHUT_RDWR);
		int err = close(udp_fd_);
		clog_warning("close udp:%d.", err); 
		thread_rev_->join();

		vrt_producer_eof(producer_);
		clog_warning("[%s] End exit", module_name_);
	}
}

int32_t MDProducer::Push(const YaoQuote& md)
{
	static int32_t yaoQuote_cursor = MD_BUFFER_SIZE - 1;
	yaoQuote_cursor++;
	if (yaoQuote_cursor % MD_BUFFER_SIZE == 0)
	{
		yaoQuote_cursor = 0;
	}
	yaoQuote_buffer_[yaoQuote_cursor] = md;

	return yaoQuote_cursor;
}

YaoQuote* MDProducer::GetData(int32_t index)
{
	return &yaoQuote_buffer_[index];
}

bool MDProducer::IsDominant(const char *contract)
{
	return IsDominantImp((char*)contract, dominant_contracts_, dominant_contract_count_);
}


YaoQuote* MDProducer::GetNewDepthData()
{
	for(int i=0; i<MAX_DOMINANT_CONTRACT_COUNT; i++)
	{
		if(0 == depth_buffer_[i].symbol[0])
		{
			return &(depth_buffer_[i]);
		}
	}

	return NULL;
}

MDOrderStatistic* MDProducer::GetNewOrderStatData()
{
	for(int i=0; i<MAX_DOMINANT_CONTRACT_COUNT; i++)
	{
		if(0 == orderstat_buffer_[i].ContractID[0])
		{
			return &(orderstat_buffer_[i]);
		}
	}

	return NULL;
}

YaoQuote* MDProducer::GetDepthData(const char* contract)
{
	for(int i=0; i<MAX_DOMINANT_CONTRACT_COUNT; i++)
	{
		if(0 == strcmp(depth_buffer_[i].symbol,contract))
		{
			return &(depth_buffer_[i]);
		}
	}

	return NULL;
}

MDOrderStatistic* MDProducer::GetOrderStatData(const char* contract)
{
	for(int i=0; i<MAX_DOMINANT_CONTRACT_COUNT; i++)
	{
		if(0 == strcmp(orderstat_buffer_[i].ContractID, contract))
		{
			return &(orderstat_buffer_[i]);
		}
	}

	return NULL;
}

YaoQuote* MDProducer::ProcessOrderStatData(MDOrderStatistic* newOrderStat)
{
	clog_info("[%s] rev MDOrderStatistic: %s", 
				module_name_,
				DceQuoteFormat::ToString(newOrderStat).c_str());

	YaoQuote* valid_quote = NULL;

	MDOrderStatistic* orderStat = this->GetOrderStatData(newOrderStat->ContractID);
	if(NULL == orderStat)
	{
		orderStat = this->GetNewOrderStatData();
	}
	*orderStat = *newOrderStat;

	YaoQuote* quote = this->GetDepthData(newOrderStat->ContractID);
	if(NULL == quote)
	{
		valid_quote = NULL;
    //    clog_info("[%s] can not find MDBestAndDeep %s", 
	//				module_name_,
	//				newOrderStat->ContractID);
	}
	else
	{
		quote->total_buy_ordsize =  orderStat->TotalBuyOrderNum;	
		quote->total_sell_ordsize = orderStat->TotalSellOrderNum;
		quote->weighted_buy_px =  InvalidToZeroD(orderStat->WeightedAverageBuyOrderPrice);   
		quote->weighted_sell_px = InvalidToZeroD(orderStat->WeightedAverageSellOrderPrice);

		valid_quote = quote;
	}

	return valid_quote;
}

