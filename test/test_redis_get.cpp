// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <thread>
#include<chrono>
#include "ft/utils/ring_buffer.h"
#include "ft/base/market_data.h"

using namespace std::chrono;
using ft::RingBuffer;

struct sse_hpf_head
{
	unsigned int						m_sequence;
	unsigned short						m_tick1;
	unsigned short						m_tick2;
	unsigned char						m_message_type;
	unsigned short						m_message_len;		/// 包括此消息头的长度
	unsigned char						m_exchange_id;
	unsigned short						m_quote_date_year;
	unsigned char						m_quote_date_month;
	unsigned char						m_quote_date_day;
	unsigned int						m_send_time;
	unsigned char						m_category_id;
	unsigned int						m_msg_seq_id;
	unsigned char						m_seq_lost_flag;	/// 1=有丢包，0=没有丢包
};
struct sse_hpf_exe_pkt
{
	sse_hpf_exe_pkt()
	{
		memset(this, 0, sizeof(sse_hpf_exe_pkt));
	}
	sse_hpf_head						m_head;

	unsigned int						m_trade_index;
	unsigned int						m_channel;
	char								m_symbol[9];
	unsigned int						m_trade_time;
	unsigned int						m_trade_price;
	unsigned long long					m_trade_qty;
	unsigned long long					m_trade_money;
	unsigned long long					m_trade_buy_no;
	unsigned long long					m_trade_sell_no;
	char								m_trade_bs_flag;
	unsigned long long					m_biz_index;
	unsigned int						m_reserved;
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <hiredis.h>

  int n_sub=0;
    const int n = 1000000;
 
TEST(REDISTest, Case_0) {
  #ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

      redisOptions options = {0};
     REDIS_OPTIONS_SET_TCP(&options, "192.168.148.130", 6379);
    struct timeval tv = {0};
    tv.tv_sec = 10;
    options.connect_timeout = &tv;

    redisContext *c = redisConnectWithOptions(&options);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return;
    }

    std::cout<<"get start,"<<duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count()<<std::endl;
    for (size_t i=0;i<n;i++)
    {
        sse_hpf_exe_pkt info;   
        info.m_channel=i;
        info.m_symbol[0]='a';
        info.m_symbol[1]='b'; 
        info.m_head.m_sequence=22;          
        // std::cout <<i<<std::endl;  
        // std::string key=std::to_string(i);
        redisReply *r = (redisReply*)redisCommand(c, (char *)"get %d",i);
        while(r->type==6 || r->type==4){
            r = (redisReply*)redisCommand(c, (char *)"get %d",i);
            printf("reget: %zu\n", i);
        };
    
        // printf("reply: %s\n", r->str);

// =----------------------------------------------------------
//  std::string key=std::to_string(i);
//     printf("start set key value\n");
//     redisReply *reply = (redisReply*)redisCommand(c, "SET %s %b",key.c_str(),&info, sizeof(info));
//     freeReplyObject(reply);

//     reply =  (redisReply*)redisCommand(c, "GET %s", key.c_str());

//     sse_hpf_exe_pkt *result = (sse_hpf_exe_pkt *)reply->str;
//     printf("%d\n",result->m_channel);
//     printf("%s\n",result->m_symbol);
//     freeReplyObject(reply); 
    }

    std::cout<<"get   end,"<<duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count()<<std::endl;
  
    return;
}
