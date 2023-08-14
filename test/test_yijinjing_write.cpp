#include <gtest/gtest.h>

#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/component/yijinjing/journal/PageProvider.h"
#include "ft/base/market_data.h"
#include <thread>

using namespace std::chrono;

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
 

TEST(YIJINJING, JOURNAL1) {
 const int n = 100;
 std::cout<<sizeof(sse_hpf_exe_pkt)<<std::endl;
   
time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
auto writer = yijinjing::JournalWriter::create(".", "test_y_writer", "writer");

writer->seekEnd();
for (int i = 0; i < n; ++i) {
  sse_hpf_exe_pkt info;
  info.m_channel=i;
  info.m_head.m_sequence=i;
  info.m_symbol[0]='a';
  info.m_symbol[1]='b';
  
  writer->write_data(info, 0, 0);
}
  std::cout <<"w:" <<  duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
}
