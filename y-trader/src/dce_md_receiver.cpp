#include <functional>   // std::bind
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "dce_md_receiver.h"
#include "quote_cmn_utility.h"

using namespace std;
using namespace std::placeholders;


DceQuote::DceQuote(struct vrt_queue  *queue)
	:module_name_("DceQuote")
{
	udp_fd_ = 0;

	ended_ = false;
	ParseConfig();

#ifdef PERSISTENCE_ENABLED 
	 p_save_quote_ = new QuoteDataSave<YaoQuote>( "y-dcequote", YAO_QUOTE_TYPE);
#endif

	this->producer_ = vrt_producer_new("dce_quote", 1, queue);
	this->producer_ ->yield = vrt_yield_strategy_threaded();

    thread_rev_ = new std::thread(&DceQuote::RevData, this);
}

void DceQuote::ParseConfig()
{
	TiXmlDocument config = TiXmlDocument("x-trader.config");
    config.LoadFile();
    TiXmlElement *RootElement = config.RootElement();    

	// yield strategy
    TiXmlElement *comp_node = RootElement->FirstChildElement("Disruptor");

	// addr
    TiXmlElement *md_node = RootElement->FirstChildElement("DceMarketData");
	strcpy(config_.LocalIp, md_node->Attribute("localIp"));
	this->config_.Port = atoi(md_node->Attribute("port"));
}

DceQuote::~DceQuote()
{
#ifdef PERSISTENCE_ENABLED 
    if (p_save_quote_) delete p_save_quote_;
#endif
}


void DceQuote::RevData()
{
	int udp_fd = InitMDApi();
	udp_fd_ = udp_fd; 
    if (udp_fd < 0) 
	{
        clog_error("[%s] MY_DCE_MD - CreateUdpFD failed.",module_name_);
        return;
    }
	else
	{
        clog_warning("[%s] MY_DCE_MD - CreateUdpFD succeeded.",module_name_);
	}

    char buf[2048];
    int data_len = 0;
    sockaddr_in src_addr;
    unsigned int addr_len = sizeof(sockaddr_in);
    while (!ended_)
	{
#ifdef  DCE_UDP_SEND_DATA
        data_len = recvfrom(udp_fd, 
					buf, 
					sizeof(buf), 
					0, 
					(sockaddr *) &src_addr, 
					&addr_len);
#endif

#ifdef  DCE_TCP_SEND_DATA
        data_len  = recv(udp_fd_, buf, sizeof(buf), 0);
#endif

        if (data_len > 0)
		{
			YaoQuote *quote = (YaoQuote*)buf;

			clog_info("[%s] rev DceYaoData:%s", 
					module_name_,
					YaoQuote::ToString(quote).c_str());

			struct vrt_value  *vvalue;
			struct vrt_hybrid_value  *ivalue;
			vrt_producer_claim(producer_, &vvalue);
			ivalue = cork_container_of (vvalue, struct vrt_hybrid_value, parent);
			ivalue->index = Push(*quote);
			ivalue->data = DCE_YAO_DATA;
			vrt_producer_publish(producer_);

#ifdef PERSISTENCE_ENABLED 
				timeval t;
				gettimeofday(&t, NULL);
				p_save_quote_->OnQuoteData(t.tv_sec * 1000000 + t.tv_usec, quote);
#endif
        } // end if (data_len > 0)
    } // while (running_flag_)

	clog_warning("[%s] RevData exit.",module_name_);
}


void DceQuote::End()
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

int32_t DceQuote::Push(const YaoQuote& md)
{
	static int32_t yaoQuote_cursor = DCE_MD_BUFFER_SIZE - 1;
	yaoQuote_cursor++;
	if (yaoQuote_cursor % DCE_MD_BUFFER_SIZE == 0)
	{
		yaoQuote_cursor = 0;
	}
	yaoQuote_buffer_[yaoQuote_cursor] = md;

	return yaoQuote_cursor;
}

YaoQuote* DceQuote::GetData(int32_t index)
{
	return &yaoQuote_buffer_[index];
}


#ifdef  DCE_UDP_SEND_DATA
int DceQuote::InitMDApi()
{
    // init udp socket
    int udp_client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int son = 1;
    setsockopt(udp_client_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &son, sizeof(son));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; //IPv4协议
    servaddr.sin_addr.s_addr = inet_addr(config_.LocalIp);
    servaddr.sin_port = htons(config_.Port);
    if (bind(udp_client_fd, (sockaddr *) &servaddr, sizeof(servaddr)) != 0)
	{
        clog_error("[%s] UDP - bind failed: %s:%d", 
					module_name_, 
					config_.LocalIp,config_.Port);
        return -1;
    }
	else
	{
        clog_error("[%s] UDP - bind succeeded: %s:%d", 
					module_name_, 
					config_.LocalIp,
					config_.Port);
	}

    int rcvbufsize = UDP_RCV_BUF_SIZE;
    int ret = setsockopt(udp_client_fd, 
				SOL_SOCKET, 
				SO_RCVBUF, 
				(const void *) &rcvbufsize, 
				sizeof(rcvbufsize));
    if (ret != 0)
	{
        clog_error("UDP - set SO_RCVBUF failed.");
    }

    return udp_client_fd;
}
#endif

#ifdef  DCE_TCP_SEND_DATA
int DceQuote::InitMDApi()
{
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1) 
	{
		 clog_error("[%s] accept socket error: %s(errno: %d)",
					 module_name_,
					 strerror(errno),
					 errno);
    }
    // connect
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config_.Port);
    serverAddr.sin_addr.s_addr = inet_addr(config_.LocalIp);
    if (connect(client, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		 clog_error("[%s] accept socket error: %s(errno: %d)",
					 module_name_,
					 strerror(errno),
					 errno);
    }

	return client;
}

#endif
