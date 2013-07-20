
/*! \file
 * \brief FastCGIサービスの実装
 */

#include "uc/container.hpp"

//#include "uc/fastcgi.hpp"

#include <string>
#include <vector>
#include "uc/elog.hpp"
#include "uc/datetime.hpp"

namespace uc {

  /// HTTPリクエストの基本情報を入手する
  struct Http_Context {
    virtual ~Http_Context();
    /// リクエスト・パラメータを入手する
    virtual const char *request_parameter(const char *name) = 0;
    /// クエリ・パラメータを入手する
    virtual const char *query_parameter(const char *name) = 0;
    /// クエリ・パラメータ（複数の値）を入手する
    virtual std::vector<std::string> *get_query_parameters(const char* name) = 0;
    /// クエリ・パラメータ名を入手する
    virtual int get_query_parameter_names(std::vector<std::string> &names) = 0;

    /// HTTPリクエスト種別を入手する
    virtual const char *get_request_method() = 0;
    /// HTTPリクエストのパス情報を入手する
    virtual const char *get_path_info() = 0;
    /// HTTPリクエストのヘッダ情報を入手する
    virtual const char *get_header(const char *name) = 0;
    /// コンテンツ長を入手する（取得できないケースもある）
    virtual long get_content_length() = 0;
    /// クエリ文字列を入手する
    virtual const char *get_query_string() = 0;

    /// printf書式に従ってブラウザにテキストを返す
    virtual int vprintf(const char *format, va_list ap) = 0;
    /// printf書式に従ってブラウザにテキストを返す
    virtual int printf(const char *format, ...) = 0;
    /// ブラウザにテキストを返す
    virtual int puts(const char *str) = 0;
    /// ブラウザにデータを返す（バイナリデータ用）
    virtual int write(const char *str, size_t len) = 0;
    /// ブラウザにストリームから読込んだデータを返す（バイナリデータ用）
    virtual int copy_stream(FILE *fin) = 0;
    /// ブラウザ出力をフラッシュする
    virtual int flush() = 0;
  };

  /// HTTP処理の基本インタフェース
  /**
     ユーザコードはこのインタフェースを実装する。
   */
  struct Http_Servlet {
    virtual ~Http_Servlet();
    /// サービスが起動するタイミングで初期化のために呼び出される
    virtual void init(Property *props) = 0;
    /// サービスが停止するタイミングで後始末処理のために呼び出される
    virtual void destroy() = 0;
    /// HTTPリクエストの度に呼び出される。
    virtual int do_request(Http_Context *req) = 0;
    /// コンテナが情報確認のために不定期に呼び出す。
    virtual std::string get_info() { return ""; }
  };

  /// HTTPサーブレットのインスタンスを作成する。
  struct Http_Servlet_Factory {
    Http_Servlet_Factory();
    virtual ~Http_Servlet_Factory();
    /// サーブレットのインスタンスを入手する
    virtual Http_Servlet *create_servlet() = 0;
    /// アプリケーション・クラス名を入手する
    virtual const char *get_class_name() { return 0; }
    /// バージョン情報を入手する
    virtual const char *get_version() { return "0.1"; }
  };

};

extern "C" {
  extern char *trim(char *t);
  extern char *demangle(const char *demangle);
};

#include <cstdio>
#include <cctype>
#include <csignal>
#include <fcgiapp.h>
#include <map>
#include <sys/stat.h>
#include <typeinfo>

using namespace std;
using namespace uc;

/// 一連のファクトリを記録する
static vector<Http_Servlet_Factory *> servlet_factories;

#include <ctime>
#include <cerrno>
#include <cstdlib>
#include <threadpool.hpp>

/// FastCGI を操作する基本APIセット
namespace fcgi {

  /// FastCGIコンテキスト
  struct Http_Context_FastCGI_Impl : public uc::Http_Context {
  protected:
    FCGX_Request request;
    map<string, vector<string> > *params;
    map<string, vector<string> > *extract_parameters();
    string inputEncoding, outputEncoding;

  public:
    Http_Context_FastCGI_Impl(int fd, int flags);
    ~Http_Context_FastCGI_Impl();
    int accept();

    virtual const char *query_parameter(const char *name);
    virtual std::vector<std::string> *get_query_parameters(const char* name);
    virtual int get_query_parameter_names(std::vector<std::string> &names);
    virtual const char *request_parameter(const char *name);
    virtual int vprintf(const char *format, va_list ap);
    virtual int printf(const char *format, ...);
    virtual int puts(const char *str);
    virtual int write(const char *str, size_t len);
    virtual int copy_stream(FILE *fin);
    virtual int flush();
    virtual const char *get_request_method();
    virtual const char *get_path_info();
    virtual const char *get_header(const char *name);
    virtual long get_content_length();
    virtual const char *get_query_string();
  };

  Http_Context_FastCGI_Impl::Http_Context_FastCGI_Impl(int fd, int flags)
    : params(0)
  {
    FCGX_InitRequest(&request, fd, flags);
  }

  Http_Context_FastCGI_Impl::~Http_Context_FastCGI_Impl() {
    FCGX_Finish_r(&request);
    delete params;
  }

  int Http_Context_FastCGI_Impl::accept() {
    return FCGX_Accept_r(&request);
  }

  const char *Http_Context_FastCGI_Impl::request_parameter(const char *name) {
    return FCGX_GetParam(name, request.envp);
  }

  int Http_Context_FastCGI_Impl::vprintf(const char *format, va_list ap) {
    return FCGX_VFPrintF(request.out, format, ap);
  }

  int Http_Context_FastCGI_Impl::printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int result = vprintf(format, ap);
    va_end(ap);
    return result;
  }

  int Http_Context_FastCGI_Impl::puts(const char *str) {
    return FCGX_PutS(str, request.out);
  }

  int Http_Context_FastCGI_Impl::write(const char *str, size_t len) {
    return FCGX_PutStr(str, len, request.out);
  }

  int Http_Context_FastCGI_Impl::copy_stream(FILE *fin) {
    char buf[1024];
    long sum = 0;
    int n;
    while ((n = fread(buf, 1, sizeof buf, fin)) > 0) {
      FCGX_PutStr(buf, n, request.out);
      sum += n;
    }
    return sum;
  }

  int Http_Context_FastCGI_Impl::flush() {
    return FCGX_FFlush(request.out);
  }

  const char *Http_Context_FastCGI_Impl::get_request_method() {
    return request_parameter("REQUEST_METHOD");
  }

  const char *Http_Context_FastCGI_Impl::get_path_info() {
    return request_parameter("PATH_INFO");
  }

  const char *Http_Context_FastCGI_Impl::get_header(const char *name) {
    string cginame = "HTTP_";
    for (; *name; name++) {
      if (*name == '-')
	cginame += '_';
      else
	cginame += toupper(*name);
    }
    return request_parameter(cginame.c_str());
  }

  long Http_Context_FastCGI_Impl::get_content_length() {
    const char *tt = request_parameter("CONTENT_LENGTH");
    return !tt ? 0 : atol(tt);
  }

  const char *Http_Context_FastCGI_Impl::get_query_string() {
    return request_parameter("QUERY_STRING");
  }

  static string url_decode(const char* str) {
    string dbuf;

    for (const char* cp = str; *cp; cp++) {
      if (*cp == '%' && *(cp+1) && *(cp+2)) {
	if (strchr("0123456789ABCDEFabcdef", *(cp+1))
	    && strchr("0123456789ABCDEFabcdef", *(cp+2))) {
	  char d1 = *(cp+1);
	  if (islower(d1)) d1 = toupper(d1);
	  char d2 = *(cp+2);
	  if (islower(d2)) d2 = toupper(d2);
	  dbuf += (char)((d1 >= 'A' ? d1 - 'A' + 10 : d1 - '0' ) * 16 +
			       (d2 >= 'A' ? d2 - 'A' + 10 : d2 - '0'));
	  cp += 2;
	}
      } else if (*cp == '+') {
	dbuf += ' ';
      } else {
	dbuf += *cp;
      }
    }
    return dbuf;
  }

  static void splitByAmp(const char *line, vector<string> &ampSplitted) {
    ampSplitted.clear();
    if (!line || !*line) return;

    string str;
    for (const char* p = line; *p; p++) {
      str += *p;
      if (*(p + 1) == '&') {
	ampSplitted.push_back(str);
	str = "";
	p++;
      }
    }
    ampSplitted.push_back(str);
  }

  static pair<string, string> splitByEq(const char* str) {

    pair<string,string> result;
    const char* p;
    for (p = str; *p && *p != '='; p++) {
      result.first += *p;
    }
    if (*p != '=') return result;
    result.second = ++p;
    return result;
  }

  extern string convert_charset(const string &buf, const char *inputEncoding, const char *outputEncoding) {
    return buf;
  }

  static void parse_parameters(const char *line, map< string, vector<string> > &params,
			      const char *inputEncoding, const char *outputEncoding) {

    vector<string> ampSplitted;

    splitByAmp(line, ampSplitted);

    for (vector<string>::iterator i = ampSplitted.begin(); i != ampSplitted.end(); i++) {
      pair<string, string> p = splitByEq((*i).c_str());
      string name, value;
      name = convert_charset(url_decode(p.first.c_str()), inputEncoding, outputEncoding);
      value = convert_charset(url_decode(p.second.c_str()), inputEncoding, outputEncoding);

      params[name].push_back(value);
    }
  }

  map<string, vector<string> >* Http_Context_FastCGI_Impl::extract_parameters() {
    const char* method = get_request_method();
    if (!method) return NULL;

    char* line = NULL;

    if (strcmp(method, "POST") == 0) {
      int content_length = get_content_length();
      line = new char[content_length + 1];
      int real_length = FCGX_GetStr(line, content_length, request.in);
      line[real_length] = '\0';
    }
    else if (strcmp(method, "GET") == 0) {
      const char* queryString = get_query_string();
      line = new char[strlen(queryString) + 1];
      strcpy(line, queryString);
    }

    if (!line) return NULL;

    map< string, vector<string> >*params = new map< string, vector<string> >;
    parse_parameters(line, *params, inputEncoding.c_str(), outputEncoding.c_str());
    delete [] line;
    return params;
  }

  vector<string> *Http_Context_FastCGI_Impl::get_query_parameters(const char* name) {
    if (!params) params = extract_parameters();
    if (!params) return NULL;

    map<string, vector<string> >::iterator it = params->find(name);
    if (it == params->end()) return NULL;
    return &it->second;
  }

  int Http_Context_FastCGI_Impl::get_query_parameter_names(vector<string> &names) {
    names.clear();
    if (!params) params = extract_parameters();
    if (!params) return 0;

    map<string,vector<string> >::iterator it = params->begin();
    for (; it != params->end(); it++) {
      names.push_back(it->first);
    }
    return names.size();
  }

  const char *Http_Context_FastCGI_Impl::query_parameter(const char *name) {
    vector<string>* param = get_query_parameters(name);
    if (!param) return NULL;
    return param->begin()->c_str();
  }

  /// FastCGI サービスの実装
  class FastCGI_Service_Impl : public Service, ELog, Property {
    string socket_name, service_name;
    time_t start_time;
    int back_logs;
    const char *status;
    volatile bool stop_flag;

    virtual void run();

    /// コンテキストと対応する処理
    map<string, Http_Servlet *> servlet;

    Http_Servlet *find_servlet(const char *context);
    void store_servlet(const char *context, Http_Servlet *serv);
    Http_Servlet *remove_servlet(const char *context);
    void destroy_all_servlet();
    void load_servlet();

    /// スレッド処理用
    struct Worker {
      Http_Context_FastCGI_Impl *con;
      Http_Servlet *serv;
      Worker(Http_Context_FastCGI_Impl *_con, Http_Servlet *_serv) : con(_con), serv(_serv) { }
      Worker(const Worker &wt) : con(wt.con), serv(wt.serv) { }
      void operator()() {
	serv->do_request(con);
	delete con;
	con = 0;
      }
    };
    boost::threadpool::pool thread_pool;

  public:
    FastCGI_Service_Impl();
    virtual ~FastCGI_Service_Impl();
    virtual void start();
    virtual void stop();
    virtual const char *get_service_status();
    virtual const char *get_service_name();
    virtual const char *get_service_version();

    virtual const char * get_property(const char *name, const char *default_value="");
    virtual long get_property_value(const char *name, long default_value=0);
    virtual bool get_property_names(std::vector< const char * > &names);

    virtual void set_socket_name(const char *sock) { socket_name = sock; }
  };

  static vector<Service *> services;

  static void int_handler(int signum) {
    vector<Service *>::iterator it = services.begin();
    while (it != services.end()) { (*it)->stop(); it++; }
    fputs("interupted!\n",stderr);
  }

  FastCGI_Service_Impl::FastCGI_Service_Impl()
    : start_time(0), back_logs(50), status("init"), stop_flag(false), thread_pool(32)
  {
    init_elog("FastCGI_Service_Impl");
  }

  FastCGI_Service_Impl::~FastCGI_Service_Impl() { }

  Http_Servlet *FastCGI_Service_Impl::find_servlet(const char *context) {
    map<string, Http_Servlet *>::iterator it = servlet.find(context);
    if (it == servlet.end()) return 0;
    return it->second;
  }

  void FastCGI_Service_Impl::store_servlet(const char *context, Http_Servlet *serv) {
    servlet.insert(map<string, Http_Servlet *>::value_type(context, serv));
  }

  Http_Servlet *FastCGI_Service_Impl::remove_servlet(const char *context) {
    map<string, Http_Servlet *>::iterator it = servlet.find(context);
    if (it == servlet.end()) return 0;

    Http_Servlet *serv = it->second;
    servlet.erase(it++);
    return serv;
  }

  void FastCGI_Service_Impl::destroy_all_servlet() {
    map<string, Http_Servlet *>::iterator it = servlet.begin();
    while (it != servlet.end()) {
      string context = it->first;
      Http_Servlet *serv = it->second;
      servlet.erase(it++);
      serv->destroy();
      elog(T, "%s servlet destroyed.\n", context.c_str());
    }
  }

  void FastCGI_Service_Impl::load_servlet() {
    map<string, Http_Servlet_Factory *> servlets;

    vector<Http_Servlet_Factory *>::iterator it = servlet_factories.begin();
    for (;it != servlet_factories.end(); it++) {
      Http_Servlet_Factory *sf = *it;

      const char *name = sf->get_class_name();
      if (!name) {
	name = demangle(typeid(*sf).name());
	const char *p;
	while ((p = strstr(name,"::"))) { name = p + 2; }
      }

      /// 取りあえずファクトリ名とバージョンを表示してみる
      printf("factory: %s: %s\n", name, sf->get_version());
      servlets.insert(map<string, Http_Servlet_Factory *>::value_type(name, sf));
    }

    struct { char *context, *servlet_name; } url_map[] = {
      { "hello", "Hello_Servlet_Factory", },
      { "hello1", "Hello_Servlet_Factory", },
      { 0, },
    }, *mp = url_map;

    for (; mp->context; mp++) {
      Http_Servlet_Factory *sf = servlets[mp->servlet_name];
      if (!sf) continue;
      Http_Servlet *serv = sf->create_servlet();
      elog(T, "/%s: %s servlet initialiing..\n", mp->context, mp->servlet_name );
      serv->init(this);
      store_servlet(mp->context, serv);
    }

  }

  const char * FastCGI_Service_Impl::get_property(const char *name, const char *default_value) {
    return default_value;
  }

  long FastCGI_Service_Impl::get_property_value(const char *name, long default_value) {
    return default_value;
  }

  bool FastCGI_Service_Impl::get_property_names(std::vector< const char * > &names) {
    names.clear();
  }

  void FastCGI_Service_Impl::run() {

    status = "INIT";

    if (FCGX_Init()) {
      elog("FCGX_Init() failed");
      return;
    }

    const char *socket_path = socket_name.c_str();

    int fd = FCGX_OpenSocket(socket_path, back_logs);
    if (fd < 0) {
      elog("FCGX_OpenSocket(%s) failed : %d", socket_path, fd);
      return;
    }

    if (socket_path[0] != ':') {// is unix domain socket
      struct stat sbuf;
      if (stat(socket_path, &sbuf) == 0) {
	sbuf.st_mode |= S_IRWXU|S_IRWXG|S_IRWXO;
	if (chmod(socket_path, sbuf.st_mode) != 0)
	  elog(W, "chmod %s failed:(%d):%s\n",socket_path,errno,strerror(errno));
      }
    }

    status = "RUNNING";

    elog(I, "service %s started.\n", socket_name.c_str());

    while (!stop_flag) {
      Http_Context_FastCGI_Impl *req = new Http_Context_FastCGI_Impl(fd, 0);
      int result = req->accept();
      if (result != 0) {
	elog(W, "accept() returns non-zero value(%d)", result);
	delete req;
	continue;
      }

      string context_path;
      const char* path_info = req->get_path_info();
      if (path_info) {
	const char *pi = path_info;
	if (*pi == '/') pi++;
	for (; *pi && *pi != '/'; pi++) { context_path += *pi; }
      }

      Http_Servlet *serv = find_servlet(context_path.c_str());
      if (serv) {
#if 1
	Worker aa(req, serv);
	thread_pool.schedule(aa);
#else
	serv->do_request(req);
	delete req;
#endif
      }
      else {
	req->puts("Status: 404 Not Found\n");
	req->puts("Content-Type: text/html\n\n");
	req->printf("<html><body><h1>404 not found</h1>"
		    "The requested resource %s is not found.</body></html>", path_info);
	delete req;
      }
    }

    status = "STOPED";

    destroy_all_servlet();
  }

  void FastCGI_Service_Impl::start() {
    if (time(&start_time) == (time_t)-1)
      elog(W, "time:(%d):%s\n",errno,strerror(errno));

    services.push_back(this);

    load_servlet();
    signal(SIGPIPE , SIG_IGN);
    //signal(SIGINT , int_handler);
    stop_flag = false;

    run();
    /*
      これは仮実装。あとでスレッド化する
     */
  }

  void FastCGI_Service_Impl::stop() {
    status = "STOPING";
    stop_flag = true;
  }

  const char *FastCGI_Service_Impl::get_service_status() { return status; }

  const char *FastCGI_Service_Impl::get_service_name() {
    return "FastCGI";
  }

  const char *FastCGI_Service_Impl::get_service_version() {
    return "0.1";
  }


};

namespace uc {

  Http_Context::~Http_Context() { }

  Http_Servlet::~Http_Servlet() { }

  Http_Servlet_Factory::Http_Servlet_Factory() { servlet_factories.push_back(this); }

  Http_Servlet_Factory::~Http_Servlet_Factory() { }

  Service::~Service() { }

  Property::~Property() { }
};

namespace fcgi {

  /// 単純なメッセージを返すだけのサーブレット
  class Hello_Servlet : public Http_Servlet, ELog {
    Date dd;

  public:
    Hello_Servlet() {
      init_elog("Hello_Servlet");
    }

    ~Hello_Servlet() {}

    void init(Property *props) {
      elog(T,"init %p\n",this);
    }

    void destroy() {
      elog(T,"destroy %p\n",this);
    }

    int do_request(Http_Context *req) {
      elog(T,"req %p\n",this);
      req->printf("Content-type: %s\n\n", "text/plain");
      req->printf("Hello .. %s\n", dd.now().get_date_text().c_str());

      const char *asleep = req->query_parameter("asleep");
      if (asleep) {
	int tt = atoi(asleep);
	if (tt) {
	  req->printf("asleep %d sec..\n",tt);
	  req->flush();
	  sleep(tt);
	}
      }

      req->printf("done .. %s\n", dd.now().get_date_text().c_str());

      return 0;
    }
  };

  /// Hello_Servlet をインスタンス化する
  class Hello_Servlet_Factory : public Http_Servlet_Factory {
    Http_Servlet *create_servlet() { return new Hello_Servlet(); }
  };

  static int cmd_fcgi(int argc, char **argv) {
    Hello_Servlet_Factory hello;
    FastCGI_Service_Impl service;
    service.set_socket_name(":6100");
    service.start();
    return 0;
  }

};


// --------------------------------------------------------------------------------


#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd fcgi_cmap[] = {
  { "fcgi", fcgi::cmd_fcgi, },
  { 0 },
};

#endif

