#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <locale.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

#include <alpm.h>

/* macros {{{ */
#define STREQ(x,y)            (strcmp((x),(y)) == 0)

#ifndef PACMAN_ROOT
	#define PACMAN_ROOT "/"
#endif
#ifndef PACMAN_DB
	#define PACMAN_DBPATH "/var/lib/pacman"
#endif

enum {
	CONF_PACNEW  = 1,
	CONF_PACSAVE = (1 << 1),
	CONF_PACORIG = (1 << 2)
};

enum {
	OP_DEBUG = 1000
};

typedef struct __file_t {
	char *path;
	char *hash;
} file_t;

typedef struct __backup_t {
	const char *pkgname;
	file_t system;
	file_t local;
	const char *hash;
} backup_t;

typedef struct __strings_t {
	const char *error;
	const char *warn;
	const char *info;
	const char *pkg;
	const char *nc;
} strings_t;

static int check_pacfiles(const char *);
static void alpm_find_backups(alpm_pkg_t *);
static void alpm_all_backups(void);
static int parse_options(int, char*[]);
static void usage(void);
static void version(void);

/* runtime configuration */
static struct {
	char lineEnd;
	alpm_list_t *targets;
} cfg;

alpm_handle_t *pmhandle;

int check_pacfiles(const char *file) /* {{{ */
{
	char path[PATH_MAX];
	int ret = 0;

	snprintf(path, PATH_MAX, "%s.pacnew", file);
	if (access(path, R_OK) == 0)
		ret |= CONF_PACNEW;

	snprintf(path, PATH_MAX, "%s.pacsave", file);
	if (access(path, R_OK) == 0)
		ret |= CONF_PACSAVE;

	snprintf(path, PATH_MAX, "%s.pacorig", file);
	if (access(path, R_OK) == 0)
		ret |= CONF_PACORIG;

	return ret;
} /* }}} */

void alpm_find_backups(alpm_pkg_t *pkg) /* {{{ */
{
	const alpm_list_t *i;
	char path[PATH_MAX];

	//const char *pkgname = alpm_pkg_get_name(pkg);

	for (i = alpm_pkg_get_backup(pkg); i; i = i->next) {
		const alpm_backup_t *backup = i->data;

		snprintf(path, PATH_MAX, "%s%s", PACMAN_ROOT, backup->name);

		/* check if we can access the file */
		if (access(path, R_OK) != 0) {
			fprintf(stderr, "Warning: can't access %s\n", path);
			continue;
		}

		/* check if there is a pacnew/pacsave/pacorig file */
		int pacfiles = check_pacfiles(path);

		if (pacfiles & CONF_PACNEW){
			fprintf(stdout, "%s.pacnew", path);
			putchar(cfg.lineEnd);
		}	
		if (pacfiles & CONF_PACSAVE){
			fprintf(stdout, "%s.pacsave", path);
			putchar(cfg.lineEnd);
		}	
		if (pacfiles & CONF_PACORIG){
			fprintf(stdout, "%s.pacorig", path);
			putchar(cfg.lineEnd);
		}	

	}

} /* }}} */

void alpm_all_backups(void) /* {{{ */
{
	const alpm_list_t *i;

	alpm_db_t *db = alpm_get_localdb(pmhandle);
	alpm_list_t *targets = cfg.targets ? alpm_db_search(db, cfg.targets) : alpm_db_get_pkgcache(db);

	for (i = targets; i; i = i->next) {
		alpm_find_backups(i->data);
	}

} /* }}} */

int parse_options(int argc, char *argv[]) /* {{{ */
{
	int opt, option_index = 0;

	static const struct option opts[] = {
		/* options */
		{"help",    no_argument,       0, 'h'},
		{"version", no_argument,       0, 'V'},
		{"print0",  no_argument,       0, 'p'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "plac:hvV", opts, &option_index)) != -1) {
		switch(opt) {
			case 'h':
				usage();
				return 1;
			case 'V':
				version();
				return 2;
			case 'p':
				cfg.lineEnd = 0x0;
				return 0;
			default:
				return 1;
		}
	}

	while (optind < argc) {
		if (!alpm_list_find_str(cfg.targets, argv[optind])) {
			fprintf(stderr, "adding target: %s\n", argv[optind]);
			cfg.targets = alpm_list_add(cfg.targets, strdup(argv[optind]));
		}
		optind++;
	}

	return 0;
} /* }}} */

void usage(void) /* {{{ */
{
	fprintf(stderr, "pacnewrat %s\n"
			"Usage: pacnewrat \n\n", PACNEWRAT_VERSION);
	fprintf(stderr, " General options:\n"
			"  -p, --print0            separate found files with null instead of newline\n"
			"  -h, --help              display this help and exit\n"
			"  -V, --version           display version\n\n");
} /* }}} */

void version(void) /* {{{ */
{
	printf("\n " PACNEWRAT_VERSION "\n");
	printf("     \\   (\\,/)\n"
		   "      \\  oo   '''//,        _\n"
		   "       ,/_;~,       \\,     / '\n"
		   "       \"'   \\    (    \\    !\n"
		   "             ',|  \\    |__.'\n"
		   "             '~  '~----''\n"
		   "\n"
		   "             Pacnewrat is...stealing your configs!\n\n");
} /* }}} */

void free_backup(void *ptr) { /* {{{ */
	backup_t *backup = ptr;
	free(backup->system.path);
	free(backup->system.hash);
	free(backup->local.path);
	free(backup->local.hash);
	free(backup);
} /* }}} */

int main(int argc, char *argv[])
{
	int ret;
	enum _alpm_errno_t err;

	setlocale(LC_ALL, "");

	cfg.lineEnd = 0x0a;
	
	if ((ret = parse_options(argc, argv)) != 0)
		return ret;

	//fprintf(stderr, "initializing alpm\n");
	pmhandle = alpm_initialize(PACMAN_ROOT, PACMAN_DBPATH, &err);
	if (!pmhandle) {
		fprintf(stderr, "failed to initialize alpm library\n");
		goto finish;
	}

	alpm_all_backups();

finish:
	alpm_release(pmhandle);
	return ret;
}
/* vim: set ts=4 sw=4 noet: */
