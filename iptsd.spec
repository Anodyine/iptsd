%global debug_package %{nil}

Name: iptsd
Version: 0.5.1
Release: 1%{?dist}
Summary: Userspace daemon for Intel Precise Touch & Stylus
License: GPLv2+

URL: https://github.com/linux-surface/iptsd
Source: {{{ create_tarball }}}

BuildRequires: meson
BuildRequires: gcc-g++

# Some of our dependencies can only be resolved with cmake
BuildRequires: cmake

# Daemon
BuildRequires: pkgconfig(fmt)
BuildRequires: pkgconfig(inih)
BuildRequires: cmake(Microsoft.GSL)
BuildRequires: pkgconfig(spdlog)
BuildRequires: pkgconfig(libdrm)
BuildRequires: hidrd-devel

# Debug Tools
BuildRequires: cmake(CLI11)
BuildRequires: pkgconfig(cairomm-1.0)
BuildRequires: pkgconfig(gtkmm-3.0)

BuildRequires: pkgconfig(systemd)
BuildRequires: pkgconfig(udev)
BuildRequires: systemd-rpm-macros

%description
iptsd is a userspace daemon that processes touch events from the IPTS
kernel driver, and sends them back to the kernel using uinput devices.

%prep
%autosetup

%build
# Give us all the O's
%global optflags %(echo %{optflags} | sed 's|-O2||g' | sed 's|-mtune=generic||g')

%meson --buildtype=release
%meson_build

%install
%meson_install

%check
%meson_test

%files
%license LICENSE
%doc README.md
%config(noreplace) %{_sysconfdir}/ipts.conf
%{_bindir}/iptsd
%{_bindir}/iptsd-finger-size
%{_bindir}/ipts-dump
%{_bindir}/ipts-proto-plot
%{_bindir}/ipts-proto-rt
%{_unitdir}/iptsd@.service
%{_udevrulesdir}/50-ipts.rules
%{_datadir}/ipts/*
