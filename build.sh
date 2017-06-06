DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

CXXFLAGS="-g3 -O0 -fexceptions" CFLAGS="-g3 -O0" ./configure \
            --enable-cxx11 \
            --with-stp=../tools/stp/build \
            --enable-posix-runtime \
            --with-llvmsrc=$DIR/../tools/llvm/llvm3.4 \
            --with-llvmobj=$DIR/../tools/llvm/llvm3.4 \
            --with-llvmcc=$DIR/../tools/llvm/llvm3.4/Debug+Asserts/bin/clang \
            --with-llvmcxx=$DIR/../tools/llvm/llvm3.4/Debug+Asserts/bin/clang++ \
            --with-uclibc=$DIR/../tools/klee-uclibc \
            --with-runtime='Debug+Asserts'

if [ $? != 0 ]; then
        echo "FAIL to configure Klee !"
        exit;
fi


#make -j 4 all KLEE_USE_CXX11 -C ../..

make -j8 all

if [ $? != 0 ]; then
        echo "FAIL to compile Klee"
        exit;
fi



echo "Press any key to install klee"

read i

sudo -S rm /usr/bin/klee
sudo -S ln -s $DIR/Debug+Asserts/bin/klee /usr/bin/klee
