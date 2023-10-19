cp .devcontainer/greeting.fish $HOME/.config/fish/conf.d/

cp .devcontainer/llvm-toolchain.list /etc/apt/sources.list.d/
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key > /etc/apt/trusted.gpg.d/apt.llvm.org.asc

apt update && apt install -y clangd-18
