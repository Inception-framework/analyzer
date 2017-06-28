make clean

LLVM_VERSION=3.6

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

./autoconf/AutoRegen.sh $DIR/../tools/llvm/llvm3.6/

CXXFLAGS="-g3 -O0 -fexceptions" CFLAGS="-g3 -O0" ./configure \
            --enable-cxx11 \
            --with-stp=$DIR/../tools/stp/ \
            --enable-posix-runtime \
            --with-llvmsrc=$DIR/../tools/llvm/llvm$LLVM_VERSION \
            --with-llvmobj=$DIR/../tools/llvm/build_debug \
            --with-llvmcc=$DIR/../tools/llvm/build_debug/Debug+Asserts/bin/clang \
            --with-llvmcxx=$DIR/../tools/llvm/build_debug/Debug+Asserts/bin/clang++ \
            --with-uclibc=$DIR/../tools/klee-uclibc \
            --with-runtime='Debug+Asserts'

if [ $? != 0 ]; then
        echo "FAIL to configure Klee !"
        exit;
fi


#make -j 4 all KLEE_USE_CXX11 -C ../..

make clean; make -j12 all

if [ $? != 0 ]; then
        echo "FAIL to compile Klee"
        exit;
fi



#echo "Press any key to install klee"

#read i

sudo -S rm /usr/bin/klee
sudo -S ln -s $DIR/Debug+Asserts/bin/klee /usr/bin/klee
