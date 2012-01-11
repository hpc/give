give
====
Give-ng is a reimplementation of our give and take programs, with careful
consideration of security in mind.  A small suid give-assist program is
used to remove all privilege from the "give" and "take" programs, which
can now be implemented in any convienient language- even as shell scripts.

Depends on the string_m library for secure string manipulation.
