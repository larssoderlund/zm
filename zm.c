#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<unistd.h>
#include<errno.h>
#include<sqlite3.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>

#include "link.h"

char *get_current_dir_name(void);

#define LOG(x) fprintf(stderr,"%s\n", (x));

s_linked_t *rules;

sqlite3 *db;

char *wd;

char *envp[10] = {"GCC_EXEC_PREFIX=/usr/lib/gcc/", "PATH=/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin/:/bin/", NULL};

typedef struct rule {
    char *cfile;
    char *ofile;
    char *args;
} rule_t;

void 
print_help_message(void)
{
    printf("-help\t\tprint this help message\n");
    printf("-list\t\tlist all stored compilation rules\n");
    printf("-delete X\t\tdelete rule compilation rule number X\n");
    printf("-purge \t\tdelete entire database .zm.db\n");
    printf("-clean \t\tclean (remove) object files\n");
    printf("-tags \t\treturn parameters to ctags\n");
}

int
vfork_exec(char *file, char *argv[], char *env[])
{
    pid_t cpid = vfork();

    if(cpid == 0){
        execve(file, argv, env);
        exit(EXIT_FAILURE);
    }else{

        if(cpid > 0){
            int status;

            if (waitpid(cpid, &status, 0) != -1){

                if (WIFEXITED(status))
                    return WEXITSTATUS(status);
            }
        }
    }

    return EXIT_FAILURE;
}

void check_file(void *val, void *arg)
{
    char path[512];
    char *argv[50];
    struct stat cbuf, obuf;
    rule_t *t = (rule_t *)val;
    int rebuild = 1;    
    int i;

    sprintf(path, "%s/%s", wd, t->cfile);

    if(!stat(path, &cbuf)){

        sprintf(path, "%s/%s", wd, t->ofile);

        if(!stat(path, &obuf)){

            if(obuf.st_mtime >= cbuf.st_mtime)
                rebuild = 0;
            
        }else
            fprintf(stderr, "zm: error processing object file %s : %s\n", path, strerror(errno));

    }else
        fprintf(stderr, "zm: error processing c-file %s : %s\n", path, strerror(errno));

    if (rebuild){
        printf("zm rebuilding %s with %s\n", t->cfile, t->args);

        argv[0] = strtok(t->args, " ");
        if(argv[0] != (char *)NULL){
            for(i = 1 ; i < 49; i++)
                if( (argv[i] = strtok(NULL, " ")) == (char *)NULL)
                    break;
        }

        if(vfork_exec(argv[0], argv, envp) != EXIT_SUCCESS)
        {
            fprintf(stderr, "zm failed rebuilding %s\n", t->cfile);
        }
    }

}

void print_rule(void *val, void *arg)
{
    rule_t *t = (rule_t *)val;
    printf("%i: %s [[%s]]\t->\t%s\n", (*(int *)arg)++, t->cfile, t->args, t->ofile);
}

void delete_rule(void *val, void *arg)
{
    char query[512];
    rule_t *t = (rule_t *)val;

    if((*(int *)arg) == 0){
        if( snprintf(query, 512, 
            "DELETE FROM rules WHERE cfile == '%s' AND ofile == '%s';", t->cfile, t->ofile) < 0){
            LOG("zm: could not execute query:");
            LOG(query);
        }

        if(sqlite3_exec(db, query, NULL, NULL, (char **)NULL)){
            LOG("zm: could not execute query");
            LOG(query);
        }
    }
    (*(int *)arg)--;
}

/* Adds verification points to rule list */
int
sql_callback(void *r, int ncol, char **columns, char **values)
{
    s_linked_t **l = (s_linked_t **)r;
    if (l != (void *)NULL){
        int i;
        rule_t *t = calloc(1, sizeof(rule_t));
        for(i = 0 ; i < ncol; i++){
            if(!strcmp(values[i], "cfile"))
                t->cfile =  strdup(columns[i]);
            if(!strcmp(values[i], "ofile"))
                t->ofile = strdup(columns[i]);
            if(!strcmp(values[i], "args"))
                t->args = strdup(columns[i]);
        }
        *l = append(*l, (void *)t);
    }

    return 0;
}

int 
main(int argc, char *argv[])
{
    int c;
    char gcc[] = "/usr/bin/gcc";

    char query[512];
    char *dbname = ".zm.db"; 

    char *cc = getenv("CC");

    wd = get_current_dir_name();

    if(cc == (char *)NULL)
        cc = gcc;

    if(argc == 2 && !strcmp("-help", argv[1])) {
        print_help_message();
        return EXIT_SUCCESS;
    }

    /* Tries to open otherwise create one */ 
    if((dbname != (char *)NULL && sqlite3_open(dbname, &db) == SQLITE_ERROR)){
        LOG("zm: Error could not open database")
        LOG(dbname);
        return EXIT_FAILURE;
    }
    
    // Check if table exists - otherwise create 
    if(sqlite3_exec(db, "SELECT * FROM rules LIMIT 1", NULL, NULL, (char **)NULL)){

        // Create database
        if(sqlite3_exec(db,
                    "CREATE TABLE rules(            \
                    cfile varchar(256) NOT NULL,    \
                    ofile varchar(256) not null,    \
                    args varchar(512) not null)", NULL, NULL, (char **)NULL) == SQLITE_ERROR){
                LOG("m: could not create rules table - exit.");
                LOG("m: please try to manually delete .zm.db");
                return EXIT_FAILURE;
        }
    }

    // Load the database
    if(sqlite3_exec(db, "SELECT * FROM rules", sql_callback,(void *)&rules, (char **)NULL)){
        LOG("zm: could not execute query");
        LOG("SELECT * FROM rules");
    }
    if(argc == 2 && !strcmp("-list", argv[1])) {
        int c = 0;     
        foreach((void *)rules, (void *)&c, print_rule);
        return EXIT_SUCCESS;
    }

    if(argc == 2 && !strcmp("-purge", argv[1])) {
        return remove(".zm.db");
    }

    if(argc == 3 && !strcmp("-delete", argv[1])) {
        int c = atoi(argv[2]);     
        foreach((void *)rules, (void *)&c, delete_rule);
        return EXIT_SUCCESS;
    }

    // Make mode
    if(argc == 1){

        foreach(rules, (void *)NULL, check_file);

    // Compile mode
    }else{

        char *cfile;
        char *ofile;

        cfile = ofile = (char *)NULL;

        for(c = 1; c < argc; c++) {
            if(strstr(argv[c], ".c") != (char *)NULL || strstr(argv[c], ".C") != (char *)NULL)
                cfile =  argv[c];
            if(strstr(argv[c], ".o") != (char *)NULL || strstr(argv[c], ".O") != (char *)NULL)
                ofile =  argv[c];
        }

        if(cfile != (char *)NULL) {
            char t[512], astr[512];

            // ofile not defined - assume cfile -'.c' + '.o'
            if(ofile == (char *)NULL){
                strcpy(t, cfile);
                t[strlen(t) - 1 ] = (char)'o';
            }else
                strcpy(t, ofile);

            // if we already has a delete it (and later updated)
            if( snprintf(query, 512, 
                "DELETE FROM rules WHERE cfile == '%s' AND ofile == '%s';", cfile, t) < 0){
                LOG("zm: could not execute query:");
                LOG(query);
            }

            if(sqlite3_exec(db, query, NULL, NULL, (char **)NULL)){
                LOG("zm: could not execute query");
                LOG(query);
            }

            int i;
            for(astr[0] = i = 0 ; i < argc  ; i++){  
                strcpy(astr + strlen(astr) , (i == 0 ? cc : argv[i]));
                strcpy(astr + strlen(astr) , " ");
            }

            // If we succeded compiling - add rule - currently we always succeed
            argv[0] = cc; 
            if(vfork_exec(argv[0], argv, envp) == EXIT_SUCCESS){
                if( snprintf(query, 512, 
                    "INSERT INTO rules(cfile, ofile, args) VALUES   \
                     ('%s', '%s', '%s')", cfile, t, astr) < 0){
                    LOG("zm: snprintf failed : "); 
                    LOG(query);
                }

                if(sqlite3_exec(db, query, NULL, NULL, (char **)NULL)){
                    LOG("zm: could not insert rule");
                    LOG(query);
                }
            }else
                fprintf(stderr, "EXIT_FAILURE when running: %s\n", astr);
            
        }else
            fprintf(stderr, "Warning could not identify c-file in input always execute rule\n");
    }

    if(sqlite3_close(db))
    {
        LOG("zm: warning could not close database zm.db");
    }

    sync();

    free(wd);

    return EXIT_SUCCESS;
}
