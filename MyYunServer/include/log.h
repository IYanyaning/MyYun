#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "blockqueue.h"
#include "locker.h"

class Log {
 public:
  // C++11以后，使用局部变量懒汉不用加锁
  static Log *get_instance() {
    static Log log;
    return &log;
  }

  //异步写的线程
  static void *flush_log_thread(void *args) {
    Log::get_instance()->async_write_log();
    return nullptr;
  }

  //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
  bool init(const char *file_name, int close_log, int log_buf_size = 8192,
            int split_lines = 5000000, int max_queue_size = 0);

  //写入消息
  void write_log(int level, const char *format, ...);

  //刷新缓冲区
  void flush(void);

 private:
  Log();
  virtual ~Log();
  void *async_write_log();

 private:
  char dir_name[128];  //路径名
  char log_name[128];  // log文件名

  int m_split_lines;   //日志最大行数
  int m_log_buf_size;  //日志缓冲区大小
  long long m_count;   //入职行数记录
  int m_today;         //记录当前时间是哪一天
  FILE *m_fp;          //打开log的文件描述符
  char *m_buf;
  BlockDeque<std::string> *m_log_queue;  //阻塞队列
  bool m_is_async;                       //是否同步标识位
  // std::mutex m_mutex;
  locker m_mutex;
  int m_close_log;  //关闭日志
                    // pthread_t writeThread_;
  // std::unique_ptr<std::thread> write_thread;
};

//使用宏定义，便于调用， 使用##__VA_ARGS__，支持format后面可有0到多个参数
#define LOG_DEBUG(format, ...)                                \
  if (0 == m_close_log) {                                     \
    Log::get_instance()->write_log(0, format, ##__VA_ARGS__); \
    Log::get_instance()->flush();                             \
  }
#define LOG_INFO(format, ...)                                 \
  if (0 == m_close_log) {                                     \
    Log::get_instance()->write_log(1, format, ##__VA_ARGS__); \
    Log::get_instance()->flush();                             \
  }
#define LOG_WARN(format, ...)                                 \
  if (0 == m_close_log) {                                     \
    Log::get_instance()->write_log(2, format, ##__VA_ARGS__); \
    Log::get_instance()->flush();                             \
  }
#define LOG_ERROR(format, ...)                                \
  if (0 == m_close_log) {                                     \
    Log::get_instance()->write_log(3, format, ##__VA_ARGS__); \
    Log::get_instance()->flush();                             \
  }

#endif  // LOG_H
