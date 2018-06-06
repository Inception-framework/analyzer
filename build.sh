# build script
make clean

LLVM_VERSION=3.6
MODE=DEBUG

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DIR=/media/noname/3a224af4-22de-4deb-ad88-08422268a9fc/Inception/Analyzer/

#./autoconf/AutoRegen.sh
./autoconf/AutoRegen.sh $DIR/../tools/llvm/llvm3.6/

if [ $MODE = DEBUG ]; then
CXXFLAGS="-g3 -O0 -fexceptions" CFLAGS="-g3 -O0" ./configure \
            --enable-cxx11 \
            --with-stp=$DIR/../tools/stp/ \
            --enable-posix-runtime \
            --with-llvmsrc=$DIR/../tools/llvm/llvm$LLVM_VERSION \
            --with-llvmobj=$DIR/../tools/llvm/build_debug \
            --with-llvmcc=$DIR/../tools/llvm/build_debug/Debug+Asserts/bin/clang \
            --with-llvmcxx=$DIR/../tools/llvm/build_debug/Debug+Asserts/bin/clang++ \
            --with-uclibc=$DIR/../tools/klee-uclibc \
            --with-runtime='Debug+Asserts' \
	    --with-llvm-build-mode='Debug+Asserts'
else
CXXFLAGS="-O3 -fexceptions" CFLAGS="-O3" ./configure \
            --enable-cxx11 \
            --with-stp=$DIR/../tools/stp/ \
            --enable-posix-runtime \
            --with-llvmsrc=$DIR/../tools/llvm/llvm$LLVM_VERSION \
            --with-llvmobj=$DIR/../tools/llvm/build_release \
            --with-llvmcc=$DIR/../tools/llvm/build_release/Release/bin/clang \
            --with-llvmcxx=$DIR/../tools/llvm/build_release/Release/bin/clang++ \
            --with-uclibc=$DIR/../tools/klee-uclibc \
            --with-runtime='Release' \
            --with-llvm-build-mode='Release'
fi

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
