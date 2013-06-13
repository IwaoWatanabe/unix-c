/*! \file
 * \brief ファイルの基本操作をサポートするクラス
 */

#include <cstdlib>
#include <string>
#include <vector>

#include "elog.hpp"

namespace uc {

  /// ファイルの基本操作をサポートする
  class File_Manager {
  public:
    File_Manager() { }
    virtual ~File_Manager() { }
  };
};

namespace uc {

  /// ファイルの基本操作の素朴な実装クラス
  /** それぞれの処理を並行して処理する場合は、
      固有の処理インスタンスを作成すること。
  */
  class File_Manager_Impl: public File_Manager, ELog {
  public:
    /// 基準ディレクトリを指定しないで初期化する
    /*
      相対パスは通常のワークディレクトリとして処理する
     */
    File_Manager_Impl();

    /// 相対パスを渡して処理するときの、基準ディレクトリを指定して初期化する
    File_Manager_Impl(const char *base_dir);
    virtual ~File_Manager_Impl();

    /// ディレクトリであるか診断する
    bool isdir(const char *dirpath);
    /// 再帰的にディレクトリを作成する。
    bool mkdirs(const char *dirpath);
    /// 再帰的にディレクトリを削除する。空では削除できない
    int rmdirs(const char *dirpath);
    /// パスのファイル名部を得る
    std::string basename(const char *path);
    /// パスのディレクトリ部を得る
    std::string dirname(const char *path);

    /// 一般ファイルを削除する
    int remove_file(const char *filepath, bool recurce);
    /// 一般ファイルを複製する
    int copy_file(const char *dst, const char * const *src, size_t src_count, bool recurce);
    /// 一般ファイルを移動する
    int move_file(const char *dst, const char * const *src, size_t src_count);

    /// ファイル操作の進捗状態を表すテキストを入手する
    std::string get_work_status();

    enum { dirent_file = 1, dirent_dir = 2, dirent_all = 1|2, };

    /// ディレクトリに含まれるエントリ一式を入手する
    int load_dirent(char *dirpath, std::vector<std::string> &entries, int ent_type, 
		    bool with_hidden_file = false);

    /// ファイルマネージャがサポートするアーカイブ処理を行う
    int archive_file(const char *archive_file,
		     const char * const *src, size_t src_count = 0);

    /// アーカイブからファイルを抽出する
    int extract(const char *archive_file, const char *out_dir = 0,
		const char * const *target = 0, size_t target_count = 0);
  };

};

// --------------------------------------------------------------------------------

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>


using namespace std;

namespace uc {

  File_Manager_Impl::File_Manager_Impl() {
    init_elog("fm");
  }

  File_Manager_Impl::~File_Manager_Impl() { }

  /// パスの最後の構成要素を返す
  string File_Manager_Impl::basename(const char *path) {
    if (!path || !*path) return ".";

    size_t len = strlen(path);

    if (path[len -1] == '/') {
      // 末尾のスラッシュを除く
      char *dirp = (char *)alloca(len + 1);
      path = strcpy(dirp, path);
      dirp[--len] = 0;

      while (len > 1 && dirp[len - 1] == '/') 
	dirp[--len] = 0;
    }

    char *p = strrchr(path, '/');
    if (!p || p == path) return path;
    return p + 1;
  }

  /// パスのディレクトリ部を返す
  string File_Manager_Impl::dirname(const char *path) {

    if (!path || !*path) return ".";

    size_t len = strlen(path);

    if (path[len -1] == '/') {
      // 末尾のスラッシュを除く
      char *dirp = (char *)alloca(len + 1);
      path = strcpy(dirp, path);
      dirp[--len] = 0;

      while (len >= 1 && dirp[len - 1] == '/') 
	dirp[--len] = 0;
    }

    char *p = strrchr(path, '/');
    if (!p) return ".";

    int pos = p - path;
    char *dirp2 = (char *)alloca(pos + 1);
    strcpy(dirp2, path)[pos] = 0;

    return dirp2;
  }

  bool File_Manager_Impl::isdir(const char *dirpath) {
    struct stat sbuf;
    if (stat(dirpath, &sbuf) == 0) {
      // アクセスできるエントリが存在する
      if (S_ISDIR(sbuf.st_mode)) return true;
    }
    return false; 
  }


  /// ディレクトリがなければ作成する
  /**
     @return ディレクトリでないエントリが存在するか、作成できなかった場合にfalse
  */
  bool File_Manager_Impl::mkdirs(const char *dirpath) {
    if (!dirpath) return false;

    int len = strlen(dirpath);
    if (!len) return true;

    if (dirpath[len -1] == '/') {
      // 最後のスラッシュを除く
      char *dirp = (char *)alloca(len + 1);
      dirpath = strcpy(dirp, dirpath);
      dirp[len - 1] = 0;
    }

    struct stat sbuf;
    if (stat(dirpath, &sbuf) == 0) {
      // アクセスできるエントリが存在する
      if (S_ISDIR(sbuf.st_mode)) {
	elog(T, "directory already exists: %s\n", dirpath);
	return true;
      }
      elog(W, "not directory: %s", dirpath);
      return false;
    }

    if (errno != ENOENT) {
      elog(W, "stat: %s (%d):%s\n", dirpath, errno, strerror(errno));
      return false;
    }

    char *p = strrchr(dirpath, '/');
    if (p) {
      // スラッシュが含まれていれば、その親ディレクトリが有効であるか確認する
      int pos = p - dirpath;
      char *dirp2 = (char *)alloca(pos + 1);
      strcpy(dirp2, dirpath)[pos] = 0;
      if (!mkdirs(dirp2)) return false;
    }

    if (mkdir(dirpath,0777) != 0) {
      elog(W, "mkdir: %s:(%d):%s", dirpath, errno, strerror(errno));
      return false;
    }
    elog(D, "mkdir: %s:%d\n", dirpath, len);
    
    return true;
  }

  /// ディレクトリの削除
  /**
     ディレクトリしか削除できない。
     削除できる権限がないと失敗する。
  */

  int File_Manager_Impl::rmdirs(const char *dirpath) {
    int len = strlen(dirpath);
    if (!len) {
      elog(W, "empty directory name\n");
      return 0;
    }

    if (strchr(dirpath, '/')) {
      char *dirp = (char *)alloca(len + 1);
      dirpath = strcpy(dirp, dirpath);
      // 最後のスラッシュを除く
      if (dirpath[len -1] == '/') dirp[len - 1] = 0;
    }
    
    for (;;) {
      elog(T, "rmdir: %s\n", dirpath);
      if (rmdir(dirpath) != 0) {
	elog("rmdir: %s:(%d):%s\n", dirpath, errno, strerror(errno));
	return 1;
      }
      
      char *st;

      // スラッシュを探して、そこに終端文字を入れる
      if ((st = strrchr(dirpath, '/')) == NULL) break;
      *st = 0;
    }
    return 0;
  }

  static int max_file_name_len = 256;

  /// ファイルを削除する
  /*
    ファイルやディレクトリを削除する。
    対象のアクセス権がなければ削除できない。
  */
  int File_Manager_Impl::remove_file(const char *filepath, bool recurce) {

    if (!isdir(filepath)) {
      if (unlink(filepath) != 0) {
	elog("unlink: %s:(%d):%s\n", filepath, errno, strerror(errno));
	return 1;
      }
      elog(D, "unlink: %s\n", filepath);
      return 0;
    }

    if (!recurce) {
      elog("directory not removed: %s\n",filepath);
      return 1;
    }

    // 以下、ディレクトリを走査して、ファイルを削除していく
    const char *dirpath = filepath;
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
      elog("opendir: %s:(%d):%s\n", dirpath, errno, strerror(errno));
      return 1;
    }
    elog(T, "opendir: %s\n", dirpath);
    int rc = 0;

    struct dirent *ent;
    size_t dirnlen = strlen(dirpath);
    char *path = (char *)alloca(dirnlen + max_file_name_len + 2);
    if (!path) abort();
    strcat(strcpy(path, dirpath), "/"); dirnlen++;
    /*
      pathはファイル・パスを組み立てる作業領域
    */

    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_name[0] == '.') {
	// 自身は対象から除外する
	if (ent->d_name[1] == 0) continue;
	// 親ディレクトリは対象から除外する
	if (ent->d_name[1] == '.' && ent->d_name[2] == 0) continue;
      }
      strcpy(path + dirnlen, ent->d_name);
      int rc0 = remove_file(path, true);
      if (rc0 != 0)  { rc = rc0; break; }
    }

    if (closedir(dir) == 0)
      elog(T, "closedir: %s\n", dirpath);
    else {
      elog("closedir: %s:(%d):%s\n", dirpath, errno, strerror(errno));
    }

    if (rc == 0) {
      // ここまでにエラーが生じていなければ、ディレクトリを削除する
      if (rmdir(dirpath) == 0)
	elog(D, "rmdir: %s\n", dirpath);
      else {
	elog("rmdir: %s:(%d):%s\n", dirpath, errno, strerror(errno));
	rc = 2;
      }
    }
    return rc;
  }

};

// --------------------------------------------------------------------------------

#include <iostream>

namespace uc {

  /// ディレクトリ名入手のテスト
  static int cmd_basename(int argc,char **argv) {
    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <file> .." << endl;
      return 1;
    }

    File_Manager_Impl fm;

    for (int i = 1; i < argc; i++) {
      cout << fm.basename(argv[i]) << endl;
    }
    return 0;
  }

  /// ディレクトリ名の入手のテスト
  static int cmd_dirname(int argc,char **argv) {

    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <file> .." << endl;
      return 1;
    }

    File_Manager_Impl fm;

    for (int i = 1; i < argc; i++) {
      cout << fm.dirname(argv[i]) << endl;
    }
    return 0;
  }

  /** mkdirsの試験
   */
  static int cmd_mkdirs(int argc,char **argv) {

    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <dir> .." << endl;
      return 1;
    }

    File_Manager_Impl fm;

    for (int i = 1; i < argc; i++) {
      if (fm.mkdirs(argv[i]))
	cerr << "INFO: " << argv[i] << " ok" << endl;
    }

    return 0;
  }

  /** rmdirsの試験
   */
  static int cmd_rmdirs(int argc,char **argv) {
    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <dir> .." << endl;
      return 1;
    }

    File_Manager_Impl fm;

    int rc = 0;
    for (int i = 1; i < argc; i++) {
      int rc0 = fm.rmdirs(argv[i]);
      if (rc0 == 0)
	cerr << "INFO: " << argv[i] << " ok" << endl;
      else
	rc = rc0;
    }
    return rc;
  }

  /// ファイルの削除の実験
  static int cmd_remove_file(int argc,char **argv) {
    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <file> .." << endl;
      return 1;
    }

    File_Manager_Impl fm;
    bool recurce = getenv("FILE_ONLY") == NULL;
    int rc = 0;
    
    for (int i = 1; i < argc; i++) {
      int rc0 = fm.remove_file(argv[1], recurce);
      if(rc0) rc = rc0;
    }
    return rc;
  }

};


// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd fileop_cmap[] = {
  { "basename", uc::cmd_basename, },
  { "dirname", uc::cmd_dirname, },
  { "mkdirs", uc::cmd_mkdirs, },
  { "rmdirs", uc::cmd_rmdirs, },
  { "rm", uc::cmd_remove_file, },
  { 0 },
};

#endif

