Name: ::package_name::
Version: ::package_version::
Release: ::package_build_version::%{?dist}
Summary: ::package_description_short::
License: ::package_licence::
URL: ::package_url::
Source0: %{name}-%{version}.tar.gz
BuildArch: ::package_architecture_el::
Requires: ::package_dependencies_el::

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
::package_title::
::package_description_long::

%prep
%setup -q

%build
make -j$(nproc)

%install
make DESTDIR=%{buildroot} PACKAGING=1 install

%clean
make DESTDIR=%{buildroot} clean

%files
%defattr(-,root,root,-)
%config(noreplace) %{_sysconfdir}/cephfssyncd.conf
/lib/systemd/system/cephfssyncd.service
%{_bindir}/*
/opt/45drives/%{name}/*
%docdir /usr/share/man/man8
%doc /usr/share/man/man8/*
/usr/share/bash-completion/completions/*

%post
systemctl daemon-reload

%preun
systemctl stop cephfssyncd.service

%postun
systemctl daemon-reload

%changelog
* Fri Jun 18 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.11-1
- Replace ':' with '-' in log file names for portability of files.

* Thu Jun 17 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.10-1
- Retry batch on partial transfer failure.

* Wed May 19 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.9-1
- When execution fails from too many arguments, the argv headroom
  is increased and execution is tried again.
- STDOUT and STDERR of sync process are logged to a file on failure.

* Mon Apr 26 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.8-1
- Signigicant optimizations for modifying file paths and for comparing
  file size and type to make crawl time up to 6 times faster.
- Lowered memory usage by using iterators and c-string pointers rather
  than copying the vector of file objects into partitions to distribute
  paths amongst sync processes.
- Make systemd service restart on abnormal exit (killed by signal).

* Thu Apr 22 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.7-1
- Fixed deadlock issue with ConcurrentQueue while multithreaded crawling.
- Optimized multithreaded crawl to cut crawl time almost in half.
- Optimized vector sort compare function to speed up sort before launching
  sync program.

* Mon Apr 19 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.6-1
- Remove many redundant filesystem stat() calls while gathering and
  sorting files to speed up vector sort.
- Change config to noreplace in packaging.

* Wed Apr 07 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.5-2
- First EL8 build.

* Wed Mar 24 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.5-1
- Add EXIT_FAILED (4) status for when cephfssyncd exits with EXIT_FAILURE.
- Check that Source Directory exists after loading config.

* Tue Mar 23 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.4-1
- Implement status sharing for prometheus metrics exporter.

* Wed Mar 03 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.3-1
- Add oneshot flag to manually sync changes once and exit.

* Tue Mar 02 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.2-2
- Overhaul packaging using Docker.

* Mon Mar 01 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.2-1
- Use parallel execution policy for sorting file list, shortening time between
  finding files and sending files. (Not implemented in Centos 7)
- Added verbose exit status logging for rsync errors.

* Wed Feb 17 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.1-1
- Updated man page formatting.
- Moved version definition to its own header file.
- --version now prints cephgeorep VERS instead of cephgeorep vVERS
- add -V flag along with --version
- add bash tab completion script

* Fri Feb 05 2021 Josh Boudreau <jboudreau@45drives.com> 1.1.4-1
- Fix crash with self-referencing symlinks

* Tue Feb 02 2021 Josh Boudreau <jboudreau@45drives.com> 1.2.0-1
- Added -S --set-last-change flag to prime the last change time to only 
  sync changes that occur after running with this flag.
- Added --version flag to print version and exit.
- Implemented destination failover with multiple destinations
  when SSH fails. Output of rsync is muted to clean up log messages.
- Last rctime now flushes to disk and snap path is deleted in the
  signal handler, and before exiting in Logging::log.error().

* Tue Feb 02 2021 Josh Boudreau <jboudreau@45drives.com> 1.1.3-3
- Updated man page to be installed to man 8 instead of man 1.

* Mon Feb 01 2021 Josh Boudreau <jboudreau@45drives.com> 1.1.3-2
- Updated man page with new configuration file path.

* Mon Feb 01 2021 Josh Boudreau <jboudreau@45drives.com> 1.1.3-1
- Fixed a bug where new_rctime isn't initialized when declared so
  a random value would be assigned.
- Periodically flush last sync timestamp to disk, at most once per hour.
- Add postinst script to trigger systemctl daemon-reload.
- Add prerm and postrm scripts to stop systemd unit and
  tirgger daemon-reload.

* Thu Jan 21 2021 Josh Boudreau <jboudreau@45drives.com> 1.1.2-1
- Bug fix in configuration parsing where an underflow would cause a crash where = was missing

* Thu Jan 21 2021 Josh Boudreau <jboudreau@45drives.com> 1.1.1-2
- Add man page entry for cephgeorep(1)

* Wed Jan 20 2021 Josh Boudreau <jboudreau@45drives.com> 1.1.1-1
- Add log message for when a sync process is relaunched with more files to show that all files have synced

* Tue Jan 19 2021 Josh Bodureau <jboudreau@45drives.com> 1.1.0
- Implement multithreaded BFS directory tree traversal to speed up finding new files
- Add -d --dry-run flag to check function before actually syncing
- Add -t --threads flag to specify number of searcher threads (overrides config)

* Mon Jan 11 2021 Josh Boudreau <jboudreau@45drives.com> 0.4.0
- Create multiple parallel processes of the sync binary

* Thu Aug 13 2020 Josh Boudreau <jboudreau@45drives.com> 0.3.4
- Standard CLI flag parsing with getopt

* Wed Jul 22 2020 Josh Boudreau <jboudreau@45drives.com> 0.3.3
- Split queue of files into batches based on ARG_MAX

* Tue Dec 10 2019 Josh Boudreau <jboudreau@45drives.com> 0.3
- First Build
