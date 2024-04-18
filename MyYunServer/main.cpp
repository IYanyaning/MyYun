#include "include/http_conn.h"
#include "include/threadpool.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

// #include"lst_timer.h"

// #include"log.h"
// #include"sql_connection_pool.h"
static int m_close_log = 0;

#define FD_LIMIT 65535 // time
#define TIMESLOT 5
#define MAX_FD 65536           // 最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000 // 监听的最大的事件数量

static int epollfd = 0;
static sort_timer_lst timer_lst;
static int pipefd[2];

extern int setnonblocking(int fd);
extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);
/*int setnonblocking( int fd);
void addfd( int epollfd, int fd, bool one_shot);
void removefd( int epollfd, int fd);*/
/*int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}*/
void addfd(int epollfd, int fd) {
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLET;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  setnonblocking(fd);
}

void sig_hanlder(int sig) {
  int save_errno = errno;
  int msg = sig;
  send(pipefd[1], (char *)&msg, 1, 0);
  errno = save_errno;
}

void addsig(int sig) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = sig_hanlder;
  sa.sa_flags |= SA_RESTART;
  sigfillset(&sa.sa_mask);
  assert(sigaction(sig, &sa, NULL) != -1);
}

void timer_handler() {
  // 定时处理任务，实际上就是调用tick()函数
  timer_lst.tick();
  // 因为一次 alarm 调用只会引起一次SIGALARM
  // 信号，所以我们要重新定时，以不断触发 SIGALARM信号。
  alarm(TIMESLOT);
}

void cb_func(client_data *user_data) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
  assert(user_data);
  close(user_data->sockfd);
  LOG_INFO("------------------client [%s] is closed----------------",
           inet_ntoa(user_data->address.sin_addr));
  printf("------------------client [%s] is closed----------------\n",
         inet_ntoa(user_data->address.sin_addr));
}

void addsig(int sig, void(handler)(int)) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  assert(sigaction(sig, &sa, NULL) != -1);
}
int main(int argc, char *argv[]) {

  if (argc <= 1) {
    printf("Usage: %s port_number\n", basename(argv[0]));
    return 1;
  }

  int port = atoi(argv[1]);

  addsig(SIGPIPE, SIG_IGN);

  // log
  Log::get_instance()->init("../log_info", 0, 2000, 800000, 20);
  LOG_INFO("------------------LOG init----------------");
  // 创建数据库连接池
  connection_pool *connPool = connection_pool::GetInstance();

  connPool->init("localhost", "root", "root", "db_Client", 3306, 8);

  threadpool<http_conn> *pool = NULL;
  try {
    pool = new threadpool<http_conn>;
  } catch (...) {
    LOG_DEBUG("DEBUG:main.cpp-threadpool create wrong");
    return 1;
  }

  LOG_INFO("------------------thread pool OK---------------");
  http_conn *users = new http_conn[MAX_FD];
  assert(users);

  // 初始化数据去读取表
  users->initmysql_result(connPool);

  LOG_INFO("----------------------initmysql---------------");
  int listenfd = socket(PF_INET, SOCK_STREAM, 0);
  if (listenfd == -1) {
    perror("socket");
    exit(-1);
  }

  int ret = 0;
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // 端口复用
  int reuse = 1;
  ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  if (ret == -1) {
    LOG_ERROR("------------------setsockopt wrong---------------");
    exit(-1);
  }

  ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
  if (ret == -1) {
    LOG_ERROR("------------------bind wrong---------------");
    exit(-1);
  }

  ret = listen(listenfd, 5);
  if (ret == -1) {
    LOG_ERROR("------------------listen wrong---------------");
    exit(-1);
  }

  // 创建epoll对象
  epoll_event events[MAX_EVENT_NUMBER];
  // int epollfd = epoll_create( 5 );
  epollfd = epoll_create(5);
  if (epollfd == -1) {
    LOG_ERROR("------------------epoll_create wrong---------------");
    exit(-1);
  }

  // 把fd添加到epoll对象中
  addfd(epollfd, listenfd, false);
  http_conn::m_epollfd = epollfd;

  // 创建管道
  ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
  assert(ret != -1);
  setnonblocking(pipefd[1]);
  addfd(epollfd, pipefd[0]);

  // 设置信号处理函数
  addsig(SIGALRM);
  addsig(SIGTERM);
  bool stop_server = false;

  bool timeout = false;
  alarm(TIMESLOT); // 定时,5秒后产生SIGALARM信号

  while (!stop_server) {

    int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

    if ((number < 0) && (errno != EINTR)) {
      LOG_ERROR("%s", "epoll failure");
      break;
    }

    for (int i = 0; i < number; i++) {

      int sockfd = events[i].data.fd;

      if (sockfd == listenfd) {

        struct sockaddr_in client_address;
        socklen_t client_addrlength = sizeof(client_address);
        int connfd = accept(listenfd, (struct sockaddr *)&client_address,
                            &client_addrlength);
        if (connfd < 0) {
          LOG_ERROR("%s:errno is:%d", "accept error", errno);
          continue;
        }
        if (http_conn::m_user_count >= MAX_FD) {

          LOG_ERROR("%s", "Internal server busy");
          continue;
        }

        users[connfd].init(connfd, client_address);
        users[connfd].initmysql_result(connPool);
        // 创建定时器，设置其回调函数与超时时间，然后绑定定时器与用户数据，最后将定时器添加到链表timer_lst中

        util_timer *timer = new util_timer;

        timer->user_data = users[connfd].get_client_data();

        timer->cb_func = cb_func;

        time_t cur = time(NULL);

        timer->expire = cur + 3 * TIMESLOT;

        users[connfd].set_client_data_timer(timer);

        timer_lst.add_timer(timer);

        LOG_INFO("------------------client [%s] is coming----------------",
                 inet_ntoa(users[connfd].get_client_data()->address.sin_addr));
        printf("------------------client [%s] is coming----------------\n",
               inet_ntoa(users[connfd].get_client_data()->address.sin_addr));
        continue;
      } else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
        // 处理信号
        char signals[1024];
        ret = recv(pipefd[0], signals, sizeof(signals), 0);
        if (ret == -1) {
          continue;
        } else if (ret == 0) {
          continue;
        } else {
          for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
            case SIGALRM: {
              // 用timeout变量标记有定时任务需要处理，但不立即处理定时任务
              // 因为定时任务的优先级不是很高，优先处理更重要的IO任务
              timeout = true;
              break;
            }
            case SIGTERM: {
              stop_server = true;
            }
            }
          }
        }
      } else if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        // users[sockfd].close_conn();
        cb_func(users[sockfd].get_client_data());
        cout << "in events[i].events & ( EPOLLHUP | EPOLLRDHUP |EPOLLERR\n" << errno << endl;
        if (users[sockfd].get_client_data()->timer) {
          timer_lst.del_timer(users[sockfd].get_client_data()->timer);
        }
      } else if (events[i].events & EPOLLIN) {

        if (int read_byte = users[sockfd].read()) {
          printf("epollin\n");
          LOG_INFO(
              "deal with the client(%s)",
              inet_ntoa(users[sockfd].get_client_data()->address.sin_addr));
          if (users[sockfd].get_client_data()->timer) {
            time_t cur = time(NULL);
            users[sockfd].get_client_data()->timer->expire = cur + 3 * TIMESLOT;

            LOG_INFO("%s", "adjust timer once");
            timer_lst.adjust_timer(users[sockfd].get_client_data()->timer);
            // LOG_INFO("INFO: adjust timer once for client[%d]",sockfd);
          }
          pool->append(users + sockfd);
        } else {
          cb_func(users[sockfd].get_client_data());
          cout << "in events[i].events & EPOLLIN\n";
          if (users[sockfd].get_client_data()->timer) {
            timer_lst.del_timer(users[sockfd].get_client_data()->timer);
          }
          users[sockfd].close_conn();
        }
      } else if (events[i].events & EPOLLOUT) {
        if (users[sockfd].write()) {
          printf("epollout\n");
          LOG_INFO(
              "send data to the client(%s)",
              inet_ntoa(users[sockfd].get_client_data()->address.sin_addr));
          
          if (users[sockfd].get_client_data()->timer) {
            time_t cur = time(NULL);
            users[sockfd].get_client_data()->timer->expire = cur + 3 * TIMESLOT;

            LOG_INFO("%s", "adjust timer once");
            timer_lst.adjust_timer(users[sockfd].get_client_data()->timer);
            printf("afteradjust\n");
          }
        } else {
          cb_func(users[sockfd].get_client_data());
          if (users[sockfd].get_client_data()->timer) {
            timer_lst.del_timer(users[sockfd].get_client_data()->timer);
          }
        }
      }
    }
    if (timeout) {
      timer_handler();
      timeout = false;
    }
  }

  close(epollfd);
  close(listenfd);
  close(pipefd[1]);
  close(pipefd[0]);
  delete[] users;
  delete pool;
  return 0;
}
