/*! \file                                                                                                           
 * \brief 標準Cライブラリの機能を確認する
 */

#include <cstdio>
#include <cstdlib>

// --------------------------------------------------------------------------------

extern "C" {
  
  /// atoi でテキストを整数値に変換する
  static int test_atoi(int argc, char **argv) {
    int sum = 0;

    for (int i = 1; i < argc; i++) {
      sum += printf("%s => %d\n", argv[i], atoi(argv[i]));
    }
    fprintf(stderr,"%s: %d bytes output.\n", argv[0], sum);
    
    return EXIT_SUCCESS;
  }


  /// sscanf でテキストを整数値に変換する
  static int test_sscanf(int argc, char **argv) {
    int sum = 0;

    for (int i = 1; i < argc; i++) {
      int value = 0;
      
      if (1 == sscanf(argv[i], "%i", &value))
	sum += printf("%s => %d\n", argv[i], atoi(argv[i]));
      else
	sum += printf("%s => X\n", argv[i]);
    }
    fprintf(stderr,"%s: %d bytes output.\n", argv[0], sum);
    
    return EXIT_SUCCESS;
  }
};

// --------------------------------------------------------------------------------

#include "subcmd.h"

subcmd stdc_cmap[] = {
  { "stdc-atoi", test_atoi, },
  { "stdc-sscanf01", test_sscanf, },
  { NULL, },
};

