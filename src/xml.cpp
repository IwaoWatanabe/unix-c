/*! \file
 * \brief XMLを操作するインタフェース
 */

#include <algorithm>
#include <string>
#include <vector>

namespace uc {
  namespace xml {

    class Element;

    /// XMLドキュメントを照会する基本APIを提供する
    /**
       このクラスのインスタンスをdelete すると関連メモリが開放される。
     */
    class Document {
    public:
      virtual ~Document();
      /// ルート・エレメントの入手
      virtual Element *get_root_element() = 0;
      /// XMLテキストをパースする
      virtual bool parse_text(const char *xml_text) = 0;
    };

    /// XMLファイルをパースしてドキュメントを入手する
    Document *load_XML_document(const char *xml_file, const char *parser ="");

    /// XMLの各エレメントを照会するオブジェクト
    class Element {
    public:
      virtual ~Element();
      /// ドキュメントの参照
      virtual Document *get_document() = 0;
      /// エレメント名を入手する
      virtual std::string get_name() = 0;
      /// テキストを入手する
      virtual std::string get_text() = 0;
      /// 子エレメントのテキストを入手する
      virtual std::string get_child_text(const char *child_name) = 0;
      /// 子エレメントを入手する
      virtual Element *get_child_at(int n) = 0;
      /// 子エレメント数を入手する
      virtual int get_child_count() = 0;
      /// 子エレメントをまとめて入手する
      virtual bool fetch_children(std::vector<Element *> &children) = 0;
      /// 指定する名称の子ノードを入手する
      virtual bool find_children(const char *name, std::vector<Element *> &children) = 0;
      /// 属性値を入手する
      virtual std::string attribute_value(const char *name) = 0;
      /// 属性名称を入手する
      virtual bool fetch_attribute_names(std::vector<std::string> &attr_names) = 0;
    };
  };
};

// --------------------------------------------------------------------------------

#include "libxml/tree.h"

#include <cstdio>
#include <cerrno>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

using namespace std;
using namespace uc::xml;

namespace {

  class Element_LibXML_Impl;

  /// gnome の libxmlによるDOM実装
  class Document_LibXML_Impl : public Document {
    Element_LibXML_Impl *root;
    xmlDocPtr doc;
    friend class Element_LibXML_Impl;
  public:
    Document_LibXML_Impl();
    ~Document_LibXML_Impl();
    Element *get_root_element();
    bool parse_text(const char *xml_text);
  };

  class Element_LibXML_Impl : public Element {
    xmlNodePtr node;
    Document_LibXML_Impl *doc;
    vector<Element_LibXML_Impl *> sub_nodes;
    void fetch_children();
  public:
    Element_LibXML_Impl(xmlNodePtr np, Document_LibXML_Impl *ref) : doc(ref), node(np) { }
    ~Element_LibXML_Impl();
    Document *get_document() { return doc; }
    string get_name() { return (const char *)node->name; }
    string get_text();
    string get_child_text(const char *child_name);
    Element *get_child_at(int n);
    int get_child_count();
    bool fetch_children(vector<Element *> &children);
    bool find_children(const char *name, vector<Element *> &children);
    string attribute_value(const char *name);
    bool fetch_attribute_names(vector<string> &attr_names);
  };

  Document_LibXML_Impl::Document_LibXML_Impl() : doc(0), root(0) { }
  Document_LibXML_Impl::~Document_LibXML_Impl() { delete root; }

  Element *Document_LibXML_Impl::get_root_element() { return root; }

  bool Document_LibXML_Impl::parse_text(const char *xml_text) {
    doc = xmlParseMemory(xml_text, strlen(xml_text));
    if (!doc) return false;
    root = new Element_LibXML_Impl(xmlDocGetRootElement(doc), this);
    return true;
  }

  Element_LibXML_Impl::~Element_LibXML_Impl() {
    vector<Element_LibXML_Impl *>::iterator it = sub_nodes.begin();
    for (; it != sub_nodes.end(); it++) { delete *it; }
  }

  /// テキストを入手する
  string Element_LibXML_Impl::get_text() {
    char *text = (char *)xmlNodeListGetString(doc->doc, node->children, 1);
    if (!text) return "";
    string buf(text);
    free(text);
    return buf;
  }

  /// 子エレメントのテキストを入手する
  string Element_LibXML_Impl::get_child_text(const char *child_name) {
    for (xmlNodePtr childNode = node->children; childNode; childNode = childNode->next) {
      if (strcasecmp((const char*)childNode->name, child_name) != 0) continue;
      if (childNode->children && childNode->children->content)
	return (const char*)childNode->children->content;
    }
    return "";
  }

  void Element_LibXML_Impl::fetch_children() {
    if (!sub_nodes.empty()) return;
    for (xmlNodePtr childNode = node->children; childNode; childNode = childNode->next) {
      if (childNode->type == XML_ELEMENT_NODE)
	sub_nodes.push_back(new Element_LibXML_Impl(childNode, doc));
    }
  }

  /// 子エレメントをまとめて入手する
  bool Element_LibXML_Impl::fetch_children(vector<Element *> &children) {
    fetch_children();
    children.clear();
    copy(sub_nodes.begin(), sub_nodes.end(), back_inserter(children));
    return children.size() > 0;
  }

  /// 子エレメントを入手する
  Element *Element_LibXML_Impl::get_child_at(int n) {
    fetch_children();
    return sub_nodes[n];
  }

  /// 子エレメント数を入手する
  int Element_LibXML_Impl::get_child_count() {
    fetch_children();
    return sub_nodes.size();
  }

  /// 指定する名称の子ノードを入手する
  bool Element_LibXML_Impl::find_children(const char *name, vector<Element *> &children) {
    fetch_children();
    return 0;
  }

  /// 属性値を入手する
  string Element_LibXML_Impl::attribute_value(const char *name) {
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
      if (strcasecmp((const char*)attr->name, name) == 0)
	return (const char*)attr->children->content;
    }
    return "";
  }

  /// 属性名称を入手する
  bool Element_LibXML_Impl::fetch_attribute_names(vector<string> &attr_names) {
    attr_names.clear();
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
      attr_names.push_back((const char *)attr->name);
    }
    return true;
  }
};

namespace uc {
  namespace xml {

    Document::~Document() { }
    Element::~Element() { }


    /// テキスト・ファイルをメモリに読み込む
    static string load_text(const char *path, size_t *file_len, time_t *lastmod) {
      FILE *fp;
      struct stat sbuf;

      int res = stat(path, &sbuf);
      if (res) {
	cerr << "ERROR: stat faild:" << strerror(errno) << ":" << path << endl;
      err_return:
	return "";
      }

      int len = sbuf.st_size, n;
      char *buf = (char *)malloc(len + 10), *ptr = buf;

      if (ptr == NULL) {
	cerr << "ERROR: cannnot allocate memory(" << len << " bytes.)" << path << endl;
	goto err_return;
      }
  
      if ((fp = fopen(path, "r")) == NULL) {
	cerr << "ERROR: fopen faild:" << strerror(errno) << ":" << path << endl;
	free(buf);
	goto err_return;
      }

      if (file_len)
	*file_len = 0;

      while(len > 0 && (n = fread(ptr, 1, len, fp)) > 0) {
	ptr += n;
	len -= n;
	if (file_len)
	  *file_len += n;
      }

      if (fclose(fp) != 0) {
	cerr << "WARNING: fclose faild:" << strerror(errno) << ":" << path << endl;
      }

      if (len) {
	cerr << "ERROR: cannot read all data:" << len << endl;
	free(buf);
	buf = 0;
      }

      if (lastmod)
	*lastmod = sbuf.st_mtime;
      
      if (buf) {
	if (file_len) {
	  string ret(buf,*file_len);
	  free(buf);
	  return ret;
	}

	string ret(buf);
	free(buf);
	return ret;
      }
      return "";
    }

    /// XMLファイルをパースしてドキュメントを入手する
    Document *load_XML_document(const char *xml_file, const char *parser) {
      Document *doc = new Document_LibXML_Impl();
      const char *xml_text = load_text(xml_file, 0, 0).c_str();
      if (!doc->parse_text(xml_text)) { delete doc; doc = 0; }
      return doc;
    }

  };
};

// --------------------------------------------------------------------------------


/// 簡易XML表示
static void show_xml_doc(uc::xml::Element *el, int level = 0) {
  string el_name(el->get_name());

  int ts = level * 2;
  printf("%*s<%s>",ts, "", el_name.c_str());
  vector<Element *> vec;
  if (!el->fetch_children(vec)) {
    printf("%s</%s>\n",el->get_text().c_str(), el_name.c_str());
    return;
  }

  putchar('\n');
  vector<Element *>::iterator it = vec.begin();
  for (; it != vec.end(); it++) {
    show_xml_doc(*it, level + 1);
  }
  printf("%*s</%s>\n",ts, "", el_name.c_str());
}

/// XMLの読み込みテスト
static int cmd_xml_load(int argc, char **argv) {
  int opt;
  bool verbose = false;

  while ((opt = getopt(argc,argv,"v")) != -1) {
    switch(opt) { case 'v': verbose = true; }
  }
  
  int rc = 0;
  for (int i = optind; i < argc; i++) {
    uc::xml::Document *doc = load_XML_document(argv[i], "");
    if(!doc) rc = 1;
    else {
      show_xml_doc(doc->get_root_element());
      delete doc;
    }
  }
  return rc;
}


// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd xml_cmap[] = {
  { "xml-load", cmd_xml_load, },
  { 0 },
};

#endif

