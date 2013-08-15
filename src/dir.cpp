/*! \file
 * \brief ディレクトリ・サービスの基本操作の実験
 */

#include <map>
#include <string>
#include <vector>

namespace uc {

  /// LDAPの基本接続情報を入手する
  struct Dir_Info {
    virtual ~Dir_Info() = 0;
    virtual const char *get_dir_name() = 0;
    virtual const char *get_dir_host() = 0;
    virtual int get_dir_port() = 0;
    virtual const char *get_bind_dn() = 0;
    virtual const char *get_bind_password() = 0;
  };

  class Directory {
  };

  class Directory_Manager {
  public:
    static Directory_Manager *get_instance(const char *name = "");

    /// 登録済み接続名の入手
    virtual void get_dir_names(std::vector<std::string> &name_list) = 0;
    /// 接続情報の保存
    virtual void store_dir_parameter(const char *name, const std::map<std::string,std::string> &params) = 0;
    /// 接続情報の入手
    virtual bool fetch_dir_parameter(const char *name, std::map<std::string,std::string> &params) = 0;
    /// 接続済みのDB接続を得る
    virtual Directory *bind(const char *name) = 0;

    virtual ~Directory_Manager();

  protected:
    Directory_Manager();
    /// 接続情報の入手
    virtual Dir_Info *get_Dir_Info(const char *name) = 0;
  };

};

#include "uc/local-file.hpp"
#include "uc/elog.hpp"
#include <ldap.h>
#include <unistd.h>

using namespace std;
using namespace uc;

namespace {

  /// LDAPの接続情報を保持する
  class My_Dir_Info : public Dir_Info {
    int name,user,passwd,socket,host,port;
    string sbuf;
  };

  static int cmd_dir(int argc, char **argv) {
    printf("%s\n", argv[0]);
    return 0;
  }

  static int show_entry(LDAPMessage *res, LDAP *ld) {
    if (!res || !ld) return 1;
    int ct = ldap_count_entries(ld, res);

    fprintf(stderr,"INFO: total result: %d\n", ct);

    return 0;
  }

  // OpenLDAP API の実験コード
  static int cmd_dir02(int argc, char **argv) {
    const char *dir_server = "localhost";
    const char *bind_dn = "cn=Manager,dc=my-domain,dc=com";
    const char *bind_pass = "secret";
    int dir_port = 389;
    int dir_scope = LDAP_SCOPE_SUBTREE;
    const char *base_dn = "dc=my-domain,dc=com";
    const char *filter = "(objectclass=*)";
    char *attrs[] = { NULL, };

    int opt;
    while ((opt = getopt(argc,argv,"h:p:s:D:w:")) != -1) {
      switch(opt) {
      case 'h': dir_server = optarg; break;
      case 'p': dir_port = atoi(optarg); break;
      case 'D': bind_dn = optarg; break;
      case 'w': bind_pass = optarg; break;
      case 's':
	if (strcasecmp("sub", optarg) == 0)
	  dir_scope = LDAP_SCOPE_SUBTREE;
	else if (strcasecmp("once", optarg) == 0)
	  dir_scope = LDAP_SCOPE_ONE;
	else if (strcasecmp("base", optarg) == 0)
	  dir_scope = LDAP_SCOPE_BASE;
	break;
      }
    }

    LDAP *ld;
    if ((ld = ldap_init(dir_server, dir_port)) == NULL) {
      perror("ldap_init");
      return 1;
    }

    int value = LDAP_VERSION3;
    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &value);

    /// 同期メソッドを使ってLDAPサーバに接続する
    int rc = ldap_simple_bind_s(ld, bind_dn, bind_pass);
    if (rc) {
      fprintf(stderr,"ERROR: ldap_simple_bind_s %s:(%d):%s\n", bind_dn, rc, ldap_err2string(rc));
      return 1;
    }

    fprintf(stderr,"INFO: bind %s\n", bind_dn);

    LDAPMessage *res;
    rc = ldap_search_ext_s(ld, base_dn, dir_scope, filter, attrs, 0, 0, 0, LDAP_NO_LIMIT, 0, &res);
    if (rc != LDAP_SUCCESS) {
      fprintf(stderr,"ERROR: ldap_searc_ext_s %s:(%d):%s\n", base_dn, rc, ldap_err2string(rc));
    }
    else {
      show_entry(res, ld);
    }

    rc = ldap_unbind_s(ld);
    if (rc) {
      fprintf(stderr,"ERROR: ldap_unbind_s %p:(%d):%s\n", ld, rc, ldap_err2string(rc));
    }

    fprintf(stderr,"INFO: unbind %s\n", bind_dn);

    return 0;
  }

};

// --------------------------------------------------------------------------------


#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd dir_cmap[] = {
  { "dir", cmd_dir02, },
  { 0 },
};

#endif

