#!/bin/bash
# script to build naali and most deps.

set -e
set -x


viewer=$(dirname $(readlink -f $0))/..
deps=$viewer/../naali-deps
mkdir -p $deps
deps=$(cd $deps && pwd)
viewer=$(cd $viewer && pwd)

prefix=$deps/install
build=$deps/build
tarballs=$deps/tarballs
tags=$deps/tags



# -j<n> param for make, for how many processes to run concurrently

nprocs=`grep -c "^processor" /proc/cpuinfo` 

mkdir -p $tarballs $build $prefix/{lib,share,etc,include} $tags

export PATH=$prefix/bin:$PATH
export PKG_CONFIG_PATH=$prefix/lib/pkgconfig
export NAALI_DEP_PATH=$prefix
export LDFLAGS="-L$prefix/lib -Wl,-rpath -Wl,$prefix/lib"
export LIBRARY_PATH=$prefix/lib
export C_INCLUDE_PATH=$prefix/include
export CPLUS_INCLUDE_PATH=$prefix/include
export CC="ccache gcc"
export CXX="ccache g++"
export CCACHE_DIR=$deps/ccache

private_ogre=true # build own ogre by default, since ubuntu shipped ogre is too old and/or built without thread support

if [ x$private_ogre != xtrue ]; then
    more="libogre-dev"
else
    more="nvidia-cg-toolkit" # ubuntu libogre-dev build-deps don't include cg
    export OGRE_HOME=$prefix
fi

if lsb_release -c | egrep -q "lucid|maverick|natty|oneiric|precise" && tty >/dev/null; then
        which aptitude > /dev/null 2>&1 || sudo apt-get install aptitude
	sudo aptitude -y install git-core python-dev libogg-dev libvorbis-dev \
	 build-essential g++ libboost-all-dev libois-dev \
	 ccache libqt4-dev python-dev freeglut3-dev \
	 libxml2-dev cmake libalut-dev libtheora-dev \
	 liboil0.3-dev mercurial unzip xsltproc $more
fi

what=bullet-2.79-rev2440
if test -f $tags/$what-done; then
    echo $what is done
else
    cd $build
    whatdir=${what%%-rev*}
    rm -rf $whatdir
    test -f $tarballs/$what.tgz || wget -P $tarballs http://bullet.googlecode.com/files/$what.tgz
    tar zxf $tarballs/$what.tgz
    cd $whatdir
    sed -i s/OpenCL// src/BulletMultiThreaded/GpuSoftBodySolvers/CMakeLists.txt
    cmake -DCMAKE_INSTALL_PREFIX=$prefix -DBUILD_DEMOS=OFF -DBUILD_{NVIDIA,AMD,MINICL}_OPENCL_DEMOS=OFF -DBUILD_CPU_DEMOS=OFF -DINSTALL_EXTRA_LIBS=ON -DCMAKE_CXX_FLAGS_RELEASE="-O2 -g -fPIC" .
    make -j $nprocs
    make install
    touch $tags/$what-done
fi

what=qtscriptgenerator
if test -f $tags/$what-done; then 
   echo $what is done
else
    cd $build
    rm -rf $what
    git clone git://gitorious.org/qt-labs/$what.git
    cd $what

    cd generator
    qmake
    make -j $nprocs
    ./generator --include-paths=`qmake -query QT_INSTALL_HEADERS`
    cd ..

    cd qtbindings
    sed -i 's/qtscript_phonon //' qtbindings.pro 
    sed -i 's/qtscript_webkit //' qtbindings.pro 
    qmake
    make -j $nprocs
    cd ..
    cd ..
    touch $tags/$what-done
fi
mkdir -p $viewer/bin/qtplugins/script
cp -lf $build/$what/plugins/script/* $viewer/bin/qtplugins/script/


what=kNet
if test -f $tags/$what-done; then 
   echo $what is done
else
    cd $build
    rm -rf kNet
    git clone https://github.com/juj/kNet
    cd kNet
    git checkout stable
    sed -e "s/USE_TINYXML TRUE/USE_TINYXML FALSE/" -e "s/kNet STATIC/kNet SHARED/" < CMakeLists.txt > x
    mv x CMakeLists.txt
    cmake . -DCMAKE_BUILD_TYPE=Debug
    make -j $nprocs
    cp lib/libkNet.so $prefix/lib/
    rsync -r include/* $prefix/include/
    touch $tags/$what-done
fi


if [ x$private_ogre = xtrue ]; then
    sudo apt-get build-dep libogre-dev
    what=ogre
    if test -f $tags/$what-done; then
        echo $what is done
    else
        cd $build
        rm -rf $what
	test -f $tarballs/$what.tar.bz2 || wget -O $tarballs/$what.tar.bz2 'http://downloads.sourceforge.net/project/ogre/ogre/1.7/ogre_src_v1-7-3.tar.bz2?r=http%3A%2F%2Fwww.ogre3d.org%2Fdownload%2Fsource&ts=1319633319&use_mirror=switch'
	tar jxf $tarballs/$what.tar.bz2
        cd ${what}_src_v1-7-3/
        mkdir -p $what-build
        cd $what-build
        cmake .. -DCMAKE_INSTALL_PREFIX=$prefix
        make -j $nprocs VERBOSE=1
        make install
        touch $tags/$what-done
    fi
fi

# HydraX, SkyX and PythonQT are build from the realxtend own dependencies.
# At least for the time being, until changes to those components flow into
# upstream..

cd $build
depdir=realxtend-tundra-deps
if [ ! -e $depdir ]
then
    echo "Cloning source of HydraX/SkyX/PythonQT/NullRenderer..."
    git init $depdir
    cd $depdir
    git fetch https://code.google.com/p/realxtend-tundra-deps/ sources:refs/remotes/origin/sources
    git remote add origin https://code.google.com/p/realxtend-tundra-deps/
    git checkout sources
else
    cd $depdir
    git fetch https://code.google.com/p/realxtend-tundra-deps/ sources:refs/remotes/origin/sources
    if [ -z "`git merge sources origin/sources|grep "Already"`" ]; then
        echo "Changes in GIT detected, rebuilding HydraX, SkyX and PythonQT"
        rm -f $tags/hydrax-done $tags/skyx-done $tags/pythonqt-done
    else
        echo "No changes in realxtend deps git."
    fi
fi

# HydraX build:
if test -f $tags/hydrax-done; then
    echo "Hydrax-done"
else
    cd $build/$depdir/hydrax
    sed -i "s!^OGRE_CFLAGS.*!OGRE_CFLAGS = $(pkg-config OGRE --cflags)!" makefile
    sed -i "s!^OGRE_LDFLAGS.*!OGRE_LDFLAGS = $(pkg-config OGRE --libs)!" makefile
    make -j $nprocs PREFIX=$prefix
    make PREFIX=$prefix install
    touch $tags/hydrax-done
fi
# SkyX build

if test -f $tags/skyx-done; then
   echo "SkyX-done"
else
   cd $build/$depdir/skyx
   my_ogre_home=$OGRE_HOME
   if [ -z "$my_ogre_home" ]; then
	    my_ogre_home=`pkg-config --variable=prefix OGRE`
       if [ -z "$my_ogre_home" ]; then
           echo "OGRE_HOME not defined, check your pkg-config or set OGRE_HOME manually.";
           exit 0;
       fi
   fi
   echo "Using OGRE_HOME = $OGRE_HOME"
   SKYX_SOURCE_DIR=`pwd`
   env OGRE_HOME=$my_ogre_home/lib/OGRE cmake -DCMAKE_INSTALL_PREFIX=$prefix .
   make -j $nprocs install
   touch $tags/skyx-done
fi

# PythonQT build
if test -f $tags/pythonqt-done; then
    echo "PythonQt-done"
else
    cd $build/$depdir/PythonQt
    pyver=$(python -c 'import sys; print sys.version[:3]')
    sed -i "s/PYTHON_VERSION=.*/PYTHON_VERSION=$pyver/" build/python.prf
    fn=generated_cpp/com_trolltech_qt_core/com_trolltech_qt_core0.h
    sed 's/CocoaRequestModal = QEvent::CocoaRequestModal,//' < $fn > x
    mv x $fn
    qmake
    make -j $nprocs
    rm -f $prefix/lib/libPythonQt*
    cp -a lib/libPythonQt* $prefix/lib/
    
    # work around PythonQt vs Qt 4.8 incompatibility
    cd src
    make moc_PythonQtStdDecorators.cpp
    ed moc_PythonQtStdDecorators.cpp <<EOF
/qt_static_metacall
-
a
#undef emit
.
w
q
EOF
    cd ..
    # end of workaround

    cp src/PythonQt*.h $prefix/include/
    cp extensions/PythonQt_QtAll/PythonQt*.h $prefix/include/
    touch $tags/pythonqt-done
fi

cd $build
what=qtpropertybrowser
if test -f $tags/$what-done; then
    echo $what is done
else
    pkgbase=${what}-2.5_1-opensource
    rm -rf $pkgbase
    zip=../tarballs/$pkgbase.tar.gz
    test -f $zip || wget -O $zip http://get.qt.nokia.com/qt/solutions/lgpl/$pkgbase.tar.gz
    tar zxf $zip
    cd $pkgbase
    echo yes | ./configure -library
    qmake
    make -j$nprocs
    cp lib/lib* $prefix/lib/
    # luckily only extensionless headers under src match Qt*:
    cp src/qt*.h src/Qt* $prefix/include/
    touch $tags/$what-done
fi

if test "$1" = "--depsonly"; then
    exit 0
fi

cd $viewer
cat > ccache-g++-wrapper <<EOF
#!/bin/sh
exec ccache g++ -O -g \$@
EOF
chmod +x ccache-g++-wrapper
TUNDRA_DEP_PATH=$prefix cmake -DCMAKE_CXX_COMPILER="$viewer/ccache-g++-wrapper" . -DCMAKE_MODULE_PATH=$prefix/lib/SKYX/cmake
make -j $nprocs VERBOSE=1
