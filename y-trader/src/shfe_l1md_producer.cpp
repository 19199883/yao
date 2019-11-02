#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <functional>   // std::bind
#include "shfe_l1md_producer.h"
#include "quote_cmn_utility.h"
#include <tinyxml.h>
#include <tinystr.h>

using namespace std::placeholders;
using namespace std;

CDepthMarketDataField* ShfeL1MDProducerHelper::GetLastDataImp(const char *contract, 
			int32_t last_index, 
			CDepthMarketDataField *buffer, 
			int32_t buffer_size) 
{
	CDepthMarketDataField* data = NULL;

	// 全息行情需要一档行情时，从缓存最新位置向前查找13个
	// 位置（假设有13个主力合约），找到即停
	int i = 0;
	for(; i<traverse_count; i++)
	{
		int data_index = last_index - i;
		if(data_index < 0)
		{
			data_index = data_index + buffer_size;
		}

		CDepthMarketDataField &tmp = buffer[data_index];
		if(IsEqualContract((char*)contract, tmp.InstrumentID))
		{
			data = &tmp; 
			break;
		}
	}

	return data;
}

ShfeL1MDProducer::ShfeL1MDProducer(struct vrt_queue  *queue) 
		: module_name_("ShfeL1MDProducer")
{
	l1data_cursor_ = L1MD_BUFFER_SIZE - 1;
	is_multicast_ = false;
	ended_ = false;
    api_ = NULL;
	clog_warning("[%s] L1MD_BUFFER_SIZE:%d;",
				module_name_,
				L1MD_BUFFER_SIZE);

	ParseConfig();

	// init dominant contracts
	memset(dominant_contracts_, 0, sizeof(dominant_contracts_));
	contract_count_ = LoadDominantContracts(config_.contracts_file, 
				dominant_contracts_);

	memset(&md_buffer_, 0, sizeof(md_buffer_));
	InitMDApi();

	this->producer_ = vrt_producer_new("l1md_producer", 1, queue);
	clog_warning("[%s] yield:%s", module_name_, config_.yield); 
	if(strcmp(config_.yield, "threaded") == 0)
	{
		this->producer_ ->yield	= vrt_yield_strategy_threaded();
	}
	else if(strcmp(config_.yield, "spin") == 0)
	{
		this->producer_ ->yield	= vrt_yield_strategy_spin_wait();
	}
	else if(strcmp(config_.yield, "hybrid") == 0)
	{
		this->producer_ ->yield	 = vrt_yield_strategy_hybrid();
	}
}

void ShfeL1MDProducer::ParseConfig()
{
	TiXmlDocument config = TiXmlDocument("x-trader.config");
    config.LoadFile();
    TiXmlElement *RootElement = config.RootElement();    

	// yield strategy
    TiXmlElement *disruptor_node = RootElement->FirstChildElement("Disruptor");
	if (disruptor_node != NULL)
	{
		strcpy(config_.yield, disruptor_node->Attribute("yield"));
	} 
	else 
	{ 
		clog_error("[%s] x-shmd.config error: Disruptor "
				"node missing.", 
				module_name_); 
	}

    TiXmlElement *l1md_node = RootElement->FirstChildElement("ShfeL1Md");
	if (l1md_node != NULL)
	{
		strcpy(config_.efh_sf_eth, l1md_node->Attribute("efh_sf_eth"));

		strcpy(config_.mcIp, l1md_node->Attribute("mcIp"));
		int mcPort = 0;
		 l1md_node->QueryIntAttribute("mcPort", &mcPort);
		this->config_.mcPort = mcPort;

		strcpy(config_.mcLocalIp, l1md_node->Attribute("mcLocalIp"));
		int localUDPPort = 0;
		 l1md_node->QueryIntAttribute("mcLocalPort", &localUDPPort);
		this->config_.mcLocalPort = localUDPPort;

		strcpy(config_.userid, l1md_node->Attribute("userid"));
		strcpy(config_.password, l1md_node->Attribute("password"));

		strcpy(config_.is_multicast, l1md_node->Attribute("isMulticast"));
		if(strcmp(config_.is_multicast, "true")==0)
		{
			is_multicast_ = true;
		}
		else
		{ 
			is_multicast_ = false;
		}
		clog_warning("[%s] is_multicast:%d ", module_name_,is_multicast_); 

	} 
	else
	{
		clog_error("[%s] x-shmd.config error: ShfeL1Md node missing.", module_name_); 
	}
	
	// contracts file
    TiXmlElement *contracts_file_node = 
		RootElement->FirstChildElement("Subscription");
	if (contracts_file_node != NULL)
	{
		strcpy(config_.contracts_file, 
					contracts_file_node->Attribute("contracts"));
	} 
	else 
	{ 
		clog_error("[%s] x-shmd.config error: Subscription "
					"node missing.", 
					module_name_); 
	}
}

ShfeL1MDProducer::~ShfeL1MDProducer()
{
}


int32_t ShfeL1MDProducer::Push(const CDepthMarketDataField& md){
	l1data_cursor_++;
	if (l1data_cursor_ % L1MD_BUFFER_SIZE == 0)
	{
		l1data_cursor_ = 0;
	}
	md_buffer_[l1data_cursor_] = md;

	return l1data_cursor_;
}

CDepthMarketDataField* ShfeL1MDProducer::GetData(int32_t index)
{
	return &md_buffer_[index];
}

// lic
CDepthMarketDataField* ShfeL1MDProducer::GetLastDataForIllegaluser(const char *contract)
{
	CDepthMarketDataField* data = 
		ShfeL1MDProducerHelper::GetLastDataImp( contract,
					0,
					md_buffer_,
					L1MD_BUFFER_SIZE);
	return data;
}

CDepthMarketDataField* ShfeL1MDProducer::GetLastData(const char *contract, 
			int32_t last_index)
{
	CDepthMarketDataField* data = 
		ShfeL1MDProducerHelper::GetLastDataImp(contract,
					last_index,
					md_buffer_,
					L1MD_BUFFER_SIZE);
	return data;
}

bool ShfeL1MDProducer::IsDominant(const char *contract)
{
#ifdef PERSISTENCE_ENABLED 
	// 持久化行情时，需要记录所有合约
	//clog_warning("[%s] return TRUE in IsDominant.",module_name_);
	return true;
#else
	return IsDominantImp((char*)contract, 
				dominant_contracts_, 
				contract_count_);
#endif
}

void ShfeL1MDProducer::ToString(CDepthMarketDataField &data)
{
	clog_warning("[%s] CDepthMarketDataField\n"
		"TradingDay:%s\n"
		"SettlementGroupID:%s\n"
		"SettlementID:%d\n"
		"LastPrice:%f \n"
		"PreSettlementPrice:%f \n"
		"PreClosePrice:%f \n"
		"PreOpenInterest:%f \n"
		"OpenPrice:%f \n"
		"HighestPrice:%f \n"
		"LowestPrice:%f \n"
		"Volume:%d \n"
		"Turnover:%f \n"
		"OpenInterest:%f \n"
		"ClosePrice:%f \n"
		"SettlementPrice:%f \n"
		"UpperLimitPrice:%f \n"
		"LowerLimitPrice:%f \n"
		"PreDelta:%f \n"
		"CurrDelta:%f \n"
		"UpdateTime[9]:%s \n"
		"UpdateMillisec:%d \n"
		"InstrumentID:%s \n"
		"BidPrice1:%f \n"
		"BidVolume1:%d \n"
		"AskPrice1:%f \n"
		"AskVolume1:%d \n"
		"BidPrice2:%f \n"
		"BidVolume2:%d \n"
		"AskPrice2:%f \n"
		"AskVolume2:%d \n"
		"BidPrice3:%f \n"
		"BidVolume3:%d \n"
		"AskPrice3:%f \n"
		"AskVolume3:%d \n"
		"BidPrice4:%f \n"
		"BidVolume4:%d \n"
		"AskPrice4:%f \n"
		"AskVolume4:%d \n"
		"BidPrice5:%f \n"
		"BidVolume5:%d \n"
		"AskPrice5:%f \n"
		"AskVolume5:%d \n"
		"ActionDay:%s \n",
		module_name_,
		data.TradingDay,
		data.SettlementGroupID,
		data.SettlementID,
		data.LastPrice,
		data.PreSettlementPrice,
		data. PreClosePrice,
		data.PreOpenInterest,
		data.OpenPrice,
		data. HighestPrice,
		data. LowestPrice,
		data.Volume,
		data.Turnover,
		data.OpenInterest,
		data.ClosePrice,
		data.SettlementPrice,
		data.UpperLimitPrice,
		data.LowerLimitPrice,
		data.PreDelta,
		data.CurrDelta,
		data.UpdateTime,
		data.UpdateMillisec,
		data.InstrumentID,
		data.BidPrice1,
		data.BidVolume1,
		data.AskPrice1,
		data.AskVolume1,
		data.BidPrice2,
		data.BidVolume2,
		data.AskPrice2,
		data.AskVolume2,
		data.BidPrice3,
		data.BidVolume3,
		data.AskPrice3,
		data.AskVolume3,
		data.BidPrice4,
		data.BidVolume4,
		data.AskPrice4,
		data.AskVolume4,
		data.BidPrice5,
		data.BidVolume5,
		data.AskPrice5,
		data.AskVolume5,
		data.ActionDay);

}

/////////////////
//A使用飞马UDP行情
/////////////////
#ifdef FEMAS_TOPSPEED_QUOTE
void ShfeL1MDProducer::OnFrontConnected()
{
    clog_warning("[%s] shfe_ex(CTP): OnFrontConnected", module_name_);

    CThostFtdcReqUserLoginField req_login;
    memset(&req_login, 0, sizeof(CThostFtdcReqUserLoginField));
    api_->ReqUserLogin(&req_login, 0);

    clog_warning("[%s] CTP - request logon", module_name_);
}

void ShfeL1MDProducer::OnFrontDisconnected(int nReason)
{
    clog_warning("[%s] CTP - OnFrontDisconnected, nReason: %d", 
				module_name_, 
				nReason);
}

void ShfeL1MDProducer::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, 
			CThostFtdcRspInfoField *pRspInfo, 
			int nRequestID, 
			bool bIsLast)
{
    int error_code = pRspInfo ? pRspInfo->ErrorID : 0;
    clog_warning("[%s] CTP - OnRspUserLogin, error code: %d", 
				module_name_,
				error_code);

	std::ifstream is;
	int count = 0;
	is.open (config_.contracts_file);
	string contrs = "";
	if (is) 
	{
		getline(is, contrs);
		contrs += " ";
		size_t start_pos = 0;
		size_t end_pos = 0;
		string contr = "";
		while ((end_pos=contrs.find(" ",start_pos)) != string::npos)
		{
			contr = contrs.substr (start_pos, end_pos-start_pos);
			pp_instruments_[count] = new char(strlen(contr.c_str())+1);
			strcpy(pp_instruments_[count],contr.c_str());
			clog_warning("[%s] ThostFtdcMdApi subscribe:%d, %s",module_name_, count, 
						pp_instruments_[count]);
			start_pos = end_pos + 1;
			count++;
		}
	}
	else 
	{ 
		clog_error("[%s] ThostFtdcMdApi can't open: %s", 
					module_name_, 
					config_.contracts_file); 
	}

    if (error_code == 0)
	{
        int err = api_->SubscribeMarketData(pp_instruments_, count);
			clog_warning("[%s] TSubscribeMarketData:%d",module_name_,err);
    } 
	else 
	{
        std::string err_str("null");
        if (pRspInfo && pRspInfo->ErrorMsg[0] != '\0') 
		{
            err_str = pRspInfo->ErrorMsg;
        }
        clog_warning("[%s] CTP-Logon fail, error code: %d; error info: %s",
					module_name_, 
					error_code, 
					err_str.c_str());
    }
}

void ShfeL1MDProducer::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
			CThostFtdcRspInfoField *pRspInfo, 
			int nRequestID, 
			bool bIsLast)
{
    clog_warning("[%s] CTP - OnRspSubMarketData, code: %s", 
				module_name_, 
				pSpecificInstrument->InstrumentID);
    if (bIsLast) { }
}

void ShfeL1MDProducer::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
			CThostFtdcRspInfoField *pRspInfo, 
			int nRequestID, 
			bool bIsLast)
{
    clog_warning("[%s] CTP - OnRspUnSubMarketData, code: %s", 
				module_name_, 
				pSpecificInstrument->InstrumentID);
}

void ShfeL1MDProducer::Convert(CDepthMarketDataField &quote_level1,
			const CThostFtdcDepthMarketDataField &ctp_data)
{
    memcpy(quote_level1.TradingDay, ctp_data.TradingDay, 9); /// char       TradingDay[9];
    //SettlementGroupID[9];       /// char       SettlementGroupID[9];
    //SettlementID ;        /// int            SettlementID;
    quote_level1.LastPrice = ctp_data.LastPrice;           /// double LastPrice;
    quote_level1.PreSettlementPrice = ctp_data.PreSettlementPrice;  /// double PreSettlementPrice;
    quote_level1.PreClosePrice = ctp_data.PreClosePrice;       /// double PreClosePrice;
    quote_level1.PreOpenInterest = ctp_data.PreOpenInterest;     /// double PreOpenInterest;
    quote_level1.OpenPrice = ctp_data.OpenPrice;           /// double OpenPrice;
    quote_level1.HighestPrice = ctp_data.HighestPrice;        /// double HighestPrice;
    quote_level1.LowestPrice = ctp_data.LowestPrice;         /// double LowestPrice;
    quote_level1.Volume = ctp_data.Volume;              /// int            Volume;
    quote_level1.Turnover = ctp_data.Turnover;            /// double Turnover;
    quote_level1.OpenInterest = ctp_data.OpenInterest;        /// double OpenInterest;
    quote_level1.ClosePrice = ctp_data.ClosePrice;          /// double ClosePrice;
    quote_level1.SettlementPrice = ctp_data.SettlementPrice;     /// double SettlementPrice;
    quote_level1.UpperLimitPrice = ctp_data.UpperLimitPrice;     /// double UpperLimitPrice;
    quote_level1.LowerLimitPrice = ctp_data.LowerLimitPrice;     /// double LowerLimitPrice;
    quote_level1.PreDelta = ctp_data.PreDelta;            /// double PreDelta;
    quote_level1.CurrDelta = ctp_data.CurrDelta;           /// double CurrDelta;
    memcpy(quote_level1.UpdateTime, ctp_data.UpdateTime, 9);       /// char       UpdateTime[9]; typedef char TThostFtdcTimeType[9];
    quote_level1.UpdateMillisec = ctp_data.UpdateMillisec;      /// int            UpdateMillisec;
    memcpy(quote_level1.InstrumentID, ctp_data.InstrumentID, 31); /// char       InstrumentID[31]; typedef char TThostFtdcInstrumentIDType[31];
    quote_level1.BidPrice1 = ctp_data.BidPrice1;           /// double BidPrice1;
    quote_level1.BidVolume1 = ctp_data.BidVolume1;          /// int            BidVolume1;
    quote_level1.AskPrice1 = ctp_data.AskPrice1;           /// double AskPrice1;
    quote_level1.AskVolume1 = ctp_data.AskVolume1;          /// int            AskVolume1;
        //ActionDay[9];        /// char       ActionDay[9];
}

void ShfeL1MDProducer::OnRspError(CThostFtdcRspInfoField *pRspInfo, 
			int nRequestID, 
			bool bIsLast)
{
    int error_code = pRspInfo ? 0 : pRspInfo->ErrorID;
    if (error_code != 0) 
	{
        clog_warning("[%s] CTP - OnRspError, code: %d; info: %s", 
					module_name_, 
					error_code, 
					pRspInfo->ErrorMsg);
    }
}

void ShfeL1MDProducer::InitMDApi()
{
	if(is_multicast_)
	{
		api_ = CThostFtdcMdApi::CreateFtdcMdApi("", true, true);
	}
	else
	{
		api_ = CThostFtdcMdApi::CreateFtdcMdApi("", false, false);
	}
    api_->RegisterSpi(this);
	char addr[100];
	sprintf (addr, "tcp://%s:%d", config_.mcIp, config_.mcPort);
	api_->RegisterFront(addr);
    api_->Init();
	clog_warning("[%s] CTP - RegisterFront, addr: %s", module_name_, addr);
}

void ShfeL1MDProducer::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *data)
{
	if (ended_) return;

	// 抛弃非主力合约
	if(!(IsDominant(data->InstrumentID))) return;

	if((data->InstrumentID[0]=='s' && data->InstrumentID[1]=='c') ||
		data->InstrumentID[0]=='n' && data->InstrumentID[1]=='r')
	{
		Convert(quote_level1_, *data);
		RalaceInvalidValue_Femas(quote_level1_);
		
		// debug
		// ToString(quote_level1_ );

	//	clog_info("[%s] OnRtnDepthMarketData InstrumentID:%s, "
	//				"UpdateTime:%s,UpdateMillisec:%d",
	//				module_name_,
	//				quote_level1_.InstrumentID,
	//				quote_level1_.UpdateTime,
	//				quote_level1_.UpdateMillisec);

		struct vrt_value  *vvalue;
		struct vrt_hybrid_value  *ivalue;
		vrt_producer_claim(producer_, &vvalue);
		ivalue = cork_container_of(vvalue, struct vrt_hybrid_value,parent);
		ivalue->index = Push(quote_level1_);
		ivalue->data = INE_L1_MD;
		vrt_producer_publish(producer_);

	}
	else
	{
		Convert(quote_level1_, *data);
		RalaceInvalidValue_Femas(quote_level1_);
		
		// debug
		// ToString(quote_level1_ );

	//	clog_info("[%s] OnRtnDepthMarketData InstrumentID:%s, "
	//				"UpdateTime:%s,UpdateMillisec:%d",
	//				module_name_,
	//				quote_level1_.InstrumentID,
	//				quote_level1_.UpdateTime,
	//				quote_level1_.UpdateMillisec);

		struct vrt_value  *vvalue;
		struct vrt_hybrid_value  *ivalue;
		vrt_producer_claim(producer_, &vvalue);
		ivalue = cork_container_of(vvalue, struct vrt_hybrid_value,parent);
		ivalue->index = Push(quote_level1_);
		ivalue->data = SHFE_L1_MD;
		vrt_producer_publish(producer_);
	}
}

void ShfeL1MDProducer::RalaceInvalidValue_Femas(CDepthMarketDataField &d)
{
    d.Turnover = InvalidToZeroD(d.Turnover);
    d.LastPrice = InvalidToZeroD(d.LastPrice);
    d.UpperLimitPrice = InvalidToZeroD(d.UpperLimitPrice);
    d.LowerLimitPrice = InvalidToZeroD(d.LowerLimitPrice);
    d.HighestPrice = InvalidToZeroD(d.HighestPrice);
    d.LowestPrice = InvalidToZeroD(d.LowestPrice);
    d.OpenPrice = InvalidToZeroD(d.OpenPrice);
    d.ClosePrice = InvalidToZeroD(d.ClosePrice);
    d.PreClosePrice = InvalidToZeroD(d.PreClosePrice);
    d.OpenInterest = InvalidToZeroD(d.OpenInterest);
    d.PreOpenInterest = InvalidToZeroD(d.PreOpenInterest);

    d.BidPrice1 = InvalidToZeroD(d.BidPrice1);
//    d.BidPrice2 = InvalidToZeroD(d.BidPrice2);
//    d.BidPrice3 = InvalidToZeroD(d.BidPrice3);
//    d.BidPrice4 = InvalidToZeroD(d.BidPrice4);
//    d.BidPrice5 = InvalidToZeroD(d.BidPrice5);

	d.AskPrice1 = InvalidToZeroD(d.AskPrice1);
//    d.AskPrice2 = InvalidToZeroD(d.AskPrice2);
//    d.AskPrice3 = InvalidToZeroD(d.AskPrice3);
//    d.AskPrice4 = InvalidToZeroD(d.AskPrice4);
//    d.AskPrice5 = InvalidToZeroD(d.AskPrice5);

	d.SettlementPrice = InvalidToZeroD(d.SettlementPrice);
	d.PreSettlementPrice = InvalidToZeroD(d.PreSettlementPrice);

    d.PreDelta = InvalidToZeroD(d.PreDelta);
    d.CurrDelta = InvalidToZeroD(d.CurrDelta);
}

void ShfeL1MDProducer::End()
{
	if(!ended_)
	{
		ended_ = true;

		if (api_) 
		{
			api_->RegisterSpi(NULL);
			api_->Release();
			api_ = NULL;
			clog_warning("[%s] ThostFtdcMdApi stop", module_name_);
		}

		vrt_producer_eof(producer_);
		clog_warning("[%s] End exit", module_name_);
	}
	fflush (Log::fp);
}
#endif

///////////////////////////////////
// 使用盛立API接收行情
//////////////////////////////////////
#ifdef EES_UDP_TOPSPEED_QUOTE

EESQuoteApi *ShfeL1MDProducer::LoadQuoteApi()
{
	m_handle =  dlopen(EES_QUOTE_DLL_NAME, RTLD_LAZY);
	if (!m_handle)
	{
		clog_error("[%s] 加载EES行情动态库(%s)失败", 
					module_name_,
					EES_QUOTE_DLL_NAME);
		return NULL;
	}

	funcCreateEESQuoteApi createFun = 
		(funcCreateEESQuoteApi)dlsym(m_handle, CREATE_EES_QUOTE_API_NAME);
	if (!createFun)
	{
		clog_error("[%s] 获取EES创建函数地址失败!", module_name_);
		return NULL;
	}

	m_distoryFun = (funcDestroyEESQuoteApi)dlsym(m_handle,
				DESTROY_EES_QUOTE_API_NAME);
	if (!createFun)
	{
		clog_error("[%s] 获取EES销毁函数地址失败!", module_name_);
		return NULL;
	}

	api_ = createFun();
	if (!api_)
	{
		clog_error("[%s] 创建EES行情对象失败!", module_name_);
		return false;
	}
}

void ShfeL1MDProducer::UnloadQuoteApi()
{
	if (api_)
	{
		api_->DisConnServer();
		clog_warning("[%s] DisConnServer!", module_name_);
		m_distoryFun(api_);
		api_ = NULL;
		m_distoryFun = NULL;
	}

	if (m_handle)
	{
		dlclose(m_handle);
		m_handle = NULL;
	}
}

void ShfeL1MDProducer::InitMDApi()
{
    api_ = LoadQuoteApi();

	// tcp
	EqsTcpInfo tcp_info; 
	memset(&tcp_info, 0, sizeof(EqsTcpInfo));
	strcpy(tcp_info.m_eqsIp, this->config_.mcIp);
	tcp_info.m_eqsPort = this->config_.mcPort;
	vector<EqsTcpInfo> vecTcp;
	vecTcp.push_back(tcp_info);
	bool rtn = api_->ConnServer(vecTcp, this);
	clog_warning("[%s] ConnectServer invoke:%d - mc ip:%s, mc port:%d,",
				module_name_, 
				(int)rtn, 
				vecTcp[0].m_eqsIp, 
				vecTcp[0].m_eqsPort);

	// multicast
//	EqsMulticastInfo emi;
//	strcpy(emi. m_mcIp, this->config_.mcIp);
//	emi.m_mcPort = this->config_.mcPort;
//	strcpy(emi. m_mcLoacalIp, this->config_.mcLocalIp);
//	emi. m_mcLocalPort = this->config_.mcLocalPort;	
//	strcpy(emi.m_exchangeId, "SHFE");
//	vector<EqsMulticastInfo> vecEmi;
//	vecEmi.push_back(emi);
//	bool rtn = this->api_->InitMulticast(vecEmi, this);
//	clog_warning("[%s] InitMulticast invoke:%d - mc ip:%s, mc port:%d,"
//				"mc local ip:%s, mc local port:%d, exchange:%s",
//				module_name_, (int)rtn, vecEmi[0].m_mcIp, vecEmi[0].m_mcPort, 
//				vecEmi[0].m_mcLoacalIp, vecEmi[0].m_mcLocalPort, vecEmi[0].m_exchangeId);
}


void ShfeL1MDProducer::OnEqsConnected()
{
	clog_warning("[%s] EES Quote connected.", module_name_);
	EqsLoginParam loginParam;
	strcpy(loginParam.m_loginId, this->config_.userid);
	strcpy(loginParam.m_password, this->config_.password);
	this->api_->LoginToEqs(loginParam);
	clog_warning("[%s] LoginToEqs-user id:%s, pwd:%s.", 
				module_name_, 
				loginParam.m_loginId, 
				loginParam.m_password);
}

void ShfeL1MDProducer::OnLoginResponse(bool bSuccess, const char* pReason)
{
	clog_warning("[%s] EES OnLoginResponse-sucess:%d, reason:%s.", 
				module_name_, 
				bSuccess, 
				pReason);

	std::ifstream is;
	is.open (config_.contracts_file);
	string contrs = "";
	if (is) 
	{
		getline(is, contrs);
		contrs += " ";
		size_t start_pos = 0;
		size_t end_pos = 0;
		string contr = "";
		while ((end_pos=contrs.find(" ",start_pos)) != string::npos)
		{
			contr = contrs.substr (start_pos, end_pos-start_pos);
			this->api_->RegisterSymbol(EesEqsIntrumentType::EQS_FUTURE, 
						(char*)contr.c_str());
			clog_warning("[%s] EES subscribe:%s",module_name_,contr.c_str());
			start_pos = end_pos + 1;
		}
	}
	else 
	{ 
		clog_error("[%s] EES can't open: %s", 
					module_name_, 
					config_.contracts_file); 
	}
}

void ShfeL1MDProducer::OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, 
			const char* pSymbol, 
			bool bSuccess)
{
	clog_warning("[%s] OnSymbolRegisterResponse-sucess:%d, symbol:%s.", 
				module_name_, 
				(int)bSuccess, 
				pSymbol);
}

void ShfeL1MDProducer::OnEqsDisconnected()
{
	clog_warning("[%s] EES Quote disconnected.", module_name_);
}

void ShfeL1MDProducer::OnQuoteUpdated(EesEqsIntrumentType chInstrumentType, 
			EESMarketDepthQuoteData* data_src)
{
	//clog_info("[%s] OnQuoteUpdated invoked.", module_name_);
	
	if(EQS_FUTURE != chInstrumentType) return;

	if (ended_) return;

	// 抛弃非主力合约
	if(!(IsDominant(data_src->InstrumentID))) return;

	CDepthMarketDataField data;
	RalaceInvalidValue_EES(*data_src,data);
	
	// debug
	//ToString(data);
	//clog_info("[%s] OnRtnDepthMarketData InstrumentID:%s,UpdateTime:%s,UpdateMillisec:%d",
	//	module_name_,data->InstrumentID,data->UpdateTime,data->UpdateMillisec);

	struct vrt_value  *vvalue;
	struct vrt_hybrid_value  *ivalue;
	vrt_producer_claim(producer_, &vvalue);
	ivalue = cork_container_of(vvalue, struct vrt_hybrid_value,parent);
	ivalue->index = Push(data);
	ivalue->data = L1_MD;
	vrt_producer_publish(producer_);

}

void ShfeL1MDProducer::RalaceInvalidValue_EES(EESMarketDepthQuoteData &data_src, 
			CDepthMarketDataField &data_dest)
{
    strcpy(data_dest.TradingDay, data_src.TradingDay);
    strcpy(data_dest.InstrumentID, data_src.InstrumentID);
    // strcpy(data_dest.ExchangeID, data_src.ExchangeID);
    //strcpy(data_dest.ExchangeInstID, data_src.ExchangeInstID);
    data_dest.LastPrice =			InvalidToZeroD(data_src.LastPrice);
	data_dest.PreSettlementPrice =	InvalidToZeroD(data_src.PreSettlementPrice);
    data_dest.PreClosePrice =		InvalidToZeroD(data_src.PreClosePrice);
    data_dest.PreOpenInterest =		InvalidToZeroD(data_src.PreOpenInterest);
    data_dest.OpenPrice =			InvalidToZeroD(data_src.OpenPrice);
    data_dest.HighestPrice =		InvalidToZeroD(data_src.HighestPrice);
    data_dest.LowestPrice =			InvalidToZeroD(data_src.LowestPrice);
    data_dest.Volume =				data_src.Volume;
    data_dest.Turnover =			InvalidToZeroD(data_src.Turnover);
    data_dest.OpenInterest =		InvalidToZeroD(data_src.OpenInterest);
    data_dest.ClosePrice =			InvalidToZeroD(data_src.ClosePrice);
	data_dest.SettlementPrice =		InvalidToZeroD(data_src.SettlementPrice);
    data_dest.UpperLimitPrice =		InvalidToZeroD(data_src.UpperLimitPrice);
    data_dest.LowerLimitPrice =		InvalidToZeroD(data_src.LowerLimitPrice);
    data_dest.PreDelta =			InvalidToZeroD(data_src.PreDelta);
    data_dest.CurrDelta =			InvalidToZeroD(data_src.CurrDelta);
    strcpy(data_dest.UpdateTime, data_src.UpdateTime);
    data_dest.UpdateMillisec =		data_src.UpdateMillisec;

    data_dest.BidPrice1 =			InvalidToZeroD(data_src.BidPrice1);
    data_dest.BidVolume1 =			data_src.BidVolume1;
	data_dest.AskPrice1 =			InvalidToZeroD(data_src.AskPrice1);
    data_dest.AskVolume1 =			data_src.AskVolume1;

//    data_dest.BidPrice2 =			InvalidToZeroD(data_src.BidPrice2);
//    data_dest.BidVolume2 =			data_src.BidVolume2;
//	data_dest.AskPrice2 =			InvalidToZeroD(data_src.AskPrice2);
//    data_dest.AskVolume2 =			data_src.AskVolume2;
//
//    data_dest.BidPrice3 =			InvalidToZeroD(data_src.BidPrice3);
//    data_dest.BidVolume3 =			data_src.BidVolume3;
//	data_dest.AskPrice3 =			InvalidToZeroD(data_src.AskPrice3);
//    data_dest.AskVolume3 =			data_src.AskVolume3;
//
//    data_dest.BidPrice4 =			InvalidToZeroD(data_src.BidPrice4);
//    data_dest.BidVolume4 =			data_src.BidVolume4;
//	data_dest.AskPrice4 =			InvalidToZeroD(data_src.AskPrice4);
//    data_dest.AskVolume4 =			data_src.AskVolume4;
//
//    data_dest.BidPrice5 =			InvalidToZeroD(data_src.BidPrice5);
//    data_dest.BidVolume5 =			data_src.BidVolume5;
//	data_dest.AskPrice5 =			InvalidToZeroD(data_src.AskPrice5);
//    data_dest.AskVolume5 =			data_src.AskVolume5;
//
    data_dest.AveragePrice =		InvalidToZeroD(data_src.AveragePrice);
}


void ShfeL1MDProducer::End()
{
	if(!ended_)
	{
		ended_ = true;

		if (api_) 
		{
			this->UnloadQuoteApi();
			clog_warning("[%s] Level quote stop.", module_name_);
			api_ = NULL;
		}

		vrt_producer_eof(producer_);
		clog_warning("[%s] End exit", module_name_);
	}
	fflush (Log::fp);
}
#endif

////////////////////////////////////
// 使用efh_sf_api接收行情
/////////////////////////////////
#ifdef EES_EFH_SF_TOPSPEED_QUOTE
ShfeL1MDProducer ShfeL1MDProducer::This;
void ShfeL1MDProducer::InitMDApi()
{
	ShfeL1MDProducer::This = this;
	char ip[20];
	p_SlEfh	= (struct SlEfhQuote*)sl_create_eth_sf_api(config_.efh_sf_eth,
				config_.ip, 
				config_.port);
	if(NULL==p_SlEfh)
	{
		clog_error("[%s] l_create_efh_sf_api-failed.", module_name_);
	}
	else
	{
		clog_warning("[%s] sl_create_efh_sf_api-suceeded ip:%s, port:%d",
					module_name_, 
					config_.ip,
					config_.port);
	}
	int nret = sl_start_etf_quote(p_SlEfh, callback_efh_quote); 
	clog_warning("[%s] sl_start_etf_quote-nret:%d", module_name_, nret);
}

void ShfeL1MDProducer::Rev(const struct guava_udp_normal* data)
{
	if (ended_) return;

	// 抛弃非主力合约
	if(!(IsDominant(data->m_symbol))) return;

	RalaceInvalidValue_EES(*data);
	
	// debug
	// ToString(*data);

	//clog_info("[%s] OnRtnDepthMarketData InstrumentID:%s,UpdateTime:%s,UpdateMillisec:%d",
	//	module_name_,data->InstrumentID,data->UpdateTime,data->UpdateMillisec);

	struct vrt_value  *vvalue;
	struct vrt_hybrid_value  *ivalue;
	vrt_producer_claim(producer_, &vvalue);
	ivalue = cork_container_of(vvalue, struct vrt_hybrid_value,parent);
	ivalue->index = Push(*data);
	ivalue->data = L1_MD;
	vrt_producer_publish(producer_);

}

void ShfeL1MDProducer::RalaceInvalidValue_EES(CDepthMarketDataField &d)
{
    d.Turnover = InvalidToZeroD(d.Turnover);
    d.LastPrice = InvalidToZeroD(d.LastPrice);
    d.UpperLimitPrice = InvalidToZeroD(d.UpperLimitPrice);
    d.LowerLimitPrice = InvalidToZeroD(d.LowerLimitPrice);
    d.HighestPrice = InvalidToZeroD(d.HighestPrice);
    d.LowestPrice = InvalidToZeroD(d.LowestPrice);
    d.OpenPrice = InvalidToZeroD(d.OpenPrice);
    d.ClosePrice = InvalidToZeroD(d.ClosePrice);
    d.PreClosePrice = InvalidToZeroD(d.PreClosePrice);
    d.OpenInterest = InvalidToZeroD(d.OpenInterest);
    d.PreOpenInterest = InvalidToZeroD(d.PreOpenInterest);
    d.BidPrice1 = InvalidToZeroD(d.BidPrice1);
	d.AskPrice1 = InvalidToZeroD(d.AskPrice1);
	d.SettlementPrice = InvalidToZeroD(d.SettlementPrice);
	d.PreSettlementPrice = InvalidToZeroD(d.PreSettlementPrice);

    d.PreDelta = InvalidToZeroD(d.PreDelta);
    d.CurrDelta = InvalidToZeroD(d.CurrDelta);
}


void ShfeL1MDProducer::End()
{
	if(!ended_)
	{
		ended_ = true;

		if (api_) 
		{
			sl_stop_efh_quote( p_SlEfh );
			clog_warning("[%s] CMdclientApi stop: %d",module_name_, err);
			api_ = NULL;
		}

		vrt_producer_eof(producer_);
		clog_warning("[%s] End exit", module_name_);
	}
	fflush (Log::fp);
}

void  callback_efh_quote(struct sl_efh_quote* p, const struct guava_udp_normal* p_quote ) 
{
	ShfeL1MDProducer::This->Rev(p_quote);
}
#endif