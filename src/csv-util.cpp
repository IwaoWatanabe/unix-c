/*! \file
 * \brief CSVファイルを取り扱うAPI
 */

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#include "csv.h"

/// CSVを読み込み、その語の加工処理を CSV_Reader に委譲するクラスが実装するインタフェース
/// 
class CSV_Source {
protected:
  /// 処理を開始する前に呼び出される
  virtual bool begin_read_soruce() { return true; }
  /// 処理が開始されていれば、終了あるいは中断のタイミングで呼び出される
  virtual void end_read_soruce() { }

public:
  virtual ~CSV_Source() {}
  /// 読み取りを開始する
  virtual int perform_csv(CSV_Reader *reader) = 0;
};

namespace {

  /// Excel仕様のCSVテキスト・ファイルからデータを読み込む素朴な実装
  /**
     エンクォート文字(")で囲まれた区切り文字(,)および改行はスキップします。
     また、項目中のエンクォート文字(")の連続２個("")は、エンクォート文字１個(")に置き換えられます。

     http://winter-tail.sakura.ne.jp/pukiwiki/index.php?C%A1%BFC%2B%2B%A4%A2%A4%EC%A4%B3%A4%EC%2FExcel%BB%C5%CD%CD%A4%CECSV%A5%D5%A5%A1%A5%A4%A5%EB%A4%CE%C6%C9%A4%DF%B9%FE%A4%DF%A4%C8%C9%BD%BC%A8#
   */
  class ExcelCSVFile_Source : public CSV_Source {
    string readfile;
    FILE *fp;

    char SEPARATOR, QUOTE;
    int GetNextLine(string& line);
    int Parse(string& nextLine, vector<string>& tokens);
    int Read(vector<string>& tokens);

    time_t begin_time;
    int counter;

    typedef const char *charp;

  protected:
    bool begin_read_soruce();
    void end_read_soruce();

  public:
    ExcelCSVFile_Source(const char *fname) : 
      readfile(fname), SEPARATOR(','), QUOTE('"'),
      counter(0),begin_time(0) { }

    ~ExcelCSVFile_Source() { }
    int perform_csv(CSV_Reader *reader);
  };

  int 
  ExcelCSVFile_Source::Read(vector<string>& tokens) {
    tokens.clear();
    string nextLine;
    if( GetNextLine(nextLine)<=0 ) return -1;
    Parse(nextLine, tokens);
    return 0;
  }

  int
  ExcelCSVFile_Source::GetNextLine(string& line)
  {
    char buf[BUFSIZ];
    char *tt = fgets(buf,sizeof buf, fp);
    if (!tt) return -1;
    size_t n = strlen(tt);
    if (tt[n - 1] == '\n') tt[n - 1] = 0;
    line = tt;
    return (int)line.length();
  }

  int
  ExcelCSVFile_Source::Parse(string& nextLine, vector<string>& tokens)
  {
    string token;
    bool interQuotes = false;
    do {
      if (interQuotes) {
	token += '\n';
	if (GetNextLine(nextLine)<0) {
	  break;
	}
      }
      
      for (int i = 0; i < (int)nextLine.length(); i++) {
 	char c = nextLine.at(i);
	if (c == QUOTE) {
	  if( interQuotes
	      && (int)nextLine.length() > (i+1)
	      && nextLine.at(i+1) == QUOTE ){
	    token += nextLine.at(i+1);
	    i++;
	  }else{
	    interQuotes = !interQuotes;
	    if(i>2 
	       && nextLine.at(i-1) != SEPARATOR
	       && (int)nextLine.length()>(i+1) 
	       && nextLine.at(i+1) != SEPARATOR
	       ){
	      token += c;
	    }
	  }
	} else if (c == SEPARATOR && !interQuotes) {
	  tokens.push_back(token);
	  token.clear();
	} else {
	  token += c;
	}
      }
    } while (interQuotes);

    tokens.push_back(token);

    return 0;
  }

  int
  ExcelCSVFile_Source::perform_csv(CSV_Reader *reader)
  {
    if (!begin_read_soruce()) return -1;

    if (!reader->begin_read_csv()) {
      end_read_soruce();
      return -1;
    }

    vector<string> tokens;

    const char *EMPTY_ROW[] = { NULL, };
    charp *row = NULL;

    int rc = -1;

    while(!Read(tokens)) {

      int num;

      if (tokens.empty()) {
	row = EMPTY_ROW;
	row[0] = NULL;
	num = 0;
      }
      else {
	num = tokens.size();
	row = new charp[num + 1];
	for( unsigned int i = 0; i < tokens.size(); i++ ) {
	  row[i] = tokens[i].c_str();
	}
	row[num] = NULL;
      }

      rc = reader->read_csv(row, num);
      if (!tokens.empty()) delete [] row;

      if (rc) break;

      counter++;
    }

    reader->end_read_csv(rc != 0);
    end_read_soruce();
    return rc;
  }

  bool
  ExcelCSVFile_Source::begin_read_soruce()
  {
    fp = fopen(readfile.c_str(),"r");
    if (fp)
      fprintf(stderr,"INFO: try to read: %s\n", readfile.c_str());
    else
      fprintf(stderr,"ERROR: fopen %s failed:(%d) %s\n", readfile.c_str(), errno, strerror(errno));

    if (time(&begin_time) == (time_t)-1) perror("time");
    return fp != NULL;
  }

  void
  ExcelCSVFile_Source::end_read_soruce()
  {
    if (!fp) return;

    putc('\n',stderr);

    if (fclose(fp) != 0) {
      fprintf(stderr,"WARNING: fclose %s failed:(%d) %s\n", readfile.c_str(), errno, strerror(errno));
    }
    time_t end_time;

    if (time(&end_time) == (time_t)-1)
      perror("time");
    else {
      double sec = (double)end_time - begin_time;
      double rps = counter == 0 ? 0 : counter/sec;
      fprintf(stderr,"INFO: %d recoreds. treated in %.2f sec (%.0f rps)\n", counter, sec, rps);
    }

    
  }

  /// 読み込んだデータを標準出力にパイプ区切りで出力する。
  class EchoCSVReader : public CSV_Reader {
    string rsep;
  public:
    EchoCSVReader(const char *sep) : rsep(sep) {}
    EchoCSVReader() : rsep("|") {}
    int read_csv(const char **row, int columns);
  };

  int
  EchoCSVReader::read_csv(const char **row, int columns) {
    const char *sep = "";

    for (; *row; row++) {
      printf("%s%s",sep,*row);
      sep = rsep.c_str();
    }
    putchar('\n');

    return 0;
  }

};

void load_csv(const char *fname, CSV_Reader *reader) {
  ExcelCSVFile_Source file(fname);
  file.perform_csv(reader);
}

