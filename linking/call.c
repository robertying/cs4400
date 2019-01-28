#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

static char *make_relative(char *p);

int main(int argc, char **argv) {
  char *path;
  void *so;
  int (*f)(int);
  int r;

  if (argc != 4) {
    fprintf(stderr,
	    "%s: expects three arguments: <file.so> <function> <int>\n",
	    argv[0]);
    return 1;
  }

  path = argv[1];
  if (path[0] != '/')
    path = make_relative(path);
  so = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
  if (!so) {
    fprintf(stderr, "error opening %s\n", path);
    return 1;
  }
  
  f = dlsym(so, argv[2]);
  if (!f) {
    fprintf(stderr, "could not find %s in %s\n", argv[2], argv[1]);
    return 1;
  }
	    
  r = f(atoi(argv[3]));

  printf("%d\n", r);

  return 0;
}

static char *make_relative(char *p)
{
  int len = strlen(p);
  char *p2 = malloc(len + 3);
  p2[0] = '.';
  p2[1] = '/';
  strcpy(p2+2, p);
  return p2;
}
