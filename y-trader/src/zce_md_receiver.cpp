#include <functional>   // std::bind
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "zce_md_receiver.h"
#include "quote_cmn_utility.h"

using namespace std;
using namespace std::placeholders;

ZceQuote::ZceQuote(struct vrt_queue  *queue)
	:module_name_("ZceQuote")
{
	udp_fd_ = 0;
	ended_ = false;

	ParseConfig();

#ifdef PERSISTENCE_ENABLED 
	 p_save_quote_ = new QuoteDataSave<YaoQuote>( "y-zcequote", YAO_QUOTE_TYPE);
#endif

	this->producer_ = vrt_producer_new("zce_md_producer", 1, queue);
	this->producer_ ->yield = vrt_yield_strategy_threaded();

    thread_rev_ = new std::thread(&ZceQuote::RevData, this);
}

void ZceQuote::ParseConfig()
{
	TiXmlDocument config = TiXmlDocument("x-trader.config");
    config.LoadFile();
    TiXmlElement *RootElement = config.RootElement();    

	// yield strategy
    TiXmlElement *comp_node = RootElement->FirstChildElement("Disruptor");

	// addr
    TiXmlElement *md_node = RootElement->FirstChildElement("ZceMarketData");
	strcpy(config_.LocalIp, md_node->Attribute("localIp"));
	this->config_.Port = atoi(md_node->Attribute("port"));
}

ZceQuote::~ZceQuote()
{
#ifdef PERSISTENCE_ENABLED 
    if (p_save_quote_) delete p_save_quote_;
#endif
}

int ZceQuote::InitMDApi()
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

    // set nonblock flag
    //int socket_ctl_flag = fcntl(udp_client_fd, F_GETFL);
    //if (socket_ctl_flag < 0)
	//{
    //    clog_error("UDP - get socket control flag failed.");
    //}
    //if (fcntl(udp_client_fd, F_SETFL, socket_ctl_flag | O_NONBLOCK) < 0)
	//{
    //    clog_error("UDP - set socket control flag with nonblock failed.");
    //}

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

void ZceQuote::RevData()
{
	int udp_fd = InitMDApi();
	udp_fd_ = udp_fd; 
    if (udp_fd < 0) 
	{
        clog_error("[%s] MY_ZCE_MD - CreateUdpFD failed.",module_name_);
        return;
    }
	else
	{
        clog_warning("[%s] MY_ZCE_MD - CreateUdpFD succeeded.",module_name_);
	}

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
			YaoQuote *quote = (YaoQuote*)buf;

			clog_info("[%s] rev ZceYaoData:%s", 
					module_name_,
					YaoQuote::ToString(quote).c_str());

			struct vrt_value  *vvalue;
			struct vrt_hybrid_value  *ivalue;
			vrt_producer_claim(producer_, &vvalue);
			ivalue = cork_container_of (vvalue, struct vrt_hybrid_value, parent);
			ivalue->index = Push(*quote);
			ivalue->data = ZCE_YAO_DATA;
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


void ZceQuote::End()
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

int32_t ZceQuote::Push(const YaoQuote& md)
{
	static int32_t yaoQuote_cursor = ZCE_MD_BUFFER_SIZE - 1;
	yaoQuote_cursor++;
	if (yaoQuote_cursor % ZCE_MD_BUFFER_SIZE == 0)
	{
		yaoQuote_cursor = 0;
	}
	yaoQuote_buffer_[yaoQuote_cursor] = md;

	return yaoQuote_cursor;
}

YaoQuote* ZceQuote::GetData(int32_t index)
{
	return &yaoQuote_buffer_[index];
}


