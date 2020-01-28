%define realname give
%define realversion 3.1-5

#don't strip binaries
%define __os_install_post /usr/lib/rpm/brp-compress
%define debug_package %{nil}

Name: %{realname}-%{realversion}
Version: ptools
Release: 1
Summary: lc file transfer utility
License: LLNL Internal
Group: System Environment/Base
Source0: %{realname}-%{realversion}.tgz
BuildRoot: %{_tmppath}/%{name}-buildroot
URL: https://www.git.lanl.gov/filesystems/give


######################################################################
%prep
%setup -n %{name}

#Wonderful process to check for config options...wish there was an %elif for this! 
#Checks to see if both defined, if both aren't defined it checks one, then the other, 
#finally if none... just run config with defaults and make.
#For Release 3:
#Also define a text string to include in the RPM description, stating which (if any) of these options was used.
%build
%if %{?strict_checks:1}%{!?strict_checks:0} && %{?alt_givedir:1}%{!?alt_givedir:0}
	%if "%{strict_checks}" == "no" || "%{strict_checks}" == "No" || "%{strict_checks}" == "NO"
		%configure --enable-non-strict-checks --enable-givedir=%{alt_givedir}
		%define local_options Built with non-strict-checks and alt givedir=%{alt_givedir}
		make
	%else
		echo "*****BAD PARAM TO STRICT-CHECKS, ACCEPTABLE VALS: no, No, or NO. IF YOU WANT STRICT CHECKING DO NOT DEFINE STRICT CHECKS. IT IS ENABLED BY DEFAULT*****."
		exit -1
	%endif
%else
	%if %{?strict_checks:1}%{!?strict_checks:0}
		%if "%{strict_checks}" == "no" || "%{strict_checks}" == "No" || "%{strict_checks}" == "NO"
			%configure --enable-non-strict-checks
			%define local_options Built with non-strict-checks only
			make
		%else
			echo "*****BAD PARAM TO STRICT-CHECKS, ACCEPTABLE VALS: no, No, or NO. IF YOU WANT STRICT CHECKING DO NOT DEFINE STRICT CHECKS. IT IS ENABLED BY DEFAULT*****."
			exit -1
		%endif
	%else
		%if %{?alt_givedir:1}%{!?alt_givedir:0}
			%configure --enable-givedir=%{alt_givedir}
			%define local_options Built with alt givedir=%{alt_givedir} only
			make
		%else
			%define local_options Built with default values, strict checking and /usr/givedir
			%configure
			make
		%endif
	%endif
%endif


%description
Give and take are a set of companion utilities that allow a 
secure transfer of files from one user to another without 
exposing the files to third parties.  
%local_options

%install
rm -rf "$RPM_BUILD_ROOT"
mkdir -p $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
install give-assist $RPM_BUILD_ROOT%{_bindir}
install give.py $RPM_BUILD_ROOT%{_bindir}/give
install -m 644 give.1 $RPM_BUILD_ROOT%{_mandir}/man1
ln $RPM_BUILD_ROOT%{_bindir}/give $RPM_BUILD_ROOT%{_bindir}/take
install -m 644 take.1 $RPM_BUILD_ROOT%{_mandir}/man1
# # # This step fails on the Crays, and appears to be redundant. Skip it.
# # # DESTDIR="$RPM_BUILD_ROOT"%makeinstall

#######################################################################

%clean
rm -rf $RPM_BUILD_ROOT


#######################################################################
%post

%files
%defattr(-,root,root,0755)
%attr(4555,root,root)%{_bindir}/give-assist
%{_bindir}/give
%{_bindir}/take
%{_mandir}/man1/*
	

%changelog
 * Tue Aug 14 2012 Dominic Manno <dmanno@lanl.gov>
- Original; LANL version to add alt-givedir and no-strict-checking options
 * Thu Nov 01 2012 Georgia Pedicini <gap@lanl.gov>
- LANL version 3.1-2, tighten permissions
 * Tue Nov 06 2012 Georgia Pedicini <gap@lanl.gov>
- Added defined text string to include in description, citing which (if any) options were used in the build.
 * Wed May 25 2016 Dominic Manno <dmanno@lanl.gov>
- Converted to be python2 and python3 compatible, mostly print statement to function calls 
