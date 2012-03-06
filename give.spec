Name: give
Version: 3.0n_ptools
Release: 4
Summary: lc file transfer utility (LANL Build of 3.0n-2)
License: LLNL Internal
Group: System Environment/Base
Source: %{name}-%{version}-%{release}.tgz                                       
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}                            
URL: https://www.git.lanl.gov/filesystems/give

%description
Give and take are a set of companion utilities that allow a
secure transfer of files form one user to another without exposing
the files to third parties.

# Don't strip binaries                                                             
%define __os_install_post /usr/lib/rpm/brp-compress                                
%define debug_package %{nil} 

# Check for configure options
%define give_with_opt() %{expand:%%{!?_without_%{1}:%%global give_with_%{1} 1}}
%define give_without_opt() %{expand:%%{?_with_%{1}:%%global give_with_%{1} 1}}

%give_with_opt check_all_gids
%give_without_opt use_special_gid
%give_with_opt alternate_dir

###############################################################################

%prep                                                                              
%setup -n %{name}-%{version}-%{release}                                            
                                                                                   
%build                                                                             
%configure --program-prefix=%{?_program_prefix:%{_program_prefix}} \
    %{?give_with_check_all_gids:--enable-check-all-gids} \
    %{?give_with_use_special_gid:--enable-use-gid=100} \
    %{?give_with_alternate_dir:--enable-give-dir=/net/givedir}

make %{?_smp_mflags}                                                               

%install
rm -rf "$RPM_BUILD_ROOT"                                                        
mkdir -p $RPM_BUILD_ROOT%{_bindir}
install src/give-assist $RPM_BUILD_ROOT%{_bindir}
install give.py $RPM_BUILD_ROOT%{_bindir}/give
ln $RPM_BUILD_ROOT%{_bindir}/give $RPM_BUILD_ROOT%{_bindir}/take
DESTDIR="$RPM_BUILD_ROOT" make install                                          

###############################################################################

%clean                                                                          
rm -rf $RPM_BUILD_ROOT

###############################################################################

%files
%defattr(-,root,root,0755)
%attr(4555, -, root) %{_bindir}/give-assist
%{_bindir}/give
%{_bindir}/take
%{_mandir}/man1/give.1.gz
%{_mandir}/man1/take.1.gz
