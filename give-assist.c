//
// give-assist- create secure directories for file exchange
// 2010-08-27 Shawn Instenes <shawni@llnl.gov>
// 
// This program assists the give program to create a secure
// directory structure for the exchange of files, with some
// local assumptions:
//
// 1. each user has a group with the same gid as their uid,
//   and they are the only member.
// 2. both the giver and the taker usernames can be looked up with
//   getpwent()
//
// For local convienience, a group named "gt-<taker_uid>" may exist.
// If it does, then the giver must be a member of that group or the
// program will not continue.  (This is also checked in the "give"
// program.)
//
// The give spool directory structure is:
//
// SPOOL_ROOT/TAKER_UNAME/GIVER_UNAME/GIVEN_FILENAME
//
// Where:
//
// SPOOL = top level spool directory, owned by root, mode 0755.
// TAKER_UNAME = taker's username, owned taker_uid:taker_gid, mode 0711.
// GIVER_UNAME = giver's username, owned giver_uid:taker_gid, mode 0770.
// GIVEN_FILENAME = any file, owned giver_uid:giver_gid, mode 0X44
//   where X is the original owner permission bits.
//
// Goals:
// A) Files given are owned by giver until taken (for quota purposes)
// B) Filenames are private to giver/taker (ignoring root, of course)
// C) Files may be given and taken without special privledges from giver
//    to taker after this program has run at least once.
//
// This program checks return codes from everything, and dies if
// anything goes wrong.
//

// for dirfd()
#define _BSD_SOURCE
#ifdef _AIX
#define dirfd(x)        (((struct _dirdesc *)(x))->dd_fd)
#endif


#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <string_m.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <paths.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/resource.h>

#define SPOOL "/usr/give"
/* IEEE Std 1003.1-2008 says (3.429), use portable filename character set (3.276) */
#define ACCEPTABLE_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
#define GA_VERSION "give-assist 3.0n"

int check_for_debuggers(void);
void ignore_signals(void);
void sanitize_fds(void);
void limit_cores(void);

void static emit(char *a, string_m s, char *b);
void static try_m (errno_t result);
void static try_err (errno_t result, char *err);
void static safe_mkdir_m (string_m s, mode_t mode, uid_t u, gid_t g);
void static die_m(char *a, string_m s, char *b);
void static die_err_zs(char *zs, errno_t err);
void static die_2zs(char *a, char *b);
void static die_zs(char *zs);
bool static gt_access_ok(struct passwd *g, struct passwd *t);
bool static give_spool_ok (char *spooldir);
bool static taker_group_ok (struct passwd *t);

const static bool debug = false;

int main(int argc, char **argv)   {
	errno_t e;
/*	int ret; */
	string_m pathname = NULL;
	string_m taker_uname = NULL;
	char *zs_taker = NULL;

	long s_passwd_size = 0L;
	struct passwd *giver_ent = NULL, *taker_ent = NULL;
	char *taker_buf = NULL, *giver_buf = NULL;
	struct passwd taker_s, giver_s;

    /* try to ignore debuggers */
    if(check_for_debuggers()) {
        exit(EXIT_FAILURE);
    }

    /* ignore signals */
    ignore_signals();

    /* close open files */
    sanitize_fds();

    /* disable memory dumps */
    limit_cores();

	if (argc != 2) 
		die_zs(GA_VERSION " usage: give-assist <taker_username>");
	if (argv == NULL) 
		die_zs(GA_VERSION " usage: give-assist <taker_username>");

	if (geteuid() != 0) {
		die_zs("give-assist must be run as root or suid");
	}
	// We won't tolerate odd characters in the usernames- we mkdir later.
	// This is the first and last time we use argv.
	e = strcreate_m(&taker_uname, argv[1], 0, ACCEPTABLE_CHARS);
	try_err(e, "That username contains invalid characters.");
	try_m(cgetstr_m(taker_uname, &zs_taker));

	//
	// Here we setup for getpwuid_r & getpwnam_r.
	// 
	s_passwd_size = sysconf(_SC_GETPW_R_SIZE_MAX);

	if (s_passwd_size == 0L)
		die_zs("System configuration error: _SC_GETPW_R_SIZE_MAX is zero.");

	taker_buf = (char *) calloc((size_t) 1, (size_t) s_passwd_size);
	giver_buf = (char *) calloc((size_t) 1, (size_t) s_passwd_size);

	if (taker_buf == NULL || giver_buf == NULL)
		die_zs("Out of memory.");

	// Ok, sizes and buffers are all ready so:
	try_err((errno_t)getpwuid_r(getuid(), &giver_s, giver_buf, s_passwd_size, &giver_ent), "Cannot find your user information.");

	try_err((errno_t)getpwnam_r(zs_taker, &taker_s, taker_buf, s_passwd_size, &taker_ent), "Cannot find taker user information.");

	if (taker_ent == NULL || giver_ent == NULL)
		die_zs("getpw(nam|uid) returned null buffer- out of memory?");

	if (taker_ent->pw_uid != taker_ent->pw_gid)
		die_zs("Taker is not authorized to receive files (uid != gid).");
	if (giver_ent->pw_uid != giver_ent->pw_gid)
		die_zs("You are not authorized to give files (uid != gid).");

	if (!gt_access_ok(giver_ent, taker_ent))
		die_zs("You are not authorized to give files to that user.");
	
	if (!taker_group_ok(taker_ent))
		die_zs("Taker must be sole member of their group.");
	
	if (debug) {
		printf("ok giver: %s\n", giver_ent->pw_name);
		printf("ok taker: %s\n", taker_ent->pw_name);
	}

	if (!give_spool_ok(SPOOL)) {
		// this shouldn't happen- if it returns at all it's good.
		die_zs("give_spool_ok() returned zero");
	}
	// these may fail on malloc() but otherwise no.
	try_m(strcreate_m(&pathname, SPOOL, 0, NULL));
	try_m(cstrcat_m(pathname, "/"));
	try_m(cstrcat_m(pathname, taker_ent->pw_name));
	(void) umask((mode_t) 0); // we will be setting exact mkdir/chmod perms.
	// make this directory, chown
	safe_mkdir_m (pathname, 0711, taker_ent->pw_uid, taker_ent->pw_gid);
	// build 2nd level directory, again malloc() may fail
	try_m(cstrcat_m(pathname, "/"));
	try_m(cstrcat_m(pathname, giver_ent->pw_name));
	// make this directory, chown
	safe_mkdir_m (pathname, 0770, giver_ent->pw_uid, taker_ent->pw_gid);

	if (debug)
		emit("PATH = ", pathname, "\n");
	exit(EXIT_SUCCESS);
}

int check_for_debuggers() {
    int status, waitrc;
    pid_t child, parent;

    parent = getpid();

    if(!(child = fork())) {
        /* child */
        if(ptrace(PT_ATTACH, parent, 0, 0)) {
            exit(1);
        }
        do {
            waitrc = waitpid(parent, &status, 0);
        } while(waitrc == -1 && errno = EINTR);

        ptrace(PT_DETACH, parent, (caddr_t)1, SIGCONT);
        exit(0);
    }

    if(child == -1) {
        return -1;
    }

    do {
        waitrc = waitpid(child, &status, 0);
    } while(waitrc == -1 && errno != EINTR);

    return WEXITSTATUS(status);
}

void ignore_signals() {
    int i;
    for (i=0; i < NSIG; i++) {
        if(i != SIGKILL && i != SIGCHLD) {
            (void) signal(i, SIG_IGN);
        }
    }
}

void limit_cores(void) {
    struct rlimit rlim;

    rlim.rlim_cur = rlim.rlim_max = 0;
    setrlim(RLIMIT_CORE, &rlim);
}

static int open_devnull(int fd) {
    FILE *f = 0;

    if(!fd) {
        f = freopen(_PATH_DEVNULL, "rb", stdin);
    } else if(fd == 1) {
        f = freopen(_PATH_DEVNULL, "wb", stdout);
    } else if(fd == 2) {
        f = freopen(_PATH_DEVNULL, "wb", stderr);
    }

    return (f && fileno(f) == fd);
}

void sanitize_fds(void) {
    int fd, fds;
    struct stat st;

    if((fds = getdtablesize()) == -1) {
        fds = 256;
    }

    for(fd = 3; fd < fds; fd++) {
        close(fd);
    }

    for(fd = 0; fd < 3; fd++) {
        if(fstat(fd, &st) == -1 && (errno != EBADF || !open_devnull(fd))) {
            abort();
        }
    }
}

//
// emit()- print a managed string, possibly with fore & after
//
void static emit(char *a, string_m s, char *b) {
	char *out = NULL;
	if (a != NULL) printf("%s", a);
	try_m(cgetstr_m(s, &out)); printf("%s",out); free(out);
	if (b != NULL) printf("%s", b);
}

//
// safe_mkdir_m()-
//
// Make a directory (it may already exist), and make sure it
// ends up with the correct ownership and permissions.
//
// This code is intended to work within a managed directory tree-
// the topmost directory should be owned by root and writable only
// by root.
//
void static safe_mkdir_m (string_m s, mode_t mode, uid_t u, gid_t g) {
	DIR *dir;
	int result = 0, fd = 0;
	struct stat chk, buf;
	char *path = NULL;

	try_m(cgetstr_m(s, &path));
	result = mkdir(path, mode);
	if (result != 0) {
		if (errno != EEXIST) {
			die_err_zs("error making directory", errno);
		}
	}

	if (lstat(path, &chk) != 0) {
		die_2zs("directory does not exist", path);
	}
	if ((chk.st_mode & S_IFDIR) == 0)
		die_2zs("pathname is not a directory", path);

	dir = opendir(path);
	if (dir == NULL) {
		if (errno == ENOTDIR) {
			die_2zs("file where directory should be", path);
		} else {
			die_err_zs("error with path", errno);
		}
	}
	fd = dirfd(dir);
	if (fstat(fd, &buf) != 0) {
		die_2zs("directory does not exist", path);
	}
	if ((buf.st_mode & S_IFDIR) == 0)
		die_2zs("pathname is not a directory", path);

	// One last check- is this the same dev/inode we checked before?

	if ((buf.st_dev != chk.st_dev) || (buf.st_ino != chk.st_ino)) {
		die_2zs("directory is not static", path);
	}

	if ((buf.st_mode & 0777) != mode) {
		result = fchmod(fd, mode); 
		if (result != 0) {
			die_err_zs("error chmoding directory", errno);
		}
	}
	
	result = fchown(fd, u, g); 
	if (result != 0) {
		die_err_zs("error chowning directory", errno);
	}
	result = closedir(dir);
	if (result != 0) {
		die_err_zs("error closing directory", errno);
	}
	free(path);
	return;
}

//
// scanfor()- see if zs-string "s" is in a[] anywhere.
//
bool static scanfor(char *s, char **a) {
	bool wasfound = false;
	int i = 0;
	char *p = a[i];
	if (p == NULL) return false;
	if (s == NULL) return false;
	while ((!wasfound) && (p != NULL)) {
		if (debug) printf("scanfor: %s %s\n", s, p);
		if (strcmp((const char *)s, (const char *)p) == 0) {
			wasfound = true;
			break;
		}
		p = a[++i];
	}
	return wasfound;
}

//
// onlycontains()- see if zs-string "s" is the only string in a[].
//
bool static onlycontains (char *s, char **a) {
	bool wasfound = true;
	int i = 0;
	char *p = a[i];
	if (p == NULL) return false;
	if (s == NULL) return false;
	while ((wasfound) && (p != NULL)) {
		if (debug) printf("only: %s %s\n", s, p);
		if (strcmp((const char *)s, (const char *)p) != 0) {
			wasfound = false;
			break;
		}
		p = a[++i];
	}
	return wasfound;
}


//
// give_spool_ok()- spool directory must:
// 1) exist
// 2) owned by root
// 3) be a directory
// 4) is mode 755
//
// This eliminates some potential security holes with our chown() calls and race conditions, if the
// spool directory were otherwise writable by someone other than root.
//

bool static give_spool_ok (char *spooldir) {
	struct stat buf;
	if (stat(spooldir, &buf) != 0)
		die_2zs("give spool directory does not exist", spooldir);
	if (buf.st_uid != 0)
		die_2zs("give spool directory not owned by uid 0", spooldir);
	if ((buf.st_mode & S_IFDIR) == 0)
		die_2zs("give spool pathname is not a directory", spooldir);
	if ((buf.st_mode & 0777) != 0755)
		die_2zs("give spool directory is not mode 0755", spooldir);
	// if we get here, it's good.
	return (true);
}

//
// gt_access_ok()- determine if it's OK for the giver to be giving a file to the taker.
// 
// If a group named "gt-<taker-uid-in-decimal>" exists, then the giver must be a member
// or the operation will be denied.  If the group does not exist then the operation is OK.
//
bool static gt_access_ok(struct passwd *g, struct passwd *t) {
	struct group *access_grp = NULL;
	string_m s = NULL,fmt = NULL;
	char *zs = NULL;

	if (g == NULL || t == NULL)
		die_zs("null arg passed to gt_access_ok()");

	try_m(strcreate_m(&fmt, "gt-%ld", 0, NULL));
	try_m(strcreate_m(&s, "", 0, NULL));
	try_m(sprintf_m(s, fmt, NULL, t->pw_uid));
	
	try_m(cgetstr_m(s, &zs));
	access_grp = getgrnam(zs);
	free(zs);

	if (access_grp == NULL) {
		if (debug)
			printf ("gt- access group does NOT exist\n");
		return (true); // ok, no group exists
	}
	if (debug)
		printf ("gt- access group exists\n");

	if (scanfor(g->pw_name, access_grp->gr_mem)) {
		if (debug)
			printf ("giver IS in gt- access group\n");
		return (true); // ok, group exists and giver is in it.
	}
	if (debug)
		printf ("giver is NOT in gt- access group\n");
	return (false);
}

//
// taker_group_ok()- determine if taker's group only has one (or no) members
// 
//
bool static taker_group_ok (struct passwd *t) {
	struct group *access_grp = NULL;
	string_m s = NULL;
	char *zs = NULL;

	if (t == NULL)
		die_zs("null arg passed to taker_group_ok()");

	try_m(strcreate_m(&s, t->pw_name, 0, NULL));
	try_m(cgetstr_m(s, &zs));
	access_grp = getgrnam(zs);
	free(zs);

	if (access_grp == NULL) {
		if (debug)
			printf ("taker uname group does NOT exist\n");
		return (false); // supposed to be there.
	}
	if (debug)
		printf ("taker uname access group exists\n");

	if (onlycontains(t->pw_name, access_grp->gr_mem)) {
		if (debug)
			printf ("taker is alone in uname group\n");
		return (true);
	}
	if (debug)
		printf ("taker is NOT alone in uname group\n");
	return (false);
}

//
// try_m()- abort program if any managed string assertion fails.
//
void static try_m (errno_t result) {
	if ((int)result != 0) die_err_zs("generic error, errno", result);
}

//
// try_err()- abort program if any managed string assertion fails,
//   include error.
//
void static try_err (errno_t result, char *err) {
	if ((int)result != 0) die_zs(err);
}

//
// Complain and exit.
//
void static die_m(char *a, string_m s, char *b) {
	char *out = NULL;
	if (a != NULL) fprintf(stderr, "%s", a);
	try_m(cgetstr_m(s, &out));
	fprintf(stderr, "%s",out);
	free(out);
	if (b != NULL) fprintf(stderr, "%s", b);
	exit(EXIT_FAILURE);
}

void static die_err_zs(char *zs, errno_t err) {
	if (zs != NULL) {
		fprintf(stderr, "%s: %d\n", zs, err);
	}
	exit(EXIT_FAILURE);
}

void static die_2zs(char *a, char *b) {
	if (a != NULL) {
		fprintf(stderr, "%s ", a);
	}
	if (b != NULL) {
		fprintf(stderr, "%s\n", b);
	}
	exit(EXIT_FAILURE);
}

void static die_zs(char *zs) {
	if (zs != NULL) {
		fprintf(stderr, "%s\n", zs);
	}
	exit(EXIT_FAILURE);
}
