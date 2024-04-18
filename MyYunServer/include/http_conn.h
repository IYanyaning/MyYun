#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pcre.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <codecvt>
#include <fstream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <json/json.h>


#include "locker.h"
#include "log.h"
#include "lst_timer.h"
#include "sql_connection_pool.h"
static bool text = false;

using std::endl;
using std::string;

class http_conn {
 public:
  http_conn();
  ~http_conn() { delete[] myBuf; }

 public:
  //static const int FILENAME_LEN = 200;  //文件名的最大长度
  static const int READ_BUFFER_SIZE = 1024 * 1024 * 10;  //读缓冲区的大小10M
  static const int WRITE_BUFFER_SIZE = 1024 * 5;         //写缓冲区的大小

  // HTTP请求方法，这里只支持GET
  enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT };
  /*
      解析客户端请求时，主状态机的状态
      CHECK_STATE_REQUESTLINE:当前正在分析请求行
      CHECK_STATE_HEADER:当前正在分析头部字段
      CHECK_STATE_CONTENT:当前正在解析请求体
  */
  enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0,
    CHECK_STATE_HEADER,
    CHECK_STATE_CONTENT
  };
  enum FILE_TYPE {
    TEXT_PLAIN = 0,
    TEXT_HTML,
    APPLICATION_JSON,
    MULTIPART_FORM,
    IMAGE_JPG,
    IMAGE_JPEG,
    IMAGE_PNG,
    IMAGE_GIF,
    AUDIO_MPEG,
    AUDIO_WAV,
    VIDEO_MP4,
    VIDEO_AVI,
    CSS,
    CSV,
    HTML,
    MJS,
    TXT,
    VTT,

    APNG,
    AVIF,
    BMP,
    SVG,
    WEBP,
    ICO,
    TIF,
    TIFF,

    WEBM,  // video

    MPEG,  // video/audio
    MP3,   // audio
    MPGA,  // audio

    WEBA,

    OTF,
    TTF,
    WOFF,
    WOFF2,

    Z7,
    ATOM,
    PDF,
    JSON,
    RSS,
    TAR,
    XHT,
    XHTML,
    XSLT,
    XML,
    GZ,
    ZIP,
    WASM
  };

  /*
      服务器处理HTTP请求的可能结果，报文解析的结果
      NO_REQUEST          :   请求不完整，需要继续读取客户数据
      GET_REQUEST         :   表示获得了一个完整的客户请求
      BAD_REQUEST         :   表示客户请求语法错误
      NO_RESOURCE         :   表示服务器没有资源
      FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
      FILE_REQUEST        :   文件请求，获取文件成功
      REGISTER_REQUEST      :   表示请求注册账号,10代表注册成功，11代表注册失败
      LOGIN_REQUEST       ：   表示请求登录账号，20代表登录成功，21代表登录失败
      ASK_FILE_EXIST      :
     表示询问文件是否在服务器已存在,30代表已存在,31代表不存在 UPLOAD_REQUEST
     :表示客户端上传文件,40代表上传成功，41代表上传失败 DOWNLOAD_REQUEST
     :表示客户端请求下载资源，50表示请求下载成功，51表示请求下载失败
      DELETE_REQUEST :表示客户端请求删除文件，60表示删除成功，61表示删除失败
      RENAME_REQUEST      :表示客户端重命名文件，70表示成功，71表示失败
      SHARE_REQUEST
     :表示客户端分享指定文件，服务器返回这些文件对应uuid,流程成功返回80,失败返回81
      DOWNLOADSHARING_REQUEST
     :表示客户端下载分享的uuid标识的一些文件，返回uuid对应文件名和用户名，流程成功返回90.失败返回91
      ONLINEVIEW_REQUEST     ;表示客户端在线查看一些需要webview的文件
      INTERNAL_ERROR      :表示服务器内部错误
      CLOSED_CONNECTION   :   表示客户端已经关闭连接了
  */
  enum HTTP_CODE {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    REGISTER_REQUEST_10,
    REGISTER_REQUEST_11,
    LOGIN_REQUEST_20,
    LOGIN_REQUEST_21,
    ASK_FILE_EXIST_30,
    ASK_FILE_EXIST_31,
    UPLOAD_REQUEST_40,
    UPLOAD_REQUEST_41,
    DOWNLOAD_REQUEST_50,
    DOWNLOAD_REQUEST_51,
    DELETE_REQUEST_60,
    DELETE_REQUEST_61,
    RENAME_REQUEST_70,
    RENAME_REQUEST_71,
    SHARE_REQUEST_80,
    SHARE_REQUEST_81,
    DOWNLOADSHARING_REQUEST_90,
    DOWNLOADSHARING_REQUEST_91,
    ONLINEVIEW_REQUEST_100,
    ONLINEVIEW_REQUEST_101,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
  };

  //从状态机的三种可能状态：即行的读取状态，分别表示
  // 1.读取到的一个完整的行  2.行出错   3.行数据尚且不完整
  enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

 public:
  void init(int sockfd, const sockaddr_in &addr);  //初始化接受的连接
  void close_conn();                               //关闭连接
  void process();                                  //处理客户端请求
  bool read();                                     //非阻塞读
  bool write();                                    //非阻塞写

  void initmysql_result(connection_pool *connPool);

 private:
  void init();                        //初始化连接
  HTTP_CODE process_read();           //解析HTTP请求
  bool process_write(HTTP_CODE ret);  //填充HTTP应答

  //被process_read函数调用以分析HTTP请求
  HTTP_CODE parse_request_line(char *text);
  HTTP_CODE parse_headers(char *text);
  HTTP_CODE parse_content(char *text);
  HTTP_CODE do_request();

  HTTP_CODE do_register(string &name, string &password);
  HTTP_CODE do_login(string &name, string &password);
  HTTP_CODE do_delete();
  HTTP_CODE do_rename();
  HTTP_CODE do_share();
  HTTP_CODE do_download_sharing();
  HTTP_CODE do_request_upload();
  HTTP_CODE do_upload();
  HTTP_CODE do_online_view_file(const char *locationEnd);
  HTTP_CODE do_download();

  char *get_line() { return myBuf + m_start_line; }
  // char *get_line() { return m_myReadBuf + m_start_line; }
  LINE_STATUS parse_line();

  //被process_write函数调用以填充HTTP应答
  void unmap();
  bool add_response(const char *format, ...);
  bool add_status_line(int status, const char *title);
  bool add_headers(int content_length);
  bool add_content_length(int content_length);
  bool add_content_type();
  bool add_linger();
  bool add_blank_line();

  //bool add_content_text_plain();

  bool add_content(const char *content);

  bool createNewHtml(const string &uuid_key, const string &file_path);
  bool compress_folder(const char *folder_path, const char *zip_path);

 public:
  static int m_epollfd;
  static int m_user_count;
  MYSQL *mysql;
  connection_pool *m_connection_pool;

 private:
  int m_sockfd;  //该HTTP连接的socket和对方的socket地址
  sockaddr_in m_address;
  char *myBuf = new char[READ_BUFFER_SIZE];
  // char m_read_buf[READ_BUFFER_SIZE];

  int m_read_idx;  //标识读缓冲区中已经读入的客户端的数据的最后一个字符的下一个位置
  int m_checked_idx;  //当前正在解析的字符在读缓冲区中的位置
  int m_start_line;   //当前正在解析的行的起始位置
  CHECK_STATE m_check_state;  //主状态机当前所处的状态
  METHOD m_method;            //请求方法

  // char m_real_file[FILENAME_LEN]; //客户请求的目标文件的完整路径，其内容等于
  // doc_root + m_url, doc_root是网站根目录
  char *m_url;      //客户请求的目标文件的文件名
  char *m_version;  // HTTP协议版本号，仅支持HTTP1.1

  char *m_host;          //主机名
  int m_content_length;  // HTTP请求的消息总长度
  bool m_linger;         // HTTP请求是否要求保持连接
  string m_boundary;
  string m_content_type;

  char m_write_buf[WRITE_BUFFER_SIZE];  //写缓冲区
  int m_write_idx;                      //写缓冲区中待发送的字节数

  char *m_file_address;  //客户请求的目标文件被mmap到内存中的起始位置
  struct stat
      m_file_stat;  //目标文件的状态。通过他可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息。
  struct iovec m_iv
      [2];  //采用writev来执行写操作，所以定义下面连个成员，其中m_iv_count表示被写内存块的数量
  int m_iv_count;
  int bytes_to_send;    //将要发送的数据的字节数
  int bytes_have_send;  //已经发送的字节数
  //int cgi;              //是否启用POST
  char *m_string;       //存储请求头数据
  bool is_upload_request = false;
  string m_allFilesJson;
  char m_uuidString[37];

 public:
  void set_client_data_timer(util_timer *timer);
  client_data *get_client_data();

 private:
  client_data *client = nullptr;

 private:
  string user_name;
  string user_password;
  //int m_view_file_flag = 0;
  int m_file_type;
  void get_file_type(string &extension);
};

#endif  // HTTP_CONN_H
