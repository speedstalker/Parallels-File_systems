#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>
#include <stddef.h>

#include <limits.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <assert.h>

//==============================================================================
// (un)likely defines
//------------------------------------------------------------------------------
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
//==============================================================================
// Error handling macroses
//------------------------------------------------------------------------------
#define IS_DEBUG 1

#if (IS_DEBUG == 1)
//{
   #define HANDLE_ERROR HANDLE_ERROR_wL
   #define HANDLE_ERROR_wL(msg)                                                    \
                   do                                                              \
                   {                                                               \
                   char err_msg[256] = {0};                                        \
                                                                                   \
                   snprintf (err_msg, 255, "%d. " msg "%c", __LINE__, '\0');       \
                   perror (err_msg);                                               \
                   exit   (EXIT_FAILURE);                                          \
                   }                                                               \
                   while (0)
//}
#else
//{
   #define HANDLE_ERROR_wL HANDLE_ERROR
   #define HANDLE_ERROR(msg) \
                  do { perror(msg); exit(EXIT_FAILURE); } while (0)
//}
#endif
//==============================================================================

#define PROC_PATH "/proc"
#define TARGET_FILE_NAME "status"

#define MAX_PATH_LEN 256

int main ()
{
DIR *dirp = NULL, *inner_dirp = NULL;

long max_len_of_name = 0, len_of_dir_ent = 0;
struct dirent *dir_ent = NULL, *prev_dir_ent = NULL;

long pid = 0;
char *str = NULL, *endptr = NULL;

char status_folder_path[MAX_PATH_LEN] = {0};
char status_path[MAX_PATH_LEN] = {0};
FILE*  status_file_ptr = 0;
char* name_buf = NULL;

if (unlikely((dirp = opendir (PROC_PATH)) == NULL))
        HANDLE_ERROR ("opendir");


max_len_of_name = pathconf (PROC_PATH, _PC_NAME_MAX);
if (max_len_of_name == -1)             /* Limit not defined, or error */
        max_len_of_name = MAX_PATH_LEN - 1;         /* Take a guess */
len_of_dir_ent = offsetof (struct dirent, d_name) + max_len_of_name + 1;

if (unlikely((dir_ent = (struct dirent*)calloc (1, len_of_dir_ent)) == NULL))
        HANDLE_ERROR ("calloc: dir_ent");

if (unlikely(((name_buf = (char*)calloc (1024, sizeof (char))) == NULL)));

while (1)
        {
        //======================================================================
        // Get next dir entry
        //----------------------------------------------------------------------
        prev_dir_ent = dir_ent;
        if (readdir_r (dirp, dir_ent, &dir_ent) != 0)
                HANDLE_ERROR ("readdir_r (possibly do not change errno)");
        if (unlikely(dir_ent == NULL))
                {
                dir_ent = prev_dir_ent;
                break; // end of dir
                }
        //======================================================================

        //======================================================================
        // Try to convert dir name to number
        //----------------------------------------------------------------------
        str = dir_ent->d_name;
        errno = 0;
        pid = strtol (str, &endptr, 10);    // 10 - base of numerical system
        if ((errno == ERANGE && (pid == LONG_MAX || pid == LONG_MIN))
                           || (errno != 0 && pid == 0))
                HANDLE_ERROR ("strtol");

        if ((endptr == str) || (*endptr != '\0'))
                continue; // 'pid' must contain only digits => this is not proc dir

        if (pid <= 0)
                HANDLE_ERROR ("pid <= 0");

        // printf ("\t%ld\n", pid);
        //======================================================================

        //======================================================================
        // Try to get 'status' file
        //----------------------------------------------------------------------
        if (snprintf (status_folder_path, MAX_PATH_LEN - 1, PROC_PATH "/%ld/" "%c", pid, '\0') < 0)
                HANDLE_ERROR ("snprintf: status_folder_path");
        if (unlikely((inner_dirp = opendir (status_folder_path)) == NULL))
                HANDLE_ERROR ("opendir: status_folder_path");

        // printf ("%s\n", status_folder_path);

        while (1)
                {
                //==============================================================
                // Get next inner dir entry
                //--------------------------------------------------------------
                prev_dir_ent = dir_ent;
                if (readdir_r (inner_dirp, dir_ent, &dir_ent) != 0)
                        HANDLE_ERROR ("readdir_r: (inner) (possibly do not change errno)");
                if (unlikely(dir_ent == NULL))
                        {
                        dir_ent = prev_dir_ent;
                        break; // end of dir
                        }
                //==============================================================

                if (strcmp (dir_ent->d_name, TARGET_FILE_NAME) == 0)
                        {
                        // we found 'status' file
                        if (snprintf (status_path, MAX_PATH_LEN - 1, PROC_PATH "/%ld/" TARGET_FILE_NAME "%c", pid, '\0') < 0)
                                HANDLE_ERROR ("snprintf: status_path");

                        // printf ("%s\n", status_path);

                        if ((status_file_ptr = fopen (status_path, "r")) == NULL)
                                HANDLE_ERROR ("fdopen");

                        if (fgets (name_buf, MAX_PATH_LEN, status_file_ptr) == NULL)
                                HANDLE_ERROR ("fgets");

                        printf ("Pid:\t%ld\n", pid);
                        printf ("%s\n", name_buf);

                        if (fclose (status_file_ptr) != 0)
                                HANDLE_ERROR ("fclose: status_file_ptr");
                        }
                else
                        continue;
                }

        if (unlikely(closedir (inner_dirp) != 0))
                HANDLE_ERROR ("closedir: inner_dirp");

        // printf ("%s\n", dir_ent->d_name);
        }


free (name_buf);
name_buf = NULL;

free (dir_ent);
dir_ent = NULL;

if (unlikely(closedir (dirp) != 0))
        HANDLE_ERROR ("closedir");

return 0;
}
