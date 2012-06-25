#!/usr/bin/env python
#
# give/take: securely offer files to another user
#
# Last modified: 2010-05-20 Shawn Instenes <shawni@llnl.gov>
#
# Work flow:
#
# Validate args: giver, taker, gt-<taker_uid>
#
# For give:
# Invoke give-assist <taker>- if it fails, we stop.
#
# Copy (with checking), filename from current directory to
# spool directory.  Chmod destination file.  Touch file (to update ctime).
#
# Opts to honor:
#
# -Vulhnrif
# V: version (use --version)
# u: ungive
# l: list
# n: compact list (not implmented in give 1.25a)
# r: recursive give (not imp. in give 1.25a)-- WE DO NOT YET IMPLEMENT
# i: interactive
# f: force overwrite
# h: help
#
# assume give- if named 'take' assume take.
#
# if invoked with no args, then list (have given, or to take).
#
# Python 2.3+
#####################################################################
# 

import sys, os, pwd, grp, time
from optparse import OptionParser
try:
	import readline
except:
	pass


VERSION="%prog 3.0n"
SPOOL="/usr/give"
ASSIST="/usr/bin/give-assist"
COPY="/bin/cp"
TOUCH="/bin/touch"
assist_rec = {}
debug_mode = False

#####################################################################
#  Functions
#
# Try to run give-assist only once.
#
def run_assist(toname=None):
	if toname == None:
		return
	if toname not in assist_rec:
		assist_rec.setdefault(toname, True)
		try:
			os.spawnlp(os.P_WAIT, ASSIST, ASSIST, toname)
		except:
			print "Problem running %s." % ASSIST
			sys.exit(1)

def run_copy(frompath=None, topath=None, preserve=True):
	retcode = 0
	if (frompath == None or topath == None):
		print "run_copy: bad args"
		sys.exit(1)
	if os.access(frompath, os.F_OK):
		try:
			if preserve:
				retcode = os.spawnlp(os.P_WAIT, COPY, COPY, '-p', frompath, topath)
			else:
				retcode = os.spawnlp(os.P_WAIT, COPY, COPY, frompath, topath)
		except:
			print "Problem running %s." % COPY
			sys.exit(1)
	else:
		print "No such file to copy: %s." % frompath
		retcode = 1 # signal a problem
	return retcode

def fix_perms(path=None):
	if (path == None):
		print "Cannot fix permissions: no path"
		sys.exit(1)
	try:
		statbuf = os.stat(path)
	except:
		print "No such file %s." % path
		sys.exit(1)

	oldmode = statbuf.st_mode & 0777
	newmode = oldmode | 0444 # make readable
	try:
		os.chmod(path, newmode)
	except:
		print "Failed chmod on file %s." % path
		sys.exit(1)
	try:
			retcode = os.spawnlp(os.P_WAIT, TOUCH, TOUCH, path)
	except:
		print "Failed touch operation on file %s." % path
		sys.exit(1)
	

def isokto(prompt=None):
	if prompt == None:
		prompt = "? "
	yn = raw_input(prompt)
	
	if (yn[0:1] == "y" or yn[0:1] == "Y"):
		return True
	else:
		return False

def give_file(fuid=None, tuid=None, path=None, prompt=False):
	retcode = 0
	count = 1
	if (tuid != None):	
		run_assist(tuid)
	else:
		print "Attempt to give file to <None>\n"
		sys.exit(1)
	if (fuid != None):
		if prompt and not isokto("Give file %s to %s? " % (path, tuid)):
			return 0
		try:
			if debug_mode:
				print "copy ('%s','%s')" % (path, SPOOL+"/"+tuid+"/"+fuid)
			if os.access(SPOOL+"/"+tuid+"/"+fuid+"/"+path, os.F_OK):
				if opts.force:
					retcode = run_copy(path, SPOOL+"/"+tuid+"/"+fuid, True)
					if retcode == 0:
						fix_perms(SPOOL+"/"+tuid+"/"+fuid+"/"+os.path.basename(path))
				else:
					print "File already given, won't overwrite without -f (force)"
					count = 0
			else:
				retcode = run_copy(path, SPOOL+"/"+tuid+"/"+fuid, True)
				if retcode == 0:
					fix_perms(SPOOL+"/"+tuid+"/"+fuid+"/"+os.path.basename(path))
		except Exception, em:
			print "Error %s running %s.\n" % (em, COPY)
			sys.exit(1)
		if retcode:
			count = 0
		return count
	else:
		print "Attempt to give file <None>\n"
		sys.exit(1)

def ungive_file(fuid=None, tuid=None, path=None, prompt=False):
	if (tuid != None and fuid != None and path != None):
		if prompt and not isokto("Ungive file %s, given to %s? " % (path, tuid)):
			return 0
		try:
			if debug_mode:
				print "unlink('%s')" % (SPOOL+"/"+tuid+"/"+fuid+"/"+path)
			if os.access(SPOOL+"/"+tuid+"/"+fuid+"/"+path, os.F_OK):
				os.unlink(SPOOL+"/"+tuid+"/"+fuid+"/"+path)
			else:
				print "No such file to ungive: %s" % (path)
				return 0
		except Exception, em:
			print "Error %s while unlinking.\n" % em
			return 0
		return 1
	else:
		print "Attempt to use bad parameters to ungive_file\n"
		sys.exit(1) 

def take_file(fuid=None, tuid=None, path=None, prompt=False):
	count = 1
	retcode = 0
	if (tuid == None):	
		print "Attempt to use bad parameters to take_file\n"
		sys.exit(1)
	if (path == None):
		print "Attempt to use bad parameters to take_file\n"
		sys.exit(1)
	if (fuid != None):
		if prompt and not isokto("Take file %s from %s? " % (path, fuid)):
			return 0
		try:
			if debug_mode:
				print "copy ('%s','%s')" % (SPOOL+"/"+tuid+"/"+fuid+"/"+path, ".")
			if os.access("./"+path, os.F_OK):
				if opts.force:
					retcode = run_copy(SPOOL+"/"+tuid+"/"+fuid+"/"+path, ".", False)
				else:
					print "File %s already exists in local directory, won't overwrite without -f (force)" % (path)
					retcode = 1
					count = 0
			else:
				retcode = run_copy(SPOOL+"/"+tuid+"/"+fuid+"/"+path, ".", False)
		except Exception, em:
			print "Error %s running %s.\n" % (em, COPY)
			sys.exit(1)
		try:
			if retcode == 0:
				os.unlink(SPOOL+"/"+tuid+"/"+fuid+"/"+path)
			else:
				return 0
		except Exception, em:
			print "Error %s while unlinking.\n" % em
			sys.exit(1)
		return count
	else:
		print "Attempt to use bad parameters to take_file\n"
		sys.exit(1)

def enumerate_files(f=None, t=None):
	if t == None:
		return ()
	if f == None:
		try:
			dirl = os.listdir(SPOOL+"/"+t)
		except:
			dirl = []
		dlist = []
		for d in dirl:
			try:
				sd = os.listdir(SPOOL+"/"+t+"/"+d)
				dlist.append((d, sd))
			except:
				pass	
		return dlist
	else:
		try:
			dlist = os.listdir(SPOOL+"/"+t+"/"+f)
		except:
			dlist = []
	dlist.sort()
	return dlist

def list_a_file(path=None, brief=False):
	if path == None:
		return
	displayname = os.path.basename(path)
	try:
		if brief:
			print "%s" % displayname
		else:
			statbuf = os.stat(path)
			fsize = statbuf.st_size
			# love having bigints built-in to the language.
			if (fsize / 2**80) > 0:
				sc = "YB"; psize = fsize / 2**80
			elif (fsize / 2**70) > 0:
				sc = "ZB"; psize = fsize / 2**70
			elif (fsize / 2**60) > 0:
				sc = "EB"; psize = fsize / 2**60
			elif (fsize / 2**50) > 0:
				sc = "PB"; psize = fsize / 2**50
			elif (fsize / 2**40) > 0:
				sc = "TB"; psize = fsize / 2**40
			elif (fsize / 2**30) > 0:
				sc = "GB"; psize = fsize / 2**30
			elif (fsize / 2**20) > 0:
				sc = "MB"; psize = fsize / 2**20
			elif (fsize / 2**10) > 0:
				sc = "KB"; psize = fsize / 2**10
			else:
				sc = " B"; psize = fsize
			ctime = time.strftime("%b %d %R", time.localtime(statbuf.st_ctime))
			print "%6d %s %s %s" % (psize, sc, ctime, displayname)
	except:
		return

def list_a_directory(path=None, brief=False, header=None):
	try:
			dlist = os.listdir(path)
			if len(dlist) > 0:
				dlist.sort()
				if header != None:
					print "%s" % header
				for fn in dlist:
					list_a_file(path+"/"+fn, brief)
			return len(dlist)
	except:
		return 0

def get_giverpw_from_dir(giver=None, taker=None):
	# in the case we have no getpwnam() data for a giver, we
	# attempt to fake it from data in the directory.
	dirname = SPOOL+"/"+taker+"/"+giver
	try:
		statbuf = os.stat(dirname)
	except:
		return None
	return pwd.struct_passwd.__new__(pwd.struct_passwd, (giver, "x", statbuf.st_uid, statbuf.st_uid, giver, "/no_dir_exists", "/bin/false"))

def list_files_given(myuid=None, theiruid=None, brief=False):
	totalcount = 0
	if myuid != None:
		if theiruid == None:
			dlist = os.listdir(SPOOL)
			dlist.sort()
			for pname in dlist:
				if os.access(SPOOL+"/"+pname+"/"+myuid, os.F_OK):
					dheader = " %s has been given:" % pname
					dlen = list_a_directory(SPOOL+"/"+pname+"/"+myuid, brief, dheader)
					totalcount += dlen
					if not brief:
						if dlen > 0:
							print "   %d file(s)" % dlen
			if not brief:
				if totalcount > 0:
					print "You have given a total of %d file(s)." % totalcount
				else:
					print "You have given no files."
		else:
			dlen = list_a_directory(SPOOL+"/"+theiruid+"/"+myuid, brief, None)
			totalcount += dlen
			if not brief:
				if totalcount > 0:
					print "You have given %d file(s)." % totalcount
	else:
		print "Attempt to use bad parameters to list_files_given\n"
		sys.exit(1)
		
def list_files_totake(myuid=None, theiruid=None, brief=False):
	totalcount = 0
	if myuid != None:
		if theiruid == None:
			try:
				dlist = os.listdir(SPOOL+"/"+myuid)
			except:
					print "You have no files to take."
					return
			dlist.sort()
			for pname in dlist:
				if os.access(SPOOL+"/"+myuid+"/"+pname, os.F_OK):
					dheader = " %s has given:" % pname
					dlen = list_a_directory(SPOOL+"/"+myuid+"/"+pname, brief, dheader)
					totalcount += dlen
					if not brief:
						if dlen > 0:
							print "   %d file(s)" % dlen
			if not brief:
				if totalcount > 0:
					print "You have %d file(s) to take." % totalcount
				else:
					print "You have no files to take."
		else:
			dheader = "%s has given:" % theiruid
			dlen = list_a_directory(SPOOL+"/"+myuid+"/"+theiruid, brief, dheader)
			totalcount += dlen
			if not brief:
				if totalcount > 0:
					print "You have %d file(s) to take." % totalcount
	else:
		print "Attempt to use bad parameters to list_files_totake\n"
		sys.exit(1)

#
# We validate various things that should be true-
# 1) taker/giver usernames map to available users (we can look them up)
# 2) if group "gt-<taker uid>" exists, then the giver is a member.
# 3) taker's group exists, and the taker is the ONLY member.
#
def validate_transaction (giver_uname, taker_uname):
	isvalid = True
	
	try:
		g_ent = pwd.getpwnam(giver_uname)
		t_ent = pwd.getpwnam(taker_uname)
	except:
		isvalid = False
	# giver/taker can't be looked up? fail.
	if not isvalid:
		print "Both Giver and Taker must be valid in password database."
		return isvalid
	
	try:
		grp_ent = grp.getgrnam("gt-"+str(t_ent.pw_uid))
		isvalid = False
		for x in grp_ent.gr_mem:
			if x == g_ent.pw_name:
				isvalid = True # giver is in taker's gt-<uid> group
	except:
		isvalid = True
	# giver is in taker's gt-<uid> group OR gt-<uid> group does not exist.
	if not isvalid:
		print "You are not authorized to give files to that user."
		return isvalid
	
	try:
		grp_ent = grp.getgrgid(t_ent.pw_gid)
		for x in grp_ent.gr_mem:
			if x != t_ent.pw_name:
				isvalid = False # taker must be the only member of their group
				print "Taker's default group is not set up correctly, they must be the only member of their default group."
				return isvalid
	except:
		print "Taker must have a valid group"
		isvalid = False # group must exist

	return isvalid

#####################################################################
#  Setup

progname = os.path.basename(sys.argv[0])

if progname[:4] == 'take':
	give_mode = False
else:
	give_mode = True

usagestr = "%prog [options] <user> [filename]"
parser = OptionParser(usage=usagestr, version=VERSION)

parser.add_option("-q", "--quiet", action="store_false", dest="verbose", default=True, help="suppress status messages")
parser.add_option("-i", "--interactive", action="store_true", dest="interact", default=False, help="prompt for confirmation for each file")

#
# Take care of different options of give and take.
#
if give_mode:
	parser.add_option("-u", "--ungive", action="store_true", dest="ungive", default=False, help="ungive a file")
	parser.add_option("-f", "--force", action="store_true", dest="force", default=False, help="force overwriting a file already given")
	parser.add_option("-l", "--list", action="store_true", dest="list", default=False, help="list files you have given")
	parser.add_option("-n", "--compact", action="store_true", dest="compact", default=False, help="brief list files you have given")
	parser.add_option("-a", "--all", "--regive", action="store_true", dest="all", default=False, help="regive all files to user, with overwrite")
else:
	parser.add_option("-l", "--list", action="store_true", dest="list", default=False, help="list files to take")
	parser.add_option("-f", "--force", action="store_true", dest="force", default=False, help="force overwriting files")
	parser.add_option("-n", "--compact", action="store_true", dest="compact", default=False, help="brief list files to take")
	parser.add_option("-a", "--all", action="store_true", dest="all", default=False, help="all files")

(opts, args) = parser.parse_args()

#
# Sanity checking.
#
if len(args) > 0:
	who = args[0]
	args = args[1:]
else:
	who = None # special token

if give_mode:
	if opts.all:
		opts.force = True

if give_mode:
	if who == None:
		if opts.all:
			parser.error("--regive option requires a username.")
			sys.exit(1)
		else:
			opts.list = True
	else:
		if not opts.all and not opts.ungive and len(args) == 0:
			opts.list = True
else:
	if who == None:
		if not opts.all:
			opts.list = True
		
if len(args) == 0:
	opts.all = True
else:
	if opts.all:
		if give_mode:
			parser.error("Cannot specify filenames AND --all or --regive.")
		else:
			parser.error("Cannot specify filenames AND --all.")
		sys.exit(1)

if give_mode:
	if len(args) == 0:
		if not opts.compact and not opts.list and not opts.all:
			if who != None:
				opts.list = True
			else:
				parser.error("Need a username and filename.")
				sys.exit(1)

if opts.compact:
	opts.list = True

#####################################################################
#  Main

try:
	if give_mode:
		if who != None:
			taker_ent = pwd.getpwnam(who)
	else:
		taker_ent = pwd.getpwuid(os.getuid())
except:
	print "I cannot find account information for user %s." % who
	sys.exit(1)

if give_mode:
	try:
		giver_ent = pwd.getpwuid(os.getuid())
	except:
		print "I cannot find your account information.\n"
		sys.exit(1)
else:
	if who == None:
		giver_ent = None
	else:
		try:
			giver_ent = pwd.getpwnam(who)
		except:
			giver_ent = get_giverpw_from_dir(who, taker_ent.pw_name)
		if giver_ent == None:
			print "Cannot find giver's account information.\n"
			sys.exit(1)
			

if give_mode:
	if who != None:
		if not validate_transaction(giver_ent.pw_name, who):
			sys.exit(1)

# 
# Giver, taker validated.
#

if opts.list:
	if give_mode:
		list_files_given(giver_ent.pw_name, who, opts.compact)
	else:
		list_files_totake(taker_ent.pw_name, who, opts.compact)
	sys.exit(0)

totalcount = 0

if give_mode:
	if opts.ungive:
		if who == None:
			parser.error("You must supply username for -u.")
			sys.exit(1)

		if opts.all:
			flist = enumerate_files(giver_ent.pw_name, who)
		else:
			if len(args) == 0:
				flist = enumerate_files(giver_ent.pw_name, who)
			else:
				flist = args

		for fname in flist:
			if opts.verbose:
				print "Ungiving %s." % fname
			dlen = ungive_file(giver_ent.pw_name, who, fname, opts.interact)
			totalcount += dlen

		if totalcount > 0:
			print "Ungave %d file(s)." % totalcount
		else:
			print "No files ungiven."
	else:

		if opts.all:
			flist = enumerate_files(giver_ent.pw_name, who)
		else:
			flist = args

		for fname in flist:
			if opts.verbose:
				print "Giving %s." % fname
			dlen = give_file(giver_ent.pw_name, taker_ent.pw_name, fname, opts.interact)
			totalcount += dlen

		if totalcount > 0:
			print "Gave %d file(s)." % totalcount
		else:
			print "No files given."
else:
	if opts.all:
		todo = enumerate_files(who, taker_ent.pw_name)
		if who == None:
			for (who,fnames) in todo:
				for fname in fnames:
					if opts.verbose:
						print "Taking %s from %s." % (fname, who)
					dlen = take_file(who, taker_ent.pw_name, fname, opts.interact)
					totalcount += dlen
			if totalcount > 0:
				print "Taken %d file(s)." % totalcount
			else:
				print "No files taken."

		else:
			for fname in todo:
				if opts.verbose:
					print "Taking %s from %s." % (fname, who)
				dlen = take_file(who, taker_ent.pw_name, fname, opts.interact)
				totalcount += dlen
			if totalcount > 0:
				print "Taken %d file(s)." % totalcount
			else:
				print "No files taken."
			
	else:
		for fname in args:
			if opts.verbose:
				print "Taking %s." % fname
			dlen = take_file(giver_ent.pw_name, taker_ent.pw_name, fname, opts.interact)
			totalcount += dlen
		if totalcount > 0:
			print "Taken %d file(s)." % totalcount
		else:
			print "No files taken."

sys.exit(0)
#
# vim: set ts=4 sw=4:
