#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

#include "lexer.h"
#include "map.h"
#include "token.h"

int main(int argc, const char **argv)
{
  Map map;
  map_init(&map, NULL, NULL);

  {
    long i, size = 100000;
    for (i = 0; i < size; ++i) {
      map_insert(&map, (void *) i, (void *) i);
    }

    printf("insertion done\n");

    for (i = 0; i < size; ++i) {
      MapIndex index;
      if (!map_find(&map, (void *) i, &index)) {
        printf("%ld not found\n", i);
      }
    }

    printf("lookup done\n");
  }

  /*
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>", argv[0]);
    return EXIT_FAILURE;
  }

  {
    char *source;
    long  source_size;
    {
      struct stat s;
      if (stat(argv[1], &s) && !S_ISREG(s.st_mode)) {
        fprintf(stderr, "`%s` is not a valid file\n", argv[1]);
        return EXIT_FAILURE;
      }
      source = malloc(s.st_size);
      source_size = s.st_size;
    }
    {
      FILE *file = fopen(argv[1], "r");
      if (!file) {
        fprintf(stderr, "Cannot open file `%s`\n", argv[1]);
        return EXIT_FAILURE;
      }
      fread(source, sizeof source[0], source_size, file);
      fclose(file);
    }
    {
      Lexer lexer;
      long  offset = 0;
      Token token;
      lexer_init(&lexer, source, source_size);

      do {
        lexer_next_token(&lexer, &token);

        printf("kind: %4d", token.kind);
        printf("\tlength: %4ld", token.length);
        if (token.kind != TOKEN_KIND_SPACE && token.kind != TOKEN_KIND_NEWLINE && token.kind != TOKEN_KIND_EOF) {
          printf("\ttoken: %.*s", (int) token.length, source + offset);
        }
        printf("\n");

        offset += token.length;
      } while (token.kind != TOKEN_KIND_EOF);
    }
  }
*/
  return EXIT_SUCCESS;
}
