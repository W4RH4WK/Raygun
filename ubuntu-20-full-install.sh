
# ----------------------------------------------------------------------------------------------------------------------
function _install_shaderc_() {
# cant use prebuilt binaries because they are for ubuntu 22
git clone https://github.com/google/shaderc ~/git/shaderc
cd ~/git/shaderc
./utils/git-sync-deps
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -B
sudo apt-get install libgmock-dev
mkdir build
cd build
rm CMakeCache.txt
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
ctest # optional
./glslc/glslc --version
echo 'export PATH="$HOME/git/shaderc/build/glslc:$PATH"' >> ~/.bashrc
echo 'export PATH="$HOME/git/shaderc/build/glslc:$PATH"' >> ~/.zshrc
exec zsh
which glslc
glslc --version
}
_install_shaderc_
# ----------------------------------------------------------------------------------------------------------------------



# ----------------------------------------------------------------------------------------------------------------------
function _install_physx_() {
# https://github.com/NVIDIA-Omniverse/PhysX/blob/107.0-physx-5.6.0/physx/documentation/platformreadme/linux/README_LINUX.md

# git clone https://github.com/NVIDIA-Omniverse/PhysX/ ~/git/physx
# cd ~/git/physx/physx
# git checkout 107.0-physx-5.6.0
# ./generate_projects.sh
# # linux-gcc
# 7
# cd ~/git/physx/physx/compiler/linux-gcc-release
# make
# # gets segmentation fault every time!!

# https://github.com/NVIDIAGameWorks/PhysX/blob/4.1/physx/documentation/platformreadme/linux/readme_linux.html
git clone --depth 1 https://github.com/NVIDIAGameWorks/PhysX  ~/git/physx4
cd ~/git/physx4/physx
./generate_projects.sh
# linux
3
cd ~/git/physx4/physx/compiler/linux-release
sudo apt-get install -y libxxf86vm-dev
code Makefile
make # -j8
}
_install_physx_
# ----------------------------------------------------------------------------------------------------------------------


# ----------------------------------------------------------------------------------------------------------------------
function _install_raygun_() {

git clone https://github.com/W4RH4WK/Raygun ~/git/Raygun
# git clone https://github.com/qpwo/fork2-raygun ~/git/Raygun
code Raygun
sudo apt-get install libassimp-dev
sudo apt-get install libopenal-dev
sudo apt-get install libogg-dev
sudo apt install ninja-build pkg-config libassimp-dev libglfw3-dev libopenal-dev libogg-dev libopus-dev libopusfile-dev
cd ~/git/Raygun
trash -v build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DPhysX_ROOT=$HOME/git/physx4 -B build
cd $HOME/git/Raygun && ninja -C build 2>&1 | tee build.log
# ninja -C build
./build/example/example
}
_install_raygun_
# ----------------------------------------------------------------------------------------------------------------------
