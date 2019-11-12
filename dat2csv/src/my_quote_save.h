#if !defined(MY_QUOTE_SAVE_H_)
#define MY_QUOTE_SAVE_H_

#include "quote_datatype_common.h"
#include "YaoQuote.h"

// 行情类型标识定义
#define GTAEX_CFFEX_QUOTE_TYPE          1

#define DCE_MDBESTANDDEEP_QUOTE_TYPE    2
#define DCE_ARBI_QUOTE_TYPE             3
#define DCE_MDTENENTRUST_QUOTE_TYPE     4
#define DCE_MDORDERSTATISTIC_QUOTE_TYPE 5
#define DCE_MDREALTIMEPRICE_QUOTE_TYPE  6
#define DCE_MDMARCHPRICEQTY_QUOTE_TYPE  7

#define CTP_MARKETDATA_QUOTE_TYPE       8

#define SHFE_DEEP_QUOTE_TYPE            9

#define SHFE_EX_QUOTE_TYPE              10

#define GTA_UDP_CFFEX_QUOTE_TYPE        11
#define MY_SHFE_MD_QUOTE_TYPE           12

// TDF 股票行情和指数行情类型
#define TDF_STOCK_QUOTE_TYPE            13
#define TDF_INDEX_QUOTE_TYPE            14

// my derivative data of stock market
#define MY_STOCK_QUOTE_TYPE             15

// CZCE Market data id
#define CZCE_LEVEL2_QUOTE_TYPE          16
#define CZCE_CMB_QUOTE_TYPE             17

// ksg gold of sh
#define SH_GOLD_QUOTE_TYPE              18

// 台湾合并行情类型
#define TAI_FEX_MD_TYPE                 19

// CME 芝加哥交易所
#define DEPTHMARKETDATA_QUOTE_TYPE             30
#define REALTIMEDATA_QUOTE_TYPE             31
#define ORDERBOOKDATA_QUOTE_TYPE             32
#define TRADEVOLUMEDATA_QUOTE_TYPE             33

#define SHFE_LEV2_DATA_QUOTE_TYPE 70

#define YAO_QUOTE_TYPE             125

// data identities of kmds {"stockcode", "stockquote", "indexquote", "optionquote", "ordqueue", "perentrust", "perbargain"};
#define KMDS_CODETABLE_TYPE         0xc0
#define KMDS_STOCK_SNAPSHOT_TYPE    0xc1
#define KMDS_INDEX_TYPE             0xc2
#define KMDS_OPTION_QUOTE_TYPE      0xc3
#define KMDS_ORDER_QUEUE_TYPE       0xc4
#define KMDS_PER_ENTRUST_TYPE       0xc5
#define KMDS_PER_BARGAIN_TYPE       0xc6
#define KMDS_FUTURE_QUOTE_TYPE      0xc7

#pragma pack(1)
#define MAX_PAIR 120
struct PVPair
{
	double price;
	int volume;
};
struct MDPack
{
	char instrument[10];
	char islast;
	int seqno;
	char direction;
	short count;
	PVPair data[MAX_PAIR];
};
#pragma pack()

// 为兼容，对齐方式统一使用8字节对齐
#pragma pack(push)
#pragma pack(8)

// 二进制存储的文件头信息
struct SaveFileHeaderStruct
{
    int data_count;     // number of quote data items
    short data_type;    // 行情类型标识
    short data_length;  // length of one item (byte)
};

// 发送数据，也使用该结构做消息头
typedef SaveFileHeaderStruct SendQuoteDataHeader;


struct SaveData_YaoQuote
{
	long long t_;
	YaoQuote data_;
	// 缺省构造
	SaveData_YaoQuote()
	{
		t_ = 0;
	}

	// 通过时间戳、和网络数据包构造
	SaveData_YaoQuote(long long t, const YaoQuote &d)
		: t_(t), data_(d)
	{
	}
};

#pragma pack(pop)

#endif  //MY_QUOTE_SAVE_H_
