
#!/bin/bash

sys_glut_clfags=`pkg-config --cflags glut gl`
sys_glut_libs=`pkg-config --libs glut gl`
sys_harfbuzz_clfags=`pkg-config --cflags harfbuzz`
sys_harfbuzz_libs=`pkg-config --libs harfbuzz`

export OS=ios
export build=$(echo $CONFIGURATION | tr A-Z a-z)
# export ARCHS=x86_64
export ARCHS=arm64
# export ARCHS=x86_64

FLAGS="-Wno-unused-function -Wno-empty-body -Wno-implicit-function-declaration -DFZ_ENABLE_ICC=0"
#  FLAGS="-Wno-unused-function -Wno-empty-body -Wno-implicit-function-declaration"

for A in $ARCHS
do
	FLAGS="$FLAGS -arch $A"
done

#  add bitcode for Xcode 7 and up
XCODE_VER=`xcodebuild -version | head -1`
ARRAY=(${XCODE_VER// / })
XCODE_VER_NUM=${ARRAY[1]}
ARRAY=(${XCODE_VER_NUM//./ })
MAJOR=${ARRAY[0]}
if [ "$MAJOR" -ge "7" ]
then
	FLAGS="$FLAGS -fembed-bitcode"
fi

OUT=build/$build-$OS-$(echo $ARCHS | tr ' ' '-')

echo Compiling libraries for $ARCHS.
# make USE_SYSTEM_HARFBUZZ=yes USE_SYSTEM_GLUT=yes SYS_GLUT_CFLAGS="${sys_glut_clfags}" SYS_GLUT_LIBS="${sys_glut_libs}" SYS_HARFBUZZ_CFLAGS="${sys_harfbuzz_clfags}" SYS_HARFBUZZ_LIBS="${sys_harfbuzz_libs}" -j4 -C mupdf OUT=$OUT XCFLAGS="$FLAGS" XLDFLAGS="$FLAGS" third libs || exit 1

# make shared-release -j4 -C mupdf OUT=$OUT XCFLAGS="$FLAGS" XLDFLAGS="$FLAGS" third libs || exit 1
make -j4 -C mupdf OUT=$OUT XCFLAGS="$FLAGS" XLDFLAGS="$FLAGS" third libs || exit 1

echo Copying library to $BUILT_PRODUCTS_DIR/.
mkdir -p "$BUILT_PRODUCTS_DIR"
cp -f mupdf/$OUT/lib*.a "$BUILT_PRODUCTS_DIR"
ranlib "$BUILT_PRODUCTS_DIR"/lib*.a

echo Done.
