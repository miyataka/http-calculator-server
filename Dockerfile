# C 言語開発環境 (Debian)
FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gdb \
    valgrind \
    strace \
    clangd \
    clang-format \
    bear \
    git \
    curl \
    ca-certificates \
    netcat-openbsd \
    procps \
    vim \
    && rm -rf /var/lib/apt/lists/*

# clangd を "clangd" という名前で呼べるようにする (Debian はバージョン付き名で入る)
RUN ln -sf "$(ls /usr/bin/clangd-* | head -1)" /usr/local/bin/clangd || true

# コンテナ内 vim の最小設定
RUN cat > /etc/vim/vimrc.local <<'VIMRC'
syntax on
filetype plugin indent on
set number
set tabstop=4 shiftwidth=4 expandtab
set hlsearch incsearch ignorecase smartcase
set showmatch ruler laststatus=2
set encoding=utf-8
set backspace=indent,eol,start
set mouse=
set background=dark
VIMRC

EXPOSE 8080
CMD ["bash"]
