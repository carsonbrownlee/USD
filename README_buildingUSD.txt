sudo yum install OpenEXR-devel
sudo yum install OpenImageIO-devel
build OpenSubdiv from github repo
build ptex from github repo
had to modify /usr/include/OpenEXR/OpenEXRConfig.h to add 
 OPENEXR_VERSION_MINOR and MAJOR strings
sudo yum install python-pyside-devel pyside-tools PyOpenGL
cd into USD/build
mkdir install, set cmaek install dir to that
ccmake .. (note that ninja fails)
make -j



