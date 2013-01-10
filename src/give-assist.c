/**	@file give-assist.c
*	give-assist- create secure directories for file exchange
*	2010-08-27 Shawn Instenes <shawni@llnl.gov>
*
*	2012-07 Re-Write for LANL enviornment Jon Bringhurst and Dominic Manno
*   	2013-01 Created this version for LANL FTA's   Dominic Manno
*
*	@author: Shawn Instenes	
*	@author: Trent D'Hooge  
*	@author: Jim Garlick 
*	@author: Jon Bringhurst
*	@author: Dominic Manno
*	@author: Ryan Day
*	@author: Georgia Pedicini
*
*   	This program is an FTA version of give-assist, it was created
*   	to have the user enter a cluster name on which the taker would
*   	receive the file on. 
*   	This was due to the fact that fta's had both sets of givedirs, 
*   	and we needed to know in which givedir
*   	the receiver would take from.
*
*   	give.py passes cluster abbreviation to give-assist as it's 
*   	first argument (argv[1])
*
*	This program assists the give program to create a secure
*	directory structure for the exchange of files, with some
*	local assumptions:
*
*	1. each user has a group with the same gid as their uid,
*	and they are the only member.
*
*	2. both the giver and the taker usernames can be looked up with
*	getpwent()
*
*	For LLNL convienience, a group named "gt-<taker_uid>" may exist.
*	If it does, then the giver must be a member of that group or the
*	program will not continue.  (This is also checked in the "give"
*	program.)
*
*
*	The give spool directory structure is:
*
*	SPOOL_ROOT/TAKER_UNAME/GIVER_UNAME/GIVEN_FILENAME
*
*	Where:
*
*	SPOOL = top level spool directory, owned by root, mode 0755.
*
*	TAKER_UNAME = taker's username, owned taker_uid:taker_gid, mode 0711.*
*
*	GIVER_UNAME = giver's username, owned giver_uid:taker_gid, mode 0770.
*
*	GIVEN_FILENAME = any file, owned giver_uid:giver_gid, mode 0X44
*	where X is the original owner permission bits.
*	The perms on the directories are defined this way because the taker needs to own their givedir, the giver needs to own the subdir for which they have given to the taker (for ungive), but the 
*	taker's gid group also owns this because the taker needs to acess it as well (to take). 
*
*
*	Goals:
*
*	A) Files given are owned by giver until taken (for quota purposes)
*
*	B) Filenames are private to giver/taker (ignoring root, of course)
*
*	C) Files may be given and taken without special privledges from giver
*	to taker after this program has run at least once.
*
*
* 	This program checks return codes from everything, and dies if
* 	anything goes wrong.
*/

#define _BSD_SOURCE
#ifdef _AIX
#define dirfd(x)    (((struct _dirdesc *)(x))->dd_fd)
#endif

#define  _POSIX_SOURCE

#include "./config.h"

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include "./string_m/string_m.h"
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <paths.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <wchar.h>
#include <limits.h>

//this still allows SPOOL_DIRECTORY to be set an config time, but it has no
//guarantee that SPOOL will retain the value SPOOL_DIRECTORY
#ifndef SPOOL_DIRECTORY
#define SPOOL_DIRECTORY "/usr/givedir"
#endif

//SPOOL will hold the value of the root directory for giving... it's value 
//will depend on which cluster is passed in by user, after validation
static char* SPOOL = SPOOL_DIRECTORY;

#ifndef STRICT_CHECKING
#define STRICT_CHECKING 1
#endif

//IEEE Std 1003.1-2008 says (3.429), use portable filename character set (3.276)
#define ACCEPTABLE_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
#define GA_VERSION "give-assist 3.1-3fta"


//this isn't prototyped in string.h for some reason, but it is implemented there
//if it's not prototyped than we will get compile warnings
size_t strnlen(const char *s, size_t maxlen);

//these functions are used for error checks and handling
static void try_m(errno_t result);
static void try_err(errno_t result, char* err);
static void die_err_zs(char* zs, errno_t err);
static void die_2zs(char* a, char* b);
static void die_zs(char* zs);

//thes functions are used as checks to be sure user perms, uids, gids are valid etc.
static bool gt_access_ok(struct passwd *g, struct passwd *t);
static bool give_spool_ok (char *spooldir);
static bool taker_group_ok (struct passwd *t);
static void safe_mkdir_m (string_m s, mode_t mode, uid_t u, gid_t g);
static void emit(char *a, string_m s, char *b);
static int validate_cluster(char *cluster);

//these functions are used to sanitize the environment...close open files, disable memory dumps, check for dubuggers.
int check_for_debuggers(void);
void ignore_signals(void);
void sanitize_fds(void);
void limit_cores(void);
static int open_devnull(int fd);


static const bool debug = false;

int main(int argc, char** argv){

    errno_t e;													

    string_m pathname = NULL;									// safe string declared to eventually hold the pathname
    string_m taker_uname = NULL;								// safe string declared to eventually hold the taker's username, to create a safe string
    char* zs_taker = NULL;	                				// will hold the taker's username, created from safe string wrappers

    long s_passwd_size = 0L;									
    struct passwd* giver_ent = NULL;			// pointer that will point to giver's pwd struct, so info pertaining to passwd file can be looked up
    struct passwd* taker_ent = NULL;			// pointer that will point to taker's pwd struct, so info pertaining to passwd file can be looked up
    char* taker_buf = NULL;					    // string created to be buffer for taker using the s_passwd_size
    char* giver_buf = NULL;					    // string created to be buffer for giver using the s_passwd_size
    struct passwd taker_s;					    // passwd struct to hold the taker's passwd info, needed when calling getpwnam_r
    struct passwd giver_s;					    // passwd struct to hold the giver's passwd info, needed when calling getpwuid_r
	
    char *cluster = NULL;					//will hold cluster name, get from args...will be handled safely, knowing how many
						            	//chars, so no safe string lib needed
	
    int length = 0;						//use to test for length of input for clustername
	
    //try to ignore debuggers
    if(check_for_debuggers()){
        exit(EXIT_FAILURE);
    }

    //ignore signals
    ignore_signals();

    //close open files
    sanitize_fds();

    //disable memory dumps
    limit_cores();

	//these next three checks will make sure that only two args are passed in for the call to give-assist
	//if 0 or 3+ then the usage of give-assist is printed to stdout
	//the third checks to be sure that suid or root is running give-assist
    if(argc != 3){
    	die_zs(GA_VERSION " usage: give-assist <cluster> <taker_username>");
	}
    if(argv == NULL){
    	die_zs(GA_VERSION " usage: give-assist <cluster> <taker_username>");
    }
	
	    
    if(geteuid() != 0){
        die_zs("give-assist must be run as root or suid");
    }

    //set up check for cluster validity
    length = strnlen(argv[1], 5);		//should only be 2 chars, if there are any others error and exit...only compare up to 5
						//if there are more than 5, length will be set to 5 causing an error
    if(length != 2){					
        die_zs("Enter valid cluster...two letter abbreviation\n");
    }

    cluster = argv[1];
    //validate_cluster returned 0, error if not...this shouldn't happen
    //if an error occurs while validating, validate_cluster should handle it. if the function
    //finishes, it will return 0
    if(validate_cluster(cluster)){
        die_zs("Invalid cluster\n");
    }
	
    //We won't tolerate odd characters in the usernames- we mkdir later.
    //This is the last time we use argv.
    e = strcreate_m(&taker_uname, argv[2], 0, ACCEPTABLE_CHARS);
    try_err(e, "That username contains invalid characters.");
    try_m(cgetstr_m(taker_uname, &zs_taker));


    //Here we setup for getpwuid_r & getpwnam_r.
    s_passwd_size = sysconf(_SC_GETPW_R_SIZE_MAX);

    if(s_passwd_size == 0L){
    	die_zs("System configuration error: _SC_GETPW_R_SIZE_MAX is zero.");
	}
    taker_buf = (char*) calloc((size_t) 1, (size_t) s_passwd_size);
    giver_buf = (char*) calloc((size_t) 1, (size_t) s_passwd_size);

    if(taker_buf == NULL || giver_buf == NULL){
    	die_zs("Out of memory.");
	}

    // Ok, sizes and buffers are all ready so:
	//these will get the passwd structs and assign them to pointer giver_ent and taker_ent for respective person
	//accesses the passwd file for this, struct is defined in /usr/include/pwd.h
	//the actual struct is stored in giver_s and taker_s, using getpwuid_r and getpwnam_r is safer because of the size limit
    //by using try_err we make sure that we have a success when calling these functions, if info not found msg is printed
	try_err((errno_t)getpwuid_r(getuid(), &giver_s, giver_buf, s_passwd_size, &giver_ent), "Cannot find your user information.");

    try_err((errno_t)getpwnam_r(zs_taker, &taker_s, taker_buf, s_passwd_size, &taker_ent), "Cannot find taker user information.");

	    
	if(taker_ent == NULL || giver_ent == NULL){
    	die_zs("getpw(nam|uid) returned null buffer- out of memory?");
	}
	
	//if non-strict checking is enabled at config it will set STRICT_CHECKING to 0, so it will not execute these checks, the default is for STRICT_CHECKING to = 1, so that these checks will run
    if(STRICT_CHECKING){
		if(taker_ent->pw_uid != taker_ent->pw_gid){						
    		die_zs("Taker is not authorized to receive files (uid != gid).");
		}
    	if(giver_ent->pw_uid != giver_ent->pw_gid){						
    		die_zs("You are not authorized to give files (uid != gid).");
		}
	
    	if(!gt_access_ok(giver_ent, taker_ent)){						
    		die_zs("You are not authorized to give files to that user.");
		}
	}
	
    if(!taker_group_ok(taker_ent)){						
    	die_zs("Taker must be sole member of their username group.");
	}
    if(debug){
        printf("ok giver: %s\n", giver_ent->pw_name);
        printf("ok taker: %s\n", taker_ent->pw_name);
    }

	
    if(!give_spool_ok(SPOOL)){
        // this shouldn't happen- if it returns at all it's good.
        die_zs("give_spool_ok() returned zero");
    }
	
	//wrappers for safe strings, creating pathname from SPOOL (SPOOL/taker's_username)
    // these may fail on malloc() but otherwise no.
    try_m(strcreate_m(&pathname, SPOOL, 0, NULL));
    try_m(cstrcat_m(pathname, "/"));
    try_m(cstrcat_m(pathname, taker_ent->pw_name));
    (void) umask((mode_t) 0); 	// we will be setting exact mkdir/chmod perms.
    
	//make the directory SPOOL/taker, it is owned by (taker:taker_default_group) only the taker should have acess to this dir, but perms 0711 are needed so that givers can get into their specific subdir
    safe_mkdir_m(pathname, 0711, taker_ent->pw_uid, taker_ent->pw_gid);
    
	//wrappers for safe strings to be able to create next dir
    try_m(cstrcat_m(pathname, "/"));
    try_m(cstrcat_m(pathname, giver_ent->pw_name));
    
	//make the directory SPOOL/taker/giver, it is owned by (giver:taker_default_group) this is to enable ungive functionality, perms 0770 allow all access to giver and taker, none to world
    safe_mkdir_m(pathname, 0770, giver_ent->pw_uid, taker_ent->pw_gid);


    if(debug){
    	emit("PATH = ", pathname, "\n");
	}

	free(taker_buf);        //make sure we are cleaning up!
	free(giver_buf);
    exit(EXIT_SUCCESS);
}

//these functions are used as checks to be sure user and taker fit certain params to be able to give/take

/**
* print a managed string, possibly with fore & after
* ie. print a managed string between two unmanaged ones
*
* @param a string already ready to be printed
* @param s string_m managed string to be printed
* @param b string already ready to be printed
*
*/
static void emit(char* a, string_m s, char* b){

    char* out = NULL;

    if(a != NULL){
        printf("%s", a);
    }

    try_m(cgetstr_m(s, &out));
    printf("%s", out);
    free(out);

    if(b != NULL){
        printf("%s", b);
    }
    return;
}

/**
* safe_mkdir_m()-
*
* Make a directory (it may already exist), and make sure it
* ends up with the correct ownership and permissions
*
* This code is intended to work within a managed directory tree-
* the topmost directory should be owned by root and writable only
* by root
*
* @param s string_m pathname
* @param mode mode_t mode of directory
* @param u uid_t user id
* @param g gid_t group id from /etc/passwd file
*/
static void safe_mkdir_m(string_m s, mode_t mode, uid_t u, gid_t g){

    DIR* dir;
    int result = 0, fd = 0;
    struct stat chk, buf;
    char* path = NULL;

    try_m(cgetstr_m(s, &path));     //string needs to be made from string_m because mdkir requires a string for path
    result = mkdir(path, mode);     //making directory with path and mode, will return 0 on success or error if something goes bad

    if(result != 0){
        if(errno != EEXIST){
            die_err_zs("error making directory", errno);
        }
    }

    if(lstat(path, &chk) != 0){
        die_2zs("directory does not exist", path);
    }

    if((chk.st_mode & S_IFDIR) == 0){
        die_2zs("pathname is not a directory", path);
    }
    dir = opendir(path);

    if(dir == NULL){
        if(errno == ENOTDIR){
            die_2zs("file where directory should be", path);
        }
        else{
            die_err_zs("error with path", errno);
        }
    }

    fd = dirfd(dir);

    if(fstat(fd, &buf) != 0){
        die_2zs("directory does not exist", path);
    }

    if((buf.st_mode & S_IFDIR) == 0){
        die_2zs("pathname is not a directory", path);
    }

    // One last check- is this the same dev/inode we checked before?

    if((buf.st_dev != chk.st_dev) || (buf.st_ino != chk.st_ino)){
        die_2zs("directory is not static", path);
    }

    if((buf.st_mode & 0777) != mode){
        result = fchmod(fd, mode);

        if(result != 0){
            die_err_zs("error chmoding directory", errno);
        }
    }

    result = fchown(fd, u, g);  ///may seem funny here, but directory needs to be owned by giver:taker_gid, so that both have acess to this
    if(result != 0){
        die_err_zs("error chowning directory", errno);
    }

    result = closedir(dir);

    if(result != 0){
        die_err_zs("error closing directory", errno);
    }

    free(path);
    return;
}

/**
* check to see if a string is a member of the array a
* in the case where it is called, it is checking to be sure
* that the user is a member of the group, this is called from gt_access
* so in that case there could be multiple members of the group
*
* @param s string user
* @param a array(char **) group members
*/
static bool scanfor(char* s, char** a){

    bool wasfound = false;
    int i = 0;
    char* p = a[i];

    if(p == NULL){
        return false;
    }
    if(s == NULL){
        return false;
    }

    while((!wasfound) && (p != NULL)){
        if(debug){
            printf("scanfor: %s %s\n", s, p);
        }
        if(strcmp((const char*)s, (const char*)p) == 0){
            wasfound = true;
            break;
        }

        p = a[++i];
    }

    return wasfound;
}


/**
* see if s is the only item in a[]
* this makes sure that the user is the only member of 
* his/her uid group, this should be tree in all cases
*
* @param s string username
* @param a array(char**) the array that holds the members of the group
*
*/
static bool onlycontains(char* s, char** a){

    bool wasfound = true;
    int i = 0;
    char* p = a[i];

    if(p == NULL){
        return false;
    }
    if(s == NULL){
        return false;
    }

    while((wasfound) && (p != NULL)){
        if(debug){
            printf("only: %s %s\n", s, p);
        }
        if(strcmp((const char*)s, (const char*)p) != 0){
            wasfound = false;
            break;
        }

        p = a[++i];
    }

    return wasfound;
}

/**
*
* 1) exist
* 2) owned by root
* 3) be a directory
* 4) is mode 755
*
* This eliminates some potential security holes with our chown() calls and race conditions, if the
* spool directory were otherwise writable by someone other than root.
*
* @param spooldir string the SPOOL which represents the givedir
* @return if the givedir passes the checks described above
*/

static bool give_spool_ok(char* spooldir){

    struct stat buf;

    if(stat(spooldir, &buf) != 0){
        die_2zs("give spool directory does not exist", spooldir);
    }
    if(buf.st_uid != 0){
        die_2zs("give spool directory not owned by uid 0", spooldir);
    }
    if((buf.st_mode & S_IFDIR) == 0){
        die_2zs("give spool pathname is not a directory", spooldir);
    }
    if((buf.st_mode & 0777) != 0755){
        die_2zs("give spool directory is not mode 0755", spooldir);
    }
    // if we get here, it's good.
    return (true);
}

/**
* This is implemented for LLNL...and IS necessary
* It keeps control over which users can give/take with eachother...for classified clusters
*
* If a group named "gt-<taker-uid-in-decimal>" exists, then the giver must be a member
* or the operation will be denied.  If the group does not exist then the operation is OK.
*
* @param g struct passwd* giver
* @param t struct passwd* taker
*/
static bool gt_access_ok(struct passwd* g, struct passwd* t){

    struct group* access_grp = NULL;
    string_m s = NULL, fmt = NULL;
    char* zs = NULL;
    int count = 0;                      //managed string library wants this when using sprintf_m...tried using NULL and invalid args error came up... this works

    if(g == NULL || t == NULL){
        die_zs("null arg passed to gt_access_ok()");
    }

    //wrapper classes for the managed string library
    try_m(strcreate_m(&fmt, "gt-%ld", 0, NULL));
    try_m(strcreate_m(&s, "", 0, NULL));
    try_m(sprintf_m(s, fmt, &count, t->pw_uid));    //see comment a few lines up about managed string library
    try_m(cgetstr_m(s, &zs));
    access_grp = getgrnam(zs);
    free(zs);

    if(access_grp == NULL){
        if(debug){
            printf("gt- access group does NOT exist\n");
        }
        return (true); // ok, no group exists
    }

    if(debug){
        printf("gt- access group exists\n");
    }
    if(scanfor(g->pw_name, access_grp->gr_mem)){
        if(debug){
            printf("giver IS in gt- access group\n");
        }

        return (true); // ok, group exists and giver is in it.
    }

    if(debug){
        printf("giver is NOT in gt- access group\n");
    }
    return (false);
}

/**
* taker_group_ok will check to see if taker is in a valid group and is the only member of the group
* it checks the uid group, so yes they should be the only member 
*
* this also checks the /etc/grp file to see if group(uid) exists (access_grp = getgrnam(username))
* if it does, it then checks to be sure that there is only one member 
*
*@param t struct passwd* taker
*
*/
static bool taker_group_ok(struct passwd* t){

    struct group* access_grp = NULL;
    string_m s = NULL;
    char* zs = NULL;

    if(t == NULL){
        die_zs("null arg passed to taker_group_ok()");
    }

    try_m(strcreate_m(&s, t->pw_name, 0, NULL));
    try_m(cgetstr_m(s, &zs));
    access_grp = getgrnam(zs);
    free(zs);

    if(access_grp == NULL){
        if(debug){
            printf("taker uname group does NOT exist\n");
        }

        return (false); // supposed to be there.
    }

    if(debug){
         printf("taker uname access group exists\n");
    }

    //this is where we are actually checking to see if the access_grp->gr_mem has more than one member
    if(onlycontains(t->pw_name, access_grp->gr_mem)){
        if(debug){
            printf("taker is alone in uname group\n");
        }

        return (true);
    }

    if(debug){
        printf("taker is NOT alone in uname group\n");
    }

    return (false);
}

//these functions are used to handle errors if we encounter them
/**
*
* try_m()- abort program if any managed string assertion fails.
* @param result errno_t the return value of the function being called before try_m, if no errors result == 0
*/
static void try_m(errno_t result){

    if((int)result != 0){
        die_err_zs("generic error, errno", result);
    }
}


/**
* 
* try_err()- abort program if any managed string assertion fails,
* include error.
* @param result errno_t return value of the function being called before try_m, if no errors result == 0
* @param err string error msg
*/
static void try_err(errno_t result, char* err){

    if((int)result != 0) { die_zs(err); }
}


/**
* die_err_zs()- abort program if any managed string assertion fails,
* print msg and error number to stderr
*
* @param zs string message to print
* @param err errno_t error number obtained from function call that failed
*/
static void die_err_zs(char* zs, errno_t err){

    if(zs != NULL){
        fprintf(stderr, "%s: %d\n", zs, err);
    }

    exit(EXIT_FAILURE);
}



/**
* abort program 
* print two messages to stderr
*
* @param a string message to print to stderr
* @param b string second message to print to stderr
*
*/
static void die_2zs(char* a, char* b){

    if(a != NULL){
        fprintf(stderr, "%s ", a);
    }

    if(b != NULL){
        fprintf(stderr, "%s\n", b);
    }

    exit(EXIT_FAILURE);
}

/**
* abort program
* print one message to stderr
*
* @param zs string message to be printed to stderr
*/
static void die_zs(char* zs){

    if(zs != NULL){
        fprintf(stderr, "%s\n", zs);
    }

    exit(EXIT_FAILURE);
}

//these functions are used to sanitize the enviornment...close open files, disable memory dumps, check for dubuggers.

/** 
* Check for debuggers, try to ignore them
*
* @return int status of debugger check
*/
int check_for_debuggers(){

    int status, waitrc;
    pid_t child, parent;

    parent = getpid();

    if(!(child = fork())){
        //child 
        if(ptrace(PT_ATTACH, parent, 0, 0)){
            exit(1);
        }

        do{
            waitrc = waitpid(parent, &status, 0);
        }
        while(waitrc == -1 );


        //errno = EINTR);

        ptrace(PT_DETACH, parent, (caddr_t)1, SIGCONT);
        exit(0);
    }

    if(child == -1){
        return -1;
    }

    do{
        waitrc = waitpid(child, &status, 0);
    }
    while(waitrc == -1 && errno != EINTR);

    return WEXITSTATUS(status);
}

/**
*
*ignore signals
*
*/
void ignore_signals(){

    int i;

    for(i = 0; i < NSIG; i++){
        if(i != SIGKILL && i != SIGCHLD){
            (void) signal(i, SIG_IGN);
        }
    }
    return;
}

/**
*
* Disable memory dumps
*
*/
void limit_cores(void){

    struct rlimit rlim;

    rlim.rlim_cur = rlim.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rlim);

    return;
}

/**
*
* helper with closing all open files, reopens stdin, stdout, and stderr
* @param fd integer
* @return integer to determine success or fail 
*/
static int open_devnull(int fd){

    FILE* f = 0;

    if(!fd){
        f = freopen(_PATH_DEVNULL, "rb", stdin);
    }
    else if(fd == 1){
        f = freopen(_PATH_DEVNULL, "wb", stdout);
    }
    else if(fd == 2){
        f = freopen(_PATH_DEVNULL, "wb", stderr);
    }

    return (f && fileno(f) == fd);
}

/**
*
*Close all open files, also calls open_devnull to reopen stdin, stdout, stderr
*
*/
void sanitize_fds(void){

    int fd, fds;
    struct stat st;

    if((fds = getdtablesize()) == -1){
        fds = 256;
    }

    for(fd = 3; fd < fds; fd++){
        close(fd);
    }

    for(fd = 0; fd < 3; fd++){
        if(fstat(fd, &st) == -1 && (errno != EBADF || !open_devnull(fd))){
            abort();
        }
    }
    return;
}

/**
 *
 *    Make sure that the value passed in from give.py, which represents
 *    a cluster abbreviation, matches an abbreviation from one of two files.
 *    If it does we will set SPOOL to the proper value, if not error and exit.
 *
 *    @param cluster string the two letter abbreviation passed in as an arg
 */
static int validate_cluster(char *cluster){

	int found = 0;			//will remain zero unless cluster found
	char bffer[4];
	FILE *fp = fopen("./clusters/lclusters", "r");
	int result = 0;
	char* check;			//used to make sure a new line exists in the read, making sure file format is correct.

	result = ferror(fp);
	if(result != 0){
		die_err_zs("Error opening file: lclusters", result);
	}
	
	//testing lustre clusters first.
	//read in n-1 (3) chars...should be two letters and a newline
	//store result in bffer, fgets adds null termination
	//if new line isn't in read, we need to check file format
	//fgets will return null on error or EOF, so we now read will
	//be valid inside the loop
	while(fgets(bffer, 4, fp) != NULL){
		//making sure file follows XX\nXY\n scheme
		check = strchr(bffer, '\n');
		if(!check){
			die_zs("Check file format of lclusters\n");
		}
		if(strncmp(bffer, cluster, 2) == 0){
			found = 1;
			SPOOL = "/lscratch1/givedir";
			break;
		}
	}
	//make sure no read errors occured
	result = ferror(fp);
	if(result != 0){
		die_err_zs("Error when attempting lclusters read", result);
	}
	fclose(fp);
	
	//wasn't in lustre clusters, test panfs
	if(!found){
		fp = fopen("./clusters/pclusters", "r");
		result = ferror(fp);
		if(result != 0){
			die_err_zs("Error opening file: pclusters", result);
		}
		//same process as above
		while(fgets(bffer, 4, fp) != NULL){
			
			check = strchr(bffer, '\n');
			if(!check){
				die_zs("Check file format of pclusters\n");
			}
			if(strncmp(bffer, cluster, 2) == 0){
				found = 1;
				SPOOL = "/scratch/givetake";
				break;
			}
		}
		result = ferror(fp);
		if(result != 0){
			die_err_zs("Error when attempting read of pclusters", result);
		}
		fclose(fp);
	}

	if(!found){
		die_zs("Enter a valid cluster abbreviation\n");
	}
	
	return 0;		
}
