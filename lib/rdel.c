#define  _POSIX_C_SOURCE 200809L
#define  _XOPEN_SOURCE 500L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <ftw.h>

/* Call-back to the 'remove()' function called by nftw() */
static int
remove_callback(const char *pathname,
                __attribute__((unused)) const struct stat *sbuf,
                __attribute__((unused)) int type,
                __attribute__((unused)) struct FTW *ftwb)
{
  return remove (pathname);
}

int
main ()
{
  /* Create the temporary directory */
  char template[] = "/tmp/tmpdir.XXXXXX";
  char *tmp_dirname = mkdtemp (template);

  if (tmp_dirname == NULL)
  {
     perror ("tempdir: error: Could not create tmp directory");
     exit (EXIT_FAILURE);
  }

  /* Change directory */
  if (chdir (tmp_dirname) == -1)
  {
     perror ("tempdir: error: ");
     exit (EXIT_FAILURE);
  }

  /******************************/
  /***** Do your stuff here *****/
  /******************************/

  /* Delete the temporary directory */
  if (nftw (tmp_dirname, remove_callback, FOPEN_MAX,
            FTW_DEPTH | FTW_MOUNT | FTW_PHYS) == -1)
    {
      perror("tempdir: error: ");
      exit(EXIT_FAILURE);
    }

