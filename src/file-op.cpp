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
    /// ディレクトリであるか診断する
    virtual bool isdir(const char *dirpath) = 0;
    /// 作業ディレクトリ名を入手する
    virtual std::string getcwd() = 0;
    /// 作業ディレクトリ名を変更する
    virtual bool chdir(const char *dirpath) = 0;
    /// 再帰的にディレクトリを作成する。
    virtual bool mkdirs(const char *dirpath) = 0;
    /// 再帰的にディレクトリを削除する。空では削除できない
    virtual int rmdirs(const char *dirpath) = 0;
    /// パスのファイル名部を得る
    virtual std::string basename(const char *path) = 0;
    /// パスのディレクトリ部を得る
    virtual std::string dirname(const char *path) = 0;

    /// 一般ファイルを削除する
    virtual int remove_file(const char *filepath, bool recurce) = 0;
    /// 一般ファイルを複製する
    virtual int copy_file(const char *dst, const char * const *src, size_t src_count, bool recurce) = 0;
    /// 一般ファイルを移動する
    virtual int move_file(const char *dst, const char * const *src, size_t src_count) = 0;
  };
};

namespace uc {

  /// オペレーションの確認用
  /**
     ファイルシステムに作用を伴う操作を行わない
   */
  class File_Manager_Nop: public File_Manager, ELog {
  public:
    virtual ~File_Manager_Nop() { }
    bool isdir(const char *dirpath);
    bool mkdirs(const char *dirpath);
    int rmdirs(const char *dirpath);
    std::string basename(const char *path);
    std::string dirname(const char *path);
    int remove_file(const char *filepath, bool recurce);
    int copy_file(const char *dst, const char * const *src, size_t src_count, bool recurce);
    int move_file(const char *dst, const char * const *src, size_t src_count);
  };

  /// ファイルの基本操作の素朴な実装クラス
  /** それぞれの処理を並行して処理する場合は、
      固有の処理インスタンスを作成すること。
  */
  class File_Manager_Impl: public File_Manager, ELog {

    int copy_local_file(FILE *outfp, const char *src, long *outbytes);
    int copy_regular_file(const char *dst, const char *src);
    int copy_directory(const char *dstdir, const char *srcdir);
    int copy_file_or_directory(const char *dst, const char *src, bool recurce);
    int move_regular_file(const char *dst, const char *src);

  public:
    /// 基準ディレクトリを指定しないで初期化する
    /*
      相対パスは通常のワークディレクトリとして処理する
     */
    File_Manager_Impl();

    /// 相対パスを渡して処理するときの、基準ディレクトリを指定して初期化する
    File_Manager_Impl(const char *base_dir);
    virtual ~File_Manager_Impl();

    bool isdir(const char *dirpath);
    std::string getcwd();
    bool chdir(const char *dirpath);
    bool mkdirs(const char *dirpath);
    int rmdirs(const char *dirpath);
    std::string basename(const char *path);
    std::string dirname(const char *path);
    int remove_file(const char *filepath, bool recurce);
    int copy_file(const char *dst, const char * const *src, size_t src_count, bool recurce);
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
#include <deque>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>
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

  /**
     何らかの理由で取り出せないときは空テキストが返る。
   */
  string File_Manager_Impl::getcwd() {
    char *pbuf = 0, *p;
    size_t plen = 1024;

    while ((p = (char *)realloc(pbuf, plen)) != NULL) {
      pbuf = p;
      char *wd = ::getcwd(pbuf, plen);
      if (wd == NULL) {
	if (errno == ERANGE) { plen += 1024; continue; }
	elog("getcwd ,%u:(%d):%s\n", plen, errno, strerror(errno));
	free(pbuf);
	return "";
      }
      string cwd(wd);
      free(pbuf);
      return cwd;
    }

    elog("realloc ,%u:(%d):%s\n",plen,errno,strerror(errno));
    free(pbuf);
    return "";
  }

  bool File_Manager_Impl::chdir(const char *dir) {
    if (0 == ::chdir(dir)) return true;
    elog("chdir %s:(%d):%s\n",dir,errno,strerror(errno));
    return false;
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
      elog(W, "stat %s:(%d):%s\n", dirpath, errno, strerror(errno));
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
      elog(W, "mkdir %s:(%d):%s", dirpath, errno, strerror(errno));
      return false;
    }
    elog(D, "mkdir %s:%d\n", dirpath, len);
    
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
      elog(T, "rmdir %s\n", dirpath);
      if (rmdir(dirpath) != 0) {
	elog("rmdir %s:(%d):%s\n", dirpath, errno, strerror(errno));
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

    struct stat sbuf;
    /*
      複製元のファイル属性を格納する構造体。
      あとでタイムスタンプの転記にも利用する。
     */
    if (stat(filepath, &sbuf) != 0) {
      elog("stat %s:(%d):%s\n",filepath,errno,strerror(errno));
      return 1;
    }

    if (!S_ISDIR(sbuf.st_mode)) {
      if (!S_ISREG(sbuf.st_mode)) {
	elog(W, "not regular file:%s\n",filepath);
	return 1;
      }

      if (unlink(filepath) != 0) {
	elog("unlink %s:(%d):%s\n", filepath, errno, strerror(errno));
	return 1;
      }
      elog(D, "unlink %s\n", filepath);
      return 0;
    }

    if (!recurce) {
      elog("directory cannot remove(try recurce option): %s\n",filepath);
      return 1;
    }

    // 以下、ディレクトリを走査して、ファイルを削除していく
    const char *dirpath = filepath;
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
      elog("opendir %s:(%d):%s\n", dirpath, errno, strerror(errno));
      return 1;
    }
    elog(T, "opendir %s\n", dirpath);
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
      elog(T, "closedir %s\n", dirpath);
    else {
      elog("closedir %s:(%d):%s\n", dirpath, errno, strerror(errno));
    }

    if (rc == 0) {
      // ここまでにエラーが生じていなければ、ディレクトリを削除する
      if (rmdir(dirpath) == 0)
	elog(D, "rmdir %s\n", dirpath);
      else {
	elog("rmdir %s:(%d):%s\n", dirpath, errno, strerror(errno));
	rc = 2;
      }
    }
    return rc;
  }

  // --------------------------------------------------------------------------------

  /// ファイルの複製を作る
  int File_Manager_Impl::copy_local_file(FILE *outfp, const char *src, long *outbytes) {
    FILE *infp = fopen(src,"r");
    if (!infp) {
      elog("fopen %s:(%d):%s\n", src, errno, strerror(errno));
      return 2;
    }

    if (outbytes) *outbytes = 0;

    char buf[BUFSIZ], *bptr;
    size_t len;
    while((len = fread(bptr = buf, 1, sizeof buf, infp)) > 0) {
      while (len > 0) {
	size_t n = fwrite(bptr,1,len,outfp);
	if (n == 0) break;
	if (n > len) {
	  elog(F, "Internal Error: fwrite: over size reaturn.\n");
	  abort();
	  break;
	}
	bptr += n; len -= n; 
	if (outbytes) *outbytes += n;
      }
    }
    if (fclose(infp) != 0) {
      elog("fclose %s:(%d):%s\n",src,errno,strerror(errno));
      return 3;
    }
    return 0;
  }

  /// ファイルの複製を作る
  /**
     複製対象は一般ファイルに限定。
     複製元のファイルのタイムスタンプと合わせる
     複製先が存在してディレクトリであれば、そのディレクトリに同名で複製する。
  */
  int File_Manager_Impl::copy_regular_file(const char *dst, const char *src) {
    if (!src || !*src || !dst || !*dst) {
      elog(W, "empty copy target");
      return 1;
    }

    struct stat sbuf;
    /*
      複製元のファイル属性を格納する構造体。
      あとでタイムスタンプの転記にも利用する。
     */
    if (stat(src, &sbuf) != 0) {
      elog("stat %s:(%d):%s\n",src,errno,strerror(errno));
      return 1;
    }

    if (!S_ISREG(sbuf.st_mode)) {
      elog(W, "not regular file:%s\n",src);
      return 1;
    }

    string dbuf(dst);
    if (isdir(dst)) {
      /*
	アクセスできるエントリが存在して、それはディレクトリである場合は
	そのディレクトリに同名で複製する。
      */
      size_t len = dbuf.size();
      if (dst[len - 1] != '/') dbuf += "/";
      dbuf += basename(src);
      dst = dbuf.c_str();
      if (0) elog(T, "destination change: %s\n", dst);
    }

    string part(dst);
    part += ".part";

    FILE *outfp = fopen(part.c_str(),"w");
    /*
      同名では複製しない。
      .part サフィックス付きで複製して、あとで名前を変更する。
      この対応により、正規の名前でしり切れの状態は存在しなくなる。
      またディレクトリに残る .part ファイルは部分ファイルであることがわかる。
     */
    if (!outfp) {
      elog("fopen %s,w:(%d):%s\n", part.c_str(), errno, strerror(errno));
      return 2;
    }
    long bytes = 0;

    int rc = copy_local_file(outfp,src,&bytes);

    if (fclose(outfp) != 0) {
      elog("fclose %s:(%d):%s\n", part.c_str(), errno,strerror(errno));
      rc = 3;
    }

    if (rc == 0) {
      if (rename(part.c_str(), dst) == 0) {
	elog(D, "copy to %s %ld bytes.\n", dst, bytes);

	struct utimbuf ubuf;
	ubuf.actime = sbuf.st_atime;
	ubuf.modtime = sbuf.st_mtime;
	if (utime(dst, &ubuf) != 0) {
	  elog(W, "timestamp copy %s:(%d):%s\n",dst,errno,strerror(errno));
	}
	return 0;
      }
      elog("rename %s:(%d):%s\n",dst,errno,strerror(errno));
      rc = 4;
    }

    if (unlink(part.c_str()) == 0)
      elog(T, "unlink %s\n", part.c_str());
    else {
      elog(W, "unlink %s:(%d):%s\n", part.c_str(),errno,strerror(errno));
      rc = 5;
    }
    return rc;
  }

  /// ディレクトリの複製
  /*
    複製元、複製先はいずれも存在するディレクトリである必要がある。
  */
  int File_Manager_Impl::copy_directory(const char *dstdir, const char *srcdir) {
    if (!isdir(srcdir)) {
      elog("not directory: %s\n",srcdir);
      return 1;
    }
    if (!isdir(dstdir)) {
      elog("not directory: %s\n",dstdir);
      return 1;
    }
    const char *dirpath = srcdir;
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
      elog("opendir %s:(%d):%s\n", dirpath, errno, strerror(errno));
      return 1;
    }
    elog(T, "opendir %s\n", dirpath);

    size_t dirnlen = strlen(srcdir);
    char *fpath = (char *)malloc(dirnlen + max_file_name_len + 2);
    if (!fpath) abort();
    strcat(strcpy(fpath, srcdir), "/"); dirnlen++;
    /*
      fpathはファイル・パスを組み立てる作業領域
    */
    int rc = 0;
    struct dirent *ent;
    struct stat sbuf;
    deque<string> dirs;

    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_name[0] == '.') {
	// 自身は対象から除外する
	if (ent->d_name[1] == 0) continue;
	// 親ディレクトリは対象から除外する
	if (ent->d_name[1] == '.' && ent->d_name[2] == 0) continue;
      }
      strcpy(fpath + dirnlen, ent->d_name);
      if (stat(fpath, &sbuf) != 0) {
	elog("stat %s:(%d):%s\n",fpath, errno, strerror(errno));
	break;
      }

      if (S_ISDIR(sbuf.st_mode)) {
	string dname = ent->d_name;
	dirs.push_back(dname);
	continue;
      }
      if (!S_ISREG(sbuf.st_mode)) {
	elog(W, "not regular file(ignored): %s\n", fpath);
	continue;
      }
      int rc0 = copy_regular_file(dstdir, fpath);
      if (rc0 != 0)  { rc = rc0; break; }
    }

    if (closedir(dir) == 0)
      elog(T, "closedir: %s\n", dirpath);
    else {
      elog("closedir %s:(%d):%s\n", dirpath, errno, strerror(errno));
    }
    
    while (!dirs.empty()) {
      string ent(dirs.front());
      strcpy(fpath + dirnlen, ent.c_str());

      string dstent(dstdir);
      if (dstent.c_str()[dstent.size() - 1] != '/') dstent += "/";
      dstent += ent;

      elog(T, "directory copying.. %s to %s\n", fpath, dstent.c_str());
      if (!mkdirs(dstent.c_str())) break;
      dirs.pop_front();

      int rc1 = copy_directory(dstent.c_str(), fpath);
      if (rc1 != 0) break;
    }
    free(fpath);
    return rc;
  }

  /// 一つのファイルあるいはディレクトリを複製する
  /*
    複製先の名称を変更することができる。
    ただしディレクトリを複製する場合、
    対象の名称のディレクトリがあると、それの中に複製される。
  */
  int File_Manager_Impl::copy_file_or_directory(const char *dst, const char *src, bool recurce) {
    // 複製元が１個の場合は、複製先がなくてもよい

    struct stat sbuf;
    if (stat(src, &sbuf) != 0) {
      elog("src not exist or not permission: %s:(%d):%s\n", src, errno, strerror(errno));
      return 2;
    }
  
    if (!S_ISDIR(sbuf.st_mode))
      return copy_regular_file(dst, src);
  
    if (!recurce) {
      elog("%s is directory(not recurce)\n",src);
      return 1;
    }

    // 複製元がディレクトリであれば、複製先を確認する
    if (stat(dst, &sbuf) != 0) {
      if (errno != ENOENT) {
	// 複製先が存在しない場合は作成して複製する
	if (!mkdirs(dst)) {
	  elog("%s not created or not directory\n",dst);
	  return 2;
	}
	return copy_directory(dst, src);
      }
      elog("stat %s:(%d):%s\n",dst,errno,strerror(errno));
      return 2;
    }
    
    if (!S_ISDIR(sbuf.st_mode)) {
      // 存在するなら複製先もディレクトリである必要がある
      elog("%s is not directory\n",dst);
      return 2;
    }

    // 複製先ディレクトリに同名のディレクトリを作成して複製する
    string dstdir(dst);
    if (dstdir.c_str()[dstdir.size() - 1] != '/') dstdir += "/";
    dstdir += basename(src);
  
    if (!mkdirs(dstdir.c_str())) {
      elog("%s not created or not directory\n", dstdir.c_str());
      return 2;
    }
    return copy_directory(dstdir.c_str(), src);
  }

  /// 一般ファイルやディレクトリを複製する
  /*
    ディレクトリも複製対象とする場合は, recurceにtrueを渡す必要がある。
  */
  int File_Manager_Impl::copy_file(const char *dst, const char * const *src, size_t count, bool recurce) {
    if (!dst || !src || !count) {
      elog(W, "invalid argument: copy_file");
      return 1;
    }

#if 0
    for(int i = 0; i < count; i++) {
      fprintf(stderr,"%d: %s\n",i,src[i]);
    }
#endif

    if (count == 1)
      return copy_file_or_directory(dst, src[0], recurce);

    // 複製元が複数存在する場合は、複製先はディレクトリであることを期待する
    if (!mkdirs(dst)) {
      elog("%s not created or not directory\n",dst);
      return 1;
    }

    struct stat sbuf;
    int rc = 0;
    for(size_t i = 0; i < count; i++) {
      if (stat(src[i], &sbuf) != 0) {
	elog("%s not exist or not permission:(%d):%s\n",src, errno, strerror(errno));
	rc = 1;
	continue;
      }
      if (!S_ISDIR(sbuf.st_mode)) {
	int rc1 = copy_regular_file(dst, src[i]);
	if (rc1) rc = rc1;
      }
      else if (!recurce) {
	elog("%s is directory(not recurce)\n", src[i]);
	continue;
      } 
      else {
	string dstdir(dst);
	if (dstdir.c_str()[dstdir.size() - 1] != '/') dstdir += "/";
	dstdir += basename(src[i]);

	if (!mkdirs(dstdir.c_str())) {
	  elog("%s not created or not directory\n",dstdir.c_str());
	  continue;
	}
	int rc0 = copy_directory(dstdir.c_str(), src[i]);
	if (rc0) rc = rc0;
      }
    }
    return rc;
  }

  // --------------------------------------------------------------------------------

  /// ファイルを移動する
  /**
     移動対象がディレクトリであることを想定しない。
     ファイル・システムが異なる場合は、複製と削除によって移動する
     その場合は複製元のファイルのタイムスタンプを合わせようとする
     移動先が存在してディレクトリであれば、そのディレクトリに移動する。
  */
  int File_Manager_Impl::move_regular_file(const char *dst, const char *src) {
    int rc = rename(src, dst);
    if (rc == 0) {
      elog(D, "move to %s\n", dst);
      return 0;
    }
    
    if (errno != EXDEV) {
      elog("rename %s:(%d):%s\n", src, errno, strerror(errno));
      return 1;
    }
    rc = copy_regular_file(dst, src);
    if (rc != 0) return 1;
    
    if (unlink(src) == 0) {
      elog(D, "move to %s (copy and unlink)\n", dst);
      return 0;
    }
    elog(W, "copyed but unlink failed: %s:(%d):%s\n", src, errno, strerror(errno));
    return 1;
  }

  /// 一般ファイルを移動する
  int File_Manager_Impl::move_file(const char *dst, const char * const *src, size_t src_count) {
    elog("NOT IMPLEMENTED YET.\n");
    return 0;
  }

};

// --------------------------------------------------------------------------------

#include <iostream>

namespace uc {

  static File_Manager *create_File_Manager() {
    return new File_Manager_Impl();
  }

  /// ディレクトリ名入手のテスト
  static int cmd_basename(int argc,char **argv) {
    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <file> .." << endl;
      return 1;
    }

    auto_ptr<File_Manager> fm(create_File_Manager());

    for (int i = 1; i < argc; i++) {
      cout << fm->basename(argv[i]) << endl;
    }
    return 0;
  }

  /// ディレクトリ名の入手のテスト
  static int cmd_dirname(int argc,char **argv) {

    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <file> .." << endl;
      return 1;
    }

    auto_ptr<File_Manager> fm(create_File_Manager());

    for (int i = 1; i < argc; i++) {
      cout << fm->dirname(argv[i]) << endl;
    }
    return 0;
  }

  /** cd/getcwdの試験
   */
  static int cmd_pwd(int argc,char **argv) {

    auto_ptr<File_Manager> fm(create_File_Manager());

    if (argc > 1) fm->chdir(argv[1]);

    string cwd = fm->getcwd();
    cout << cwd << endl;
    return cwd.size() > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  /** mkdirsの試験
   */
  static int cmd_mkdirs(int argc,char **argv) {

    if (argc <= 1) {
      cerr << "usage: " << argv[0] << " <dir> .." << endl;
      return 1;
    }

    auto_ptr<File_Manager> fm(create_File_Manager());

    for (int i = 1; i < argc; i++) {
      if (fm->mkdirs(argv[i]))
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

    auto_ptr<File_Manager> fm(create_File_Manager());

    int rc = 0;
    for (int i = 1; i < argc; i++) {
      int rc0 = fm->rmdirs(argv[i]);
      if (rc0 == 0)
	cerr << "INFO: " << argv[i] << " ok" << endl;
      else
	rc = rc0;
    }
    return rc;
  }

  /// ファイルの削除の実験
  static int cmd_remove_file(int argc,char **argv) {

    int opt;
    bool recurce = false, verbose = false;

    while ((opt = getopt(argc,argv,"rv")) != -1) {
      switch(opt) { 
      case 'r': recurce = true; break;
      case 'v': verbose = true; break;
      }
    }

    if (argc - optind < 1) {
      cerr << "usage: " << argv[0] << " [-r] <file> .." << endl;
      return 1;
    }

    auto_ptr<File_Manager> fm(create_File_Manager());
    int rc = 0;
    
    for (int i = optind; i < argc; i++) {
      int rc0 = fm->remove_file(argv[i], recurce);
      if(rc0) rc = rc0;
    }
    return rc;
  }

  /// ファイルの複製のテスト
  static int cmd_copy_file(int argc,char **argv) {
    int opt;
    bool recurce = false, verbose = false;

    while ((opt = getopt(argc,argv,"rv")) != -1) {
      switch(opt) { 
      case 'r': recurce = true; break;
      case 'v': verbose = true; break;
      }
    }

    if (argc - optind > 1) {
      auto_ptr<File_Manager> fm(create_File_Manager());
      return fm->copy_file(argv[argc - 1], argv + optind, argc - optind - 1, recurce);
    }
    
    cerr << "usage: " << argv[0] << " [-r] <file> .. <dst>" << endl;
    return 1;
  }

  /// ファイルの複製のテスト
  static int cmd_move_file(int argc,char **argv) {
    int opt;
    bool verbose = false;

    while ((opt = getopt(argc,argv,"v")) != -1) {
      switch(opt) { case 'v': verbose = true; }
    }

    auto_ptr<File_Manager> fm(create_File_Manager());

    if (argc - optind > 1) {
      return fm->move_file(argv[argc - 1], argv + optind, argc - optind - 1);
    }
    
    cerr << "usage: " << argv[0] << " [-v] <file> .. <dst>" << endl;
    return 1;
  }

};


// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd fileop_cmap[] = {
  { "basename", uc::cmd_basename, },
  { "dirname", uc::cmd_dirname, },
  { "pwd", uc::cmd_pwd, },
  { "mkdirs", uc::cmd_mkdirs, },
  { "rmdirs", uc::cmd_rmdirs, },
  { "rm", uc::cmd_remove_file, },
  { "cp", uc::cmd_copy_file, },
  { "mv", uc::cmd_move_file, },
  { 0 },
};

#endif

