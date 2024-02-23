#pragma once

#include <assert.h>

#include "ft/base/market_data.h"

using namespace std::chrono;
using ft::RingBuffer;

struct sse_hpf_head {
  unsigned int m_sequence;
  unsigned short m_tick1;
  unsigned short m_tick2;
  unsigned char m_message_type;
  unsigned short m_message_len;  /// 包括此消息头的长度
  unsigned char m_exchange_id;
  unsigned short m_quote_date_year;
  unsigned char m_quote_date_month;
  unsigned char m_quote_date_day;
  unsigned int m_send_time;
  unsigned char m_category_id;
  unsigned int m_msg_seq_id;
  unsigned char m_seq_lost_flag;  /// 1=有丢包，0=没有丢包
};
struct sse_hpf_exe_pkt {
  sse_hpf_exe_pkt() {
    memset(this, 0, sizeof(sse_hpf_exe_pkt));
    // std::cout << "1" << std::endl;
  }
  sse_hpf_head m_head;

  unsigned int m_trade_index;
  unsigned int m_channel;
  char m_symbol[9];
  unsigned int m_trade_time;
  unsigned int m_trade_price;
  unsigned long long m_trade_qty;
  unsigned long long m_trade_money;
  unsigned long long m_trade_buy_no;
  unsigned long long m_trade_sell_no;
  char m_trade_bs_flag;
  unsigned long long m_biz_index;
  unsigned int m_reserved;

  // sse_hpf_exe_pkt(const sse_hpf_exe_pkt& a) { std::cout << "2" << std::endl; }

  // sse_hpf_exe_pkt(const sse_hpf_exe_pkt&& a) { std::cout << "3" << std::endl; }
  // sse_hpf_exe_pkt& operator=(sse_hpf_exe_pkt&& a) { std::cout << "4" << std::endl; }
  // sse_hpf_exe_pkt& operator=(sse_hpf_exe_pkt& a) { std::cout << "5" << std::endl; }

  // ~sse_hpf_exe_pkt() { std::cout << "d" << std::endl; }
};

struct IORequest {
  IORequest* p;
  int data;
  int ioType;
  unsigned int requestIndex;
};

class IORequestQueue {
 public:
  IORequestQueue() : pHead(nullptr), pTail(nullptr), count(0) {}

  // 队列是否为空
  bool empty() const { return (pHead == nullptr); }

  // 返回队列中元素个数
  std::size_t size() const { return count; }

  // 返回队头元素
  IORequest* front() {
    assert(!empty());
    return pHead;
  }

  // 返回队尾元素
  IORequest* back() {
    assert(!empty());
    return pTail;
  }

  // 将变量request从队列尾入队
  void push(IORequest* request) {
    request->p = nullptr;
    if (pHead == nullptr) {
      assert(pTail == nullptr);
      pHead = request;
    } else {
      assert(pTail != nullptr);
      pTail->p = request;
    }
    pTail = request;
    count++;
  }

  // 将队头元素弹出
  void pop() {
    assert(!empty());
    pHead = pHead->p;
    if (pHead == nullptr) {
      pTail = nullptr;
    }
    count--;
  }

 private:
  IORequest* pHead;
  IORequest* pTail;
  std::size_t count;
};
