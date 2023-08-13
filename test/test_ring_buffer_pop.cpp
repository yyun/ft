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
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>

// #define USE_MB
// #define USE_LOCK
// #define USE_POT

#define SHM_NAME_LEN 60
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define IS_POT(x) ((x) && !((x) & ((x)-1)))
#define MEMORY_BARRIER __sync_synchronize()

static uint32_t roundup_pow_of_two(uint32_t size) {
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return size+1;
}

/////////////////////////////////////////////////////////////////////
// spin lock
typedef struct {
    int lock;
} spinlock_t;

static inline void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
}
static inline void spinlock_lock(spinlock_t *lock) {
    while (!__sync_bool_compare_and_swap(&lock->lock, 0, 1)) {}
}
static inline void spinlock_unlock(spinlock_t *lock) {
    __sync_lock_release(&lock->lock);
}

/////////////////////////////////////////////////////////////////////
// ring buffer
typedef struct {
    char shm_name[SHM_NAME_LEN];
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t size;
#ifdef USE_LOCK
    spinlock_t lock;
#endif
    uint8_t *buffer;
} ringbuffer_t;

// 创建ringbuffer
ringbuffer_t *rb_create(const char *name, uint32_t sz, int master) {
    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0777);
    if (shm_fd < 0) {
        perror("shm_open");
        return NULL;
    }

#ifdef USE_POT
    if (!IS_POT(sz)) {
        sz = roundup_pow_of_two(sz);
        // printf("size=%u\n", sz);
    }
#endif

    size_t size = sizeof(ringbuffer_t) + sz;
    if (ftruncate(shm_fd, size) < 0) {
        perror("ftruncate");
        close(shm_fd);
        return NULL;
    }
    uint8_t *addr = (uint8_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return NULL;
    }
    if (close(shm_fd) == -1) {
        perror("close");
        exit(1);
    }

    ringbuffer_t *rb = (ringbuffer_t*)addr;
    // rb->size = sz;
    // strncpy(rb->shm_name, name, SHM_NAME_LEN-1);
    // rb->head = rb->tail = 0;
     rb->buffer = addr + sizeof(ringbuffer_t);

#ifdef USE_LOCK
    if (master) {
        spinlock_init(&rb->lock);
    }
#endif
    return rb;
}

// 释放ringbuffer
void rb_free(ringbuffer_t *rb, int master) {
    size_t size = sizeof(ringbuffer_t) + rb->size;
    if (munmap(rb, size) == -1) {
        perror("munmap");
        return;
    }
    if (master) {
        // if (shm_unlink(rb->shm_name) == -1) {
        //     perror("shm_unlink");
        //     return;
        // }
    }
}

// 已使用大小
inline int rb_used(ringbuffer_t *rb) {
    uint32_t head = rb->head;
    uint32_t tail = rb->tail;
    if (head <= tail)
        return tail - head;
    else
        return rb->size - (head - tail);
}

// 压入数据，成功返回0
int rb_push(ringbuffer_t *rb, const uint8_t *buff, int size) {
#ifdef USE_LOCK
    spinlock_lock(&rb->lock);
#endif
    int unused = rb->size - rb_used(rb);
    if (unused <= size) {
#ifdef USE_LOCK
    spinlock_unlock(&rb->lock);
#endif
        return -1;
    }

    uint32_t len = MIN(size, rb->size - rb->tail);
    memcpy(rb->buffer+rb->tail, buff, len);
    if (size > len)
        memcpy(rb->buffer, buff+len, size-len);

#ifdef USE_MB
    MEMORY_BARRIER;
#endif

#ifdef USE_POT
    rb->tail = (rb->tail + size) & (rb->size -1);
#else
    rb->tail = (rb->tail + size) % rb->size;
#endif

#ifdef USE_LOCK
    spinlock_unlock(&rb->lock);
#endif

    return 0;
}

// 弹出数据，成功返回0
int rb_pop(ringbuffer_t *rb, uint8_t *buff, int size) {
#ifdef USE_LOCK
    spinlock_lock(&rb->lock);
#endif

    int used = rb_used(rb);
    if (used < size) {
#ifdef USE_LOCK
    spinlock_unlock(&rb->lock);
#endif
        return -1;
    }

    uint32_t len = MIN(size, rb->size - rb->head);
    memcpy(buff, rb->buffer + rb->head, len);
    if (size > len)
        memcpy(buff + len, rb->buffer, size - len);

#ifdef USE_MB
    MEMORY_BARRIER;
#endif

#ifdef USE_POT
    rb->head = (rb->head + size) & (rb->size -1);
#else
    rb->head = (rb->head + size) % rb->size;
#endif


#ifdef USE_LOCK
    spinlock_unlock(&rb->lock);
#endif

    return 0;
}

double getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int test(size_t count)
{
    int i;
	size_t size=sizeof(sse_hpf_exe_pkt);
    struct timeval begin, end;
    // if (argc != 3) {
    //     printf("usage: ./shm2 <size> <count>\n");
    //     return 1;
    // }

    const char *path = "/shm_ring_buffer";
    // int size = atoi(argv[1]);
    // int count = atoi(argv[2]);
    // unsigned char *buf =(unsigned char*) malloc(sizeof(sse_hpf_exe_pkt));
    int rbsize = size * 1000001;

      ringbuffer_t *rb = rb_create(path, rbsize, 1);
         i = 0;

        while (i < count) {
            sse_hpf_exe_pkt info;
            if (!rb_pop(rb,(uint8_t *) &info, size)) {
        
                //  std::cout <<  info.m_channel <<std::endl;
            
                // if (*(uint32_t*)buf != i) {
                //     printf("data: %d\n", *(uint32_t*)buf);
                // }
                ++i;
            }else{
                   std::cout <<  "null" <<std::endl;
            }
        }

		std::cout <<  "pop:"<<i <<std::endl;
        rb_free(rb, 1);

    return 0;
}

TEST(RingBufferTest, Case_0) {
  const int n = 10;
  RingBuffer<sse_hpf_exe_pkt, 4096> rb;
 
  test(n);
   
}
