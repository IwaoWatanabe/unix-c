#ifndef uc_userinf_hpp
#define uc_userinf_hpp

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
     NISやLDAPと連携するように調整されていると
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

    static User_Info *create_instance(const char *impl = "");
  };
};

#endif

