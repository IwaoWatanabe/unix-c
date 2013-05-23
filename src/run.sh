#!/bin/bash

# OS標準とは異なる場所に導入されているMWの依存ライブラリの設定を行って
# バイナリ・モジュールを起動するためのサポート・スクリプト

now() { date "+%Y-%m-%d %H:%M:%S"; }
die() { set +x; echo ERROR: $* at $(now) >&2; exit 1; }

# ----------------------------------- configuration load

test -z "$CONF_DIR" && CONF_DIR=config
test -z "$CONF" && CONF="$CONF_DIR"/$(basename "$0"|sed 's/[.]sh$//').conf
test -f "$CONF" && . "$CONF" && echo INFO: $CONF loaded. >&2

NODE_CONF=$(echo "$CONF"|sed s/[.]conf$//g)-$(hostname -s).conf
test -f "$NODE_CONF" && . "$NODE_CONF" && echo INFO: $NODE_CONF loaded. >&2

AUTH_CONF=$(dirname "$CONF")/auth-$(hostname -s).conf
test -f "$AUTH_CONF" && . "$AUTH_CONF" && echo INFO: $AUTH_CONF loaded. >&2

# ----------------------------------- default values

test -z "$EXMPLAES_HOME" && EXMPLAES_HOME=/opt/examples-cpp

# ------------------------------------ execute function

export LD_LIBRARY_PATH=$EXMPLAES_HOME/lib:
export PATH=$EXMPLAES_HOME/bin:$PATH

CMD=$(basename "$0" | sed 's/.sh$//')
if [[ -x $CMD ]]; then
    time $DEBUG $CMD "$@" && echo INFO: $CMD successfully on $0 at $(now) >&2 && exit
    die "$CMD failure (rc:$?) on $0"
fi

CMD="$1"
shift

test -f $CMD && time $DEBUG $CMD "$@" && \
    echo "INFO: $CMD successfully on $0 at $(now)" >&2 && exit

die "$CMD failure (rc:$?) on $0"

# ------------------------------------ document

=head1  NAME

run.sh - モジュール起動サポートツール

=head1  SYNOPSIS

B<sh run.sh> I<module> [I<args>]..

=head1  DESCRIPTION

run.sh は OS標準とは異なる場所に導入されている
ミドルウェア等の依存ライブラリの設定を行って
バイナリ・モジュールを起動するためのサポート・スクリプトです。

モジュールが正常に終了したら I<モジュール名> successfully. と出力します。
エラー終了したら　I<モジュール名> failure. と 終了コードを出力します。
なお終了コード:0 であれば正常終了として扱います。


