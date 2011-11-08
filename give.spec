Name: give
Version: 3.0n
Release: 2%{?dist}
Summary: lc file transfer utility
License: LLNL Internal
Group: System Environment/Base
Source0: give-3.0n-2.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)
Packager: Shawn Instenes <shawni@llnl.gov>

%define debug_package %{nil}

%description
Give and take are a set of companion utilities that allow a 
secure transfer of files form one user to another without exposing 
the files to third parties.  

%prep
%setup -n give-3.0n-2

%build
cd string_m
make
cd ..
make

%install
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
install give-assist $RPM_BUILD_ROOT%{_bindir}
install give $RPM_BUILD_ROOT%{_bindir}
install -m 644 give.1 $RPM_BUILD_ROOT%{_mandir}/man1
ln $RPM_BUILD_ROOT%{_bindir}/give $RPM_BUILD_ROOT%{_bindir}/take
install -m 644 take.1 $RPM_BUILD_ROOT%{_mandir}/man1

%clean
rm -rf $RPM_BUILD_ROOT

%post

%files
%defattr(-,root,root)
%attr(4555,root,root)%{_bindir}/give-assist
%attr(0555,root,root)%{_bindir}/give
%attr(0555,root,root)%{_bindir}/take
%{_mandir}/man1/*

%changelog
* Tue Oct 26 2009 Trent D'Hooge <tdhooge@llnl.gov> 3.0-2
  -  fix the two outstanding issues of auto-correcting the permissions 
     of the give directories and touching the files copied into the give 
     repository so they aren't prematurely reaped.

* Tue Jun 2 2009 Trent D'Hooge <tdhooge@llnl.gov> 3.0-1
  - goto new take 

* Tue Jan 20 2009 Trent D'Hooge <tdhooge@llnl.gov> 3.0-0
  - new code from Shawn Instenes <shawni@llnl.gov>
  - old code for take
  - new code for give

* Mon Aug 21 2006 Jim Garlick <garlick@llnl.gov> 1.25b-1
- Initial packaging attempt
