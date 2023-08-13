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

TEST(RingBufferTest, Case_0) {
  const int n = 10000000;
  RingBuffer<sse_hpf_exe_pkt, 4096> rb;

  std::cout<<sizeof(sse_hpf_exe_pkt)<<std::endl;

  std::thread rd_thread([&]() {
		time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      sse_hpf_exe_pkt res;
      rb.GetWithBlocking(&res);
      // std::cout<<i<<std::endl;
	//   if(i % 5000==0)
	//   std::this_thread::sleep_for(std::chrono::milliseconds(10));
      ASSERT_EQ(res.m_channel, i);
    }
	 std::cout <<"r" <<  duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  std::thread wr_thread([&]() {
	time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      sse_hpf_exe_pkt info;
      info.m_channel=i;
      rb.PutWithBlocking(info);
    }
	    std::cout <<"w" <<  duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  
  });

  rd_thread.join();
  wr_thread.join();
}
