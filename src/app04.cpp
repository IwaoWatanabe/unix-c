
#include <cstdio>
#include <csetjmp>
#include <csignal>
#include <execinfo.h>
#include <iostream>
#include <stdexcept>

static int hello(int argc, char **argv) {
  printf("called by subcmd: %s\n", argv[0]);
  return 0;
}

// --------------------------------------------------------------------------------
// シグナルハンドラの検証

static std::terminate_handler orig_handler = 0;

const int MAX_BACKTRACE = 100;

static void my_terminate() {
  std::cerr << "Backtrace:" << std::endl;
  void* bt[MAX_BACKTRACE];
  int bt_num = backtrace(bt, MAX_BACKTRACE);
  backtrace_symbols_fd(bt, bt_num, 2);
  std::cerr << std::endl;
  if( orig_handler ) orig_handler();
  abort();
}

static void segv_handler(int signum) throw (char const *) {
  throw signum;
}

/// SEGVの復帰の確認

static int testSEGVhandler(int argc, char **argv) {
  char *a = 0;

  orig_handler = std::set_terminate(my_terminate);

  char buf[2048];

  signal(SIGSEGV, segv_handler);

  while (fgets(buf,sizeof buf,stdin)) {
    try {
      char *p = strchr(buf,'\n');
      if (*p) *p = 0;
      if (strcmp(buf,"segv") == 0) *a = 0;

      if (strcmp(buf,"aaa") == 0) throw "aaa";
      std::cout << buf << std::endl;
    }
    catch(int sig) {
      std::cerr << "runtime error occured: " << sig << std::endl;
    }
    catch(char const *str) {
      std::cerr << "runtime error occured: " << str << std::endl;
    }
    catch(...) {
      std::cerr << "unkown runtime error occured: " << std::endl;
    }
  }

  return 0;
}

static sigjmp_buf segvjbuf;

static void segv_handler02(int signum) throw (const char *) {
  siglongjmp(segvjbuf, signum);
}

static void proc02(char *buf) {
  char *a = 0;
  if (strcmp(buf,"segv") == 0) *a = 0;
  std::cout << buf << std::endl;
}


/// SEGVの復帰の確認
static int testSEGVhandler02(int argc, char **argv) {
  char buf[2048];

  signal(SIGSEGV, segv_handler02);

  while (fgets(buf,sizeof buf,stdin)) {
    char *p = strchr(buf,'\n');
    if (*p) *p = 0;

    // オブジェクトを利用していない、この範囲での利用ならうまく動く
    switch (sigsetjmp( segvjbuf, 1)) {
    case 0:
      proc02(buf);
      break;

    default:
      std::cerr << "runtime error occured. (recover)" << std::endl;
    }
  }

  return 0;
}

// --------------------------------------------------------------------------------

#include "subcmd.h"

static subcmd cmap[] = {
  { "segv", testSEGVhandler, },
  { "segv02", testSEGVhandler02, },
  { "hello", hello, },
  { "hello-xx", hello, },
  { 0 },
};

/// サブコマンドを使う

int main(int argc, char **argv) {
  subcmd_add(cmap);

  extern subcmd stl_cmap[];
  subcmd_add(stl_cmap);

  extern subcmd term_cmap[];
  subcmd_add(term_cmap);

  extern subcmd temp_cmap[];
  subcmd_add(temp_cmap);

  extern subcmd csv_cmap[];
  subcmd_add(csv_cmap);

  extern subcmd xwin_cmap[];
  subcmd_add(xwin_cmap);

  extern subcmd awt_cmap[];
  subcmd_add(awt_cmap);

#ifdef USE_MOTIF
  extern subcmd motif_cmap[];
  subcmd_add(motif_cmap);
#endif

#ifdef USE_XVIEW
  extern subcmd xview_cmap[];
  subcmd_add(xview_cmap);
#endif

  extern subcmd mbs_cmap[];
  subcmd_add(mbs_cmap);

  extern subcmd time_cmap[];
  subcmd_add(time_cmap);

  extern subcmd logger_cmap[];
  subcmd_add(logger_cmap);

  extern subcmd ini_cmap[];
  subcmd_add(ini_cmap);

  extern subcmd stdc_cmap[];
  subcmd_add(stdc_cmap);

  extern subcmd date_cmap[];
  subcmd_add(date_cmap);

  extern subcmd fileop_cmap[];
  subcmd_add(fileop_cmap);

  extern subcmd lineop_cmap[];
  subcmd_add(lineop_cmap);

  extern subcmd index_cmap[];
  subcmd_add(index_cmap);

  extern subcmd xml_cmap[];
  subcmd_add(xml_cmap);

  extern subcmd kvs_cmap[];
  subcmd_add(kvs_cmap);

  extern subcmd mysql_cmap[];
  subcmd_add(mysql_cmap);

  extern subcmd user_cmap[];
  subcmd_add(user_cmap);

  extern subcmd key_cmap[];
  subcmd_add(key_cmap);

  int rc = subcmd_run(argc, argv);
  return rc;
}

