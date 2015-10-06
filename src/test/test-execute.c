/* The pdfout document modification and analysis tool.
   Copyright (C) 2015 AUTHORS (see AUTHORS file)
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <limits.h>

#include <xalloc.h>
#include <progname.h>
#include <full-read.h>
#include <full-write.h>
#ifndef GNULIB_DIRNAME
 #define GNULIB_DIRNAME 1
#endif
#include <dirname.h>

/* specification */
#include "test-execute.h"


/* execute programs and check their output */

bool use_valgrind = false;

enum {
  IO_SIZE = 32 * 1024,
};
  
static void
print_command (char **argv)
{
  if (pdfout_batch_mode == false)
    {
      char **list;
      
      fprintf (stderr, "spawning: '%s", argv[0]);
      
      for (list = &argv[1]; *list; ++list)
	fprintf (stderr, " %s", *list);
      
      fprintf (stderr, "'\n");
    }
}

static pid_t spawn (char *argv[], int fds[2], bool read_from_child,
		    bool write_to_child)
{
  posix_spawn_file_actions_t actions;
  pid_t pid;

  /* pipe for reading from child's stdout */
  int pipe_from_child[2];

  /* pipe for writing to child's stdin */
  int pipe_to_child[2];

  print_command (argv);
  
  /* open pipes */
  if (posix_spawn_file_actions_init (&actions)
    ||  (read_from_child
       && (pipe (pipe_from_child)
	|| posix_spawn_file_actions_adddup2 (&actions, pipe_from_child[1], 1)
	|| posix_spawn_file_actions_addclose (&actions, pipe_from_child[0])
	|| posix_spawn_file_actions_addclose (&actions, pipe_from_child[1])))
    || ((write_to_child
       && (pipe (pipe_to_child)
	|| posix_spawn_file_actions_adddup2 (&actions, pipe_to_child[0], 0)
	|| posix_spawn_file_actions_addclose (&actions, pipe_to_child[0])
	|| posix_spawn_file_actions_addclose (&actions, pipe_to_child[1])))))
    error (99, errno, "spawn_pipe: cannot open pipe");

  /* spawn child */
  if (posix_spawnp (&pid, argv[0],
		    /* if we don't have file actions, pass NULL:
		       glibc then uses 'vfork' instead of the slower 'fork'
		    */
		    (read_from_child || write_to_child) ? &actions : NULL,
		    /* use default attribute values */
		    NULL,
		    argv, environ))
    error (99, errno, "spawn_pipe: posix_spawnp");

  if ((read_from_child
       && (fds[0] = pipe_from_child[0], close (pipe_from_child[1])))
   || (write_to_child
       && (fds[1] = pipe_to_child[1], close (pipe_to_child[0]))))
    error (99, errno, "spawn_pipe: close");

  posix_spawn_file_actions_destroy (&actions);
  
  return pid;
}

static char **
va_list_to_argv (const char *command, va_list ap)
{
  va_list ap_copy;
  int arg_num, i;
  char **argv;
  
  /* count args */
  va_copy (ap_copy, ap);
  arg_num = 0;
  while (va_arg (ap_copy, char *) != NULL)
    ++arg_num;
  
  va_end (ap_copy);

  argv = XNMALLOC (arg_num + 2, char *);

  argv[0] = xstrdup (command);
  
  for (i = 0; i < arg_num; ++i)
    argv[i + 1] = xstrdup (va_arg (ap, char *));
  
  argv[arg_num + 1] = NULL;
  va_end (ap);
  
  return argv;
}

static void
argv_free (char **argv)
{
  char **p;
  for (p = argv; *p; ++p)
    free (*p);
  free (argv);
}

/* returns child's exit status */
static int wait_child (pid_t pid)
{
  int status;

  if (waitpid (pid, &status, 0) == -1)
    error (99, errno, "waitpid");

  if (WIFEXITED (status) == false
      || (status = WEXITSTATUS (status), status == 127))
    error (99, errno, "child did not terminate normally");

  return status;
}

static int
lsystem (char *argv[])
{
  pid_t pid = spawn (argv, NULL, false, false);
  return wait_child (pid);
}

static void print_header (const char *source_file, int line_number)
{
  char *source_file_base = base_name (source_file);
  fprintf (stderr, "------------------------------------------------------------------------------\n");
  
  fprintf (stderr, "file: %s\nline: %d\n\n", source_file_base, line_number);
  free (source_file_base);
}

static void xclose (int fd)
{
  if (close (fd) == -1)
    error (99, errno, "close");
}

static int xopen (const char *file, int mode)
{
  int fd;

  if (mode & O_CREAT)
    fd = open (file, mode, 0600);
  else
    fd = open (file, mode);
  
  if (fd == -1)
    error (99, errno, "xopen %s", file);
  return fd;
}

/* fails if more than IO_SIZE bytes can be read from FD */
static int fd_to_buffer (char buffer[IO_SIZE], int fd)
{
  size_t count;
  
  errno = 0;
  count = full_read (fd, buffer, IO_SIZE);
  if (count == IO_SIZE)
    error (99, 0, "fd_to_buffer: result does not fit into buffer");
  if (errno)
    error (99, errno, "fd_to_buffer: read error");
  
  return count;
}

static void buffer_to_fd (int fd, const char *buffer, size_t len)
{
  if (full_write (fd, buffer, len) != len)
    error (99, errno, "buffer_to_fd: full_write");
}

/* FIXME: at regex options argument for case-insensitive matching? */
static void
pattern_search (const char *buffer, size_t buffer_len,
		const char *regex, enum test_search_mode mode)
{
  const char *error_string;
  struct re_pattern_buffer pattern_buffer = {0};
  char *fastmap = xcharalloc (256);
  int error_code;
  
  switch (mode)
    {
    case TEST_GREP: re_set_syntax (RE_SYNTAX_POSIX_BASIC); break;
    case TEST_EGREP: re_set_syntax (RE_SYNTAX_POSIX_EXTENDED); break;
    default:
      error (99, 0, "unknown mode");
    }

  error_string = re_compile_pattern (regex, strlen (regex), &pattern_buffer);
  if (error_string)
    error (99, errno, "re_compile_pattern: %s", error_string);

  pattern_buffer.fastmap = fastmap;

  /* do the search */
  error_code = re_search (&pattern_buffer, buffer, buffer_len, 0, buffer_len,
			  NULL);
  if (error_code == -2)
    error (99, errno, "re_search");
  if (error_code == -1)
    {
      fprintf (stderr, "regex '%s' does not match following buffer:\n%s\n",
	       regex, buffer);
      exit (1);
    }
  regfree (&pattern_buffer);
  
}

/* Fails if the pipe's output excceedes IO_SIZE bytes.  */
static void
pipe_output_matches_string (int pipe_fd, const char *expected,
			    enum test_search_mode mode)
{
  int counts[2];
  char *buffers[2];
  char *files[2];
  int i;
  
  buffers[0] = xmalloc (IO_SIZE + 1);
  buffers[1] = (char *) expected;

  counts[0] = fd_to_buffer ((char *) buffers[0], pipe_fd);
  buffers[0][counts[0]] = '\0';
  
  counts[1] = strlen (expected);
  
  /* Compare buffers.  */

  switch (mode)
    {
    case TEST_EXACT:
      if (counts[0] == counts[1])
	if (counts[0] == 0 || memcmp (buffers[0], buffers[1], counts[0]) == 0)
	  break;
      
      error (0, 0, "not equal, running diff:");

      /* Write both buffers to files and run diff.  */
      for (i = 0; i < 2; ++i)
	{
	  int fd;
	  files[i] = test_tempname ();
	  fd = xopen (files[i], O_WRONLY | O_CREAT);
	  buffer_to_fd (fd, buffers[i], counts[i]);
	  xclose (fd);
	}

      char *argv[] = {xstrdup ("diff"), xstrdup ("-C3"), files[0],
		      files[1], NULL};
      if (lsystem (argv) == 0)
	/* Should never happen.  */
	error (99, 0, "diff returned 0 for different files");
      exit (1);
      
    case TEST_FIXED:
      if (memmem (buffers[0], counts[0], expected, strlen (expected)))
	break;
      fprintf (stderr, "did not find string '%s' in following buffer:\n%s\n",
	       buffers[1], buffers[0]);
      exit (1);

    case TEST_GREP:
    case TEST_EGREP:
      pattern_search (buffers[0], counts[0], buffers[1], mode);
      break;
	  
    default:
      error (99, 0, "unknown search mode");
    }
  
  /* cleanup */
  free (buffers[0]);
}
  
void
_test_command (const char *source_file, int source_line, int expected_status,
	       const char *input, const char *expected,
	       enum test_search_mode mode, const char *command, ...)
{
  va_list ap;
  char **argv;
  pid_t pid;
  int status, pipe_fds[2];

  print_header (source_file, source_line);

  va_start (ap, command);
  argv = va_list_to_argv (command, ap);

  pid = spawn (argv, pipe_fds, expected != NULL, input != NULL);
  
  if (input)
    {
      size_t len = strlen (input);
      if (len > PIPE_BUF)
	/* Avoid deadlock.  */
	error (1, 0, "_test_command: input exceeds PIPE_BUF");
      buffer_to_fd (pipe_fds[1], input, len);
      xclose (pipe_fds[1]);
    }
  
  if (expected)
    {
      pipe_output_matches_string (pipe_fds[0], expected, mode);
      xclose (pipe_fds[0]);
    }

  /* check childs exit status */
  status = wait_child (pid);
  
 
  if (status != expected_status)
    {
      fprintf (stderr, "\nexit status: %d, expected status: %d\n", status,
	       expected_status);
      if (input)
	fprintf (stderr, "with input:\n%s\n", input);
      if (expected)
	fprintf (stderr, "expected output:\n%s\n", expected);
      exit (1);
    }
  
  /* cleanup */
  argv_free (argv);
}
	       			   
static int
file_fds_ok (int fds[2])
{
  char *buffers[2];
  int i, IO_SIZE =  32 * 1024;
  size_t count[2];
  bool equal = true;
  
  for (i = 0; i < 2; ++i)
    buffers[i] = xmalloc (IO_SIZE);
  
  for (;;)
    {
      for (i = 0; i < 2; ++i)
	{
	  count[i] = full_read (fds[i], buffers[i], IO_SIZE);
	  if (count[i] < (size_t) IO_SIZE && errno)
	    error (99, errno, "error in full_read");
	}
      
      /* at this point both reads succeeded  */
      if (count[0] != count[1])
	{
	  equal = false;
	  break;
	}

      if (count[0] == 0)
	/* we read everything */
	break;
      
      if (memcmp (buffers[0], buffers[1], count[0]))
	{
	  equal = false;
	  break;
	}
    }

  /* cleanup */
  for (i = 0; i < 2; ++i)
    free (buffers[i]);
  
  return !equal;
}

static int
files_equal (const char *file1, const char *file2)
{
  int i, result, fds[2];
  const char *files[2];
  files[0] = file1;
  files[1] = file2;

  for (i = 0; i < 2; ++i)
    {
      fds[i] = xopen (files[i], O_RDONLY);
    }
  
  result = file_fds_ok (fds);

  errno = 0;
  for (i = 0; i < 2; ++i)
    xclose (fds[i]);

  return result;
}

void
test_files_equal (const char *file1, const char *file2)
{
  if (strcmp (file1, file2) == 0)
    error (99, 0, "test_files_equal: file1 == file2");

  error (0, 0, "test_files_equal: %s %s", file1, file2);
  if (files_equal (file1, file2))
    {
      /* files differ. run diff to show the problem */
      char *argv[] = {xstrdup ("diff"), xstrdup (file1),
		      xstrdup (file2), NULL};
      if (lsystem (argv) == 1)
	exit (1);
      else
	error (99, 0,
	       "test_files_equal: diff returned 0 for different files?");
    }
}

/* search for patterns in files */

void
_test_grep (const char *file, const char *pattern, enum test_search_mode mode)
{
  int fd = xopen (file, O_RDONLY);

  pipe_output_matches_string (fd, pattern, mode);

  xclose (fd);
}

void
test_write_string_to_file (const char *file, const char *string)
{
  int fd = xopen (file, O_WRONLY | O_CREAT | O_EXCL);
  buffer_to_fd (fd, string, strlen (string));
  xclose (fd);
}
