#include "include/log.h"

Log::Log() {
  m_split_lines = 0;
  m_log_buf_size = 0;
  m_count = 0;
  m_today = 0;
  m_fp = nullptr;
  m_buf = nullptr;
  m_log_queue = nullptr;
  // write_thread = nullptr;
  m_is_async = false;
  m_close_log = 0;
}

void Log::flush() {
  m_mutex.lock();
  if (m_is_async) {
    m_log_queue->flush();
  }
  fflush(m_fp);
  m_mutex.unlock();
}

Log::~Log() {
  // if(write_thread && write_thread->joinable()){
  while (!m_log_queue->empty()) {
    m_log_queue->flush();
  }
  m_log_queue->_close();
  // write_thread->join();
  //}
  if (m_fp) {
    m_mutex.lock();
    flush();
    fclose(m_fp);
    m_mutex.unlock();
  }

  //        while(!m_log_queue->empty()) {
  //            m_log_queue->flush();
  //        };
  //        m_log_queue->_close();
  //        //writeThread_->join();

  //    if(m_fp) {
  //        m_mutex.lock();
  //        flush();
  //        fclose(m_fp);
  //        m_mutex.unlock();
  //    }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size,
               int split_lines, int max_queue_size) {
  //如果设置了max_queue_size,则设置为异步
  if (max_queue_size >= 1) {
    m_is_async = true;

    if (!m_log_queue) {
      //            std::unique_ptr<BlockDeque<std::string>> *newDeque(new
      //            BlockDeque<std::string>(max_queue_size)); m_log_queue =
      //            std::move(newDeque);
      m_log_queue = new BlockDeque<std::string>(max_queue_size);
      //            std::unique_ptr<std::thread> NewThread(new
      //            std::thread(flush_log_thread));

      //            writeThread_ = std::move(NewThread);
      // writeThread_ = new std::thread(flush_log_thread);

      // pthread_create(&writeThread_, NULL, flush_log_thread, NULL);
      //            if(pthread_detach(writeThread_)){
      //                throw std::exception();
      //            }

      pthread_t tid;
      pthread_create(&tid, NULL, flush_log_thread, NULL);

      //            std::unique_ptr<std::thread> NewThread(new
      //            std::thread([]{Log::get_instance()->async_write_log();}));
      //                        write_thread = std::move(NewThread);
    }
  } else {
    m_is_async = false;
  }

  m_close_log = close_log;
  m_log_buf_size = log_buf_size;
  m_buf = new char[m_log_buf_size];
  memset(m_buf, '\0', m_log_buf_size);
  m_split_lines = split_lines;

  time_t t = time(NULL);
  struct tm *sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;

  // '/'最后出现的位置
  const char *p = strrchr(file_name, '/');
  char log_full_name[256] = {0};

  if (p == NULL) {
    //没有 ‘/‘ ，直接用时间+file_name作为文件名
    snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900,
             my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
  } else {
    // '/' ,把前面的路径提取出来作为i而文件路径
    strcpy(log_name, p + 1);
    strncpy(dir_name, file_name, p - file_name + 1);
    snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name,
             my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
  }

  m_today = my_tm.tm_mday;

  m_fp = fopen(log_full_name, "a");
  if (m_fp == NULL) {
    return false;
  }

  return true;
}

void Log::write_log(int level, const char *format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, NULL);
  time_t t = now.tv_sec;
  struct tm *sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;
  char s[16] = {0};

  switch (level) {
    case 0:
      strcpy(s, "[debug]:");
      break;
    case 1:
      strcpy(s, "[info]:");
      break;
    case 3:
      strcpy(s, "[warn]:");
      break;
    case 4:
      strcpy(s, "[erro]:");
      break;
    case 5:
      strcpy(s, "[info]:");
      break;
    default:
      strcpy(s, "[info]:");
      break;
  }

  //写入一个log,对m_count++, m_split_lines最大行数
  m_mutex.lock();
  m_count++;

  //日期改变（不是同一天）， 或者达到最大行， 新建log文件

  if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
    // everyday log
    char new_log[256] = {0};
    fflush(m_fp);
    fclose(m_fp);
    char tail[16] = {0};

    snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1,
             my_tm.tm_mday);
    if (m_today != my_tm.tm_mday) {
      snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
      m_today = my_tm.tm_mday;
      m_count = 0;
    } else {
      snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name,
               m_count / m_split_lines);
    }

    m_fp = fopen(new_log, "a");
  }

  m_mutex.unlock();

  va_list valst;
  //获取可变参数， 传入的format后的参数
  va_start(valst, format);

  std::string log_str;
  m_mutex.lock();

  //写入具体时间内容格式
  int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                   my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                   my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
  //这里m是输入字符串的长度， vsnprintf最后一位默认为'\0'
  int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);

  // m_log_buf_size如果小于输入的n +m ,会发生数组越界

  // m_buff[ n + m] = '\0';
  // m_buff[ n + m + ] ='\0';

  //修改
  if (n + m + 1 > m_log_buf_size - 1) {
    m_buf[m_log_buf_size - 2] = '\n';
    m_buf[m_log_buf_size - 1] = '\0';
  } else {
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
  }

  log_str = m_buf;
  m_mutex.unlock();
  std::string str(log_str);
  if (m_is_async && !m_log_queue->full()) {
    //异步模式放到队列里等待处理
    m_log_queue->push_back(str);
  } else {
    //同步模式直接写入
    m_mutex.lock();
    fputs(log_str.c_str(), m_fp);
    m_mutex.unlock();
  }
  va_end(valst);
}

//其中异步模式，是将处理好的字符串放到了阻塞队列中，等待处理线程进行处理，处理线程只需要取出该字符串，将其写入文件
void *Log::async_write_log() {
  std::string single_log;
  //从阻塞队列中取出一直日志string, 写入文件流
  while (m_log_queue->pop(single_log)) {
    m_mutex.lock();
    fputs(single_log.c_str(), m_fp);
    m_mutex.unlock();
  }
  return nullptr;
}
