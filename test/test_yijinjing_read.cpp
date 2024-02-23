#include <gtest/gtest.h>

#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/component/yijinjing/journal/PageProvider.h"
#include "ft/base/market_data.h"
#include <thread>

using namespace std::chrono;
typedef char                    T_I8;
typedef unsigned char           T_U8;

typedef short int               T_I16;
typedef unsigned short int      T_U16;

typedef int                     T_I32;
typedef unsigned int            T_U32;

typedef long long               T_I64;
typedef unsigned long long      T_U64;

typedef float                   T_F32;
typedef double                  T_F64;

//1.3 逐笔成交(Transaction)
typedef struct t_SH_StockStepTrade {
  T_I32 nActionDay;                 //自然日
  T_I32 nTradeIndex;                //成交序号
  T_I32 nTradeChannel;              //成交通道
  T_I32 nTradeTime;                 //成交时间 HHMMSSmmm
  T_I32 nTradePrice;                //成交价格 扩大10000倍
  T_I64 iTradeQty;                  //成交数量 股票：股 权证：份 债券：张
  T_I64 iTradeMoney;                //成交金额(元)
  T_I64 iTradeBuyNo;                //买方订单号
  T_I64 iTradeSellNo;               //卖方订单号
  char cTradeBSflag;                //内外盘标识 B -外盘，主动买  B-内盘,主动卖 N 未知
  T_I64 iBizIndex;					//业务序列号 与成交统一编号，从 1 开始， 按 Channel 连续 
  char sRes[3];                     //保留字段1
} Stock_Transaction_SH,T_SH_StockStepTrade,*PSH_StockStepTrade; //上交所逐笔成交，对应交易所UA3201

typedef struct t_SH_StockStepOrder
{
	T_I32 nActionDay;               //自然日
	T_I32 nOrderIndex;				//委托序号 从 1 开始，按 Channel 连续 
	T_I32 nChannel;					//通道
	T_I32 nOrderTime;				//委托时间 HHMMSSmmm
	char  cOrderType;				//订单类型 A – 委托订单（增加） D – 委托订单（删除） 
	T_I64 iOrderNO;					//原始订单号 
	T_I32 nOrderPrice;				//委托价格(元)
	T_I64 iBalance;					//剩余委托量 
	char  sOrderBSFlag;			//对于委托订单： B – 买单 S – 卖单  
	T_I64 iBizIndex;				//业务序列号 与成交统一编号，从 1 开始， 按 Channel 连续 
} T_SH_StockStepOrder, *PSH_StockStepOrder;

struct L2StockMarketData1 {
  int exchange_id;
  char instrument_id[16];
  int market_data_type;
  union stock_market_data{
	t_SH_StockStepTrade trade;
	t_SH_StockStepOrder order;
  } data;
};


#include <signal.h>
 
 #define SIG_MY_DEFINE_TEST    __SIGRTMIN+10
void    sigactionProcess(int nsig)
{
    std::cout<< "rec signal"<<std::endl;
}
//信号处理函数注册
void    sigactionReg(void)
{
 struct sigaction act,oldact;
    act.sa_handler  = sigactionProcess;
    act.sa_flags = 0;
    //sigaction(SIGINT,&act,&oldact);
    sigaction(SIG_MY_DEFINE_TEST,&act,&oldact);
}


TEST(YIJINJING, JOURNAL1) {
	 sigactionReg();
 const int n = 10000000;
 std::cout<<sizeof(t_SH_StockStepOrder)<<std::endl;
  std::cout<<sizeof(t_SH_StockStepTrade)<<std::endl;
   std::cout<<sizeof(L2StockMarketData1)<<std::endl;

  auto reader = yijinjing::JournalReader::create(0, "reader");
  reader->addJournal("/home/yfxu/ft/build/bin/", "test_y_writer");

  time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
  size_t i=0;
  L2StockMarketData1* rsp=nullptr;
  while (i<n) {
      auto frame = reader->getNextFrame();
      if (frame){
        if (frame->getDataLength() != sizeof(L2StockMarketData1)) {
          printf("invalid order rsp len\n");
            continue;
        }else{
            L2StockMarketData1* rsp1 = reinterpret_cast<L2StockMarketData1*>(frame->getData());
			// if (rsp && rsp1){
			// 	if (rsp1->m_channel - rsp->m_channel !=1){
			// 		std::cout << rsp1->m_channel <<"," << rsp->m_channel <<std::endl;
			// 	}
			// }
			if (rsp1->market_data_type==1){
				std::cout <<"trade:" << rsp1->data.trade.iBizIndex <<"," <<rsp1->data.trade.iTradeBuyNo <<"," <<rsp1->data.trade.iTradeSellNo <<std::endl;	
			}else{
				std::cout <<"order:"<< rsp1->data.order.iBizIndex <<"," <<rsp1->data.order.iBalance <<"," <<rsp1->data.order.nChannel <<std::endl;
			}
			rsp=rsp1;
            // std::cout << rsp->m_channel <<"," <<rsp->m_head.m_sequence <<"," <<rsp->m_symbol <<std::endl;
            // ASSERT_EQ(rsp->m_channel, i);
			// std::this_thread::sleep_for(std::chrono::seconds(1));
            i++;
        }
      }else{
        // printf("continue\n");
        continue;
      }
  }
	//  std::cout <<"r:" <<i<<"," <<  duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;   
	//            std::cout << rsp->m_channel <<"," <<"," <<rsp->m_symbol <<std::endl;
}
