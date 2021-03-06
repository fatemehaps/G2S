if ismac
    mex('../../src/g2s.cpp','-I/usr/local/include','-lut','-I/opt/local/include','-I../../include','-L/opt/local/lib','-lzmq','-ljsoncpp',strcat('-DMATLAB_VERSION=0x',version('-release')));%../../src/cvtZMQ2WS.cpp
elseif isunix
    mex('../../src/g2s.cpp','-lut','-I/usr/include','-I../../include','-I/usr/include/jsoncpp','-L/usr/lib','-lzmq','-ljsoncpp',strcat('-DMATLAB_VERSION=0x',version('-release')));
elseif ispc
    if(exist('C:\Program Files\ZeroMQ 4.0.4')==0)
        websave('ZeroMQ-4.0.4~miru1.0-x64.exe','https://miru.hk/archive/ZeroMQ-4.0.4~miru1.0-x64.exe');
        winopen('ZeroMQ-4.0.4~miru1.0-x64.exe')
    end
    if(exist('cppzmq-master')==0)
        websave('cppzmq-master.zip','https://codeload.github.com/zeromq/cppzmq/zip/master');
        unzip('cppzmq-master.zip');
    end
    if(exist('jsoncpp-master')==0)
        websave('jsoncpp-master.zip','https://codeload.github.com/open-source-parsers/jsoncpp/zip/master')
        unzip('jsoncpp-master.zip');
        cd 'jsoncpp-master'
        ! python amalgamate.py
        cd ..
    end
    mex('../../src/g2s.cpp','jsoncpp-master/dist/jsoncpp.cpp','-I/usr/include','-I../../include','-I"C:\Program Files\ZeroMQ 4.0.4\include"','-I"cppzmq-master"','-L"C:\Program Files\ZeroMQ 4.0.4\lib"','-llibzmq-v120-mt-4_0_4','-I"jsoncpp-master\dist"',strcat("-L",matlabroot,"\extern\lib\win64\mingw64"),'-lut','-DNOMINMAX',strcat('-DMATLAB_VERSION=0x',version('-release')));
    path=getenv('PATH');
    newpath=strcat(path,'C:\Program Files\ZeroMQ 4.0.4\bin;');
    setenv('PATH',newpath);
else
    disp('Platform not supported')
end