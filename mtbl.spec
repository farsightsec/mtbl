Name:           mtbl
Version:        1.3.0
Release:        1%{?dist}
Summary:	immutable sorted string table utilities

License:        Apache-2.0
URL:            https://github.com/farsightsec/%{name}
Source0:        https://dl.farsightsecurity.com/dist/%{name}/%{name}-%{version}.tar.gz


BuildRequires:  zlib-devel lz4-devel libzstd-devel snappy-devel
#Requires:       
# TODO: will Requires be set automatically?

%description
mtbl is a C library implementation of the Sorted String Table (SSTable)
data structure. mtbl exposes primitives for creating, searching and
merging SSTable files.

This package contains the shared library for libmbtl and the mtbl
command-line tools.

%package devel
Summary:	immutable sorted string table library (development files)
Requires:	%{name}%{?_isa} = %{version}-%{release} zlib lz4 libzstd snappy

%description devel
mtbl is a C library implementation of the Sorted String Table (SSTable)
data structure. mtbl exposes primitives for creating, searching and
merging SSTable files.

This package contains the static library, headers, and development
documentation for libmtbl.

%prep
%setup -q

%build
[ -x configure ] || autoreconf -fvi
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
%make_install


%files
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%exclude %{_libdir}/libmtbl.la
%_bindir/*
%_mandir/man1/*

%files devel
%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/pkgconfig/*
%{_includedir}/*
%_mandir/man3/*
%_mandir/man7/*


%doc



%changelog
