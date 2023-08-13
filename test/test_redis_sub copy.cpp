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
#include <async.h>
#include <adapters/libevent.h>

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply =reinterpret_cast<redisReply*>(r);
    if (reply == NULL) {
        if (c->errstr) {
            printf("errstr: %s\n", c->errstr);
        }
        return;
    }

    if (privdata){
        printf("privdata: %s\n",(char *) (privdata));
    }
    
    printf("redisReply elements size:%d\n",reply->elements);  
    for (size_t i=0;i<reply->elements;i++){
        if (reply->element[i]->len==120){
             sse_hpf_exe_pkt *info =reinterpret_cast<sse_hpf_exe_pkt*>(reply->element[i]->str);
             printf("quote data:%d\n",info->m_channel);
        }
        printf("redisReply len:%d\n",reply->element[i]->len);        
        printf("redisReply type:%d\n",reply->element[i]->type);        
        printf("redisReply str:%s\n",reply->element[i]->str); 
        printf("---------------------\n");
    }
             
    //printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

 
TEST(REDISTest, Case_0) {
  #ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    struct event_base *base = event_base_new();
    redisOptions options = {0};
    REDIS_OPTIONS_SET_TCP(&options, "192.168.148.130", 6379);
    struct timeval tv = {0};
    tv.tv_sec = 1;
    options.connect_timeout = &tv;


    redisAsyncContext *c = redisAsyncConnectWithOptions(&options);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return ;
    }

    redisLibeventAttach(c,base);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    char user_password[]="123456";
    // redisAsyncCommand(c, NULL, NULL, "AUTH %s", user_password);
    //redisAsyncCommand(c, NULL, NULL, "SUBSCRIBE runoobChat");

    redisAsyncCommand(c, getCallback, (void *)"SUBSCRIBE", "SUBSCRIBE quote");
    event_base_dispatch(base);
    return;
}
