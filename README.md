give
====
A tool to transfer permission of files to others in a linux based environment.

Currently in development!
-------------------------
This tool is currently undergoing significant modifications. Please do NOT
depend on this version of the give program in any production environment if
you care about security. This codebase is a starting point that we intend to
significantly update.

How does it work?
-----------------
Give is a reimplementation of our give and take programs, with careful
consideration of security in mind.  A small suid give-assist program is
used to remove all privilege from the "give" and "take" programs, which
can now be implemented in any convienient language- even as shell scripts.

Depends on the string_m library for secure string manipulation.

Quick Start
-----------

```tar -czvf give-3.0n-2.tgz give-3.0n-2```
```rpmbuild -ta give-3.0n-2.tgz --with check_all_gids --with use_special_gid --with alternate_dir```
