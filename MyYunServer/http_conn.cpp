#include "include/http_conn.h"
using std::map;
using std::string;
static int m_close_log = 0;
namespace Mycompress {
static int MAX_FILENAME = 256;
static int WRITEBUFFERSIZE = 8192;
}  
// 定义HTTP响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form =
    "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form =
    "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form =
    "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form =
    "There was an unusual problem serving the requested file.\n";

// 网站根目录
const char *doc_root = "./resources";

// 将表中的用户名和密码放入map

map<string, string> users;
map<string, string> uuid_file_path;
locker m_locker;
void http_conn::initmysql_result(connection_pool *connPool) {
  this->m_connection_pool = connPool;
  if (users.empty() && uuid_file_path.empty()) {
    // 先从连接池取一个连接
    connectionRAII mysqlcon(&mysql, connPool);

    // 在user表中检索username， passwd数据， 浏览器输入
    if (mysql_query(mysql, "SELECT user_account, user_password FROM test")) {
      LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
      m_connection_pool->ReleaseConnection(mysql);
      return;
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);
    if (result == NULL) {
      LOG_ERROR("无法获取查询结果: %s\n", mysql_error(mysql));
      mysql_free_result(result);
      m_connection_pool->ReleaseConnection(mysql);
      return;
    }

    // 返回结果集中的列数
    int num_fields = mysql_num_fields(result);
    // cout<<"num_fields:"<<num_fields<<std::endl;
    // 返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    // 从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
      string temp1(row[0]);
      string temp2(row[1]);
      users[temp1] = temp2;
    }
    for (const auto &pair : users) {
      std::cout << "username: " << pair.first << ", password: " << pair.second
                << std::endl;
    }

    // 查询语句
    const char *query = "SELECT uuid, file_path FROM user_files";

    // 执行查询
    if (mysql_query(mysql, query) != 0) {
      fprintf(stderr, "查询数据时出错: %s\n", mysql_error(mysql));
      mysql_free_result(result);
      m_connection_pool->ReleaseConnection(mysql);
      return;
    }

    // 获取查询结果
    result = mysql_store_result(mysql);
    if (result == NULL) {
      fprintf(stderr, "无法获取查询结果: %s\n", mysql_error(mysql));
      mysql_free_result(result);
      m_connection_pool->ReleaseConnection(mysql);
      return;
    }
    // 保存查询结果的键值对容器

    // 遍历查询结果并保存到容器中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
      const string uuid = row[0];
      const string file_path = row[1];

      // 将 uuid 和 file_path 保存到键值对容器中
      uuid_file_path[uuid] = file_path;
    }
    // 释放结果集
    mysql_free_result(result);
  }
}
http_conn::http_conn() {}

int setnonblocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

// 向epoll中添加需要监听的文件描述符

void addfd(int epollfd, int fd, bool one_shot) {
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLRDHUP;
  if (one_shot) {
    // 放置同一个通信被不同的线程处理
    event.events |= EPOLLONESHOT;
  }

  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  // 设置文件描述符非阻塞
  setnonblocking(fd);
}

// 向epoll中移除监听的文件描述符
void removefd(int epollfd, int fd) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
  close(fd);
}

// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev) {
  epoll_event event;
  event.data.fd = fd;
  event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
  epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 所有的客户数
int http_conn::m_user_count = 0;
// 所有socket上的事件都被注册到同一个epoll内核事件中，所以设置成静态的
int http_conn::m_epollfd = -1;

// 关闭连接
void http_conn::close_conn() {
  if (m_sockfd != -1) {
    removefd(m_epollfd, m_sockfd);
    m_sockfd = -1;
    m_user_count--;
  }
}

// 初始化连接，外部调用初始化套接字地址
void http_conn::init(int sockfd, const sockaddr_in &addr) {
  m_sockfd = sockfd;
  m_address = addr;

  // 端口复用

  int reuse = 1;
  setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  addfd(m_epollfd, sockfd, true);

  client = new client_data;
  client->address = m_address;
  client->sockfd = m_sockfd;

  m_user_count++;
  std::cout << "httpinit----------\n";
  init();
}

void http_conn::init() {
  cout << "do init------------------------------------------" << endl;
  mysql = NULL;
  if (!m_linger) {
    m_connection_pool = NULL;
  }

  bytes_to_send = 0;
  bytes_have_send = 0;

  m_check_state = CHECK_STATE_REQUESTLINE;  // 初始化为检查请求行
  m_linger = false;  // 默认不保持连接， Connection: keep-alive

  m_method = GET;  // 默认请求方式为GET
  m_url = 0;

  m_string = NULL;
  m_version = 0;
  m_content_length = 0;
  m_host = 0;
  m_start_line = 0;
  m_checked_idx = 0;
  m_read_idx = 0;
  m_write_idx = 0;
  //cgi = 0;
  is_upload_request = false;
  memset(myBuf, '\0', READ_BUFFER_SIZE);
  memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
}

// 循环读取客户端数据，直到无数据可读或者对方关闭连接
bool http_conn::read() {
  if (m_read_idx >= READ_BUFFER_SIZE) {
    return false;
  }
  int bytes_read = 0;
  // 定义数据包长度
  int packetSize = 0;
  while (true) {
    if (is_upload_request && m_read_idx >= m_content_length) {
      break;
    }
    // 从myBuf + m_read_idx 索引开始保存数据，大小是READ_BUFFER_SIZE
    bytes_read =
        recv(m_sockfd, myBuf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 没有数据
        cout << "没有数据\n";
        if (!is_upload_request) {
          break;
        }
      }
      if (!is_upload_request) {
        return false;
      }
    } else if (bytes_read == 0) {  // 对方关闭连接
      cout << "对方关闭连接\n";
    }
    if (bytes_read >= 0) {
      m_read_idx += bytes_read;
    }
    if (!is_upload_request) {
      if (strstr(myBuf, "upload") != NULL) {
        is_upload_request = true;
        if (m_content_length == 0) {
          const char *content_length_pos = strstr(myBuf, "Content-Length: ");
          if (content_length_pos != NULL) {
            content_length_pos += strlen("Content-Length: ");
            m_content_length = atoi(content_length_pos);
          }
        }
      }
    }
  }
  return true;
}

// 解析一行，判断依据\r\n
http_conn::LINE_STATUS http_conn::parse_line() {
  char temp;
  for (; m_checked_idx < m_read_idx; ++m_checked_idx) {
    temp = myBuf[m_checked_idx];
    if (temp == '\r') {
      if ((m_checked_idx + 1) == m_read_idx) {
        return LINE_OPEN;
      } else if (myBuf[m_checked_idx + 1] == '\n') {
        myBuf[m_checked_idx++] = '\0';
        myBuf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    } else if (temp == '\n') {
      if ((m_checked_idx > 1) && (myBuf[m_checked_idx - 1] == '\r')) {
        myBuf[m_checked_idx - 1] = '\0';
        myBuf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
  }
  return LINE_OPEN;
}

/*
 * C 库函数 char *strpbrk(const char *str1, const char *str2) 检索字符串 str1
 * 中第一个匹配字符串 str2
 * 中字符的字符，不包含空结束字符。也就是说，依次检验字符串 str1
 * 中的字符，当被检验字符在字符串 str2
 * 中也包含时，则停止检验，并返回该字符位置。
 */
// 解析HTTP请求行，获得请求方法， 目标URL,以及HTTL版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
  printf("%s\n", text);
  // GET /index.html HTTP/ 1.1
  m_url = strpbrk(text, " \t");  // 判断第二个参数中的字符在text中最先出现的位置
  // cout << m_url << endl;
  if (!m_url) {
    return BAD_REQUEST;
  }
  // GET\0/index.html HTTP/1.1
  *m_url++ = '\0';  // 置位空字符，字符串结束符
  char *method = text;
  if (strcasecmp(method, "GET") == 0) {  // 忽略大小写比较
    m_method = GET;
  } else if (strcasecmp(method, "POST") == 0) {
    m_method = POST;
    //cgi = 1;
  } else {
    return BAD_REQUEST;
  }

  // /index.html HTTP/1.1
  // 检索字符串str1中第一个不在字符串str2中出现的字符下标
  m_url += strspn(m_url, " \t");
  m_version = strpbrk(m_url, " \t");
  if (!m_version) {
    return BAD_REQUEST;
  }
  *m_version++ = '\0';
  m_version += strspn(m_version, " \t");
  if (strcasecmp(m_version, "HTTP/1.1") != 0) {
    return BAD_REQUEST;
  }
  /*
      http://192.168.110.129:10000/index.html
  */
  if (strncasecmp(m_url, "http://", 7) == 0) {
    m_url += 7;
    // 在参数str所指向的字符串中搜索第一次出现字符 c (一个无符号字符)的位置
    m_url = strchr(m_url, '/');
  }
  if (strncasecmp(m_url, "https://", 8) == 0) {
    m_url += 8;
    m_url = strchr(m_url, '/');
  }
  if (!m_url || m_url[0] != '/') {
    return BAD_REQUEST;
  }
  // cout << m_url << endl;
  if (strlen(m_url) == 1) {
    strcat(m_url, "judge.html");
  }
  // cout << m_url << endl;
  m_check_state = CHECK_STATE_HEADER;
  return NO_REQUEST;
}

// 解析HTTP请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text) {
  // 遇到空行，表示头部字段解析完毕
  if (text[0] == '\0') {
    // 如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体
    // 状态机转移到CHECK_STATE_CONTENT状态
    if (m_content_length != 0) {
      m_check_state = CHECK_STATE_CONTENT;
      return NO_REQUEST;
    }
    // 否则说明已经得到了一个完整的HTTP请求
    return GET_REQUEST;
  } else if (strncasecmp(text, "Connection:", 11) == 0) {
    // 处理Connection头部字段， Connection: keep-alive
    text += 11;
    text += strspn(text, "\t");
    if (strcasecmp(text, "keep-alive") == 0) {
      m_linger = true;
    }
  } else if (strncasecmp(text, "Content-Length:", 15) == 0) {
    // 处理Content-Length头部字段
    text += 15;
    text += strspn(text, "\t");
    m_content_length = atol(text);
  } else if (strncasecmp(text, "Host:", 5) == 0) {
    // 处理Host头部字段
    text += 5;
    text += strspn(text, "\t");
    m_host = text;
  } else if (strncasecmp(text, "Content-Type:", 13) == 0) {
    char *bb = strstr(text, "boundary_");
    if (bb != NULL) {
      m_boundary = string(bb);
      m_boundary.pop_back();
      cout << "boundary: " << m_boundary << endl;
    }

    /*
        Content-Type: multipart/form-data;
       boundary="boundary_.oOo._sOaSRjU+Y8yIIrsZ3JFVYvrlM3ffAFpx"
    */
  } else {
    LOG_INFO("oop!unknow header: %s", text);
  }
  return NO_REQUEST;
}

// 没有真正的解析HTTP请求的消息体，只是判断它是否完整读入了
http_conn::HTTP_CODE http_conn::parse_content(char *text) {
  if (m_read_idx >= (m_content_length + m_checked_idx)) {
    text[m_content_length] = '\0';
    // POST请求中最后为输入的用户名和密码
    m_string = text;
    return GET_REQUEST;
  }
  return NO_REQUEST;
}

// 主状态机，解析请求
http_conn::HTTP_CODE http_conn::process_read() {
  LINE_STATUS line_status = LINE_OK;
  HTTP_CODE ret = NO_REQUEST;
  char *text = 0;
  while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) ||
         ((line_status = parse_line()) == LINE_OK)) {
    // 获取到了一行数据
    text = get_line();
    m_start_line = m_checked_idx;
    // LOG_INFO("%s", text);

    switch (m_check_state) {
      case CHECK_STATE_REQUESTLINE: {
        ret = parse_request_line(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        }
        break;
      }
      case CHECK_STATE_HEADER: {
        ret = parse_headers(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        } else if (ret == GET_REQUEST) {
          cout << "to do request process read" << endl;
          return do_request();
        }
        break;
      }
      case CHECK_STATE_CONTENT: {
        ret = parse_content(text);
        if (ret == GET_REQUEST) {
          return do_request();
        }
        line_status = LINE_OPEN;
        break;
      }
      default: {
        return INTERNAL_ERROR;
      }
    }
  }
  return NO_REQUEST;
}

bool directoryExists(const string &directory) {
  struct stat buffer;
  return (stat(directory.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool createDirectory(const string &directory) {
  if (directoryExists(directory)) {
    // cout << "目录已经存在" << std::endl;
    LOG_INFO("目录已经存在", directory);
    return true;
  } else {
    if (mkdir(directory.c_str(), 0777) == 0) {
      // cout << "目录创建成功" << std::endl;
      LOG_INFO("目录创建成功", directory);
      return true;
    } else {
      // std::cerr << "无法创建目录:" << directory << std::endl;
      LOG_ERROR("无法创建目录", directory);
      return false;
    }
  }
}

bool http_conn::createNewHtml(const string &uuid_key, const string &file_path) {
  std::ifstream inFile("./resources/pdf.html");  // 打开原始的 HTML 文件
  if (!inFile.is_open()) {
    return false;
  }
  std::string content((std::istreambuf_iterator<char>(inFile)),
                      std::istreambuf_iterator<char>());
  inFile.close();

  std::string placeholder = "这里是src";
  size_t pos = content.find(placeholder);
  if (pos != std::string::npos) {
    content.replace(pos, placeholder.length(),
                    uuid_key);  // 替换密钥值占位符为实际的密钥值
  }

  std::ofstream outFile(file_path);  // 创建新的 HTML 文件
  if (!outFile.is_open()) {
    return false;
  }
  outFile << content;
  outFile.close();
  return true;
}


std::string urlDecode(const std::string &input) {
  std::ostringstream decoded;
  for (size_t i = 0; i < input.length(); ++i) {
    if (input[i] == '%') {
      if (i + 2 < input.length()) {
        int value;
        std::istringstream hexStream(input.substr(i + 1, 2));
        hexStream >> std::hex >> value;
        decoded << static_cast<char>(value);
        i += 2;
      }
    } else if (input[i] == '+') {
      decoded << ' ';
    } else {
      decoded << input[i];
    }
  }
  return decoded.str();
}
void http_conn::get_file_type(string &extension) {
  if (extension == "html") {
    m_file_type = HTML;
    return;
  } else if (extension == "txt") {
    m_file_type = TXT;
    return;
  } else if (extension == "apng") {
    m_file_type = APNG;
    return;
  } else if (extension == "avif") {
    m_file_type = AVIF;
    return;
  } else if (extension == "bmp") {
    m_file_type = BMP;
    return;
  } else if (extension == "gif") {
    m_file_type = IMAGE_GIF;
    return;
  } else if (extension == "png") {
    m_file_type = IMAGE_PNG;
    return;
  } else if (extension == "svg") {
    m_file_type = SVG;
    return;
  } else if (extension == "webp") {
    m_file_type = WEBP;
    return;
  } else if (extension == "ico") {
    m_file_type = ICO;
    return;
  } else if (extension == "tif") {
    m_file_type = TIF;
    return;
  } else if (extension == "tiff") {
    m_file_type = TIFF;
    return;
  } else if (extension == "jpg") {
    m_file_type = IMAGE_JPG;
    return;
  } else if (extension == "jpeg") {
    m_file_type = IMAGE_JPEG;
    return;
  } else if (extension == "mp4") {
    m_file_type = VIDEO_MP4;
    return;
  } else if (extension == "mpeg") {
    m_file_type = MPEG;
    return;
  } else if (extension == "webm") {
    m_file_type = WEBM;
    return;
  } else if (extension == "mp3") {
    m_file_type = MP3;
    return;
  } else if (extension == "mpga") {
    m_file_type = MPGA;
    return;
  } else if (extension == "weba") {
    m_file_type = WEBA;
    return;
  } else if (extension == "wav") {
    m_file_type = AUDIO_WAV;
    return;
  } else if (extension == "json") {
    m_file_type = JSON;
    return;
  } else if (extension == "tar") {
    m_file_type = TAR;
    return;
  } else if (extension == "zip") {
    m_file_type = ZIP;
    return;
  } else if (extension == "pdf") {
    m_file_type = PDF;
    return;
  } else if (extension == "html") {
    m_file_type = HTML;
    return;
  }
}
// 当得到了一个完整的、正确的HTTP请求时，就分析目标文件的属性
// 如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其
// 映射到内存地址m_file_address处，并告诉调用者获取文件成功
http_conn::HTTP_CODE http_conn::do_request() {
  const char *locationEnd = strrchr(m_url, '/');
  // std::cout<< (string)(locationEnd + 1) <<std::endl;
  if ((strncmp(locationEnd + 1, "register", strlen("register")) == 0) ||
      (strncmp(locationEnd + 1, "login", strlen("login")) == 0)) {
    printf("login's m_string:%s\n", m_string);  // user=chen&passwd=yyy
    string afterUrlDecodem_string = urlDecode((string)m_string);
    string name;
    string password;
    const char *pattern = "user=([^&]*)&passwd=([^&]*)";
    pcre *re = NULL;
    const char *error = NULL;
    int erroffset;
    int ovector[30];

    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);

    if (re == NULL) {
      LOG_ERROR("PCRE compilaction failed at offset %d: %s\n", erroffset,
                error);
      printf("PCRE compilaction failed at offset %d: %s\n", erroffset, error);
      return REGISTER_REQUEST_11;
    }

    int rc = pcre_exec(re, NULL, m_string, strlen(m_string), 0, 0, ovector, 30);

    if (rc < 0) {
      if (rc == PCRE_ERROR_NOMATCH) {
        printf("PCRE No match\n");
        LOG_INFO("PCRE No match:%s\n", m_string);
      } else {
        printf("PCRE matching error: %d\n", rc);
        LOG_ERROR("PCRE matching error: %d\n", rc);
      }
      pcre_free(re);
      return REGISTER_REQUEST_11;
    }
    char tmpname[20];  // 临时存储由用户上传的用户名和密码
    char tmppassword[20];
    for (int i = 1; i < rc; i++) {
      int start = ovector[2 * i];
      int end = ovector[2 * i + 1];
      int length = end - start;

      if (i == 1) {  // "user" capture group
        strncpy(tmpname, m_string + start, length);
        tmpname[length] = '\0';
        name = urlDecode((string(tmpname)));
        LOG_INFO("Match %d: %s\n", i, name.c_str());
        printf("Match %d: %s\n", i, name.c_str());
      } else if (i == 2) {  // "passwd" capture group
        strncpy(tmppassword, m_string + start, length);
        tmppassword[length] = '\0';
        password = urlDecode((string)tmppassword);
        LOG_INFO("Match %d: %s\n", i, password.c_str());
        printf("Match %d: %s\n", i, password.c_str());
      }
    }
    cout << "match ok" << std::endl;
    cout << "username: " << name << "password: " << password << endl;

    if (strncmp(locationEnd + 1, "register", strlen("rigster")) == 0) {
      return do_register(name, password);
    } else {
      return do_login(name, password);
    }
  } else if (strncmp(locationEnd + 1, "delete", strlen("delete")) == 0) {
    return do_delete();
  } else if (strncmp(locationEnd + 1, "rename", strlen("rename")) == 0) {
    return do_rename();
  } else if (strncmp(locationEnd + 1, "share", strlen("share")) == 0) {
    return do_share();
  } else if (strncmp(locationEnd + 1, "downloadSharing",
                     strlen("downloadSharing")) == 0) {
    return do_download_sharing();
  } else if (strncmp(locationEnd + 1, "requestUpload",
                     strlen("requestUpload")) == 0) {
    return do_request_upload();
  } else if (strncmp(locationEnd + 1, "upload", strlen("upload")) == 0) {
    return do_upload();
  } else if (strncmp(locationEnd + 1, "onlineviewfile",
                     strlen("onlineviewfile")) == 0) {
    return do_online_view_file(locationEnd);
  } else if (strncmp(locationEnd + 1, "download", strlen("download")) == 0) {
    return do_download();
  }
  return UPLOAD_REQUEST_40;
}

http_conn::HTTP_CODE http_conn::do_register(string &name, string &password) {
  char *sql_insert = (char *)malloc(sizeof(char) * 200);
  strcpy(sql_insert, "INSERT INTO test(user_account, user_password) VALUES(");
  strcat(sql_insert, "'");
  strcat(sql_insert, name.c_str());
  strcat(sql_insert, "', '");
  strcat(sql_insert, password.c_str());
  strcat(sql_insert, "')");

  connectionRAII mysqlcon1(&mysql, m_connection_pool);

  m_locker.lock();

  int res = mysql_query(mysql, sql_insert);

  users.insert(std::pair<string, string>(name, password));
  m_locker.unlock();

  if (res == 0) {
    // 处理注册成功
    m_linger = true;
    user_name = name;
    user_password = password;
    users[user_name] = user_password;
    LOG_INFO("client: %s register success!", name.c_str());
    return REGISTER_REQUEST_10;
  } else {
    // 处理注册失败
    LOG_ERROR("client: %s register failed!", name.c_str());
    return REGISTER_REQUEST_11;
  }
}

http_conn::HTTP_CODE http_conn::do_login(string &name, string &password) {
  // LOG_INFO("client: %s login success!", name.c_str());
  if (users.find(name) != users.end() && users.find(name)->second == password) {
    // 处理登录成功
    LOG_INFO("client: %s login success!", name.c_str());
    connectionRAII mysqlcon(&mysql, m_connection_pool);
    MYSQL_RES *res;
    MYSQL_ROW row;
    if (mysql == NULL) {
      m_connection_pool->ReleaseConnection(mysql);
      return LOGIN_REQUEST_21;
    }
    string query =
        "SELECT file_name, size FROM file_info WHERE user_name='" + name + "'";
    m_locker.lock();
    if (mysql_query(mysql, query.c_str()) != 0) {
      LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
      std::cerr << "Error querying database: " << mysql_error(mysql)
                << std::endl;
      m_locker.unlock();
      m_connection_pool->ReleaseConnection(mysql);
      return LOGIN_REQUEST_21;
    }
    res = mysql_store_result(mysql);

    Json::Value root;
    Json::Value files(Json::arrayValue);
    while ((row = mysql_fetch_row(res))) {
      Json::Value file;
      file["file_name"] = row[0];
      file["size"] = std::stoi(row[1]);
      files.append(file);
    };
    query = "SELECT uuid, file_name FROM file_share_table WHERE user_name='" +
            name + "'";
    if (mysql_query(mysql, query.c_str()) != 0) {
      std::cerr << "mysql_query failed: " << mysql_error(mysql) << std::endl;
      m_locker.unlock();
      mysql_free_result(res);
      m_connection_pool->ReleaseConnection(mysql);
      return LOGIN_REQUEST_21;
    }
    res = mysql_store_result(mysql);
    if (res == NULL) {
      std::cerr << "mysql_store_result failed" << std::endl;
    }
    Json::Value file_shared(Json::arrayValue);
    while ((row = mysql_fetch_row(res)) != NULL) {
      Json::Value file;
      file["uuid"] = row[0];
      file["file_name"] = row[1];
      file_shared.append(file);
    }

    root["username"] = name;
    root["files"] = files;

    root["file_shared"] = file_shared;
    root["CODE"] = 20;
    Json::StreamWriterBuilder builder;
    m_allFilesJson = Json::writeString(builder, root);
    m_locker.unlock();

    mysql_free_result(res);
    m_connection_pool->ReleaseConnection(mysql);
    m_linger = true;
    user_name = name;
    user_password = password;

    LOG_INFO("client: %s login success!", name.c_str())
    return LOGIN_REQUEST_20;
  } else {
    // 处理登录失败
    LOG_ERROR("client: %s login failed!", name.c_str())
    return LOGIN_REQUEST_21;
  }
}

http_conn::HTTP_CODE http_conn::do_delete() {
  char tmpdownloadUserName[20] = {0};
  char tmpdownloadFileName[255] = {0};
  string downloadFileName;
  string downloadUserName;
  std::string md5_size;
  const char *pattern = "username=([^&]*)&file_name=([^&]*)";
  pcre *re = NULL;
  const char *error = NULL;
  int erroffset;
  int ovector[30];

  re = pcre_compile(pattern, 0, &error, &erroffset, NULL);

  if (re == NULL) {
    LOG_ERROR("PCRE compilaction failed at offset %d: %s\n", erroffset, error);
    printf("PCRE compilaction failed at offset %d: %s\n", erroffset, error);
    return DELETE_REQUEST_61;
  }

  int rc = pcre_exec(re, NULL, m_string, strlen(m_string), 0, 0, ovector, 30);

  if (rc < 0) {
    if (rc == PCRE_ERROR_NOMATCH) {
      printf("PCRE No match\n");
      LOG_INFO("PCRE No match:%s\n", m_string);
      return DELETE_REQUEST_61;
    } else {
      printf("PCRE matching error: %d\n", rc);
      LOG_ERROR("PCRE matching error: %d\n", rc);
      return DELETE_REQUEST_61;
    }
    pcre_free(re);
  }

  for (int i = 1; i < rc; i++) {
    int start = ovector[2 * i];
    int end = ovector[2 * i + 1];
    int length = end - start;

    if (i == 1) {  // "user" capture group
      strncpy(tmpdownloadUserName, m_string + start, length);
      tmpdownloadUserName[length] = '\0';
      downloadUserName = urlDecode((string)tmpdownloadUserName);
      LOG_INFO("Match %d: %s\n", i, downloadUserName.c_str());
      printf("Match %d: %s\n", i, downloadUserName.c_str());
    } else if (i == 2) {  // "passwd" capture group
      strncpy(tmpdownloadFileName, m_string + start, length);
      tmpdownloadFileName[length] = '\0';
      downloadFileName = urlDecode((string)tmpdownloadFileName);
      LOG_INFO("Match %d: %s\n", i, downloadFileName.c_str());
      printf("Match %d: %s\n", i, downloadFileName.c_str());
    }
  }

  std::cout << "username: " << downloadUserName
            << "filename: " << downloadFileName << std::endl;
  connectionRAII mysqlcol(&mysql, m_connection_pool);
  if (mysql == NULL) {
    LOG_ERROR("mysqlcon(&mysql, m_connection_poop) failed");
    m_connection_pool->ReleaseConnection(mysql);
    return DELETE_REQUEST_61;
  }
  string selectQuery = "SELECT md5, size FROM file_info WHERE user_name = '" +
                       downloadUserName + "' AND file_name = '" +
                       downloadFileName + "'";
  m_locker.lock();  //加锁
  if (mysql_query(mysql, selectQuery.c_str())) {
    std::cerr << "Query execution failed: " << mysql_error(mysql) << std::endl;
    LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();
    return DELETE_REQUEST_61;
  }
  MYSQL_RES *res;
  MYSQL_ROW row;
  res = mysql_store_result(mysql);

  string selectedMd5;
  string selectedSize;
  if ((row = mysql_fetch_row(res))) {
    selectedMd5 = row[0];
    selectedSize = row[1];
  }
  mysql_free_result(res);

  string selectCounterQuery = "SELECT counter FROM md5_size WHERE md5 = '" +
                              selectedMd5 + "' AND size = '" + selectedSize +
                              "'";
  if (mysql_query(mysql, selectCounterQuery.c_str())) {
    std::cerr << "Query execution failed: " << mysql_error(mysql) << std::endl;
    LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();  //解锁
    return DELETE_REQUEST_61;
  }
  res = mysql_store_result(mysql);
  int tmpCounter = 0;
  if ((row = mysql_fetch_row(res))) {
    tmpCounter = atoi(row[0]);
  }
  mysql_free_result(res);
  if (tmpCounter == 1) {
    string queryDelete = "DELETE FROM md5_size WHERE md5 = '" + selectedMd5 +
                         "' AND size = '" + selectedSize + "'";
    if (mysql_query(mysql, queryDelete.c_str())) {
      std::cerr << "Query execution failed: " << mysql_error(mysql)
                << std::endl;
      LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
      m_connection_pool->ReleaseConnection(mysql);
      m_locker.unlock();  //解锁
      return DELETE_REQUEST_61;
    }
    ///删除文件
  } else {
    string queryDecrease =
        "UPDATE md5_size SET counter = counter - 1 WHERE md5 = '" +
        selectedMd5 + "' AND size = '" + selectedSize + "'";
    if (mysql_query(mysql, queryDecrease.c_str())) {
      std::cerr << "Query execution failed: " << mysql_error(mysql)
                << std::endl;
      LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
      m_connection_pool->ReleaseConnection(mysql);
      m_locker.unlock();  //解锁
      return DELETE_REQUEST_61;
    }
  }
  string query = "DELETE FROM file_info WHERE user_name = '" +
                 downloadUserName + "' AND file_name = '" + downloadFileName +
                 "'";
  if (mysql_query(mysql, query.c_str())) {
    std::cerr << "Query execution failed: " << mysql_error(mysql) << std::endl;
    LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();
    return DELETE_REQUEST_61;
  }
  m_locker.unlock();  //解锁
  m_connection_pool->ReleaseConnection(mysql);
  return DELETE_REQUEST_60;
}

http_conn::HTTP_CODE http_conn::do_rename() {
  string input = (string)m_string;

  std::stringstream ss(input);
  std::map<std::string, std::string> valueMap;

  std::string keyValue;
  while (std::getline(ss, keyValue, '&')) {
    std::stringstream keyValueStream(keyValue);
    std::string key, value;
    std::getline(keyValueStream, key, '=');
    std::getline(keyValueStream, value);

    valueMap[key] = value;
  }

  std::string username = valueMap["username"];
  std::string oldFileName = valueMap["old_file_name"];
  std::string newFileName = valueMap["new_file_name"];
  oldFileName = urlDecode(oldFileName);
  newFileName = urlDecode(newFileName);
  std::cout << "Username: " << username << std::endl;
  std::cout << "Old File Name: " << oldFileName << std::endl;
  std::cout << "New File Name: " << newFileName << std::endl;
  connectionRAII mysqlcol(&mysql, m_connection_pool);
  string query = "UPDATE file_info SET file_name = '" + newFileName +
                 "' WHERE user_name = '" + username + "' AND file_name = '" +
                 oldFileName + "'";
  m_locker.lock();
  if (mysql_query(mysql, query.c_str())) {
    std::cerr << "Query execution failed: " << mysql_error(mysql) << std::endl;
    LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
    m_locker.unlock();  //解锁
    m_connection_pool->ReleaseConnection(mysql);
    return RENAME_REQUEST_71;
  }
  query = "UPDATE file_share_table SET file_name = '" + newFileName +
          "' WHERE user_name = '" + username + "' AND file_name = '" +
          oldFileName + "'";
  if (mysql_query(mysql, query.c_str())) {
    std::cerr << "Query execution failed: " << mysql_error(mysql) << std::endl;
    LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
    m_locker.unlock();  //解锁
    m_connection_pool->ReleaseConnection(mysql);
    return RENAME_REQUEST_71;
  }
  m_locker.unlock();
  m_connection_pool->ReleaseConnection(mysql);
  return RENAME_REQUEST_70;
}

http_conn::HTTP_CODE http_conn::do_share() {
  cout << "share m_string: " << (string)m_string << endl;
  /*
    CREATE TABLE file_share_table (
      id INT AUTO_INCREMENT PRIMARY KEY,
      uuid VARCHAR(36),
      user_name VARCHAR(20),
      file_name VARCHAR(255),
      keep_time INT
    );
    ALTER TABLE file_share_table
    ADD COLUMN created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP;
  */
  Json::Value root;
  Json::Reader reader;

  bool parsingSuccessful = reader.parse((string)m_string, root);
  if (parsingSuccessful) {
    connectionRAII mysqlcol(&mysql, m_connection_pool);
    m_locker.lock();
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
      std::cerr << "mysql_stmt_init() failed\n";
      m_locker.unlock();
      m_connection_pool->ReleaseConnection(mysql);
      return SHARE_REQUEST_81;
    }
    string username = root.get("username", "").asString();
    int keepTime = root.get("keep_time", 0).asInt();
    uuid_t uuid;

    uuid_generate(uuid);
    uuid_unparse(uuid, m_uuidString);
    const Json::Value shareFilesList = root["shareFilesList"];
    if (shareFilesList.isArray()) {
      for (unsigned int i = 0; i < shareFilesList.size(); ++i) {
        // 使用参数化查询
        string fileName = shareFilesList[i].asString();
        cout << "shared File: " << fileName << endl;
        const char *query =
            "INSERT INTO file_share_table (uuid, user_name, file_name, "
            "keep_time) VALUES (?, ?, ?, ?)";
        if (mysql_stmt_prepare(stmt, query, strlen(query))) {
          std::cerr << "mysql_stmt_prepare() failed\n";
          m_locker.unlock();
          m_connection_pool->ReleaseConnection(mysql);
          return SHARE_REQUEST_81;
        }

        MYSQL_BIND bind[4];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;  // uuid
        bind[0].buffer = (void *)m_uuidString;
        bind[0].buffer_length = strlen(m_uuidString);

        bind[1].buffer_type = MYSQL_TYPE_STRING;  // user_name
        bind[1].buffer = (void *)username.c_str();
        bind[1].buffer_length = username.length();

        bind[2].buffer_type = MYSQL_TYPE_STRING;  // file_name
        bind[2].buffer = (void *)fileName.c_str();
        bind[2].buffer_length = fileName.length();

        bind[3].buffer_type = MYSQL_TYPE_LONG;  // keep_time
        bind[3].buffer = (void *)&keepTime;

        if (mysql_stmt_bind_param(stmt, bind)) {
          std::cerr << "mysql_stmt_bind_param() failed\n";
          m_locker.unlock();
          m_connection_pool->ReleaseConnection(mysql);
          return SHARE_REQUEST_81;
        }

        if (mysql_stmt_execute(stmt)) {
          std::cerr << "mysql_stmt_execute() failed\n";
          m_locker.unlock();
          m_connection_pool->ReleaseConnection(mysql);
          return SHARE_REQUEST_81;
        }
      }
      mysql_stmt_close(stmt);
      m_locker.unlock();
      m_connection_pool->ReleaseConnection(mysql);
    }
    return SHARE_REQUEST_80;
  } else {
    std::cerr << " Failed to parse Json" << endl;
    return SHARE_REQUEST_81;
  }
}

http_conn::HTTP_CODE http_conn::do_download_sharing() {
  string uuid = (string)(strstr(m_string, "=") + 1);
  cout << "uuid: " << uuid << endl;
  connectionRAII mysqlcon(&mysql, m_connection_pool);
  m_locker.lock();
  MYSQL_RES *res;
  MYSQL_ROW row;
  if (mysql == NULL) {
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();
    return DOWNLOADSHARING_REQUEST_91;
  }
  string query =
      "SELECT user_name, file_name FROM file_share_table WHERE uuid='" + uuid +
      "'";
  if (mysql_query(mysql, query.c_str()) != 0) {
    LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    std::cerr << "Error querying database: " << mysql_error(mysql) << std::endl;
    m_locker.unlock();
    m_connection_pool->ReleaseConnection(mysql);
    return DOWNLOADSHARING_REQUEST_91;
  }
  res = mysql_store_result(mysql);
  m_locker.unlock();
  Json::Value root;
  Json::Value files(Json::arrayValue);
  while ((row = mysql_fetch_row(res))) {
    Json::Value file;
    file["file_name"] = row[1];
    files.append(file);
    root["username"] = row[0];
    cout << "share_filename: " << row[1] << " username: " << row[0] << endl;
  }
  if (files.size() <= 0) {
    mysql_free_result(res);
    m_connection_pool->ReleaseConnection(mysql);
    m_linger = true;
    return DOWNLOADSHARING_REQUEST_91;
  }
  root["CODE"] = 90;
  root["files"] = files;
  Json::StreamWriterBuilder builder;
  m_allFilesJson = Json::writeString(builder, root);

  mysql_free_result(res);
  m_connection_pool->ReleaseConnection(mysql);
  m_linger = true;
  return DOWNLOADSHARING_REQUEST_90;
}

http_conn::HTTP_CODE http_conn::do_request_upload() {
  /*
  CREATE TABLE file_transfer (
  id INT AUTO_INCREMENT PRIMARY KEY,
  user_name VARCHAR(50) NOT NULL,
  file_name VARCHAR(100) NOT NULL,
  file_size INT NOT NULL,
  transferred_size INT DEFAULT 0,
  file_status ENUM('uploading', 'downloading', 'completed') NOT NULL
);
*/
  //客户端发送md5、size询问这个文件是否需要在服务器存储
  string md5;
  int size;
  string username;
  string filename;
  Json::Value jsonData;
  Json::CharReaderBuilder builder;
  Json::CharReader *reader = builder.newCharReader();
  JSONCPP_STRING errs;

  if (reader->parse(m_string, m_string + strlen(m_string), &jsonData, &errs)) {
    md5 = jsonData["md5"].asString();
    size = jsonData["size"].asInt();
    username = jsonData["username"].asString();
    filename = jsonData["file_name"].asString();
    filename = urlDecode(filename);
    std::cout << "MD5: " << md5 << std::endl;
    std::cout << "Size: " << size << std::endl;
  } else {
    std::cerr << "Failed to parse JSON: " << errs << std::endl;
  }

  delete reader;
  LOG_INFO("ask file: md5: %s, size: %d", md5.c_str(), size);

  connectionRAII mysqlcon(&mysql, m_connection_pool);
  if (mysql == NULL) {
    return INTERNAL_ERROR;
  }
  /*
          CREATE TABLE md5_size (
          id INT AUTO_INCREMENT PRIMARY KEY,
          md5 VARCHAR(32) NOT NULL,
          size INT NOT NULL
      );
  */

  string query = "SELECT * FROM md5_size WHERE md5='" + md5 +
                 "' AND size=" + std::to_string(size);
  m_locker.lock();  //加锁
  if (mysql_query(mysql, query.c_str()) != 0) {
    LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    fprintf(stderr, "查询数据时出错: %s\n", mysql_error(mysql));
    m_locker.unlock();  //解锁
    return ASK_FILE_EXIST_31;
  }
  MYSQL_RES *result = mysql_store_result(mysql);
  if (result == NULL) {
    LOG_ERROR("无法获取查询结果: %s\n", mysql_error(mysql));
    mysql_free_result(result);
    fprintf(stderr, "无法获取查询结果: %s\n", mysql_error(mysql));
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();  //解锁
    return ASK_FILE_EXIST_31;
  }
  MYSQL_ROW row;
  if ((row = mysql_fetch_row(result)) != NULL) {
    LOG_INFO("ask file: md5: %s, size: %d is existed", md5.c_str(), size);
    std::cout << "Record found in the database for md5: " << md5
              << " and size: " << size << std::endl;
    mysql_free_result(result);  // 释放未成功获取结果集时分配的资源
    std::string query =
        "UPDATE md5_size SET counter = counter + 1 WHERE md5 = '" + md5 +
        "' AND size = " + std::to_string(size);
    if (mysql_query(mysql, query.c_str()) != 0) {
      LOG_ERROR("UPDATE error:%s\n", mysql_error(mysql));
      fprintf(stderr, "更新数据时出错: %s\n", mysql_error(mysql));
      m_connection_pool->ReleaseConnection(mysql);
      m_locker.unlock();  //解锁
      return ASK_FILE_EXIST_31;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    const char *queryFileInfo =
        "INSERT INTO file_info (user_name, file_name, md5, "
        "size) VALUES (?, ?, ?, ?)";
    if (mysql_stmt_prepare(stmt, queryFileInfo, strlen(queryFileInfo))) {
      std::cerr << "mysql_stmt_prepare() failed\n";
      m_locker.unlock();
      m_connection_pool->ReleaseConnection(mysql);
      return ASK_FILE_EXIST_31;
    }
    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;  // user_name
    bind[0].buffer = (void *)username.c_str();
    bind[0].buffer_length = username.length();

    bind[1].buffer_type = MYSQL_TYPE_STRING;  // file_name
    bind[1].buffer = (void *)filename.c_str();
    bind[1].buffer_length = filename.length();

    bind[2].buffer_type = MYSQL_TYPE_STRING;  // md5
    bind[2].buffer = (void *)md5.c_str();
    bind[2].buffer_length = md5.length();

    bind[3].buffer_type = MYSQL_TYPE_LONG;  // size
    bind[3].buffer = (void *)&size;

    if (mysql_stmt_bind_param(stmt, bind)) {
      std::cerr << "mysql_stmt_bind_param() failed\n";
      m_locker.unlock();
      m_connection_pool->ReleaseConnection(mysql);
      return ASK_FILE_EXIST_31;
    }

    if (mysql_stmt_execute(stmt)) {
      std::cerr << "mysql_stmt_execute() failed\n";
      m_locker.unlock();
      m_connection_pool->ReleaseConnection(mysql);
      return ASK_FILE_EXIST_31;
    }
    mysql_stmt_close(stmt);
    m_locker.unlock();  //解锁
    m_connection_pool->ReleaseConnection(mysql);
    return ASK_FILE_EXIST_30;
  } else {
    LOG_INFO("ask file: md5: %s, size: %d is not existed", md5.c_str(), size);
    std::cout << "Record not found in the database for md5: " << md5
              << " and size: " << size << std::endl;
    mysql_free_result(result);  // 释放未成功获取结果集时分配的资源
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();  //解锁
    return ASK_FILE_EXIST_31;
  }
}

http_conn::HTTP_CODE http_conn::do_upload() {
  // printf("%s client upload file\n", locationEnd);
  //  cout << "**********************************************************\n";
  //   30位 3345 0425 0724 7164 7520 2816 9270 24
  //   29位 ---- ---- ---- ---- ---- ---- ---- -

  /*
          --boundary\r\n
          Content-Disposition: form-data; name="key1"\r\n
          \r\n
          value1\r\n
          --boundary\r\n
          Content-Disposition: form-data; name="key2"\r\n
          \r\n
          value2\r\n
          --boundary--\r\n
  */
  string filePath = "./user_file/";
  std::string filename, username, md5;
  int size;
  int savechangedaction = 0;
  m_string += m_boundary.length() + 3;  //首部--和结尾 /r/n；切换到第二行首部

  std::stringstream ss((string)m_string);

  std::string line;
  bool isFileContent = false;  // 标记是否开始处理文件内容
  std::string bodyContent;     // 保存文件内容部分

  while (std::getline(ss, line)) {
    m_string += line.length() + 1;
    if (line.find("Content-Disposition: form-data;") != string::npos) {
      line = line.substr(32);
      std::istringstream iss(line);
      std::string token;
      while (std::getline(iss, token, ';')) {
        if (token.find("filename") != std::string::npos) {
          filename =
              token.substr(token.find("\"") + 1,
                           token.find_last_of("\"") - token.find("\"") - 1);
          filename = urlDecode(filename);
        } else if (token.find("username") != std::string::npos) {
          username =
              token.substr(token.find("\"") + 1,
                           token.find_last_of("\"") - token.find("\"") - 1);
        } else if (token.find("size") != std::string::npos) {
          size = std::stoi(token.substr(token.find("=") + 1));
        } else if (token.find("md5") != std::string::npos) {
          md5 = token.substr(token.find("\"") + 1,
                             token.find_last_of("\"") - token.find("\"") - 1);
        } else if (token.find("savechangedaction") != std::string::npos) {
          savechangedaction = std::stoi(token.substr(token.find("=") + 1));
        }
      }
      std::cout << "filename: " << filename << std::endl;
      std::cout << "username: " << username << std::endl;
      std::cout << "size: " << size << std::endl;
      std::cout << "md5: " << md5 << std::endl;
      filePath += md5 + "_" + std::to_string(size);
    } else if (line.find("Content-Type:") != string::npos) {
      size_t pos = line.find(": ");
      if (pos != string::npos) {
        string contentType = line.substr(pos + 2);
      }
    } else if (line == "\r") {
      // cout << "我走出了循环落\n";
      break;
    }
  }
  bool file_err = false;
  std::ofstream file(filePath, std::ios::out | std::ios::binary);
  if (file.is_open()) {
    cout << "start write" << endl;
    file.write(m_string, size);
    cout << "end write" << endl;
    file.close();
    std::cout << "File saved: " << filename << std::endl;
  } else {
    std::cerr << "Unable to open file for writing" << std::endl;
  }

  if (file_err) {
    m_linger = true;
    return UPLOAD_REQUEST_41;

  } else {
    /*
    CREATE TABLE file_info (
    user_name VARCHAR(20),
    file_name VARCHAR(255),
    md5 VARCHAR(32),
    size INT
    ); */
    connectionRAII mysqlcon(&mysql, m_connection_pool);
    if (mysql == NULL) {
      LOG_ERROR("mysqlcon(&mysql, m_connection_poop) failed");
      m_connection_pool->ReleaseConnection(mysql);
      return UPLOAD_REQUEST_41;
    }
    if (savechangedaction == 1) {  //处理用户上传的更改的文件信息
      string query = "SELECT md5, size FROM file_info WHERE user_name = '" +
                     username + "' AND file_name = '" + filename + "'";
      m_locker.lock();  //加锁
      if (mysql_query(mysql, query.c_str()) != 0) {
        std::cerr << "Error inserting data into database: "
                  << mysql_error(mysql) << std::endl;
        LOG_ERROR("mysql_query(mysql, query.c_str()) failed");
        m_connection_pool->ReleaseConnection(mysql);
        m_locker.unlock();  //解锁
        return UPLOAD_REQUEST_41;
      }
      MYSQL_RES *res;
      MYSQL_ROW row;
      res = mysql_store_result(mysql);

      string selectedMd5;
      string selectedSize;
      if ((row = mysql_fetch_row(res))) {
        selectedMd5 = row[0];
        selectedSize = row[1];
      }
      mysql_free_result(res);

      query = "UPDATE md5_size SET md5 = '" + md5 + "', size = '" +
              std::to_string(size) + "' WHERE md5 = '" + selectedMd5 +
              "' AND size = '" + selectedSize + "'";
      if (mysql_query(mysql, query.c_str()) != 0) {
        std::cerr << "Error UPDATE md5_size SET md5 into database: "
                  << mysql_error(mysql) << std::endl;
        LOG_ERROR("mysql_query(mysql, query.c_str()) failed");
        m_connection_pool->ReleaseConnection(mysql);
        m_locker.unlock();  //解锁
        return UPLOAD_REQUEST_41;
      }

      query = "UPDATE file_info SET md5 = '" + md5 + "', size = '" +
              std::to_string(size) + "' WHERE user_name = '" + username +
              "' AND file_name = '" + filename + "'";
      if (mysql_query(mysql, query.c_str()) != 0) {
        std::cerr << "UPDATE file_info SET md5 = into database: "
                  << mysql_error(mysql) << std::endl;
        LOG_ERROR("mysql_query(mysql, query.c_str()) failed");
        m_connection_pool->ReleaseConnection(mysql);
        m_locker.unlock();  //解锁
        return UPLOAD_REQUEST_41;
      }
    } else {
      string query = "INSERT INTO md5_size (md5, size) VALUES ('" + md5 +
                     "', " + std::to_string(size) + ")";
      m_locker.lock();  //加锁
      if (mysql_query(mysql, query.c_str()) != 0) {
        std::cerr << "INSERT INTO md5_size (md5, size) data into database: "
                  << mysql_error(mysql) << std::endl;
        LOG_ERROR("mysql_query(mysql, query.c_str()) failed");
        m_connection_pool->ReleaseConnection(mysql);
        m_locker.unlock();  //解锁
        return UPLOAD_REQUEST_41;
      }

      query =
          "INSERT INTO file_info (user_name, file_name, md5, size) VALUES ('" +
          username + "', '" + filename + "', '" + md5 + "', " +
          std::to_string(size) + ")";
      if (mysql_query(mysql, query.c_str()) != 0) {
        std::cerr << "INSERT INTO file_info data into database: "
                  << mysql_error(mysql) << std::endl;
        LOG_ERROR("mysql_query(mysql, query.c_str()) failed");
        m_connection_pool->ReleaseConnection(mysql);
        m_locker.unlock();  //解锁
        return UPLOAD_REQUEST_41;
      }
    }

    m_locker.unlock();  //解锁
    m_connection_pool->ReleaseConnection(mysql);
  }

  m_linger = true;
  return UPLOAD_REQUEST_40;
}

http_conn::HTTP_CODE http_conn::do_online_view_file(const char *locationEnd) {
  cout << (string)(locationEnd + 1) << endl;
  string usernameAndFilename =
      (string)(locationEnd + strlen("onlineviewfile") + 1);
  // std::string usernameAndFilename = "username=chen&filename=zhuzhu";

  // 查找用户名
  size_t usernameStart = usernameAndFilename.find("username=");
  size_t usernameEnd = usernameAndFilename.find("&", usernameStart);
  std::string username = usernameAndFilename.substr(
      usernameStart + 9, usernameEnd - usernameStart - 9);

  // 查找文件名
  size_t filenameStart = usernameAndFilename.find("filename=");
  std::string filename = usernameAndFilename.substr(filenameStart + 9);
  username = urlDecode(username);
  filename = urlDecode(filename);
  size_t dotIndex = filename.find_last_of('.');
  if (dotIndex != string::npos) {
    string extension = filename.substr(dotIndex + 1);
    get_file_type(extension);
  }
  cout << username << "   " << filename << endl;
  string md5_size;
  connectionRAII mysqlcol(&mysql, m_connection_pool);
  if (mysql == NULL) {
    LOG_ERROR("mysqlcon(&mysql, m_connection_poop) failed");
    m_connection_pool->ReleaseConnection(mysql);
    return INTERNAL_ERROR;
  }
  std::string query = "SELECT md5, size FROM file_info WHERE user_name = '" +
                      username + "' AND file_name = '" + filename + "'";
  m_locker.lock();  //加锁
  if (mysql_query(mysql, query.c_str())) {
    std::cerr << "Query execution failed: " << mysql_error(mysql) << std::endl;
    LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();              //解锁
    return ONLINEVIEW_REQUEST_101;  
  }
  MYSQL_RES *result = mysql_store_result(mysql);
  m_locker.unlock();  //解锁
  if (result == NULL) {
    std::cerr << "No result set returned" << std::endl;
    LOG_ERROR("No result set returned");
    mysql_free_result(result);
    m_connection_pool->ReleaseConnection(mysql);
    return ONLINEVIEW_REQUEST_101;
  }
  // int num_fields = mysql_num_fields(result);
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    md5_size = (string)row[0] + "_" + row[1];
  }
  mysql_free_result(result);
  m_connection_pool->ReleaseConnection(mysql);
  std::cout << "md5_size: " << md5_size << std::endl;
  LOG_INFO("download filename: %s, username: %s, md5_size: %s",
           filename.c_str(), username.c_str(), md5_size.c_str());
  string realFilePath = (string) "./user_file/" + md5_size;
  if (stat(realFilePath.c_str(), &m_file_stat) < 0) {
    return ONLINEVIEW_REQUEST_101;
  }
  if (!(m_file_stat.st_mode & S_IROTH)) {
    return ONLINEVIEW_REQUEST_101;
  }
  if (S_ISDIR(m_file_stat.st_mode)) {
    return ONLINEVIEW_REQUEST_101;
  }
  int fd = open(realFilePath.c_str(), O_RDONLY);
  if (fd == -1) {
    return ONLINEVIEW_REQUEST_101;
  }
  m_file_address =
      (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);

  return ONLINEVIEW_REQUEST_100;
}

http_conn::HTTP_CODE http_conn::do_download() {
  // const char* downloadUserNameAndFileName = strstr(locationEnd + 1, "/");
  std::cout << "downloadUserNameAndFileName:" << (string)(m_string)
            << std::endl;
  char tmpdownloadUserName[20] = {0};
  char tmpdownloadFileName[255] = {0};
  string downloadFileName;
  string downloadUserName;
  std::string md5_size;
  const char *pattern = "username=([^&]*)&file_name=([^&]*)";
  pcre *re = NULL;
  const char *error = NULL;
  int erroffset;
  int ovector[30];

  re = pcre_compile(pattern, 0, &error, &erroffset, NULL);

  if (re == NULL) {
    LOG_ERROR("PCRE compilaction failed at offset %d: %s\n", erroffset, error);
    printf("PCRE compilaction failed at offset %d: %s\n", erroffset, error);
    return BAD_REQUEST;
  }

  int rc = pcre_exec(re, NULL, m_string, strlen(m_string), 0, 0, ovector, 30);

  if (rc < 0) {
    if (rc == PCRE_ERROR_NOMATCH) {
      printf("PCRE No match\n");
      LOG_INFO("PCRE No match:%s\n", m_string);
    } else {
      printf("PCRE matching error: %d\n", rc);
      LOG_ERROR("PCRE matching error: %d\n", rc);
    }
    pcre_free(re);
  }

  for (int i = 1; i < rc; i++) {
    int start = ovector[2 * i];
    int end = ovector[2 * i + 1];
    int length = end - start;

    if (i == 1) {  // "user" capture group
      strncpy(tmpdownloadUserName, m_string + start, length);
      tmpdownloadUserName[length] = '\0';
      downloadUserName = urlDecode((string)tmpdownloadUserName);
      LOG_INFO("Match %d: %s\n", i, downloadUserName.c_str());
      printf("Match %d: %s\n", i, downloadUserName.c_str());
    } else if (i == 2) {  // "passwd" capture group
      strncpy(tmpdownloadFileName, m_string + start, length);
      tmpdownloadFileName[length] = '\0';
      downloadFileName = urlDecode((string)tmpdownloadFileName);
      LOG_INFO("Match %d: %s\n", i, downloadFileName.c_str());
      printf("Match %d: %s\n", i, downloadFileName.c_str());
    }
  }

  std::cout << "username: " << downloadUserName
            << "filename: " << downloadFileName << std::endl;
  connectionRAII mysqlcol(&mysql, m_connection_pool);
  if (mysql == NULL) {
    LOG_ERROR("mysqlcon(&mysql, m_connection_poop) failed");
    m_connection_pool->ReleaseConnection(mysql);
    return INTERNAL_ERROR;
  }

  std::string query = "SELECT md5, size FROM file_info WHERE user_name = '" +
                      downloadUserName + "' AND file_name = '" +
                      downloadFileName + "'";
  m_locker.lock();  //加锁
  if (mysql_query(mysql, query.c_str())) {
    std::cerr << "Query execution failed: " << mysql_error(mysql) << std::endl;
    LOG_ERROR("Query execution failed: %s", mysql_error(mysql));
    m_connection_pool->ReleaseConnection(mysql);
    m_locker.unlock();  //解锁
    return DOWNLOAD_REQUEST_51;
  }
  MYSQL_RES *result = mysql_store_result(mysql);
  m_locker.unlock();  //解锁
  if (result == NULL) {
    std::cerr << "No result set returned" << std::endl;
    LOG_ERROR("No result set returned");
    mysql_free_result(result);
    m_connection_pool->ReleaseConnection(mysql);
    return DOWNLOAD_REQUEST_51;
  }
  // int num_fields = mysql_num_fields(result);
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    md5_size = (string)row[0] + "_" + row[1];
  }
  mysql_free_result(result);
  m_connection_pool->ReleaseConnection(mysql);
  std::cout << "md5_size: " << md5_size << std::endl;
  LOG_INFO("download filename: %s, username: %s, md5_size: %s",
           downloadFileName.c_str(), downloadUserName.c_str(),
           md5_size.c_str());
  std::string realFilePath = (string) "./user_file/" + md5_size;
  size_t dotIndex = downloadFileName.find_last_of('.');
  if (dotIndex != string::npos) {
    string extension = downloadFileName.substr(dotIndex + 1);
    get_file_type(extension);
  }
  if (stat(realFilePath.c_str(), &m_file_stat) < 0) {
    return DOWNLOAD_REQUEST_51;
  }
  if (!(m_file_stat.st_mode & S_IROTH)) {
    return DOWNLOAD_REQUEST_51;
  }
  if (S_ISDIR(m_file_stat.st_mode)) {
    return DOWNLOAD_REQUEST_51;
  }
  int fd = open(realFilePath.c_str(), O_RDONLY);
  if (fd == -1) {
    return DOWNLOAD_REQUEST_51;
  }
  m_file_address =
      (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return DOWNLOAD_REQUEST_50;
}
// 对内存映射区执行munmap操作
void http_conn::unmap() {
  if (m_file_address) {
    munmap(m_file_address, m_file_stat.st_size);
    m_file_address = 0;
  }
}

// 写HTTP响应
bool http_conn::write() {
  int temp = 0;
  if (bytes_to_send == 0) {
    // 将要发送的字节为0.这一次响应结束
    modfd(m_epollfd, m_sockfd, EPOLLIN);
    init();
    return true;
  }
  //    if(!text){
  while (1) {
    // 分散写
    temp = writev(m_sockfd, m_iv, m_iv_count);
    if (temp <= -1) {
      // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间
      // 服务器无法立即接收到统一客户的下一个请求，但可以保证连接的完整性
      if (errno == EAGAIN) {
        modfd(m_epollfd, m_sockfd, EPOLLOUT);
        return true;
      }
      unmap();
      return false;
    }

    bytes_have_send += temp;
    bytes_to_send -= temp;

    if (bytes_have_send >= m_iv[0].iov_len) {
      m_iv[0].iov_len = 0;
      m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
      m_iv[1].iov_len = bytes_to_send;
    } else {
      m_iv[0].iov_base = m_write_buf + bytes_have_send;
      m_iv[0].iov_len = m_iv[0].iov_len - temp;
    }
    printf("write\n:%s", m_write_buf);
    if (bytes_to_send <= 0) {
      cout << " 没有数据要发送了\n";
      unmap();
      modfd(m_epollfd, m_sockfd, EPOLLIN);

      if (m_linger) {
        init();
        return true;
      } else {
        init();
        return false;
      }
    }
  }
}

// 往写缓冲写入待发送的数据
bool http_conn::add_response(const char *format, ...) {
  if (m_write_idx >= WRITE_BUFFER_SIZE) {
    return false;
  }
  va_list arg_list;
  va_start(arg_list, format);
  int len = vsnprintf(m_write_buf + m_write_idx,
                      WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
  if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)) {
    return false;
  }
  m_write_idx += len;
  va_end(arg_list);

  return true;
}

bool http_conn::add_status_line(int status, const char *title) {
  return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_length) {
  add_content_length(content_length);
  add_content_type();
  // if (!text) {
  //   add_content_type();
  // } else {
  //   add_content_text_plain();  // 下载文本文件
  //   text = false;
  // }

  add_linger();
  add_blank_line();

  return true;
}

bool http_conn::add_content_length(int content_length) {
  return add_response("Content-Length: %d\r\n", content_length);
}

bool http_conn::add_content_type() {
  switch (m_file_type) {
    case IMAGE_JPG:
      return add_response("Content-Type:%s\r\n", "image/jpg");
    case IMAGE_JPEG:
      return add_response("Content-Type:%s\r\n", "image/jpeg");
    case IMAGE_GIF:
      return add_response("Content-Type:%s\r\n", "image/gif");
    case IMAGE_PNG:
      return add_response("Content-Type:%s\r\n", "image/png");
    case APNG:
      return add_response("Content-Type:%s\r\n", "image/apng");
    case AVIF:
      return add_response("Content-Type:%s\r\n", "image/avif");
    case BMP:
      return add_response("Content-Type:%s\r\n", "image/bmp");
    case SVG:
      return add_response("Content-Type:%s\r\n", "image/svg+xml");
    case WEBP:
      return add_response("Content-Type:%s\r\n", "image/webp");
    case ICO:
      return add_response("Content-Type:%s\r\n", "image/x-icon");
    case TIF:
      return add_response("Content-Type:%s\r\n", "image/tiff");
    case TIFF:
      return add_response("Content-Type:%s\r\n", "image/tiff");
    case VIDEO_MP4:
      return add_response("Content-Type:%s\r\n", "video/mp4");
    case WEBM:
      return add_response("Content-Type:%s\r\n", "video/webm");
    case MPEG:
      return add_response("Content-Type:%s\r\n", "video/mpeg");
    case AUDIO_WAV:
      return add_response("Content-Type:%s\r\n", "audio/wave");
    case MP3:
      return add_response("Content-Type:%s\r\n", "audio/mp3");
    case MPGA:
      return add_response("Content-Type:%s\r\n", "audio/mpeg");
    case WEBA:
      return add_response("Content-Type:%s\r\n", "audio/webm");
    case JSON:
      return add_response("Content-Type:%s\r\n", "application/json");
    case ZIP:
      return add_response("Content-Type:%s\r\n", "application/zip");
    case TAR:
      return add_response("Content-Type:%s\r\n", "application/x-tar");
    case PDF:
      return add_response("Content-Type:%s\r\n", "application/pdf");
    case HTML:
      return add_response("Content-Type:%s\r\n", "text/html");
    case TXT:
      return add_response("Content-Type:%s\r\n", "text/plain");
  };

  return add_response("Content-Type:%s\r\n", "application/json");
}

// bool http_conn::add_content_text_plain() {
//   return add_response("Content-Type:%s\r\n", "multipart/form-data");
//   /*


//       application/json：如果你的服务器返回一个 JSON 格式的响应，这是最常用的
//      Content-Type。你可以在响应中包含用户的身份验证令牌、用户信息或其他相关数据。前端可以方便地解析
//      JSON 数据并进行相应的处理。

//       text/html：如果你想返回一个 HTML 页面作为响应，可以使用该
//      Content-Type。在登录成功后，你可以将用户重定向到一个特定的 HTML
//      页面，展示欢迎信息或其他相关内容。

//       application/xml：如果你的服务器返回 XML 格式的响应，你可以选择使用该
//      Content-Type。这在某些特定的场景中可能有用，比如与其他系统进行数据交换。

//       text/plain：如果你只需要返回简单的文本响应，可以使用该
//      Content-Type。例如，你可以返回一个简单的成功消息或错误消息。

//   */
// }

bool http_conn::add_linger() {
  return add_response("Connection: %s\r\n",
                      (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line() { return add_response("%s", "\r\n"); }

bool http_conn::add_content(const char *content) {
  return add_response("%s", content);
}
// 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool http_conn::process_write(HTTP_CODE ret) {
  switch (ret) {
    case INTERNAL_ERROR:
      add_status_line(500, error_500_title);
      add_headers(strlen(error_500_form));
      if (!add_content(error_500_form)) {
        return false;
      }
      break;

    case BAD_REQUEST:
      add_status_line(400, error_400_title);
      add_headers(strlen(error_400_form));
      if (!add_content(error_400_form)) {
        return false;
      }
      break;

    case NO_RESOURCE:
      add_status_line(404, error_404_title);
      add_headers(strlen(error_404_form));
      if (!add_content(error_404_form)) {
        return false;
      }
      break;

    case FORBIDDEN_REQUEST:
      add_status_line(403, error_403_title);
      add_headers(strlen(error_403_form));
      if (!add_content(error_403_form)) {
        return false;
      }
      break;

    case FILE_REQUEST:
      add_status_line(200, ok_200_title);
      if (m_file_stat.st_size != 0) {
        add_headers(m_file_stat.st_size);
        /*


        m_iv[0].iov_base = m_write_buf;
        这行代码将一个缓冲区的起始地址 m_write_buf 赋值给了第一个 iovec 结构体的
        iov_base 成员。 iovec 结构体用于描述数据缓冲区的地址和长度。

        m_iv[0].iov_len = m_write_idx;
        这行代码将缓冲区的长度 m_write_idx 赋值给了第一个 iovec 结构体的 iov_len
        成员。 这表示了要发送的数据的长度。

        m_iv[1].iov_base = m_file_address;
        这行代码将文件映射到内存后的地址 m_file_address 赋值给了第二个 iovec
        结构体的 iov_base 成员。 这意味着我们希望发送这块内存中的数据。

        m_iv[1].iov_len = m_file_stat.st_size;
        这行代码将文件的大小 m_file_stat.st_size 赋值给了第二个 iovec 结构体的
        iov_len 成员。 这表示了要发送的文件数据的长度。

        m_iv_count = 2;
        这行代码设置了 iovec 数组中元素的数量，这里是2，表示有两个 iovec
        结构体。 每个结构体描述了一个要发送的数据块。

        bytes_to_send = m_write_idx + m_file_stat.st_size;
        这行代码计算了要发送的总字节数，包括了缓冲区中的数据字节数和文件数据的字节数之和。
        这个值通常用于进一步的处理，比如发送数据的循环发送直到全部数据都被发送完毕为止。

        综合来说，这段代码将一个缓冲区和一个文件映射到内存后的地址都描述成了
        iovec 结构体，方便后续的数据发送操作。 iovec 结构体常用于类似 writev()
        这样的系统调用，用于一次性发送多个不相邻的数据块。
        */
        m_iv[0].iov_base = m_write_buf;
        m_iv[0].iov_len = m_write_idx;
        m_iv[1].iov_base = m_file_address;
        m_iv[1].iov_len = m_file_stat.st_size;
        m_iv_count = 2;

        bytes_to_send = m_write_idx + m_file_stat.st_size;

        // printf("%s\n",m_file_address);
        return true;
      } else {
        const char *ok_string = "<html><body></body></html>";
        add_headers(strlen(ok_string));
        if (!add_content(ok_string)) {
          return false;
        }
        bytes_to_send = m_write_idx + strlen(ok_string);
      }

    case REGISTER_REQUEST_10: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":10, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }
    case REGISTER_REQUEST_11: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":11, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case LOGIN_REQUEST_20: {
      add_status_line(200, ok_200_title);
      add_headers(m_allFilesJson.length());
      if (!add_content(m_allFilesJson.c_str())) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case LOGIN_REQUEST_21: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":21, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case ASK_FILE_EXIST_30: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":30, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }
    case ASK_FILE_EXIST_31: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":31, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case UPLOAD_REQUEST_40: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":40, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case UPLOAD_REQUEST_41: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":41, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }
    case DOWNLOAD_REQUEST_50: {
      if (m_file_stat.st_size != 0) {
        add_status_line(200, ok_200_title);
        add_headers(m_file_stat.st_size);
        m_iv[0].iov_base = m_write_buf;
        m_iv[0].iov_len = m_write_idx;
        m_iv[1].iov_base = m_file_address;
        m_iv[1].iov_len = m_file_stat.st_size;
        m_iv_count = 2;

        bytes_to_send = m_write_idx + m_file_stat.st_size;

      } else {
        add_status_line(404, error_404_form);
        m_iv[0].iov_base = m_write_buf;
        m_iv[0].iov_len = m_write_idx;
        m_iv_count = 1;
        bytes_to_send = m_write_idx;
      }

      return true;
    }
    case DOWNLOAD_REQUEST_51: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":51, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }
    case DELETE_REQUEST_60: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":60, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case DELETE_REQUEST_61: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":61, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case RENAME_REQUEST_70: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":70, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case RENAME_REQUEST_71: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":71, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case SHARE_REQUEST_80: {
      add_status_line(200, ok_200_title);
      string jsonStr =
          "{\"CODE\":80, \"uuid\": \"" + (string)m_uuidString + "\"}";
      add_headers(jsonStr.length());
      if (!add_content(jsonStr.c_str())) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case SHARE_REQUEST_81: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":81, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case DOWNLOADSHARING_REQUEST_90: {
      add_status_line(200, ok_200_title);
      add_headers(m_allFilesJson.length());
      if (!add_content(m_allFilesJson.c_str())) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case DOWNLOADSHARING_REQUEST_91: {
      add_status_line(200, ok_200_title);
      const char *jsonStr = "{\"CODE\":91, \"zhuzhu\":\"ni\"}";
      add_headers(strlen(jsonStr));
      if (!add_content(jsonStr)) {
        cout << " add content failed" << endl;
      }
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv_count = 1;
      bytes_to_send = m_write_idx;

      return true;
    }

    case ONLINEVIEW_REQUEST_100: {
      add_status_line(200, ok_200_title);
      if (m_file_stat.st_size != 0) {
        add_headers(m_file_stat.st_size);
        m_iv[0].iov_base = m_write_buf;
        m_iv[0].iov_len = m_write_idx;
        m_iv[1].iov_base = m_file_address;
        m_iv[1].iov_len = m_file_stat.st_size;
        m_iv_count = 2;

        bytes_to_send = m_write_idx + m_file_stat.st_size;
        return true;
      } else {
        const char *ok_string = "<html><body></body></html>";
        add_headers(strlen(ok_string));
        if (!add_content(ok_string)) {
          return false;
        }
      }
    }

    case ONLINEVIEW_REQUEST_101: {
      add_status_line(200, ok_200_title);
      const char *ok_string = "<html><body></body></html>";
      add_headers(strlen(ok_string));
      if (!add_content(ok_string)) {
        return false;
      }
    }
    default:
      return false;
  }
  m_iv[0].iov_base = m_write_buf;
  m_iv[0].iov_len = m_write_idx;
  m_iv_count = 1;
  bytes_to_send = m_write_idx;

  return true;
}

// 由线程池中的工作线程调用，这是处理HTTP请求的入口函数
void http_conn::process() {
  // 解析HTTP请求
  HTTP_CODE read_ret = process_read();
  if (read_ret == NO_REQUEST) {
    modfd(m_epollfd, m_sockfd, EPOLLIN);
    return;
  }

  // 生成响应
  bool write_ret = process_write(read_ret);
  if (!write_ret) {
    close_conn();
  }
  modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

// 设置client_data的timer
void http_conn::set_client_data_timer(util_timer *timer) {
  client->timer = timer;
}

// client_data的getter
client_data *http_conn::get_client_data() { return client; }
