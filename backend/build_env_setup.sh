#!/bin/bash -i

# execute this after 'chmod +x build_env_setup.sh' command
# execute this without sudo

# lib versions
python_version=3.12.11
node_version=20.17.0
gcc_version=13.1.0

# file paths
rc_file="$HOME/.bashrc"
profile_file="$HOME/.profile"

# dir paths 
vcpkg_dir="$HOME/vcpkg"

sudo echo 'Start to run ZEM server environment setup script...'

function install_package() {
    REQUIRED_PKG="$1"
    PKG_OK=$(dpkg-query -W --showformat='${Status}\n' "$REQUIRED_PKG" | grep "install ok installed")
    echo Checking for "$REQUIRED_PKG": "$PKG_OK"
    if [ "" = "$PKG_OK" ]; then
        echo "No $REQUIRED_PKG. Setting up $REQUIRED_PKG."
        sudo apt-get --yes install "$REQUIRED_PKG"
    fi
}

function insert_content_to_file() {
    file_name=$1
    content=$2

    if ! grep -q ^"$content"\$ "$file_name"; then
        echo $'\n'"$content" >> "$file_name"
    fi
}

function update_value_to_file() {
    file_name=$1
    key=$2
    value=$3

    sed "s,$key=[^;]*,$key=$value," -i "$file_name"
}

sudo apt-get update

packages=( "make" "build-essential" "libssl-dev" "zlib1g-dev" "libbz2-dev" "libreadline-dev" "libsqlite3-dev" "wget" "curl" "llvm" "libncursesw5-dev" "xz-utils" "tk-dev" "libxml2-dev" "libxmlsec1-dev" "libffi-dev" "liblzma-dev" "zip" "tar" "unzip" "git" "openssl" "bzip2" "libc++-dev" "libc++abi-dev" "bison" "flex" "autoconf" "graphviz" "ninja-build" "postgresql" "postgresql-contrib" "libpq-dev" "postgresql-server-dev-all")
for (( i=0; i<${#packages[@]}; i++ )); do
    install_package "${packages[i]}"
done

# Install nvm
if [ -n "$(nvm --version 2>&1 > /dev/null)" ]; then
    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | bash
fi

# shellcheck source=/dev/null
. "$rc_file"

nvm install "$node_version"
nvm alias default "$node_version"
nvm use default

# Install pyenv
if ! [ -d "$HOME"/.pyenv ]; then
    git clone https://github.com/pyenv/pyenv.git "$HOME"/.pyenv

    insert_content_to_file "$rc_file" "export PYENV_ROOT=\"\$HOME/.pyenv\""
    insert_content_to_file "$rc_file" "command -v pyenv >/dev/null || export PATH=\"\$PYENV_ROOT/bin:\$PATH\""
    insert_content_to_file "$rc_file" "eval \"\$(pyenv init -)\""

    if [ -f "$profile_file" ]; then
        insert_content_to_file "$profile_file" "export PYENV_ROOT=\"\$HOME/.pyenv\""
        insert_content_to_file "$profile_file" "command -v pyenv >/dev/null || export PATH=\"\$PYENV_ROOT/bin:\$PATH\""
        insert_content_to_file "$profile_file" "eval \"\$(pyenv init -)\""
    fi
else
    cd "$HOME"/.pyenv || exit
    git pull
fi

# shellcheck source=/dev/null
. "$rc_file"

# Install target python version in pyenv
if ! pyenv versions | grep -E -q " $python_version "; then
    pyenv install -v "$python_version"
fi

# Set target python to global
pyenv global "$python_version"

# install gcc
if ! command -v gcc &>/dev/null; then
    echo "gcc is not installed."
    gcc_cur_ver="0.0.0"
else
    gcc_cur_ver=$(gcc -dumpfullversion -dumpversion)
    echo "Current gcc version: $gcc_cur_ver"
fi

if ! command -v gcc &>/dev/null || [ "$(printf '%s\n%s\n' "$gcc_cur_ver" "$gcc_version" | sort -V | head -n1)" = "$gcc_cur_ver" ] && [ "$gcc_cur_ver" != "$gcc_version" ]; then
    sudo apt update
    sudo apt install -y software-properties-common
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt update
    sudo apt install -y g++-13 gcc-13
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 120
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 120
    echo "gcc-13 installation is done."
else
    echo "No need to install gcc"
fi

#install cmake
if [ -n "$(cmake --version 2>&1 > /dev/null)" ]; then
    cmake_url="https://github.com/Kitware/CMake/releases/download/v4.0.3/cmake-4.0.3.tar.gz"
    cmake_zip_file="cmake-4.0.3.tar.gz"
    cmake_src_dir="cmake-4.0.3"
    wget "$cmake_url" -O "$cmake_zip_file"
    tar -xzf "$cmake_zip_file"
    cd "$cmake_src_dir"
    ./configure
    make -j"$(nproc)"
    sudo make install
    cd ..
    rm -rf "$cmake_src_dir" "$cmake_zip_file"
fi

# install vcpkg
if [ -d "$vcpkg_dir" ]; then
    echo "vcpkg is already installed in $vcpkg_dir"
else
    git clone https://github.com/microsoft/vcpkg.git "$vcpkg_dir"
    cd "$vcpkg_dir"
    git checkout master
    ./bootstrap-vcpkg.sh
    insert_content_to_file "$rc_file" "export PATH=\"\$PATH:$vcpkg_dir\""
fi

echo "Installation is done!"

