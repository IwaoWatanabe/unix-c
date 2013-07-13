/*! \file
 * \brief UNIXのユーザ/グループ情報を検索する
 */

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

namespace uc {

  /// UNIXのユーザ/グループ情報を検索する
  /**
     ユーザ情報は頻繁に更新されるものではないので、読み取ったエントリはキャッシュする。
     そのため利用者は適当なタイミングでフラッシュする必要がある。

     構造体のデータは、フラッシュするまで有効である。

     passwd 構造体は、<pwd.h> で以下のように定義されている:

     struct passwd {
     char   *pw_name;       // ユーザ名
     char   *pw_passwd;     // ユーザのパスワード
     uid_t   pw_uid;        // ユーザ ID
     gid_t   pw_gid;        // グループ ID
     char   *pw_gecos;      // 実名
     char   *pw_dir;        // ホームディレクトリ
     char   *pw_shell;      // シェルプログラム
     };

     group 構造体は、<grp.h> で以下のように定義されている:

     struct group {
     char   *gr_name;       // グループ名
     char   *gr_passwd;     // グループのパスワード
     gid_t   gr_gid;        // グループ ID
     char  **gr_mem;        // グループのメンバ
     };

     通常は /etc/passwd 、/etc/group を照会するが
     OSがNISやLDAPと連携するように調整されていると
     ネットワーク通信が生じることになる点を留意すること。
  */

  struct User_Info {
    virtual ~User_Info();
    /// ユーザ名を元にユーザ情報を入手する
    virtual struct passwd *getpwnam(const char *name) = 0;
    /// ユーザIDを元にユーザ情報を入手する
    virtual struct passwd *getpwuid(uid_t uid) = 0;
    /// getpwent の呼び出しを開始する
    virtual void setpwent(bool cache_only = false) = 0;
    /// ユーザ情報を走査して入手する
    virtual struct passwd *getpwent(const char *name_prefix = "") = 0;
    /// getpwent の呼び出しを終了する
    virtual int endpwent(void) = 0;
    /// グループ名を元にグループ情報を入手する
    virtual struct group *getgrnam(const char *name) = 0;
    /// グループIDを元にグループ情報を入手する
    virtual struct group *getgrgid(gid_t gid) = 0;
    /// getgrent の呼び出しを開始する
    virtual void setgrent(bool cache_scan = false) = 0;
    /// グループ情報を走査して入手する
    virtual struct group *getgrent(const char *name_prefix = "") = 0;
    /// getgrent の呼び出しを終了する
    virtual int endgrent(void) = 0;
    /// 保持しているキャッシュ情報を開放する
    virtual void release() = 0;

    static User_Info *get_instance(const char *impl = "");
  };
};

#include <map>
#include <string>
#include "uc/elog.hpp"

using namespace std;

namespace {

  class User_Info_Impl : public uc::User_Info, uc::ELog {

    map<string, struct passwd *> pwent;
    map<uid_t, struct passwd *> pwent_uid;
    map<string, struct group *> grent;
    map<uid_t, struct group *> grent_gid;

    struct passwd *copy_pwent(struct passwd *pw);
    struct group *copy_grent(struct group *gr);

    map<string, struct passwd *>::iterator pw_scan;
    map<string, struct group *>::iterator gr_scan;
    int pw_count, gr_count;

  public:
    User_Info_Impl() : pw_scan(0), gr_scan(0), pw_count(0), gr_count(0) {
      init_elog("User_Info_Impl");
    }

    ~User_Info_Impl();

    struct passwd *getpwnam(const char *name);
    struct passwd *getpwuid(uid_t uid);
    void setpwent(bool cache_scan = false);
    struct passwd *getpwent(const char *name_prefix = "");
    int endpwent(void);
    struct group *getgrnam(const char *name);
    struct group *getgrgid(gid_t gid);
    void setgrent(bool cache_scan = false);
    struct group *getgrent(const char *name_prefix = "");
    int endgrent(void);
    void release();
  };

  User_Info_Impl::~User_Info_Impl() { release(); }

  /// pwent の複製。free 一つで開放できるように配置している
  struct passwd *User_Info_Impl::copy_pwent(struct passwd *pw) {
    size_t len = sizeof *pw;
    size_t p1 = strlen(pw->pw_name) + 1,
      p2 = strlen(pw->pw_passwd) + 1,
      p3 = strlen(pw->pw_gecos) + 1,
      p4 = strlen(pw->pw_dir) + 1,
      p5 = strlen(pw->pw_shell) + 1;

    len += p1;
    len += p2;
    len += p3;
    len += p4;
    len += p5;

    struct passwd *ent = (struct passwd *)malloc(len);
    if (ent) {
      char *ptr = (char *)&ent[1];
      ent->pw_name = strcpy(ptr, pw->pw_name);
      ent->pw_passwd = strcpy(ptr += p1, pw->pw_passwd);
      ent->pw_gecos = strcpy(ptr += p2, pw->pw_gecos);
      ent->pw_dir = strcpy(ptr += p3, pw->pw_dir);
      ent->pw_shell = strcpy(ptr += p4, pw->pw_shell);
      ent->pw_uid = pw->pw_uid;
      ent->pw_gid = pw->pw_gid;
    }
    return ent;
  }

  /// グループのメンバ名を保持するために必要なメモリサイズを求める。
  static size_t count_member(const char * const * mem, size_t *len) {
    size_t ct = 1;
    *len = 0;
    if (mem) {
      while (*mem) {
	len += strlen(*mem++) + 1;
	ct++;
      }
    }
    return ct;
  }

  /// grent の複製。free 一つで開放できるように配置している
  struct group *User_Info_Impl::copy_grent(struct group *gr) {
    size_t len = sizeof *gr;
    size_t p1 = strlen(gr->gr_name) + 1,
      p2 = strlen(gr->gr_passwd) + 1;
    size_t p3;
    size_t mct = count_member(gr->gr_mem, &p3);

    len += p1;
    len += p2;
    len += p3;
    len += mct * sizeof ( char * );

    struct group *ent = (struct group *)malloc(len);
    if (ent) {
      char **pptr = (char **)&ent[1], **mem = gr->gr_mem;
      char *ptr = (char *)(pptr + mct);
      ent->gr_name = strcpy(ptr, gr->gr_name);
      ent->gr_passwd = strcpy(ptr += p1, gr->gr_passwd);
      ptr += p2;

      ent->gr_gid = gr->gr_gid;
      ent->gr_mem = pptr;

      while (*mem) {
	*pptr = strcpy(ptr, *mem++);
	ptr += strlen(*pptr++) + 1;
      }
      *pptr = 0;
    }
    return ent;
  }


  /// ユーザ名を元にユーザ情報を入手する
  struct passwd *User_Info_Impl::getpwnam(const char *name) {
    map<string, struct passwd *>::iterator it = pwent.find(name);
    if (it != pwent.end()) return it->second;

    struct passwd *ent = ::getpwnam(name);
    if (!ent) return 0;

    ent = copy_pwent(ent);
    pwent.insert(map<string, struct passwd *>::value_type(name, ent));
    pwent_uid.insert(map<uid_t, struct passwd *>::value_type(ent->pw_uid, ent));

    return ent;
  }

  /// ユーザIDを元にユーザ情報を入手する
  struct passwd *User_Info_Impl::getpwuid(uid_t uid) {
    map<uid_t, struct passwd *>::iterator it = pwent_uid.find(uid);
    if (it != pwent_uid.end()) return it->second;

    struct passwd *ent = ::getpwuid(uid);
    if (!ent) return 0;

    ent = copy_pwent(ent);
    pwent.insert(map<string, struct passwd *>::value_type(ent->pw_name, ent));
    pwent_uid.insert(map<uid_t, struct passwd *>::value_type(uid, ent));

    return ent;
  }

  /// getpwent の呼び出しを開始する
  void User_Info_Impl::setpwent(bool cache_scan) {
    ::setpwent();
    pw_count = 0;
  }

  /// ユーザ情報を走査して入手する
  /**
     キャッシュされていれば、そのエントリを返す。
     されてなければ、登録してエントリを返す。
   */
  struct passwd *User_Info_Impl::getpwent(const char *name_prefix) {

    struct passwd *pw = ::getpwent();
    if (!pw) return pw;

    pw_count++;

    map<string, struct passwd *>::iterator it = pwent.find(pw->pw_name);
    if (it != pwent.end()) {
      if (it->second->pw_uid == pw->pw_uid) return it->second;
    }
    else {
      struct passwd *ent = copy_pwent(pw);
      pwent.insert(map<string, struct passwd *>::value_type(ent->pw_name, ent));
      pwent_uid.insert(map<uid_t, struct passwd *>::value_type(ent->pw_uid, ent));
      return ent;
    }

    map<uid_t, struct passwd *>::iterator it2 = pwent_uid.find(pw->pw_uid);
    if (it2 != pwent_uid.end()) return it2->second;

    struct passwd *ent = copy_pwent(pw);
    pwent_uid.insert(map<uid_t, struct passwd *>::value_type(ent->pw_uid, ent));
    return ent;
  }

  /// getpwent の呼び出しを終了する
  int User_Info_Impl::endpwent(void) {
    ::endpwent();
    return pw_count;
  }

  /// グループ名を元にグループ情報を入手する
  struct group *User_Info_Impl::getgrnam(const char *name) {
    return 0;
  }

  /// グループIDを元にグループ情報を入手する
  struct group *User_Info_Impl::getgrgid(gid_t gid) {
    return 0;
  }

  /// getgrent の呼び出しを開始する
  void User_Info_Impl::setgrent(bool cache_scan) {
    gr_count = 0;
  }

  /// グループ情報を走査して入手する
  struct group *User_Info_Impl::getgrent(const char *name_prefix) {
    gr_count++;
    return 0;
  }

  /// getgrent の呼び出しを終了する
  int User_Info_Impl::endgrent(void) {
    return gr_count;
  }

  /// 保持しているキャッシュ情報を開放する
  void User_Info_Impl::release() {

    int ct = 0;

    {
      map<string, struct passwd *>::iterator it = pwent.begin();
      while (it != pwent.end()) {
	map<uid_t, struct passwd *>::iterator it2 = pwent_uid.find(it->second->pw_uid);
	if (it2 != pwent_uid.end()) {
	  if (it2->second == it->second) pwent_uid.erase(it2);
	}
	pwent.erase(it++);
	ct++;
      }
    }

    {
      map<uid_t, struct passwd *>::iterator it = pwent_uid.begin();
      while (it != pwent_uid.end()) { pwent_uid.erase(it++); ct++; }
    }

    elog(T, "%d entries released.\n", ct);
  }

};

namespace uc {

  User_Info *User_Info::get_instance(const char *impl) {
    static User_Info_Impl *ui = new User_Info_Impl();
    return ui;
  }

  User_Info::~User_Info() { }

};

#include <cctype>

enum { I = uc::ELog::I };

/// ユーザ・エントリの検索
static int getpwent01(int argc, char **argv) {

  uc::User_Info *ent = uc::User_Info::get_instance();
  int rc = 0;

  optind = 1;

  if (optind == argc) {
    passwd *pwent;
    ent->setpwent();
    while ((pwent = ent->getpwent())) {
      if (putpwent(pwent,stdout) != 0) {
	perror("ERROR: putpwent");
	rc = 2;
	break;
      }
    }

    elog(I, "%d user entries\n", ent->endpwent());
    ent->release();

    return rc;
  }

  for (int i = optind; i < argc; i++) {
    const char *arg = argv[i];
    passwd *pwent;

    if (!isdigit(*arg))
      pwent = ent->getpwnam(arg);
    else {
      uid_t uid = (uid_t)atol(arg);
      pwent = ent->getpwuid(uid);
    }

    if (!pwent) { rc = 1; continue; }

    if (putpwent(pwent,stdout) != 0) {
      perror("ERROR: putpwent");
      rc = 2;
      break;
    }
  }

  ent->release();

  return rc;
}

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd user_cmap[] = {
  { "getpwent", getpwent01, },
  { 0 },
};

#endif
