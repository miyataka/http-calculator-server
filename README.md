# http-calculator-server

## 開発環境 (Docker / Debian)

編集はホストの Neovim、コンパイル・実行・デバッグは Debian コンテナで行う。
プロジェクトはホストと同じパスでコンテナにマウントされるため、
コンテナで生成した `compile_commands.json` をホスト側の clangd がそのまま利用できる。

```sh
make image          # イメージをビルド (初回のみ / Dockerfile 変更時)
make shell          # コンテナ内で bash (8080 番ポートを公開)
make docker-build   # コンテナ内で make build
make docker-run     # コンテナ内でビルドしてサーバを起動
make docker-compdb  # bear で compile_commands.json を生成 (clangd 用)
```

コンテナ内では `make build` / `make run` / `make compdb` / `make clean` を直接使う。
入っているツール: gcc, make, gdb, valgrind, strace, clangd, clang-format, bear, nc, vim (最小設定は `/etc/vim/vimrc.local`)
