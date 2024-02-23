#include <gtest/gtest.h>

#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/component/yijinjing/journal/PageProvider.h"
#include "ft/base/market_data.h"
#include <thread>

using namespace std::chrono;

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
  sse_hpf_exe_pkt() { memset(this, 0, sizeof(sse_hpf_exe_pkt)); }
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

  int64_t _nano;
};

int64_t getNano() {
  return 0;
  // int64_t _nano = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::get_time_now().time_since_epoch()).count();
  // return _nano;
}

TEST(YIJINJING, JOURNAL) {
  auto writer = yijinjing::JournalWriter::create(".", "test_yijinjing_writer", "writer");

  writer->seekEnd();
  writer->write_frame("aaa", 3, 1, 0);

  auto reader = yijinjing::JournalReader::create(0, "reader");
  reader->addJournal(".", "test_yijinjing_writer");

  auto frame = reader->getNextFrame();
  ASSERT_TRUE(frame != nullptr);
  ASSERT_EQ(frame->getDataLength(), 3);

  char data[4];
  memcpy(data, frame->getData(), frame->getDataLength());
  data[frame->getDataLength()] = 0;
  ASSERT_STREQ(data, "aaa");
}

TEST(YIJINJING, JOURNAL1) {
  return;
  const int n = 100;
  std::cout << sizeof(sse_hpf_exe_pkt) << std::endl;

  std::thread rd_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    auto reader = yijinjing::JournalReader::create(0, "reader");
    reader->addJournal(".", "test_y_writer");
    size_t i = 0;
    while (i < n) {
      auto frame = reader->getNextFrame();
      if (frame) {
        if (frame->getDataLength() != sizeof(sse_hpf_exe_pkt)) {
          printf("invalid order rsp len\n");
          continue;
        } else {
          sse_hpf_exe_pkt* rsp = reinterpret_cast<sse_hpf_exe_pkt*>(frame->getData());
          std::cout << rsp->m_channel << "," << rsp->m_head.m_sequence << "," << rsp->m_symbol << std::endl;
          ASSERT_EQ(rsp->m_channel, i);
          i++;
        }
      } else {
        continue;
      }
    }
    std::cout << "r:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  std::thread wr_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    auto writer = yijinjing::JournalWriter::create(".", "test_y_writer", "writer");

    writer->seekEnd();
    for (int i = 0; i < n; ++i) {
      sse_hpf_exe_pkt info;
      info.m_channel = i;
      info.m_head.m_sequence = i;
      info.m_symbol[0] = 'a';
      info.m_symbol[1] = 'b';

      writer->write_data(info, 0, 0);
    }
    std::cout << "w:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}

#include <iomanip>

struct test_sec {
  int index;
  timespec tp;
};

TEST(YIJINJING, JOURNAL2) {
  const int n = 1000000;
  std::cout << sizeof(test_sec) << std::endl;

  std::thread rd_thread([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();

    auto reader = yijinjing::JournalReader::create(0, "reader");
    reader->addJournal(".", "test_y_writer");
    size_t i = 0;
    std::vector<int64_t> vec(n);
    while (i < n) {
      auto frame = reader->getNextFrame();
      if (frame) {
        if (frame->getDataLength() != sizeof(test_sec)) {
          printf("invalid order rsp len\n");
          continue;
        } else {
          test_sec* rsp = reinterpret_cast<test_sec*>(frame->getData());
          test_sec tp;
          clock_gettime(CLOCK_MONOTONIC, &tp.tp);
          vec[i] = (tp.tp.tv_sec - rsp->tp.tv_sec) * 1000000000L + tp.tp.tv_nsec - rsp->tp.tv_nsec;

          i++;
        }
      } else {
        continue;
      }
    }

    int64_t t = 0;
    for (int i = 0; i < n; ++i) {
      // std::cout << vec[i] << std::endl;
      t = vec[i] + t;
    }
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "r:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << ", " << t / (n * 1.0) << std::endl;
  });

  std::thread wr_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    auto writer = yijinjing::JournalWriter::create(".", "test_y_writer", "writer");

    writer->seekEnd();

    std::this_thread::sleep_for(std::chrono::seconds(4));
    for (int i = 0; i < n; ++i) {
      test_sec tp;
      tp.index = i;
      clock_gettime(CLOCK_MONOTONIC, &tp.tp);
      writer->write_data(tp, 0, 0);
      // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "w:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}
