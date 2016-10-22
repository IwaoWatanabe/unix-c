/*! \file
 * \brief C/C++の端末制御のサンプルコード
 */

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <sys/ioctl.h>
#include <vector>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

using namespace std;

namespace term {

  /// 端末の行数、カラム数を入手する
  bool get_screen_size(int fd, int *LINES, int *COLUMNS) {
    struct winsize size;
    int rc = ioctl(fd, TIOCGWINSZ, &size);
    if (rc == 0) {
      if (LINES) *LINES = size.ws_row;
      if (COLUMNS) *COLUMNS = size.ws_col;
      return true;
    }
    return false;
  }

  /// エントリの文字幅からカラムのステップ数を計算する
  static int calc_column_step(vector<string> &enties, int &columns, int &width) {
    width = 1;

    vector<string>::iterator it = enties.begin();
    while( it != enties.end() ) {
      int len = strlen((*it).c_str());
      if (len > width) width = len;
      it++;
    }

    int rows = 24, cols = 80;
    get_screen_size(2, &rows, &cols);

    columns = cols / ++width;
    if (!columns) columns = 1;

    return enties.size() / columns + 1;
  }

  /// エントリをカラム表示する
  static void show_column_entries(vector<string> &enties, FILE *fp, bool sort_flag) {
    if (sort_flag)
      sort( enties.begin(), enties.end() );

    int cols, width, step = calc_column_step(enties,cols,width);
  
    for (int i = 0; i < step; i++) {
      int idx = i;
      for (int j = 0; j < cols; j++, idx += step) {
	if (idx >= enties.size()) continue;
	fprintf(fp, "%s%*s", enties[idx].c_str(), (int)(width - enties[idx].size()), " ");
      }
      putc('\n', fp);
    }
  }

  /// 端末のカラム数に併せてリストを表示する
  void show_column_entries(const char **names, FILE *fp, bool sort_flag = false) {
    vector<string> enties;
    while (*names) {
      enties.push_back(string(*names));
      names++;
    }
    show_column_entries(enties, fp, sort_flag);
  }

};

/// 端末の行数、桁数を入手する
static int term_show_size(int argc,char **argv) {

  int LINES = 0, COLUMNS = 0;
  
  // 環境変数から初期値を取る
  const char *t = getenv("COLUMNS");
  if (t) COLUMNS = atoi(t);
  t = getenv("LINES");
  if (t) LINES = atoi(t);

  if (!term::get_screen_size(0, &LINES, &COLUMNS))
    fputs("not a tty\n",stderr);
  
  printf("LINES:%d, COLUMNS:%d\n", LINES, COLUMNS);

  return 0;
}

// --------------------------------------------------------------------------------

/// curses を使ったサンプルコード
static int cur_sample01(int argc,char **argv) {
  /*
   * 日本語テキストを扱うために
   * ncursesの場合は libcursesw をリンクします。
   */

  const char *lang = "";
  const char *ltype = setlocale(LC_ALL, lang);
  fprintf(stderr,"INFO: current locale: %s\n", ltype);

  initscr();
  noecho();
  cbreak();

  WINDOW *win = newwin(LINES,COLS-1,0,0);

  box(win,'|','-');

  mvwaddstr(win,1,3,"SAMPLE PROGRAM");
  mvwaddstr(win,3,3,"    CODE :");
  mvwaddstr(win,5,3,"   HITKEY:");
  mvwaddstr(win,7,3,"   END:ESC");

  mvwaddstr(win,10,3,"あああいいいううう");
  mvwaddstr(win,12,3,"press [ESC] to stop.");

  int ch;
  while((ch = wgetch(win)) != 0x1b){
	char buff[36];

	sprintf(buff,"%02X",ch);

	mvwaddstr(win,3,14,buff);
	mvwaddch(win,5,14,ch);
	wrefresh(win);
  }

  wclear(win);
  wrefresh(win);
  endwin();

  return 0;
}

// --------------------------------------------------------------------------------

#ifdef USE_READLINE
// readlineライブラリの利用

static int test_GNU_readline(int argc, char **argv) {
  char *line, *prompt = getenv("PS2");
  
  if (prompt == NULL || strcmp("", prompt) == 0)
	prompt = "readline: ";

  using_history();
  /*
	historyライブラリの初期化。
  */

  string historyfile("config/");
  
  historyfile += basename(argv[0]);
  historyfile += ".history";

  if (read_history(historyfile.c_str()) == 0)
	cerr << "INFO: " << historyfile << " loaded." << endl;
  else
	cerr << "WARNING: read_history failed:" << strerror(errno) << ":" << historyfile << endl;

  while (line = readline(prompt)) {
	string line_text(line);
	free(line);
	/*
	  返り値はmalloc(3)されたものであるため、free(3)する必要がある。
	 */

	add_history(line_text.c_str());
	/*
	  次の入力候補として登録している。
	  ここでは省略していうが、実務では記録する履歴件数に上限を設けたほうがよい。
	 */
	cerr << line_text << endl;
	/*
	  入力したテキストには LF　は含まれていない
	 */
	if (line_text == "quit" || line_text == "exit") break;
  }

  if (write_history(historyfile.c_str()) != 0)
	cerr << "WARNING: write_history failed:" << strerror(errno) << ":" << historyfile << endl;

  /*
	historyデータを保存している。
	ここでは省略しているが、実務では保存する件数を調整したほうがよい。
   */

  return 0;
}
#endif

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD

#include "subcmd.h"

subcmd term_cmap[] = {
  { "term-size", term_show_size, },
  { "cur", cur_sample01, },
#ifdef USE_READLINE
  { "readline", test_GNU_readline, },
#endif
  { 0 },
};

#endif
