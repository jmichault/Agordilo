%define         name 		accordeur
%define         version 	1.0.1

# this needs to be changed depending on build number
%define		release 	1

# This should be set to your os.
# Possible values are  Mandrake, Red Hat, Turbolinux, Caldera, SuSE, Debian, etc.
%define		ostype 		Mandrake

# This should be set to the version of your OS (6.0, 6.1, 6.2, 7.0, 7.1, 7.2, 8.0, etc.)
%define		osversion 	2007.0

# This is your cpu i486, i586, i686, ppc, sparc, alpha, etc.
%define		buildarch 	i586

# This the RPM group on your system that this will installed into.
# Graphical desktop/KDE, X11/apps, etc.
%define		rpmgroup 	Graphical desktop/KDE

Summary:        Accordeur %{version} is an instrument tuner.
Name:		%{name}
Version:        %{version}
Release:        %{release}
License:      GPL
Vendor:         Accordeur's developers
Url:            http://accordeur.sourceforge.net/
Packager:       Jean Michault
Group:          %{rpmgroup}
BuildArch:      %{buildarch}
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-root
Prefix: 	/usr
#Requires:	libqt4-devel >= 4.1
Distribution:	%{ostype} %{osversion}
BuildRequires: 	libqt4-devel

%description
Accordeur %{version}-%{release} is a program designed to help you to tune your musical instrument.

This is  a relocatable package, you can install it on different target with
rpm -Uvh accordeur-%{version}-%{release}.rpm --prefix /usr/local/opt/apps/noncommercial
default is %{_prefix}

This rpm was compiled on a %{ostype} %{osversion} system for %{buildarch} class cpu's.


%prep
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%setup -q
qmake accordeur.pro
make
lrelease accordeur.pro


%install
mkdir -p %{buildroot}/usr/bin
#cp -p accordeur %{buildroot}/usr/bin
install -s -m 755 accordeur %{buildroot}/usr/bin
#mkdir -p %{buildroot}/usr/share/applnk-mdk/Multimedia/Sound/
#install -m 644 accordeur.desktop %{buildroot}/usr/share/applnk-mdk/Multimedia/Sound/
mkdir -p %{buildroot}/usr/share/accordeur
install -m 644 accordeur_*.qm instruments.txt %{buildroot}/usr/share/accordeur
mkdir -p %{buildroot}/usr/share/doc/accordeur
cp -Rp doc/* %{buildroot}/usr/share/doc/accordeur
install -d %{buildroot}%{_menudir}
install -m 644 accordeur.menu %{buildroot}%{_menudir}/accordeur
install -d %{buildroot}%{_iconsdir}
install -m 644 accordeur.png %{buildroot}%{_iconsdir}
install -d %{buildroot}%{_iconsdir}/large
install -m 644 accordeur.large.png %{buildroot}%{_iconsdir}/large
install -d %{buildroot}%{_iconsdir}/mini
install -m 644 accordeur.mini.png %{buildroot}%{_iconsdir}/mini
install -d %{buildroot}/usr/share/applications
install -m 644 accordeur.desktop %{buildroot}/usr/share/applications

%post
%{update_menus}

%postun
%{clean_menus}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{prefix}/bin/*
%{prefix}/share/accordeur/*
%{prefix}/share/doc/accordeur/*
%{prefix}/share/applications/*
%{_menudir}/*
%{_iconsdir}/*png
%{_iconsdir}/*/*png

%changelog

