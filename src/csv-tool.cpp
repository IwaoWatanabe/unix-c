
#include <cstdio>
#include <cerrno>
#include <string>

#include "subcmd.h"
#include "csv.h"

using namespace std;

namespace {

  // 読み込んだデータを別のファイルに書き出す
  class DumpCSVReader : public uc::CSV_Reader {
    string outfile;
    FILE *fp;

  protected:
    void out_csv(const char **row, FILE *fp);

  public:
    DumpCSVReader(const char *fname) : outfile(fname), fp(0) {}

    bool begin_read_csv();
    int read_csv(const char **row, int columns);
    void end_read_csv(bool cancel);
  };

  bool
  DumpCSVReader::begin_read_csv() {
    if (outfile.empty()) return false;

    fp = fopen(outfile.c_str(),"a+");
    if (!fp) {
      fprintf(stderr,"ERROR: fopen %s failed:(%d) %s\n", outfile.c_str(), errno, strerror(errno));
      return false;
    }
    return true;
  }

  void 
  DumpCSVReader::out_csv(const char **row, FILE *fp)
  {
    char *sep = "";

    for (; *row; row++) {
      char *p = strchr(*row, '"');
      if (!p) p = strchr(*row, '\n');

      if(!p)
	fprintf(fp,"%s%s",sep,*row);
      else {
	fputs(sep,fp);
	fputc('\"',fp);
	for (const char *t = *row; *t; t++) {
	  fputc(*t,fp);
	  if (*t == '"') fputc(*t,fp);
	}
	fputc('\"',fp);
      }
      sep = ",";
    }
    fputc('\n',fp);
  }

  int
  DumpCSVReader::read_csv(const char **row, int columns) {
    if (!fp) return 1;
    out_csv(row, fp);
    return 0;
  }

  void
  DumpCSVReader::end_read_csv(bool cancel) {
    if (!fp) return;
    if (fclose(fp) != 0) {
      fprintf(stderr,"WARNING: fclose %s failed:(%d) %s\n", outfile.c_str(), errno, strerror(errno));
    }
  }

};

/// CSVを読み込んでそのまま別ファイルに出力する
static int
test_csv01(int argc, char **argv)
{
  const char *csv_file = getenv("CSV_IN");
  if (!csv_file) csv_file = "work/aa.csv";

  const char *csv_out_file = getenv("CSV_OUT");
  if (!csv_out_file) csv_out_file = "work/bb.csv";

  load_csv(csv_file, new DumpCSVReader(csv_out_file));
  return EXIT_SUCCESS;
}

subcmd csv_cmap[] = {
  { "csv-copy", test_csv01, },
  { 0 },
};

