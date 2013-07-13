/*! \file
 * \brief UNIXのユーザ/グループ情報を検索する
 */

#include "uc/userinfo.hpp"
#include "uc/elog.hpp"

#include <map>
#include <string>

using namespace std;

namespace {

  class User_Info_Impl : public uc::User_Info, uc::ELog {

    map<string, struct passwd *> pwent;
    map<uid_t, struct passwd *> pwent_uid;
    map<string, struct group *> grent;
    map<gid_t, struct group *> grent_gid;

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

  // --------------------------------------------------------------------------------

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
	*len += strlen(*mem++) + 1;
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
    len += mct * sizeof ( char * );
    len += p1;
    len += p2;
    len += p3;

    struct group *ent = (struct group *)malloc(len);
    if (ent) {
      char **pptr = (char **)&ent[1], **mem = gr->gr_mem;
      char *ptr = (char *)(pptr + mct);
      ent->gr_mem = pptr;
      ent->gr_name = strcpy(ptr, gr->gr_name);
      ent->gr_passwd = strcpy(ptr += p1, gr->gr_passwd);
      ptr += p2;

      ent->gr_gid = gr->gr_gid;

      while (*mem) {
	*pptr = strcpy(ptr, *mem++);
	ptr += strlen(*pptr++) + 1;
      }
      *pptr = 0;
    }
    return ent;
  }


  // --------------------------------------------------------------------------------

  /// ユーザ名を元にユーザ情報を入手する
  struct passwd *User_Info_Impl::getpwnam(const char *name) {
    map<string, struct passwd *>::iterator it = pwent.find(name);
    if (it != pwent.end()) return it->second;

    struct passwd *ent = ::getpwnam(name);
    if (!ent) return 0;

    struct passwd *uent = copy_pwent(ent);
    pwent.insert(map<string, struct passwd *>::value_type(name, uent));
    pwent_uid.insert(map<uid_t, struct passwd *>::value_type(uent->pw_uid, uent));
    return uent;
  }

  /// ユーザIDを元にユーザ情報を入手する
  struct passwd *User_Info_Impl::getpwuid(uid_t uid) {
    map<uid_t, struct passwd *>::iterator it = pwent_uid.find(uid);
    if (it != pwent_uid.end()) return it->second;

    struct passwd *ent = ::getpwuid(uid);
    if (!ent) return 0;

    struct passwd *uent = copy_pwent(ent);
    pwent.insert(map<string, struct passwd *>::value_type(uent->pw_name, uent));
    pwent_uid.insert(map<uid_t, struct passwd *>::value_type(uid, uent));
    return uent;
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

  // --------------------------------------------------------------------------------

  /// グループ名を元にグループ情報を入手する
  struct group *User_Info_Impl::getgrnam(const char *name) {
    map<string, struct group *>::iterator it = grent.find(name);
    if (it != grent.end()) return it->second;

    struct group *ent = ::getgrnam(name);
    if (!ent) return 0;

    struct group *gent = copy_grent(ent);
    grent.insert(map<string, struct group *>::value_type(name, gent));
    grent_gid.insert(map<gid_t, struct group *>::value_type(gent->gr_gid, gent));
    return gent;
  }

  /// グループIDを元にグループ情報を入手する
  struct group *User_Info_Impl::getgrgid(gid_t gid) {
    map<uid_t, struct group *>::iterator it = grent_gid.find(gid);
    if (it != grent_gid.end()) return it->second;

    struct group *ent = ::getgrgid(gid);
    if (!ent) return 0;

    struct group *gent = copy_grent(ent);
    grent.insert(map<string, struct group *>::value_type(gent->gr_name, gent));
    grent_gid.insert(map<gid_t, struct group *>::value_type(gid, gent));
    return gent;
  }

  /// getgrent の呼び出しを開始する
  void User_Info_Impl::setgrent(bool cache_scan) {
    ::setgrent();
    gr_count = 0;
  }

  /// グループ情報を走査して入手する
  struct group *User_Info_Impl::getgrent(const char *name_prefix) {
    struct group *gr = ::getgrent();
    if (!gr) return gr;

    gr_count++;

    map<string, struct group *>::iterator it = grent.find(gr->gr_name);
    if (it != grent.end()) {
      if (it->second->gr_gid == gr->gr_gid) return it->second;
    }
    else {
      struct group *ent = copy_grent(gr);
      grent.insert(map<string, struct group *>::value_type(ent->gr_name, ent));
      grent_gid.insert(map<gid_t, struct group *>::value_type(ent->gr_gid, ent));
      return ent;
    }

    map<gid_t, struct group *>::iterator it2 = grent_gid.find(gr->gr_gid);
    if (it2 != grent_gid.end()) return it2->second;

    struct group *ent = copy_grent(gr);
    grent_gid.insert(map<gid_t, struct group *>::value_type(ent->gr_gid, ent));
    return ent;
  }

  /// getgrent の呼び出しを終了する
  int User_Info_Impl::endgrent(void) {
    ::endgrent();
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
	free(it->second);
	pwent.erase(it++);
	ct++;
      }
    }

    {
      map<uid_t, struct passwd *>::iterator it = pwent_uid.begin();
      while (it != pwent_uid.end()) {
	free(it->second);
	pwent_uid.erase(it++);
	ct++;
      }
    }


    {
      map<string, struct group *>::iterator it = grent.begin();
      while (it != grent.end()) {
	map<gid_t, struct group *>::iterator it2 = grent_gid.find(it->second->gr_gid);
	if (it2 != grent_gid.end()) {
	  if (it2->second == it->second) grent_gid.erase(it2);
	}
	free(it->second);
	grent.erase(it++);
	ct++;
      }
    }

    {
      map<gid_t, struct group *>::iterator it = grent_gid.begin();
      while (it != grent_gid.end()) {
	grent_gid.erase(it++);
	free(it->second);
	ct++;
      }
    }

    elog(T, "%d cache entries released.\n", ct);
  }

};

namespace uc {

  User_Info *User_Info::create_instance(const char *impl) {
    User_Info_Impl *ui = new User_Info_Impl();
    return ui;
  }

  User_Info::~User_Info() { }

};

#include <cctype>
#include <memory>

enum { I = uc::ELog::I };

/// ユーザ・エントリの検索
static int getpwent01(int argc, char **argv) {

  auto_ptr <uc::User_Info> ent(uc::User_Info::create_instance());
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

  return rc;
}


/// グループ・エントリの検索
static int getgrent01(int argc, char **argv) {

  auto_ptr<uc::User_Info> ent(uc::User_Info::create_instance());
  int rc = 0;

  optind = 1;

  if (optind == argc) {
    group *grent;
    ent->setgrent();
    while ((grent = ent->getgrent())) {
      if (putgrent(grent,stdout) != 0) {
	perror("ERROR: putgrent");
	rc = 2;
	break;
      }
    }

    elog(I, "%d group entries\n", ent->endgrent());

    return rc;
  }

  for (int i = optind; i < argc; i++) {
    const char *arg = argv[i];
    group *grent;

    if (!isdigit(*arg))
      grent = ent->getgrnam(arg);
    else {
      gid_t gid = (gid_t)atol(arg);
      grent = ent->getgrgid(gid);
    }

    if (!grent) { rc = 1; continue; }

    if (putgrent(grent,stdout) != 0) {
      perror("ERROR: putgrent");
      rc = 2;
      break;
    }
  }

  return rc;
}

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd user_cmap[] = {
  { "pwent", getpwent01, },
  { "grent", getgrent01, },
  { 0 },
};

#endif
