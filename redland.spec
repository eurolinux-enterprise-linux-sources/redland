Name:           redland
Version:        1.0.16
Release:        6%{?dist}
Summary:        RDF Application Framework

Group:          System Environment/Libraries
License:        LGPLv2+ or ASL 2.0
URL:            http://librdf.org/
Source0:        http://download.librdf.org/source/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  curl-devel
BuildRequires:  libdb-devel
BuildRequires:  libiodbc-devel
BuildRequires:  libtool-ltdl-devel
BuildRequires:  libxml2-devel >= 2.4.0
BuildRequires:  mysql-devel
BuildRequires:  postgresql-devel
BuildRequires:  raptor2-devel 
BuildRequires:  rasqal-devel >= 0.9.26
BuildRequires:  sqlite-devel

%description
Redland is a library that provides a high-level interface for RDF
(Resource Description Framework) implemented in an object-based API.
It is modular and supports different RDF/XML parsers, storage
mechanisms and other elements. Redland is designed for applications
developers to provide RDF support in their applications as well as
for RDF developers to experiment with the technology.

%package         devel
Summary:         Libraries and header files for programs that use Redland
Group:           Development/Libraries
Requires:        %{name}%{?_isa} = %{version}-%{release}
%description     devel
Header files for development with Redland.

%package         mysql
Summary:         MySQL storage support for Redland
Group:           System Environment/Libraries
Requires:        %{name}%{?_isa} = %{version}-%{release}
%description     mysql
This package provides Redland's storage support for graphs in memory and
persistently with MySQL files or URIs.

%package         pgsql
Summary:         PostgreSQL storage support for Redland
Group:           System Environment/Libraries
Requires:        %{name}%{?_isa} = %{version}-%{release}
%description     pgsql
This package provides Redland's storage support for graphs in memory and
persistently with PostgreSQL files or URIs.

%package         virtuoso
Summary:         Virtuoso storage support for Redland
Group:           System Environment/Libraries
Requires:        %{name}%{?_isa} = %{version}-%{release}
%description     virtuoso
This package provides Redland's storage support for graphs in memory and
persistently with Virtuoso files or URIs.


%prep
%setup -q

# hack to nuke rpaths
%if "%{_libdir}" != "/usr/lib"
sed -i -e 's|"/lib /usr/lib|"/%{_lib} %{_libdir}|' configure
%endif

%build
export CFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing"
export CXXFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing"
%configure \
  --enable-release \
  --disable-static 

make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT

make install DESTDIR=$RPM_BUILD_ROOT

#unpackaged files
find $RPM_BUILD_ROOT -name \*.la -exec rm {} \;


%check
make check


%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING COPYING.LIB LICENSE.txt NEWS README
%doc LICENSE-2.0.txt NOTICE TODO
%doc FAQS.html LICENSE.html NEWS.html README.html TODO.html
%{_libdir}/librdf.so.0*
%{_bindir}/rdfproc
%{_bindir}/redland-db-upgrade
%dir %{_datadir}/redland
%{_datadir}/redland/mysql-v1.ttl
%{_datadir}/redland/mysql-v2.ttl
%{_mandir}/man1/redland-db-upgrade.1*
%{_mandir}/man1/rdfproc.1*
%{_mandir}/man3/redland.3*
%dir %{_libdir}/redland
%{_libdir}/redland/librdf_storage_sqlite.so

%files mysql
%defattr(-,root,root,-)
%{_libdir}/redland/librdf_storage_mysql.so

%files pgsql
%defattr(-,root,root,-)
%{_libdir}/redland/librdf_storage_postgresql.so

%files virtuoso
%defattr(-,root,root,-)
%{_libdir}/redland/librdf_storage_virtuoso.so

%files devel
%defattr(-,root,root,-)
%doc ChangeLog RELEASE.html
%{_bindir}/redland-config
%{_datadir}/redland/Redland.i
%{_datadir}/gtk-doc/
%{_includedir}/redland.h
%{_includedir}/librdf.h
%{_includedir}/rdf_*.h
%{_libdir}/librdf.so
%{_libdir}/pkgconfig/redland.pc
%{_mandir}/man1/redland-config.1*


%changelog
* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 1.0.16-6
- Mass rebuild 2014-01-24

* Wed Jan 15 2014 Honza Horak <hhorak@redhat.com> - 1.0.16-5
- Rebuild for mariadb-libs
  Related: #1045013

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 1.0.16-4
- Mass rebuild 2013-12-27

* Tue May 28 2013 Lukáš Tinkl <ltinkl@redhat.com> 1.0.16-3
- use -fno-strict-aliasing

* Tue Mar 26 2013 Rex Dieter <rdieter@fedoraproject.org> 1.0.16-2
- rdfproc: Failed to open hashes storage (#914634)

* Tue Feb 19 2013 Rex Dieter <rdieter@fedoraproject.org> 1.0.16-1
- 1.0.16

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.15-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.15-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Mon Mar 05 2012 Rex Dieter <rdieter@fedoraproject.org> 1.0.15-1
- 1.0.15

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.14-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Sat Jul 23 2011 Rex Dieter <rdieter@fedoraproject.org> 1.0.14-1
- 1.0.14

* Wed Mar 23 2011 Rex Dieter <rdieter@fedoraproject.org> 1.0.12-3
- rebuild (mysql)

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.12-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Thu Oct 14 2010 Orcan Ogetbil <oget[DOT]fedora[AT]gmail[DOT]com> - 1.0.12-1
- Update to 1.0.12

* Sun Oct 03 2010 Orcan Ogetbil <oget[DOT]fedora[AT]gmail[DOT]com> - 1.0.11-1
- Update to 1.0.11

* Wed Sep 29 2010 jkeating - 1.0.10-8
- Rebuilt for gcc bug 634757

* Sat Sep 11 2010 Orcan Ogetbil <oget[DOT]fedora[AT]gmail[DOT]com> - 1.0.10-7
- Don't require gtk-doc RHBZ#604414

* Wed Jun 09 2010 Orcan Ogetbil <oget[DOT]fedora[AT]gmail[DOT]com> - 1.0.10-6
- Separate the Virtuoso plugin into its own subpackage

* Sat May 08 2010 Orcan Ogetbil <oget[DOT]fedora[AT]gmail[DOT]com> - 1.0.10-5
- Separate the MySQL and PostgreSQL plugins into their own subpackages

* Sat Feb 13 2010 Orcan Ogetbil <oget[DOT]fedora[AT]gmail[DOT]com> - 1.0.10-4
- Fix DSO linking error RHBZ#564859
- Link to our own libltdl

* Mon Jan 04 2010 Rex Dieter <rdieter@fedoraproject.org> - 1.0.10-3
- no_undefined patch

* Sun Jan 03 2010 Rex Dieter <rdieter@fedoraproject.org> - 1.0.10-2
- pkgconfig_requires_private patch

* Sun Jan 03 2010 Rex Dieter <rdieter@fedoraproject.org> - 1.0.10-1
- redland-1.0.10

* Tue Nov 24 2009 Caolán McNamara <caolanm@redhat.com> - 1.0.7-10.2
- Resolves: rhbz#540519 Rebuild against db4-4.8

* Fri Aug 28 2009 Rex Dieter <rdieter@fedoraproject.org> 1.0.7-10.1
- temporarily drop mysql support (restore once mysql is unbroken in rawhide)

* Thu Aug 27 2009 Rex Dieter <rdieter@fedoraproject.org> 1.0.7-10
- fix build with newer sqlite (#519781)

* Fri Aug 21 2009 Tomas Mraz <tmraz@redhat.com> - 1.0.7-9
- rebuilt with new openssl

* Sun Jul 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.7-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Fri May 01 2009 Rex Dieter <rdieter@fedoraproject.org> - 1.0.7-7
- slighgly less ugly rpath hack
- cleanup %%files

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.7-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Thu Jan 22 2009 Rex Dieter <rdieter@fedoraproject.org> 1.0.7-5 
- respin (mysql)

* Fri Jan 16 2009 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.7-4
- rebuild for new OpenSSL

* Sun Nov 23 2008 Thomas Vander Stichele <thomas at apestaart dot org>
- 1.0.7-3
- updated summary
- not rebuilt yet 

* Thu Jul 10 2008 Tom "spot" Callaway <tcallawa@redhat.com> 1.0.7-2
- rebuild for db4-4.7

* Sat Feb 09 2008 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.7-1
- update to 1.0.7
- update minimum raptor and rasqal versions

* Tue Dec 04 2007 Rex Dieter <rdieter[AT]fedoraproject.org> 1.0.6-3
- respin for openssl

* Tue Oct 16 2007 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.6-2
- fix unpackaged files and unowned directory

* Tue Oct 16 2007 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.6-1
- update to 1.0.6 (for Soprano 2, also some bugfixes)
- update minimum raptor and rasqal versions
- drop sed hacks for dependency bloat (#248106), fixed upstream

* Wed Aug 22 2007 Rex Dieter <rdieter[AT]fedoraproject.org> 1.0.5-6
- respin (BuildID)

* Fri Aug 3 2007 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.5-5
- specify LGPL version in License tag

* Sat Jul 14 2007 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.5-4
- get rid of redland-config dependency bloat too (#248106)

* Sat Jul 14 2007 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.5-3
- fix bug number in changelog

* Sat Jul 14 2007 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.5-2
- add missing Requires: pkgconfig to the -devel package
- get rid of pkgconfig dependency bloat (#248106)

* Thu Jun 28 2007 Kevin Kofler <Kevin@tigcc.ticalc.org> 1.0.5-1
- update to 1.0.5 (1.0.6 needs newer raptor and rasqal than available)
- update minimum raptor version

* Fri Dec 15 2006 Thomas Vander Stichele <thomas at apestaart dot org>
- 1.0.4-3
- use DESTDIR

* Sat Jun 17 2006 Thomas Vander Stichele <thomas at apestaart dot org>
- 1.0.4-2
- fixed x86_64 rpath issue with an ugly hack
- removed OPTIMIZE from make invocation
- added smp flags
- added make check
- updated license

* Sun May 14 2006 Thomas Vander Stichele <thomas at apestaart dot org>
- 1.0.4-1
- update to new release, needs later raptor
- remove patch

* Sat Apr 08 2006 Thomas Vander Stichele <thomas at apestaart dot org>
- 1.0.3-1
- update to latest release
- include patch for fclose() double-free

* Sat Apr 08 2006 Thomas Vander Stichele <thomas at apestaart dot org>
- 1.0.2-1
- package for Fedora Extras

* Wed Feb 15 2006  Dave Beckett <dave@dajobe.org>
- Require db4-devel

* Thu Aug 11 2005  Dave Beckett <dave.beckett@bristol.ac.uk>
- Update Source:
- Do not require python-devel at build time
- Add sqlite-devel build requirement.
- Use configure and makeinstall

* Thu Jul 21 2005  Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for gtk-doc locations

* Mon Nov 1 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- License now LGPL/Apache 2
- Added LICENSE-2.0.txt and NOTICE

* Mon Jul 19 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- move perl, python packages into redland-bindings

* Mon Jul 12 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- put /usr/share/redland/Redland.i in redland-devel

* Wed May  5 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.3.0
- require rasqal 0.2.0

* Fri Jan 30 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.2.0
- update for removal of python distutils
- require python 2.2.0+
- require perl 5.8.0+
- build and require mysql
- do not build and require threestore

* Sun Jan 4 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- added redland-python package
- export some more docs

* Mon Dec 15 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.1.0
- require libxml 2.4.0 or newer
- added pkgconfig redland.pc
- split redland/devel package shared libs correctly

* Mon Sep 8 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.0.0
 
* Thu Sep 4 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- added rdfproc
 
* Thu Aug 28 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- patches added post 0.9.13 to fix broken perl UNIVERSAL::isa
 
* Thu Aug 21 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Add redland-db-upgrade.1
- Removed duplicate perl CORE shared objects

* Sun Aug 17 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updates for new perl module names.

* Tue Apr 22 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for Redhat 9, RPM 4

* Fri Feb 12 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for redland 0.9.12

* Fri Jan 4 2002 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for new Perl module names

* Fri Sep 14 2001 Dave Beckett <dave.beckett@bristol.ac.uk>
- Added shared libraries
