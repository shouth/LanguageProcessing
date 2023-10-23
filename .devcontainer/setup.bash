cp .devcontainer/greeting.fish $HOME/.config/fish/conf.d/

source /etc/os-release
echo "deb http://apt.llvm.org/$VERSION_CODENAME/ llvm-toolchain-$VERSION_CODENAME main" > /etc/apt/sources.list.d/llvm-toolchain.list
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key > /etc/apt/trusted.gpg.d/apt.llvm.org.asc

apt update && apt install -y clangd-18
