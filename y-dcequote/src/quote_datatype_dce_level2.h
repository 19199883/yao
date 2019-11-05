#pragma once

#ifndef DLL_PUBLIC
#define DLL_PUBLIC  __attribute__ ((visibility("default")))
#endif

#include <string>

#pragma pack(push)
#pragma pack(8)

////////////////////////////////////////////////
///MDBestAndDeep：最优与五档深度行情
////////////////////////////////////////////////
struct DLL_PUBLIC MDBestAndDeep_MY
{
    char Type;
    unsigned int Length;                         //报文长度
    unsigned int Version;                        //版本从1开始
    unsigned int Time;                           //预留字段
    char Exchange[3];                    //交易所
    char Contract[80];                   //合约代码
    int SuspensionSign;                 //停牌标志
    float LastClearPrice;                 //昨结算价
    float ClearPrice;                     //今结算价
    float AvgPrice;                       //成交均价
    float LastClose;                      //昨收盘
    float Close;                          //今收盘
    float OpenPrice;                      //今开盘
    unsigned int LastOpenInterest;               //昨持仓量
    unsigned int OpenInterest;                   //持仓量
    float LastPrice;                      //最新价
    unsigned int MatchTotQty;                    //成交数量
    double Turnover;                       //成交金额
    float RiseLimit;                      //最高报价
    float FallLimit;                      //最低报价
    float HighPrice;                      //最高价
    float LowPrice;                       //最低价
    float PreDelta;                       //昨虚实度
    float CurrDelta;                      //今虚实度

    float BuyPriceOne;              //买入价格1
    unsigned int BuyQtyOne;                //买入数量1
    unsigned int BuyImplyQtyOne;           //买1推导量
    float BuyPriceTwo;
    unsigned int BuyQtyTwo;
    unsigned int BuyImplyQtyTwo;
    float BuyPriceThree;
    unsigned int BuyQtyThree;
    unsigned int BuyImplyQtyThree;
    float BuyPriceFour;
    unsigned int BuyQtyFour;
    unsigned int BuyImplyQtyFour;
    float BuyPriceFive;
    unsigned int BuyQtyFive;
    unsigned int BuyImplyQtyFive;
    float SellPriceOne;             //卖出价格1
    unsigned int SellQtyOne;               //买出数量1
    unsigned int SellImplyQtyOne;          //卖1推导量
    float SellPriceTwo;
    unsigned int SellQtyTwo;
    unsigned int SellImplyQtyTwo;
    float SellPriceThree;
    unsigned int SellQtyThree;
    unsigned int SellImplyQtyThree;
    float SellPriceFour;
    unsigned int SellQtyFour;
    unsigned int SellImplyQtyFour;
    float SellPriceFive;
    unsigned int SellQtyFive;
    unsigned int SellImplyQtyFive;

    char GenTime[13];                    //行情产生时间
    unsigned int LastMatchQty;                   //最新成交量
    int InterestChg;                    //持仓量变化
    float LifeLow;                        //历史最低价
    float LifeHigh;                       //历史最高价
    double Delta;                          //delta
    double Gamma;                          //gama
    double Rho;                            //rho
    double Theta;                          //theta
    double Vega;                           //vega
    char TradeDate[9];                   //行情日期
    char LocalDate[9];

    // HH:MM:SS.mmm
    std::string GetQuoteTime() const
    {
        return GenTime;
    }
};

////////////////////////////////////////////////
///MDBestAndDeep：arbi深度行情
////////////////////////////////////////////////
struct DLL_PUBLIC MDArbi_MY
{
    char Type;
    unsigned int Length;                         //报文长度
    unsigned int Version;                        //版本从1开始
    unsigned int Time;                           //预留字段
    char Exchange[3];                    //交易所
    char Contract[80];                   //合约代码
    int SuspensionSign;                 //停牌标志
    float LastClearPrice;                 //昨结算价
    float ClearPrice;                     //今结算价
    float AvgPrice;                       //成交均价
    float LastClose;                      //昨收盘
    float Close;                          //今收盘
    float OpenPrice;                      //今开盘
    unsigned int LastOpenInterest;               //昨持仓量
    unsigned int OpenInterest;                   //持仓量
    float LastPrice;                      //最新价
    unsigned int MatchTotQty;                    //成交数量
    double Turnover;                       //成交金额
    float RiseLimit;                      //最高报价
    float FallLimit;                      //最低报价
    float HighPrice;                      //最高价
    float LowPrice;                       //最低价
    float PreDelta;                       //昨虚实度
    float CurrDelta;                      //今虚实度

    float BuyPriceOne;              //买入价格1
    unsigned int BuyQtyOne;                //买入数量1
    unsigned int BuyImplyQtyOne;           //买1推导量
    float BuyPriceTwo;
    unsigned int BuyQtyTwo;
    unsigned int BuyImplyQtyTwo;
    float BuyPriceThree;
    unsigned int BuyQtyThree;
    unsigned int BuyImplyQtyThree;
    float BuyPriceFour;
    unsigned int BuyQtyFour;
    unsigned int BuyImplyQtyFour;
    float BuyPriceFive;
    unsigned int BuyQtyFive;
    unsigned int BuyImplyQtyFive;
    float SellPriceOne;             //卖出价格1
    unsigned int SellQtyOne;               //买出数量1
    unsigned int SellImplyQtyOne;          //卖1推导量
    float SellPriceTwo;
    unsigned int SellQtyTwo;
    unsigned int SellImplyQtyTwo;
    float SellPriceThree;
    unsigned int SellQtyThree;
    unsigned int SellImplyQtyThree;
    float SellPriceFour;
    unsigned int SellQtyFour;
    unsigned int SellImplyQtyFour;
    float SellPriceFive;
    unsigned int SellQtyFive;
    unsigned int SellImplyQtyFive;

    char GenTime[13];                    //行情产生时间
    unsigned int LastMatchQty;                   //最新成交量
    int InterestChg;                    //持仓量变化
    float LifeLow;                        //历史最低价
    float LifeHigh;                       //历史最高价
    double Delta;                          //delta
    double Gamma;                          //gama
    double Rho;                            //rho
    double Theta;                          //theta
    double Vega;                           //vega
    char TradeDate[9];                   //行情日期
    char LocalDate[9];

    // HH:MM:SS.mmm
    std::string GetQuoteTime() const
    {
        return "";
    }
};

////////////////////////////////////////////////
///MDTenEntrust：最优价位上十笔委托
////////////////////////////////////////////////
struct DLL_PUBLIC MDTenEntrust_MY
{
    char Type;                           //行情域标识
    unsigned int Len;
    char Contract[80];                   //合约代码
    double BestBuyOrderPrice;              //价格
    unsigned int BestBuyOrderQtyOne;             //委托量1
    unsigned int BestBuyOrderQtyTwo;             //委托量2
    unsigned int BestBuyOrderQtyThree;           //委托量3
    unsigned int BestBuyOrderQtyFour;            //委托量4
    unsigned int BestBuyOrderQtyFive;            //委托量5
    unsigned int BestBuyOrderQtySix;             //委托量6
    unsigned int BestBuyOrderQtySeven;           //委托量7
    unsigned int BestBuyOrderQtyEight;           //委托量8
    unsigned int BestBuyOrderQtyNine;            //委托量9
    unsigned int BestBuyOrderQtyTen;             //委托量10
    double BestSellOrderPrice;             //价格
    unsigned int BestSellOrderQtyOne;            //委托量1
    unsigned int BestSellOrderQtyTwo;            //委托量2
    unsigned int BestSellOrderQtyThree;          //委托量3
    unsigned int BestSellOrderQtyFour;           //委托量4
    unsigned int BestSellOrderQtyFive;           //委托量5
    unsigned int BestSellOrderQtySix;            //委托量6
    unsigned int BestSellOrderQtySeven;          //委托量7
    unsigned int BestSellOrderQtyEight;          //委托量8
    unsigned int BestSellOrderQtyNine;           //委托量9
    unsigned int BestSellOrderQtyTen;            //委托量10
    char GenTime[13];                    //生成时间

    // HH:MM:SS.mmm
    std::string GetQuoteTime() const
    {
        return "";
    }
};

////////////////////////////////////////////////
///MDOrderStatistic：加权平均以及委托总量行情
////////////////////////////////////////////////
struct DLL_PUBLIC MDOrderStatistic_MY
{
    char Type;                           //行情域标识
    unsigned int Len;
    char ContractID[80];                 //合约号
    unsigned int TotalBuyOrderNum;               //买委托总量
    unsigned int TotalSellOrderNum;              //卖委托总量
    double WeightedAverageBuyOrderPrice;   //加权平均委买价格
    double WeightedAverageSellOrderPrice;  //加权平均委卖价格

    // HH:MM:SS.mmm
    std::string GetQuoteTime() const
    {
        return "";
    }
};

////////////////////////////////////////////////
///MDRealTimePrice：实时结算价
////////////////////////////////////////////////
struct DLL_PUBLIC MDRealTimePrice_MY
{
    char Type;                           //行情域标识
    unsigned int Len;
    char ContractID[80];                 //合约号
    double RealTimePrice;                  //实时结算价

    // HH:MM:SS.mmm
    std::string GetQuoteTime() const
    {
        return "";
    }
};

////////////////////////////////////////////////
///分价位成交：分价位成交
////////////////////////////////////////////////
struct DLL_PUBLIC MDMarchPriceQty_MY
{
    char Type;                           //行情域标识
    unsigned int Len;
    char ContractID[80];                 //合约号
    double PriceOne;                       //价格
    unsigned int PriceOneBOQty;                  //买开数量
    unsigned int PriceOneBEQty;                  //买平数量
    unsigned int PriceOneSOQty;                  //卖开数量
    unsigned int PriceOneSEQty;                  //卖平数量
    double PriceTwo;                       //价格
    unsigned int PriceTwoBOQty;                  //买开数量
    unsigned int PriceTwoBEQty;                  //买平数量
    unsigned int PriceTwoSOQty;                  //卖开数量
    unsigned int PriceTwoSEQty;                  //卖平数量
    double PriceThree;                     //价格
    unsigned int PriceThreeBOQty;                //买开数量
    unsigned int PriceThreeBEQty;                //买平数量
    unsigned int PriceThreeSOQty;                //卖开数量
    unsigned int PriceThreeSEQty;                //卖平数量
    double PriceFour;                      //价格
    unsigned int PriceFourBOQty;                 //买开数量
    unsigned int PriceFourBEQty;                 //买平数量
    unsigned int PriceFourSOQty;                 //卖开数量
    unsigned int PriceFourSEQty;                 //卖平数量
    double PriceFive;                      //价格
    unsigned int PriceFiveBOQty;                 //买开数量
    unsigned int PriceFiveBEQty;                 //买平数量
    unsigned int PriceFiveSOQty;                 //卖开数量
    unsigned int PriceFiveSEQty;                 //卖平数量

    // HH:MM:SS.mmm
    std::string GetQuoteTime() const
    {
        return "";
    }
};

#pragma pack(pop)

class DceQuoteFormat
{
	public:
	static string ToString(MDBestAndDeep* quote)
	{
		char buffer[10240];		
		if(NULL == quote)
		{
			sprintf(buffer,"StructName=MDBestAndDeep=NULL\n");
		}
		else
		{
			sprintf(buffer, 
				"StructName=MDBestAndDeep\n"
				"\tGenTime=%s\n"
				"\tContract=%s\n"
				"\tLastPrice=%.4f\n"
				"\tMatchTotQty=%u\n"
				"\tBuyPriceOne=%.4f\n"
				"\tBuyQtyOne=%u\n"
				"\tSellPriceOne=%.4f\n"
				"\tSellQtyOne=%u\n"
				"\tOpenInterest=%u\n"
				"\tLastClearPrice=%.4f\n"
				"\tLastMatchQty=%u\n"
				"\tBuyPriceTwo=%.4f\n"
				"\tBuyPriceThree=%.4f\n"
				"\tBuyPriceFour=%.4f\n"
				"\tBuyPriceFive=%.4f\n"
				"\tBuyQtyTwo=%u\n"
				"\tBuyQtyThree=%u\n"
				"\tBuyQtyFour=%u\n"
				"\tBuyQtyFive=%u\n"
				"\tSellPriceTwo=%.4f\n"
				"\tSellPriceThree=%.4f\n"
				"\tSellPriceFour=%.4f\n"
				"\tSellPriceFive=%.4f\n"
				"\tSellQtyTwo=%u\n"
				"\tSellQtyThree=%u\n"
				"\tSellQtyFour=%u\n"
				"\tSellQtyFive=%u\n"
				"\tRiseLimit=%.4f\n"
				"\tFallLimit=%.4f\n"
				"\tOpenPrice=%.4f\n"
				"\tHighPrice=%.4f\n"
				"\tLowPrice=%.4f\n"
				"\tLastClose=%.4f\n"
				"\tLastOpenInterest=%u\n"
				"\tAvgPrice=%.4f\n"
				"\tTurnover=%.4f\n"
				"\tLastMatchQty=%u\n"
				"\tClearPrice=%.4f\n"
				"\tClose=%.4f\n"
				"\tPreDelta=%.6f\n"
				"\tCurrDelta=%.6f\n|"
				"\tInterestChg=%d\n"
				"\tLifeLow=%.6f\n"
				"\tLifeHigh=%.6f\n"
				"\tBuyImplyQtyOne=%d\n"
				"\tSellImplyQtyOne=%d\n"			
				"\tBuyImplyQtyOne=%d\n"
				"\tBuyImplyQtyTwo=%d\n"
				"\tBuyImplyQtyThree=%d\n"
				"\tBuyImplyQtyFour=%d\n"
				"\tBuyImplyQtyFive=%d\n"
				"\tSellImplyQtyOne=%d\n"
				"\tSellImplyQtyTwo=%d\n"
				"\tSellImplyQtyThree=%d\n"
				"\tSellImplyQtyFour=%d\n"
				"\tSellImplyQtyFive=%d\n",
				quote->GenTime,               // 时间			
				quote->Contract,                      // 合约代码
				InvalidToZeroF(quote->LastPrice),        // 成交价
				quote->MatchTotQty,                   // 成交量（总量）
				InvalidToZeroF(quote->BuyPriceOne),      // 买一价
				quote->BuyQtyOne,                     // 买一量
				InvalidToZeroF(quote->SellPriceOne),     // 卖一价
				quote->SellQtyOne,                    // 卖一量
				quote->OpenInterest,                  // 持仓量
				InvalidToZeroF(quote->LastClearPrice),   // 昨结算价
				quote->LastMatchQty,                       // 当前量
				InvalidToZeroF(quote->BuyPriceTwo),      // 买二价
				InvalidToZeroF(quote->BuyPriceThree),    // 买三价
				InvalidToZeroF(quote->BuyPriceFour),     // 买四价
				InvalidToZeroF(quote->BuyPriceFive),     // 买五价
				quote->BuyQtyTwo,                     // 买二量
				quote->BuyQtyThree,                   // 买三量
				quote->BuyQtyFour,                    // 买四量
				quote->BuyQtyFive,                    // 买五量
				InvalidToZeroF(quote->SellPriceTwo),     // 卖二价
				InvalidToZeroF(quote->SellPriceThree),   // 卖三价
				InvalidToZeroF(quote->SellPriceFour),    // 卖四价
				InvalidToZeroF(quote->SellPriceFive),    // 卖五价
				quote->SellQtyTwo,                    // 卖二量
				quote->SellQtyThree,                  // 卖三量
				quote->SellQtyFour,                   // 卖四量
				quote->SellQtyFive,                   // 卖五量
				quote->RiseLimit,                     // 涨停价
				quote->FallLimit,                     // 跌停价
				InvalidToZeroF(quote->OpenPrice),        // 开盘
				InvalidToZeroF(quote->HighPrice),        // 当日最高
				InvalidToZeroF(quote->LowPrice),         // 当日最低
				InvalidToZeroF(quote->LastClose),        // 昨收
				quote->LastOpenInterest,              // 昨持仓
				InvalidToZeroF(quote->AvgPrice),         // 均价			
				InvalidToZeroD(quote->Turnover),                     // 累计成交金额
				quote->LastMatchQty,                       // 
				InvalidToZeroD(quote->ClearPrice),		  //本次结算价
				InvalidToZeroD(quote->Close),					//今收盘			
				InvalidToZeroD(quote->PreDelta),				//昨虚实度
				InvalidToZeroD(quote->CurrDelta),				//今虚实度
				quote->InterestChg,							//持仓量变化
				InvalidToZeroD(quote->LifeLow),				//历史最低价
				InvalidToZeroD(quote->LifeHigh),				//历史最高价
				quote->BuyImplyQtyOne,			//申买推导量
				quote->SellImplyQtyOne,			//申卖推导量					
				quote->BuyImplyQtyOne,
				quote->BuyImplyQtyTwo,
				quote->BuyImplyQtyThree,
				quote->BuyImplyQtyFour,
				quote->BuyImplyQtyFive,
				quote->SellImplyQtyOne,
				quote->SellImplyQtyTwo,
				quote->SellImplyQtyThree,
				quote->SellImplyQtyFour,
				quote->SellImplyQtyFive);
		}

		return buffer;
	}
		
	static string ToString(MDOrderStatistic* quote)
	{
		char buffer[10240];		
		if(NULL == quote)
		{
			sprintf(buffer,"StructName=MDOrderStatistic=NULL\n");
		}
		else
		{
			sprintf(buffer,
			"\tContractID=%s\n"
			"\tTotalBuyOrderNum=%u\n"
			"\tTotalSellOrderNum=%u\n"
			"\tWeightedAverageBuyOrderPrice=%.6f\n"
			"\tWeightedAverageSellOrderPrice=%.6f\n",						
			quote->ContractID,					// 合约代码
			quote->TotalBuyOrderNum,			// 买委托总量
			quote->TotalSellOrderNum,			// 卖委托总量
			InvalidToZeroD(quote->WeightedAverageBuyOrderPrice),	// 加权平均委买价格
			InvalidToZeroD(quote->WeightedAverageSellOrderPrice)	// 加权平均委卖价格
			);
		}
		return buffer;
	}
};
