#include <stdio.h>
#include <stdlib.h>
#include <string.h>

template<int N> void cmdline<N>::parsecmdline(int argc, char **argv)
{
  char *tmp, *eq;
  char const *helpme = "--help";

  int i,j;
  for (i=1; i<argc; ++i) {
    tmp = argv[i];
    if (strncmp(tmp, helpme, strlen(helpme))==0) {
      print_help(argv[0]);
    }
    // Malformed option.
    if (tmp[0] != '-' || strlen(tmp) == 1) {
      unrecognized_opt(argv[0], tmp);
    }
    // Long form of flag.
    else if (tmp[1] == '-') {
      for (j=0; j<N; ++j) {
	if (strncmp(&tmp[2], longs[j],
		    strlen(longs[j]))==0) {
	  // Only boolean options can take no arguments.
	  if ((eq = strchr(tmp, '='))==NULL) {
	    if (bvals[j])
	      svals[j] = "true";
	    else
	      unrecognized_opt(argv[0], tmp);
	  }
	  else {
	    svals[j] = eq+1;
	  }
	  break;
	}
      }
      // Didn't match a known option.
      if (j == N)
	unrecognized_opt(argv[0], tmp);
    }
    // Short form of flag.
    else {
      for (j=0; j<N; ++j) {
	if (tmp[1] == shorts[j][0]) {
	  // Only boolean options can take no argument.
	  if (strlen(tmp) < 3) {
	    if (bvals[j])
	      svals[j] = "true";
	    else
	      unrecognized_opt(argv[0], tmp);
	  }
	  else {
	    svals[j] = tmp+2;
	  }
	  break;
	}
      }
      // No match found.
      if (j == N)
	unrecognized_opt(argv[0], tmp);
    }
  }
  atoi_opts();
}

template<int N> void cmdline<N>::unrecognized_opt(char *argv0, char *option)
{
  printf("%s: unrecognized option '%s'\n"
	 "Try `%s --help' for more information.\n",
	 argv0, option, argv0);
  exit(1);
}

template<int N> void cmdline<N>::print_help(char *argv0)
{
  printf("Usage: %s [OPTION]\n%s\n", argv0, progdesc);
  /* Note that this may not actually print the default values
   * if the --help flag is not the first flag parsed. Oh well! */
  for (int i=0; i<N; ++i) {
    printf("-%s, --%s\t%s %s%s%s\n",
	   shorts[i], longs[i], descs[i],
	   ((bvals[i]) ? "" : "[default "),
	   ((bvals[i]) ? "" : svals[i]),
	   ((bvals[i]) ? "" : "]"));
  }
  exit(0);
}

template<int N> void cmdline<N>::atoi_opts()
{
  for (int i=0; i<N; ++i) {
    if (ivals[i]) {
      ivals[i] = atoi(svals[i]);
    }
    else if (bvals[i]) {
      if (strncmp(svals[i], "false", strlen("false"))==0)
	bvals[i] = false;
    }
  }
}
