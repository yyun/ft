// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <cmath>
#include "ft/utils/ring_buffer.h"
#include "ft/base/market_data.h"
#include "ft/utils/queue_m.h"
#include "ft/utils/ringbuf.h"
#include "ft/utils/readerwriterqueue.h"

using namespace std::chrono;
using ft::RingBuffer;

#include <cpuid.h>
#include <stdio.h>

int main0() {
  unsigned int level = 0x0a;
  unsigned int eax, ebx, ecx, edx;
  if (__get_cpuid(level, &eax, &ebx, &ecx, &edx)) {
    printf("eax=%08x\tebx=%08x\tecx=%08x\tedx=%08x\n", eax, ebx, ecx, edx);
  } else {
    printf("__get_cpuid failed\n");
  }
  return 0;
}

inline bool is_leap_year(int year) { return !((year % 4) != 0 || ((year % 100) == 0 && (year % 400) != 0)); }

void nolocks_localtime(struct tm *tmp, time_t t, time_t tz, int dst) {
  const time_t secs_min = 60;
  const time_t secs_hour = 3600;
  const time_t secs_day = 3600 * 24;

  t -= tz;                       /* Adjust for timezone. */
  t += 3600 * dst;               /* Adjust for daylight time. */
  time_t days = t / secs_day;    /* Days passed since epoch. */
  time_t seconds = t % secs_day; /* Remaining seconds. */

  tmp->tm_isdst = dst;
  tmp->tm_hour = seconds / secs_hour;
  tmp->tm_min = (seconds % secs_hour) / secs_min;
  tmp->tm_sec = (seconds % secs_hour) % secs_min;

  /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure
   * where sunday = 0, so to calculate the day of the week we have to add 4
   * and take the modulo by 7. */
  tmp->tm_wday = (days + 4) % 7;

  /* Calculate the current year. */
  tmp->tm_year = 1970;
  while (1) {
    /* Leap years have one day more. */
    time_t days_this_year = 365 + is_leap_year(tmp->tm_year);
    if (days_this_year > days) break;
    days -= days_this_year;
    tmp->tm_year++;
  }
  tmp->tm_yday = days; /* Number of day of the current year. */

  /* We need to calculate in which month and day of the month we are. To do
   * so we need to skip days according to how many days there are in each
   * month, and adjust for the leap year that has one more day in February. */
  int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  mdays[1] += is_leap_year(tmp->tm_year);

  tmp->tm_mon = 0;
  while (days >= mdays[tmp->tm_mon]) {
    days -= mdays[tmp->tm_mon];
    tmp->tm_mon++;
  }

  tmp->tm_mday = days + 1; /* Add 1 since our 'days' is zero-based. */
  tmp->tm_year -= 1900;    /* Surprisingly tm_year is year-1900. */
}
struct test_sec {
  int index;
  timespec tp;
};
using namespace moodycamel;
#include <iomanip>

TEST(RingBufferTest08, Case_081) {
  ReaderWriterQueue<test_sec> *data_queue = new ReaderWriterQueue<test_sec>(1000000);

  const int n = 100000;
  // test_sec s;
  std::cout << sizeof(test_sec) << std::endl;

  std::thread rd_thread([&]() {
    std::vector<int64_t> vec(n);
    std::vector<test_sec> vec1(n);
    std::vector<test_sec> vec2(n);
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    test_sec res;
    for (size_t i = 0; i < n; ++i) {
      while (1) {
        if (data_queue->try_dequeue(res)) {
          // std::cout<<i<<std::endl;
          // if (i % 5000 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
          // ASSERT_EQ(market_data->m_channel, i);
          if (res.index == i) {
            // std::cout << "m_channel:" << res.index << "," << i << std::endl;
            test_sec tp;
            // clock_gettime(CLOCK_MONOTONIC, &tp.tp);
            clock_gettime(1, &tp.tp);
            // vec1[i].tp.tv_sec = res.tp.tv_sec;
            // vec1[i].tp.tv_nsec = res.tp.tv_nsec;

            // vec2[i].tp.tv_sec = tp.tp.tv_sec;
            // vec2[i].tp.tv_nsec = tp.tp.tv_nsec;

            vec[i] = (tp.tp.tv_sec - res.tp.tv_sec) * 1000000000L + tp.tp.tv_nsec - res.tp.tv_nsec;
            // std::cout << tp.tp.tv_sec << "," << res.tp.tv_sec << ", nas:" << tp.tp.tv_nsec << "," << res.tp.tv_nsec << std::endl;
            break;
          }

          // delete market_data;
          // std::cout << "m_channel:" << market_data->m_channel << "," << i << std::endl;
        }
      }
    }

    std::cout << "last," << res.index << std::endl;

    int64_t t = 0;
    for (int i = 0; i < n; ++i) {
      // std::cout << vec[i] << std::endl;
      t = vec[i] + t;
    }

    // for (int i = 0; i < n; ++i) {
    //   std::cout << vec2[i].tp.tv_sec << "," << vec1[i].tp.tv_sec << ", nas," << vec2[i].tp.tv_nsec << "," << vec1[i].tp.tv_nsec << std::endl;
    //   t = t + (vec2[i].tp.tv_sec - vec1[i].tp.tv_sec) * 1000000000L + vec2[i].tp.tv_nsec - vec1[i].tp.tv_nsec;
    // }
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "r:ReaderWriterQueue:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << ", " << t / (n * 1.0)
              << std::endl;
  });

  std::thread wr_thread([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(4));
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      test_sec *res = new test_sec;
      res->index = i;
      clock_gettime(1, &res->tp);
      data_queue->enqueue(*res);
    }
    std::cout << "w:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}

TEST(RingBufferTest12_Case_0_Test, Case_04) {
  const int n = 100000;
  RingBuffer<test_sec, 4096> rb;

  std::cout << sizeof(test_sec) << std::endl;

  std::thread rd_thread([&]() {
    std::vector<int64_t> vec(n);
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      test_sec res;
      rb.GetWithBlocking(&res);
      // std::cout<<i<<std::endl;
      //   if(i % 5000==0)
      //   std::this_thread::sleep_for(std::chrono::milliseconds(10));
      test_sec tp;

      clock_gettime(1, &tp.tp);

      vec[i] = (tp.tp.tv_sec - res.tp.tv_sec) * 1000000000L + tp.tp.tv_nsec - res.tp.tv_nsec;
    }

    int64_t t = 0;
    for (int i = 0; i < n; ++i) {
      // std::cout << vec[i] << std::endl;
      t = vec[i] + t;
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "r:RingBuffer:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << ", " << t / (n * 1.0) << std::endl;
  });

  std::thread wr_thread([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      test_sec info;
      info.index = i;
      clock_gettime(1, &info.tp);
      rb.PutWithBlocking(info);
    }
    std::cout << "w" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}

TEST(RingBufferTest08, Case_08) {
  return;
  ReaderWriterQueue<sse_hpf_exe_pkt> *data_queue = new ReaderWriterQueue<sse_hpf_exe_pkt>(1000000);

  const int n = 100000;
  // sse_hpf_exe_pkt s;
  std::cout << sizeof(sse_hpf_exe_pkt) << std::endl;

  std::thread rd_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i) {
      sse_hpf_exe_pkt res;
      while (1) {
        if (data_queue->try_dequeue(res)) {
          // std::cout<<i<<std::endl;
          // if (i % 5000 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
          // ASSERT_EQ(market_data->m_channel, i);
          if (res.m_channel != i) {
            std::cout << "m_channel:" << res.m_channel << "," << i << std::endl;
          }

          // delete market_data;
          // std::cout << "m_channel:" << market_data->m_channel << "," << i << std::endl;
          break;
        }
      }
    }
    std::cout << "r:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  std::thread wr_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      sse_hpf_exe_pkt *res = new sse_hpf_exe_pkt;
      res->m_channel = i;
      data_queue->enqueue(*res);
    }
    std::cout << "w:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}

enum class ExchangeId {
  SSE = 0,  // 上海证券交易所
  SZE = 1,  // 深圳证券交易所
};

enum class L2StockMarketDataType { ShL2Snapshot = 0, SzL2Snapshot = 1, ShStepTrade = 2, SzStepTrade = 3, ShStepOrder = 4, SzStepOrder = 5, Max };

struct L2StockMarketData {
  ExchangeId exchange_id;
  char instrument_id[16];
  L2StockMarketDataType market_data_type;
  void *stock_market_data;
  char sExtendFields;
};

TEST(RingBufferTest16, Case_160) {
  return;
  const int n = 10000000;

  time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
  sse_hpf_exe_pkt res;
  for (int i = 0; i < n; ++i) {
    res.m_channel = i;
    auto *ring_data = new L2StockMarketData;
    ring_data->exchange_id = ExchangeId::SSE;
    snprintf(ring_data->instrument_id, sizeof(ring_data->instrument_id), "%s", "rrrr");
    ring_data->market_data_type = L2StockMarketDataType::ShStepOrder;
    ring_data->stock_market_data = new sse_hpf_exe_pkt;

    memcpy(ring_data->stock_market_data, &res, sizeof(sse_hpf_exe_pkt));
  }
  std::cout << "w:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
}

TEST(RingBufferTest1, Case_10) {
  return;
  rte_ring *market_data_ring_ = rte_ring_create(std::pow(2, 20), RING_F_SP_ENQ | RING_F_SC_DEQ);
  const int n = 10000000;

  std::cout << sizeof(sse_hpf_exe_pkt) << std::endl;

  std::thread rd_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      void *res;
      while (1) {
        auto data_size = rte_ring_dequeue(market_data_ring_, &res);
        if (data_size > 0) {
          sse_hpf_exe_pkt *market_data = static_cast<sse_hpf_exe_pkt *>(res);
          // std::cout<<i<<std::endl;
          // if (i % 5000 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
          // ASSERT_EQ(market_data->m_channel, i);
          if (market_data->m_channel != i) {
            std::cout << "m_channel:" << market_data->m_channel << "," << i << std::endl;
          }

          delete market_data;
          // std::cout << "m_channel:" << market_data->m_channel << "," << i << std::endl;
          break;
        }
      }
    }
    std::cout << "r:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  std::thread wr_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      sse_hpf_exe_pkt *res = new sse_hpf_exe_pkt;
      res->m_channel = i;
      rte_ring_enqueue(market_data_ring_, res);
    }
    std::cout << "w:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}

TEST(RingBufferTest12_Case_0_Test, Case_0) {
  // main0();

  return;
  const int n = 10000000;
  RingBuffer<sse_hpf_exe_pkt, 4096> rb;

  std::cout << sizeof(sse_hpf_exe_pkt) << std::endl;

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
    std::cout << "r" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  std::thread wr_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      sse_hpf_exe_pkt info;
      info.m_channel = i;
      rb.PutWithBlocking(info);
    }
    std::cout << "w" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}

TEST(RingBufferTest12_Case_0, Case_07) {
  // main0();

  return;
  const int n = 10000000;
  RingBuffer<sse_hpf_exe_pkt, 4096> rb;

  std::cout << sizeof(sse_hpf_exe_pkt) << std::endl;

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
    std::cout << "r" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  std::thread wr_thread([&]() {
    time_point<high_resolution_clock> m_begin = high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
      sse_hpf_exe_pkt info;
      info.m_channel = i;
      rb.PutWithBlocking(info);
    }
    std::cout << "w" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;
  });

  rd_thread.join();
  wr_thread.join();
}

#include <queue>

TEST(QUEUETest, Case_1) {
  return;
  const int n = 10000000;

  std::vector<IORequest> requests;
  for (unsigned int index = 0; index < n; index++) {
    IORequest req;
    req.data = 0;
    req.ioType = 0;
    req.requestIndex = index;
    requests.push_back(req);
  }

  std::queue<IORequest *> q1;
  std::cout << sizeof(sse_hpf_exe_pkt) << std::endl;

  time_point<high_resolution_clock> m_begin = high_resolution_clock::now();

  for (auto &res : requests) {
    q1.emplace(&res);
  }

  for (int i = 0; i < n; ++i) {
    IORequest *res = q1.front();
    ASSERT_EQ(res->requestIndex, i);
    q1.pop();
  }

  std::cout << "r:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;

  time_point<high_resolution_clock> m_begin1 = high_resolution_clock::now();
  IORequestQueue q2;
  for (auto &res : requests) {
    q2.push(&res);
  }

  for (int i = 0; i < n; ++i) {
    IORequest *res = q2.front();
    ASSERT_EQ(res->requestIndex, i);
    q2.pop();
  }

  std::cout << "r1:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin1).count() << std::endl;
}
#include <cstring>

std::string SubstringBetweenDelimiters(const char *str, char delimiter, int n) {
  const char *start = str;
  const char *end = std::strchr(start, delimiter);
  for (int i = 0; i < n; ++i) {
    if (end == nullptr) {
      return "";
    }
    start = end + 1;
    end = std::strchr(start, delimiter);
  }
  if (end == nullptr) {
    end = str + std::strlen(str);
  }
  // return std::string(start, end - start);
  return {start, static_cast<std::size_t>(end - start)};
}

int get_stock_id(const char *inst) {
  return (inst[0] - '0') * 100000 + (inst[1] - '0') * 10000 + (inst[2] - '0') * 1000 + (inst[3] - '0') * 100 + (inst[4] - '0') * 10 + (inst[5] - '0');
}

// 获取CPU频率
uint64_t get_cpu_freq() {
  FILE *fp = popen("lscpu | grep CPU | grep MHz | awk  {'print $3'}", "r");
  if (fp == nullptr) return 0;

  char cpu_mhz_str[200] = {0};
  fgets(cpu_mhz_str, 80, fp);
  fclose(fp);

  return atof(cpu_mhz_str) * 1000 * 1000;
}

// 读取时间戳寄存器
uint64_t get_tsc()  // TSC == Time Stamp Counter寄存器
{
#ifdef __i386__
  uint64_t x;
  __asm__ volatile("rdtsc" : "=A"(x));
  return x;
#elif defined(__amd64__) || defined(__x86_64__)
  uint64_t a, d;
  __asm__ volatile("rdtscp" : "=a"(a), "=d"(d));
  return (d << 32) | a;
#else  // ARM架构CPU
  uint32_t cc = 0;
  __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(cc));
  return (uint64_t)cc;
#endif
}

#include <time.h>
#include <sys/time.h>
TEST(TIMETest, Case_2) {
  return;
  const int n = 10000000;
  uint64_t cpu_freq = get_cpu_freq();
  printf("cpu_freq is %lu\n", cpu_freq);

  const char *tmp1 = "600009.sh";
  char s[] = "600009.sh";

  std::string kk;
  int code = 0;

  time_point<high_resolution_clock> m_begin11 = high_resolution_clock::now();

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j <= 5; j++) {
      kk[j] = tmp1[j];
    }
    // kk = SubstringBetweenDelimiters(tmp1, '.', 1);
    // code = atoi(tmp1);
    code = get_stock_id(kk.c_str());
  }
  kk[6] = '\0';
  uint64_t last_tsc = get_tsc();
  std::this_thread::sleep_for(std::chrono::milliseconds(89));
  uint64_t cur_tsc = get_tsc();
  printf("TICK(s)   : %lu\n", cur_tsc - last_tsc);
  printf("ms(s) : %.02lf\n", 1000.0 * (cur_tsc - last_tsc) / cpu_freq);

  std::cout << "char:" << kk.c_str() << "," << code << "," << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin11).count()
            << std::endl;

  // std::vector<IORequest> requests;
  // for (unsigned int index = 0; index < n; index++) {
  //   IORequest req;
  //   req.data = 0;
  //   req.ioType = 0;
  //   req.requestIndex = index;
  //   requests.push_back(req);
  // }

  // time_point<high_resolution_clock> m_begin = high_resolution_clock::now();

  // for (int i = 0; i < n; ++i) {
  //   struct timeval begin;
  //   gettimeofday(&begin, NULL);
  // }

  // std::cout << "r:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin).count() << std::endl;

  time_point<high_resolution_clock> m_begin1 = high_resolution_clock::now();

  struct timespec time1 = {0, 0};
  for (int i = 0; i < n; ++i) {
    // clock_gettime(0, &time1);
  }

  std::cout << "r1:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin1).count() << std::endl;

  time_point<high_resolution_clock> m_begin2 = high_resolution_clock::now();

  for (int i = 0; i < n; ++i) {
    // clock_gettime(1, &time1);
  }

  std::cout << "r11:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin2).count() << std::endl;
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;

  time_point<high_resolution_clock> m_begin3 = high_resolution_clock::now();
  clock_gettime(1, &time1);
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;
  for (int i = 0; i < n; ++i) {
    clock_gettime(5, &time1);
  }
  std::cout << "r15:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin3).count() << std::endl;
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;

  //////////////////////////////////////////////////////////////
  // one
  time_point<high_resolution_clock> m_begin31 = high_resolution_clock::now();
  clock_gettime(1, &time1);
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;
  int64_t _nano = 0;
  for (int i = 0; i < n; ++i) {
    _nano = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  }
  std::cout << "oner11111111111113:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin31).count() << "," << _nano << std::endl;
  clock_gettime(1, &time1);
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;
  // two

  time_point<high_resolution_clock> m_begin32 = high_resolution_clock::now();
  clock_gettime(1, &time1);
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;
  timespec tp;
  for (int i = 0; i < n; ++i) {
    clock_gettime(CLOCK_MONOTONIC, &tp);
  }
  std::cout << "twor11111111111113:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin32).count() << "," << tp.tv_sec << ","
            << tp.tv_nsec << std::endl;
  clock_gettime(1, &time1);
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;

  //////////////////////////////////////////////////////////////

  time_point<high_resolution_clock> m_begin4 = high_resolution_clock::now();
  clock_gettime(6, &time1);
  char k3[18] = ("600058");
  int k;
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;
  for (int i = 0; i < n; ++i) {
    clock_gettime(6, &time1);
    // k = atoi(k3);
    k = (k3[0] - '0') * 100000 + (k3[1] - '0') * 10000 + (k3[2] - '0') * 1000 + (k3[3] - '0') * 100 + (k3[4] - '0') * 10 + (k3[5] - '0');
  }

  std::cout << "r16:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin4).count() << std::endl;
  std::cout << "ms:" << time1.tv_nsec / 1000000 << std::endl;
  std::cout << k << std::endl;

  std::clock_t t = 0;
  char tmp[64];
  time_t t1;
  struct tm today;
  int hms = 122345111;
  int mm = 0;
  time_t time_ptr;

  time_point<high_resolution_clock> m_begin5 = high_resolution_clock::now();
  for (int i = 0; i < n; ++i) {
    // t = clock();
    // t1 = time(0);
    // mm = hms % 10000000 / 100000;
    time_ptr = time(NULL);
    // tm *tm_local = localtime(&time_ptr);  // 性能很差 精度为秒
    // tm *tm_local = localtime_r(&time_ptr); //运行于linux平台下 localtime_s windows
    // t1 = t1 + 28800;  // 因为我们在+8时区 后面要用gmtime分解时间 必须手工修正一下 就是60秒*60分钟*8小时 = 28880
    // gmtime_r(&t1, &today);
    // snprintf(tmp, 64, "%04d-%02d-%02d %02d:%02d:%02d", today.tm_year + 1900, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min,
    //          today.tm_sec);

    // nolocks_localtime(&today, 0, 0, 0);  // 性能很差
  }

  std::cout << "clock:" << today.tm_hour << "," << today.tm_min << "," << mm << std::endl;
  std::cout << "r1:" << duration_cast<milliseconds>(high_resolution_clock::now() - m_begin5).count() << std::endl;
}