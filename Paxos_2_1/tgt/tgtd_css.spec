Name: tgt-css-target
Version: 1.1.1
Release: 1%{?dist}
Group: Applications/System
Vendor: triTech Inc.
URL: http://www.tritech.co.jp/
Packager: Masanari Kubo <kubo@tritech.co.jp>
License: GPL
Summary: TGT target to use Paxos Storage Service
Summary(ja): CSS(Cell Storage Service)のサーバ
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildArch: x86_64

%description
TGT target to use Cell Storage Service

%description -l ja
css(Cell Storage Service)をバッキングストアとしたTGT targetパッケージ

%install
install -m755 -d $RPM_BUILD_ROOT/sbin
install -m755 -d $RPM_BUILD_ROOT/etc/init.d
install -m755 -d $RPM_BUILD_ROOT/etc/tgt
install $ORG_DIR/tgtd.css $RPM_BUILD_ROOT/sbin/
install $ORG_DIR/createISCSIVol $RPM_BUILD_ROOT/sbin/
install $ORG_DIR/tgt-admin $RPM_BUILD_ROOT/sbin/
install $ORG_DIR/$TGT_DIR/tgtadm $RPM_BUILD_ROOT/sbin/
install $ORG_DIR/tgtd.css_init.d  $RPM_BUILD_ROOT/etc/init.d/tgtd.css
install $ORG_DIR/example_targets.conf  $RPM_BUILD_ROOT/etc/tgt

%clean
rm -rf $RPM_BUILD_ROOT

%files
/sbin/*
/etc/init.d/*
/etc/tgt/*
