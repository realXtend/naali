#!/bin/bash

# To use this script, first configure these paths!
qtdir=/Users/lc/QtSDK/Desktop/Qt/4.8.0/gcc
ogredir=/Users/lc/ogre-safe-nocrashes/lib/relwithdebinfo

echo "Assuming Qt is found at $qtdir."
echo "Assuming Ogre is found at $ogredir."

qtlibdir=$qtdir/lib
qtpluginsdir=$qtdir/plugins

# Establish directories we're working in.
cd ..
startdir=$(pwd)
echo "Assuming $startdir is the Tundra repository root directory."
cd build/Tundra.app/
bundledir=$(pwd)
frameworksdir=$bundledir/Contents/Frameworks
cd $startdir

echo "Deleting old Tundra.app bundle."
rm -rf build/Tundra.app/

echo "Creating new Tundra.app bundle structure."
mkdir -p $bundledir/Contents/{Components,Frameworks,MacOS,Plugins,Resources}
mkdir -p $bundledir/Contents/Resources/Scripts

echo "Deploying Tundra files to app bundle in $bundledir."
cp -R bin/* $bundledir/Contents/MacOS
cp -R tools/installers/mac/* $bundledir/Contents
cp tools/mac-tundra-launcher.app/Contents/MacOS/applet $bundledir/Contents/MacOS
cp tools/mac-tundra-launcher.app/Contents/Resources/Scripts/main.scpt $bundledir/Contents/Resources/Scripts

echo "Deploying Qt frameworks from $qtlibdir to app bundle."
cp -R $qtlibdir/QtCore.framework $frameworksdir
cp -R $qtlibdir/QtGui.framework $frameworksdir
cp -R $qtlibdir/QtNetwork.framework $frameworksdir
cp -R $qtlibdir/QtXml.framework $frameworksdir
cp -R $qtlibdir/QtXmlPatterns.framework $frameworksdir
cp -R $qtlibdir/QtScript.framework $frameworksdir
cp -R $qtlibdir/QtScriptTools.framework $frameworksdir
cp -R $qtlibdir/QtWebKit.framework $frameworksdir
cp -R $qtlibdir/QtSvg.framework $frameworksdir
cp -R $qtlibdir/QtSql.framework $frameworksdir

#TODO Remove this.
cp -R $qtlibdir/QtDeclarative.framework $frameworksdir

echo "Deploying Qt plugins from $qtpluginsdir to app bundle."
cp -R $qtpluginsdir/* $bundledir/Contents/Plugins

echo "Deploying Ogre plugins from $ogredir to app bundle."
cp $ogredir/*.dylib $bundledir/Contents/Plugins
cp $ogredir/lib* $bundledir/Contents/Components
ogresamples=`find $bundledir/Contents/Plugins -name "Sample_*.dylib"`
rm $ogresamples

echo "Deploying Ogre framework $ogredir to app bundle."
cp -R $ogredir/Ogre.framework $frameworksdir
echo "Deploying system Cg.framework to app bundle."
cp -R /Library/Frameworks/Cg.framework $frameworksdir
chmod -R u+w $frameworksdir/Cg.framework

echo "Cleaning redundant files from bundle (*_debug*.dylib etc.)"
debugdylibs=`find $bundledir -name "*_debug*.dylib"`
rm $debugdylibs
