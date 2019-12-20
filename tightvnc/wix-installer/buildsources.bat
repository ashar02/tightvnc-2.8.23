call vcvarsall.bat

vcbuild.exe /rebuild /M%NUMBER_OF_PROCESSORS% "..\tightvnc2015.sln" "Release|Win32"
vcbuild.exe /rebuild /M%NUMBER_OF_PROCESSORS% "..\tightvnc2015.sln" "Release|x64"

