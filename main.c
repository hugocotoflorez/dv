#include "da.h"
#include <dirent.h> // opendir - readdir - closedir
#include <regex.h> // posix regex
#include <stdio.h> // rename
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // chmod
#include <unistd.h> // chdir

// variadic args (dont kill me)
static char *args;

DA(struct dirent)
DirentDA;
static DirentDA dir_da;


// clang-format off
static const char *type_lookup[] = {
        [DT_BLK]     =  "BLK",
        [DT_CHR]     =  "CHR",
        [DT_DIR]     =  "DIR",
        [DT_FIFO]    =  "PIPE",
        [DT_LNK]     =  "LNK",
        [DT_REG]     =  "REG",
        [DT_SOCK]    =  "SOCK",
        [DT_UNKNOWN] =  "UNK",
};
static const char *color_lookup[] = {
        [DT_BLK]     =  "\e[0m",
        [DT_CHR]     =  "\e[0m",
        [DT_DIR]     =  "\e[34m",
        [DT_FIFO]    =  "\e[0m",
        [DT_LNK]     =  "\e[0m",
        [DT_REG]     =  "\e[1m",
        [DT_SOCK]    =  "\e[0m",
        [DT_UNKNOWN] =  "\e[0m",
};
// clang-format on

DirentDA
get_files(const char *path)
{
        struct dirent *dir_entry;
        DirentDA dir_da;
        DIR *dir;

        da_init(&dir_da);
        if ((dir = opendir(path))) {
                while ((dir_entry = readdir(dir)))
                        da_append(&dir_da, *dir_entry);
                closedir(dir);
        }
        return dir_da;
}

static int
__comp_dir(void const *dir1, void const *dir2)
{
        return strcmp(((struct dirent *) dir1)->d_name, ((struct dirent *) dir2)->d_name);
}

void
dir_sort(DirentDA *dir)
{
        qsort(dir->data, dir->size, sizeof dir->data[0], __comp_dir);
}

void
insert(struct dirent dir, regmatch_t *m, int re_nsub)
{
        char new_name[256];
        strcpy(new_name, args);
        strcat(new_name, dir.d_name);
        printf("INS %s -> %s%s\n", dir.d_name, args, dir.d_name);
        if (rename(dir.d_name, new_name)) {
                perror("Rename");
        }
}

void
append(struct dirent dir, regmatch_t *m, int re_nsub)
{
        char new_name[256];
        strcpy(new_name, dir.d_name);
        strcat(new_name, args);
        printf("APN %s -> %s%s\n", dir.d_name, dir.d_name, args);
        if (rename(dir.d_name, new_name)) {
                perror("Rename");
        }
}

void
rm(struct dirent dir, regmatch_t *m, int re_nsub)
{
        printf("RM %s\n", dir.d_name);
        if (remove(dir.d_name)) {
                perror("Remove");
        }
}

void
cd(struct dirent dir, regmatch_t *m, int re_nsub)
{
        printf("CD %s\n", dir.d_name);
        if (chdir(dir.d_name)) {
                perror("Cd");
        }
}

static int
atooctal(const char *str)
{
        int n = 0;
        for (; '0' <= *str && *str <= '8'; ++str) {
                n <<= 3;
                n |= (*str - '0') & 0x7;
        }
        return n;
}

void
prot(struct dirent dir, regmatch_t *m, int re_nsub)
{
        char new_name[256];
        strcpy(new_name, dir.d_name);
        strcat(new_name, args);
        printf("PROT %s -> %o\n", dir.d_name, atooctal(args));
        chmod(dir.d_name, atooctal(args));
}

void
subs(struct dirent dir, regmatch_t *m, int re_nsub)
{
        char buf[256];
        int offset = 0;
        int start;
        int finish;

        strcpy(buf, dir.d_name);
        for (int i = 0; i <= re_nsub; i++) {
                start = m[i].rm_so;
                finish = m[i].rm_eo;
                offset += strlen(args) - (finish - start);
                memmove(buf + start + strlen(args), buf + finish, strlen(buf + finish) + 1);
                memcpy(buf + start, args, strlen(args));
        }
        printf("SUB %s -> %s\n", dir.d_name, buf);
        if (rename(dir.d_name, buf)) {
                perror("Rename");
        }
}

void
print_dir(struct dirent dir, regmatch_t *m, int re_nsub)
{
        char buf[256];
        int offset = 0;
        int start;
        int finish;

        printf("%-5s", type_lookup[dir.d_type]);

        if (m)
                for (int i = 0; i <= re_nsub; i++) {
                        start = m[i].rm_so;
                        finish = m[i].rm_eo;

                        printf("%s", color_lookup[dir.d_type]);
                        printf("%.*s", start - offset, dir.d_name + offset);
                        printf("\e[7m"); // reverse
                        printf("%.*s", finish - start, dir.d_name + start);
                        offset = finish;
                        printf("\e[0m");
                }

        printf("%s", color_lookup[dir.d_type]);
        printf("%s\n", dir.d_name + offset);
        printf("\e[0m");
}

void
print_files(DirentDA dir)
{
        printf("%-5s", "TYPE");
        printf("%-10s\n", "NAME");
        for_da_each(dirp, dir) print_dir(*dirp, 0, 0);
}

void
filter(DirentDA dir_da, const char *pattern,
       void (*action)(struct dirent, regmatch_t *, int), int cflags, int eflags, int max)
{
        regex_t regex;

        if (regcomp(&regex, pattern, cflags)) {
                return;
        }

        for_da_each(entry, dir_da)
        {
                regmatch_t m[10];
                switch (regexec(&regex, entry->d_name, 10, m, eflags)) {
                case 0:
                        action(*entry, m, regex.re_nsub);
                        if (--max == 0)
                                goto exit;
                        break;
                case REG_NOMATCH:
                        break;
                default:
                        perror(entry->d_name);
                        break;
                }
        }
exit:
        regfree(&regex);
        return;
}

char *
get_first_noescaped(const char *buf, char c)
{
        const char *s = buf;

        if (*s == c)
                return (char *) s;

        while ((s = strchr(s + 1, c))) {
                if (s[-1] != '\\')
                        return (char *) s;
        }

        return NULL;
}

void
refresh()
{
        char new_name[256];
        char *ret = getcwd(new_name, sizeof new_name - 1);
        if (ret != NULL) {
                da_destroy(&dir_da);
                dir_da = get_files(ret);
                dir_sort(&dir_da);
        }
}

void
input_loop(const char *path)
{
        ssize_t len;
        char buf[256] = "\n";
        char *sep;
        char *sep2;
        int cflags = REG_ICASE;
        int eflags = REG_NOTBOL;

        dir_da = get_files(path);
        dir_sort(&dir_da);

        len = strlen(buf);

        do {
                /* trim at newline */
                buf[len - 1] = 0;

                /* Get sep at / */
                sep = get_first_noescaped(buf, '/');

                if (!sep) {
                        if (!strcmp(buf, "clear") || !strcmp(buf, "CLEAR")) {
                                printf("\e[H\e[2J");
                        } else if (!strcmp(buf, "quit") || !strcmp(buf, "QUIT") ||
                                   !strcmp(buf, "exit") || !strcmp(buf, "q")) {
                                break;
                        } else
                                /* apply filter and print*/
                                filter(dir_da, buf, print_dir, cflags, eflags, 0);
                        goto prompt;
                }

                sep[0] = '\0';
                sep2 = get_first_noescaped(sep + 1, '/');

                if (!sep2) {
                        if (!strcmp(buf, "dir") || !strcmp(buf, "DIR") ||
                            !strcmp(buf, "cd") || !strcmp(buf, "CD")) {
                                filter(dir_da, sep + 1, cd, cflags, eflags, 1);
                                refresh();
                        } else if (!strcmp(buf, "rm") || !strcmp(buf, "RM")) {
                                filter(dir_da, sep + 1, rm, cflags, eflags, 0);
                                refresh();
                        } else
                                /* apply filter and print*/
                                filter(dir_da, buf, print_dir, cflags, eflags, 0);
                        goto prompt;
                }

                sep2[0] = '\0';

                /* >> Command/Arg1/Arg2 */
                /* --[Index]--
                 * Command: buf
                 * regex pattern: sep+1
                 * command args: sep+2 */

                if (!strcmp(buf, "s") || !strcmp(buf, "S")) {
                        args = sep2 + 1;
                        filter(dir_da, sep + 1, subs, cflags, eflags, 0);
                        refresh();
                }

                else if (!strcmp(buf, "i") || !strcmp(buf, "I")) {
                        args = sep2 + 1;
                        filter(dir_da, sep + 1, insert, cflags, eflags, 0);
                        refresh();
                }

                else if (!strcmp(buf, "a") || !strcmp(buf, "A")) {
                        args = sep2 + 1;
                        filter(dir_da, sep + 1, append, cflags, eflags, 0);
                        refresh();
                }

                else if (!strcmp(buf, "prot") || !strcmp(buf, "PROT")) {
                        args = sep2 + 1;
                        filter(dir_da, sep + 1, prot, cflags, eflags, 0);
                }

        prompt:
                /* print prompt */
                printf(">> ");
                fflush(stdout);

        } while ((len = read(STDIN_FILENO, buf, sizeof buf - 1)) >= 0);

        da_destroy(&dir_da);
}

int
main(int argc, char **argv)
{
        input_loop(argc == 2 ? argv[1] : ".");
        return 0;
}
