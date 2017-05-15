CXXFLAGS="-g3 -O0 -fexceptions" CFLAGS="-g3 -O0" ./configure \
	    --enable-cxx11 \
	    --with-stp=../tools/stp/build \
            --enable-posix-runtime \
            --with-llvmsrc=../tools/llvm/llvm-3.4 \
            --with-llvmobj=../tools/llvm/llvm-3.4 \
            --with-llvmcc=../tools/llvm/clang-3.4/build/bin/clang \
            --with-llvmcxx=../tools/clang-3.4/build/bin/clang++ \
            --with-uclibc=../tools/klee-uclibc \
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
sudo -S ln -s /home/noname/Projects/Inception-analyzer/Release+Asserts/bin/klee /usr/bin/klee
