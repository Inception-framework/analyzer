CXXFLAGS="-g3 -O0 -fexceptions" CFLAGS="-g3 -O0" ./configure \
	    --enable-cxx11 \
	    --with-stp=/home/noname/Tools/stp/build \
            --enable-posix-runtime \
            --with-llvmsrc=/home/noname/Tools/llvm/llvm-3.6.0-fracture \
            --with-llvmobj=/home/noname/Tools/llvm/llvm-3.6.0-fracture \
            --with-llvmcc=/home/noname/Tools/clang/build/bin/clang \
            --with-llvmcxx=/home/noname/Tools/clang/build/bin/clang++ \
            --with-uclibc=/home/noname/Tools/klee-uclibc

#make -j 4 all KLEE_USE_CXX11 -C ../..

make -j 4 all KLEE_USE_CXX11


echo "Press any key to install klee"

read i

sudo -S rm /usr/bin/klee
sudo -S ln -s /home/noname/Projects/klee/Release+Asserts/bin/klee /usr/bin/klee
