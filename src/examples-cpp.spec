Summary: UNNIX C/C++ sample codes.
Name: examples-cpp
Version: 0.1
Release: 1%{?dist}
License: GPL
Group: local
URL: https://github.com/IwaoWatanabe/unix-c
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: doxygen libXaw-devel openmotif-devel ncurses-devel
Requires: libXaw ncurses openmotf

%define prefix /opt/examples-cpp

Prefix: %prefix
AutoReqProv: no

%description
UNIX C/C++ sample codes.

%prep
%setup -q -c

%build
make USE_MOTIF=1

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=%buildroot install

mkdir -p $RPM_BUILD_ROOT%prefix/config $RPM_BUILD_ROOT%prefix/work
chmod a+w $RPM_BUILD_ROOT%prefix/config 
chmod a+rwx,o+t $RPM_BUILD_ROOT%prefix/work

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc
%prefix

%changelog
* Thu May 23 2013 Iwao Watanabe <i.watanabe@hottolink.co.jp> - c-1
- Initial build.

