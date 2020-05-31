Summary: @ROPEN_SUMMARY@
Name: ropen
Version: @PROJECT_VERSION@
Release: 1
License: MIT
Group: Utils
Source: ropen.tar.gz
URL: https://terminalpp.com
Packager: Zduka <zduka@outlook.com>

%description
@ROPEN_DESCRIPTION@

%prep
rm -rf $RPM_BUILD_DIR/ropen
tar -xvf $RPM_SOURCE_DIR/ropen.tar.gz -C $RPM_BUILD_DIR
mkdir -p $RPM_BUILD_DIR/build

%build
cd $RPM_BUILD_DIR/build && cmake ../ropen -DCMAKE_BUILD_TYPE=Release -DSTAMP_ARGS=--no-override -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_INSTALL_PREFIX=$RPM_BUILD_ROOT/usr -DINSTALL=ropen
cd $RPM_BUILD_DIR/build && make ropen

%install
cd $RPM_BUILD_DIR/build && make install

%files
/usr/bin/ropen
