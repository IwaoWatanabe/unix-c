
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern const char **environ;

// NULLで終了するポインタ配列の要素数を数える
static int env_count(const char **env) {
  int ct = 0;
  while (*env++) ct++;
  return ct;
}

// 文字列のポインタ整列の比較
static int env_cmp(const void *a, const void *b) {
  int f = strcasecmp(*(const char **)a,*(const char **)b);
  return f ? f : strcmp(*(const char **)a,*(const char **)b);
}

// 環境変数を確認する

static void show_env(const char **env, int detail) {
  if (detail) {
    for(;*env; env++) printf("%s\n",*env);
    return;
  }

  // 変数名だけ出力する
  for(;*env; env++) {
    const char *p = strchr(*env,'=');
    if (!p) continue;
    int width = p - *env;
    printf("%.*s\n", width, *env);
  }
}

int main(int argc, char **argv) {
  const char **env = environ;

  // 環境変数を整列
  qsort(env,env_count(env), sizeof *env, env_cmp);

  int detail = getenv("DETAIL") != NULL;
  show_env(env, detail);

  return 0;
}

