/*! \file
 * \brief .INIファイルを取り扱うAPI
 */

#include "ini.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <set>
#include <iostream>

using namespace std;

namespace {

  // New BSD License
  // https://code.google.com/p/inih/source/browse/trunk/ini.c


#ifndef INI_ALLOW_MULTILINE
#define INI_ALLOW_MULTILINE 1
#endif

  /* Nonzero to allow a UTF-8 BOM sequence (0xEF 0xBB 0xBF) at the start of
     the file. See http://code.google.com/p/inih/issues/detail?id=21 */
#ifndef INI_ALLOW_BOM
#define INI_ALLOW_BOM 1
#endif

  /* Nonzero to use stack, zero to use heap (malloc/free). */
#ifndef INI_USE_STACK
#define INI_USE_STACK 1
#endif

  /* Maximum line length for any line in INI file. */
#ifndef INI_MAX_LINE
#define INI_MAX_LINE 512
#endif

#define MAX_SECTION 50
#define MAX_NAME 50

  /* Strip whitespace chars off end of given string, in place. Return s. */
  static char* rstrip(char* s)
  {
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
      *p = '\0';
    return s;
  }

  /* Return pointer to first non-whitespace char in given string. */
  static char* lskip(const char* s)
  {
    while (*s && isspace((unsigned char)(*s)))
      s++;
    return (char*)s;
  }
  
  /* Return pointer to first char c or ';' comment in given string, or pointer to
     null at end of string if neither found. ';' must be prefixed by a whitespace
     character to register as a comment. */
  static char* find_char_or_comment(const char* s, char c)
  {
    int was_whitespace = 0;
    while (*s && *s != c && !(was_whitespace && *s == ';')) {
      was_whitespace = isspace((unsigned char)(*s));
      s++;
    }
    return (char*)s;
  }
  
  /* Version of strncpy that ensures dest (size bytes) is null-terminated. */
  static char* strncpy0(char* dest, const char* src, size_t size)
  {
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
  }
  
  /* See documentation in header file. */
  int ini_parse_file(FILE* file,
		     int (*handler)(void*, const char*, const char*, const char*),
		     void* user)
  {
    /* Uses a fair bit of stack (use heap instead if you need to) */
#if INI_USE_STACK
    char line[INI_MAX_LINE];
#else
    char* line;
#endif
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";

    char* start;
    char* end;
    char* name;
    char* value;
    int lineno = 0;
    int error = 0;
    
#if !INI_USE_STACK
    line = (char*)malloc(INI_MAX_LINE);
    if (!line) {
      return -2;
    }
#endif

    /* Scan through file line by line */
    while (fgets(line, INI_MAX_LINE, file) != NULL) {
      lineno++;
      
      start = line;
#if INI_ALLOW_BOM
      if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
	  (unsigned char)start[1] == 0xBB &&
	  (unsigned char)start[2] == 0xBF) {
	start += 3;
      }
#endif
      start = lskip(rstrip(start));

      if (*start == ';' || *start == '#') {
	/* Per Python ConfigParser, allow '#' comments at start of line */
      }
#if INI_ALLOW_MULTILINE
      else if (*prev_name && *start && start > line) {
	/* Non-black line with leading whitespace, treat as continuation
	   of previous name's value (as per Python ConfigParser). */
	if (!handler(user, section, prev_name, start) && !error)
	  error = lineno;
      }
#endif
      else if (*start == '[') {
	/* A "[section]" line */
	  end = find_char_or_comment(start + 1, ']');
	  if (*end == ']') {
	    *end = '\0';
	    strncpy0(section, start + 1, sizeof(section));
	    *prev_name = '\0';
	  }
	  else if (!error) {
	    /* No ']' found on section line */
	    error = lineno;
	  }
      }
      else if (*start && *start != ';') {
	/* Not a comment, must be a name[=:]value pair */
	end = find_char_or_comment(start, '=');
	if (*end != '=') {
	  end = find_char_or_comment(start, ':');
	}
	if (*end == '=' || *end == ':') {
	  *end = '\0';
	  name = rstrip(start);
	  value = lskip(end + 1);
	  end = find_char_or_comment(value, '\0');
	  if (*end == ';')
	    *end = '\0';
	  rstrip(value);
	  
	  /* Valid name[=:]value pair found, call handler */
	  strncpy0(prev_name, name, sizeof(prev_name));
	  if (!handler(user, section, name, value) && !error)
	    error = lineno;
	}
	else if (!error) {
	  /* No '=' or ':' found on name[=:]value line */
	  error = lineno;
	}
      }
    }
   
#if !INI_USE_STACK
    free(line);
#endif
    return error;
  }

  class INI_LoaderImpl : public uc::INI_Loader {
    std::map<std::string, std::map<std::string,std::string> > section_map;
    static int handler(void *closure, const char*section, const char*name, const char*value);

  public:
    INI_LoaderImpl() {}
    ~INI_LoaderImpl() {}

    bool set_ini_filename(const char *file_name);
    void set_section(const char *name);
    void set_default_section(const char *name);
    void fetch_section_names(std::vector<std::string> &names);
    void fetch_config_names(std::vector<std::string> &names, const char *section = 0);
    std::string get_config_value(const char *name, const char *section = 0);
  };

  int INI_LoaderImpl::handler(void *closure, const char*section, const char*name, const char*value) {
    INI_LoaderImpl *ini  = (INI_LoaderImpl *)closure;
    string xsection(section), xname(name);
    map<string, string> &conf = ini->section_map[xsection];
    conf[xname] = value;

    if (section[0] && ini->current_section.empty())
      ini->current_section = section;

    return 1;
  }

  // 読み取り対象のiniファイルを設定する
  bool INI_LoaderImpl::set_ini_filename(const char *file_name)
  {
    assert(file_name);
    if (ini_file == file_name) return true;

    FILE *fp = fopen(file_name,"r");
    if (!fp) {
      fprintf(stderr,"ERROR: fopen %s for read ini file failed:(%d) %s\n", 
	      file_name, errno, strerror(errno));
      return false;
    }

    int err = ini_parse_file(fp, handler, this);

    if (fclose(fp) != 0) {
      fprintf(stderr,"WARNING: fclose %s failed:(%d) %s\n", 
	      file_name, errno, strerror(errno));
    }

    if (err) return false;

    ini_file = file_name;
    return true;
  }

  void INI_LoaderImpl::fetch_section_names(vector<string> &names)
  {
    names.clear();
    map<string, map<string,string> >::iterator it = section_map.begin();
    while (it != section_map.end()) {
      names.push_back(it->first);
      it++;
    }
  }

  // デフォルトのセッション名を設定する
  void INI_LoaderImpl::set_section(const char *name)
  {
    current_section = name;
  }

  // 登録済のパラメータ名の一覧を入手する
  void INI_LoaderImpl::fetch_config_names(vector<string> &names, const char *section)
  {
    names.clear();
    if (!section) section = current_section.c_str();
    string xsection(section);

    if (default_section == "") {
      // デフォルトが定義されていない
      map<string, string> &conf = section_map[xsection];
      map<string,string>::iterator it = conf.begin();
      while (it != conf.end()) {
	names.push_back(it->first);
	it++;
      }
      return;
    }

    set<string> dnames;
    map<string, string> &default_conf = section_map[default_section];

    map<string,string>::iterator it = default_conf.begin();
    while (it != default_conf.end()) {
      dnames.insert(it->first);
      it++;
    }

    map<string, string> &conf = section_map[xsection];
    it = conf.begin();
    while (it != conf.end()) {
      dnames.insert(it->first);
      it++;
    }
    
    copy(dnames.begin(), dnames.end(), back_inserter(names));
  }


  // デフォルトのセッション名を設定する
  void INI_LoaderImpl::set_default_section(const char *name)
  {
    default_section = name;
  }
  
  // 設定パラメータを入手する
  /*
    sectionを省略すると、デフォルトのセッション名を利用する
  */
  string INI_LoaderImpl::get_config_value(const char *name, const char *section) {
    if (!section) section = current_section.c_str();
    string xsection(section), xname(name);
    map<string, string> &conf = section_map[xsection];
    string &cval = conf[name];
    if (!cval.empty()) return cval;

    map<string, string> &default_conf = section_map[default_section];
    return default_conf[name];
  }
};

namespace uc {

  /// INI_loaderのインスタンスを入手する
  INI_Loader *new_INI_Loader() {
    return new INI_LoaderImpl();
  }

};

static ostream &
show_name_list(vector<string> &names, ostream &out, const char *sep = ", ")
{
  // リストに含まれる文字列をカンマ区切りで出力する
  vector<string>::iterator it = names.begin();
  const char *tsep = "";
  while (it != names.end()) {
    out << tsep << *it;
    tsep = sep;
    it++;
  }
  return out;
}

/// テキストファイルを生成する
static bool text_out(const char *outfile,const char *text) {
  FILE *fp = fopen(outfile,"w");
  if (!fp) {
    fprintf(stderr,"ERROR: fopen %s to write text file failed:(%d) %s\n", 
	    outfile, errno, strerror(errno));
    return false;
  }
  fputs(text, fp);

  if (fclose(fp) != 0) {
    fprintf(stderr,"WARNING: fclose %s failed:(%d) %s\n", 
	    outfile, errno, strerror(errno));
    return false;
  }
  return true;
}

static int
test_ini01(int argc, char **argv) {
  uc::INI_Loader *ini = uc::new_INI_Loader();
  const char *ini_file = getenv("INI");
  if (!ini_file) {
    ini_file = "work/init-test.ini";
    
    text_out(ini_file,"\n"
	     "[xx]\n"
	     "aa = AA\n"
	     "bb = BB\n"
	     "cc = CC\n"
	     "xx = XX\n"
	     "yy = YY\n"
	     "\n"
	     "[xx2]\n"
	     "aa = AA2\n"
	     "bb = BB2\n"
	     "\n"
	     "[xx3]\n"
	     "aa = AA3\n"
	     "bb = BB3\n"
	     "\n");
  }

  if (!ini->set_ini_filename(ini_file)) return 1;

  cout << "ini file: " << ini->get_ini_filename() << endl;

  vector<string> names;
  ini->fetch_section_names(names);
  show_name_list(names, cout << "Sections: ") << endl;

  cout << "Current Section: " << ini->get_current_section() << endl;
  cout << "Default Section: " << ini->get_default_section() << endl;

  ini->fetch_config_names(names);
  show_name_list(names, cout << "Config names: ") << endl;

  vector<string>::iterator it = names.begin();
  for (; it != names.end(); it++) {
    cout << *it << ": " << ini->get_config_value(it->c_str()) << endl;
  }


  // -------------------------------------------
  // デフォルト・セクションを利用する例

  ini->set_default_section("xx");
  ini->set_section("xx2");
  cout << "Default Section: " << ini->get_default_section() << endl;
  cout << "Current Section: " << ini->get_current_section() << endl;

  ini->fetch_config_names(names);
  show_name_list(names, cout << "Config names: ") << endl;

  it = names.begin();
  for (; it != names.end(); it++) {
    cout << *it << ": " << ini->get_config_value(it->c_str()) << endl;
  }

  return 0;
}

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd ini_cmap[] = {
  { "ini01", test_ini01, },
  { NULL, },
};

#endif
