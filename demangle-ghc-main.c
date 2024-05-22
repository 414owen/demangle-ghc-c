/*
This file lets you test the haskell_demangle function
defined in demangle-ghc.c

It can be used in a UNIX pipe, or interactively.
*/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *haskell_demangle(const char *mangled);

int main() {
  char *line = malloc(10);
  size_t line_len = 10;
  bool tty = isatty(STDIN_FILENO);
  while (true) {
    if (tty) {
      fputs("> ", stdout);
    }
    errno = 0;
    ssize_t n = getline(&line, &line_len, stdin);
    if (n <= 0 && errno != 0) {
      perror("failed to read input");
      return 1;
    }
    if (n <= 0) {
      return 0;
    }
    char *demangled = haskell_demangle(line);
    if (demangled == NULL) {
      puts("Demangler error!");
      return 1;
    }
    fputs(demangled, stdout);
    free(demangled);
  }
  free(line);
}
