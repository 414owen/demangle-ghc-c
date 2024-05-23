// SPDX-License-Identifier: MIT-0

/*
MIT No Attribution

Copyright 2024 Owen Shepherd

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
While this code is provided under a permissive licence, in the hope that it will
ease adoption, a note citing the original author (Owen Shepherd), would be
appreciated.
*/


#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
Demangles symbol names produced by the GHC haskell compiler.
See https://gitlab.haskell.org/ghc/ghc/wikis/commentary/compiler/symbol-names
*/

static
char z_chars[] = {
  ['a' - 'a'] = '&',
  ['b' - 'a'] = '|',
  ['c' - 'a'] = '^',
  ['d' - 'a'] = '$',
  ['e' - 'a'] = '=',
  ['g' - 'a'] = '>',
  ['h' - 'a'] = '#',
  ['i' - 'a'] = '.',
  ['l' - 'a'] = '<',
  ['m' - 'a'] = '-',
  ['n' - 'a'] = '!',
  ['p' - 'a'] = '+',
  ['q' - 'a'] = '\'',
  ['r' - 'a'] = '\\',
  ['s' - 'a'] = '/',
  ['t' - 'a'] = '*',
  ['u' - 'a'] = '_',
  ['v' - 'a'] = '%',
  ['z' - 'a'] = 'z'
};

static
char Z_chars[] = {
  ['C' - 'C'] = ':',
  ['L' - 'C'] = '(',
  ['M' - 'C'] = '[',
  ['N' - 'C'] = ']',
  ['R' - 'C'] = ')',
  ['Z' - 'C'] = 'Z'
};

struct str_buf {
  size_t capacity;
  size_t length;
  char *data;
};

// true signals an error
static
bool str_buf_push(struct str_buf *restrict buf, char c) {
  if (buf->length == buf->capacity) {
    // capacity := 1.5x
    buf->capacity += buf->capacity / 2;
    buf->data = realloc(buf->data, buf->capacity);
    if (buf->data == NULL) {
      return true;
    }
  }
  buf->data[buf->length++] = c;
  return false;
}

// Ensure this much free space is available
// true signals an error
static
bool str_buf_reserve(struct str_buf *restrict buf, size_t amt) {
  const size_t new_len = buf->length + amt;
  size_t capacity = buf->capacity;
  if (new_len > capacity) {
    if (new_len > capacity + capacity / 2) {
      capacity = new_len;
    } else {
      capacity += capacity / 2;
    }
    buf->data = realloc(buf->data, capacity);
    if (buf->data == NULL) {
      return true;
    }
    buf->capacity = capacity;
  }
  return false;
}

// true signals an error
static
bool str_buf_push_str(struct str_buf *restrict buf, const char *str) {
  size_t len = strlen(str);
  if (str_buf_reserve(buf, len)) {
    return true;
  }
  memcpy(buf->data, str, len);
  buf->length += len;
  return false;
}

// true signals an error
static
bool str_buf_push_char_code(struct str_buf *restrict buf, uint32_t char_code) {
  // This may look like up to three bytes too many, but GHC loves adding
  // suffixes to symbol names (_bytes, _info, _closure, _slow, _fast,
  // _srt, _str, _tbl, _btm, etc.)
  // In 99.9% of cases any overallocation will end up being used.
  if (str_buf_reserve(buf, 4)) {
    return true;
  }
#define PUSH(c) buf->data[buf->length++] = c
  if (char_code <= 0x7F) {
    // Plain ASCII
    PUSH(char_code);
  } else if (char_code <= 0x7FF) {
    PUSH(((char_code >> 6) & 0x1F) | 0xC0);
    PUSH(((char_code >> 0) & 0x3F) | 0x80);
  } else if (char_code <= 0xFFFF) {
    PUSH(((char_code >> 12) & 0x0F) | 0xE0);
    PUSH(((char_code >>  6) & 0x3F) | 0x80);
    PUSH(((char_code >>  0) & 0x3F) | 0x80);
  } else if (char_code <= 0x10FFFF) {
    PUSH(((char_code >> 18) & 0x07) | 0xF0);
    PUSH(((char_code >> 12) & 0x3F) | 0x80);
    PUSH(((char_code >>  6) & 0x3F) | 0x80);
    PUSH(((char_code >>  0) & 0x3F) | 0x80);
  } else {
    return true;
  }
  return false;
#undef PUSH
}

#define DEFAULT_BUF_SIZE 20
#define PEEK c
#define ADVANCE c = (*mangled++)
#define PUSH(c) if (str_buf_push(&buf, (c))) goto fail;
#define PUSH_STR(s) if (str_buf_push_str(&buf, (s))) goto fail;
#define RESERVE(n) if (str_buf_reserve(&buf, (n))) goto fail;

char *
haskell_demangle(const char *mangled)
{
  struct str_buf buf = {
    .capacity = DEFAULT_BUF_SIZE,
    .length = 0,
    .data = malloc(DEFAULT_BUF_SIZE)
  };

  char c;
  ADVANCE;

  while (true) {
    switch (PEEK) {
      case 'z':
        ADVANCE;
        // Parses hex code, but if it starts with 'a' - 'z',
        // it will always be prefixed by a '0'.
        if (isdigit(PEEK)) {
          uint32_t char_code = 0;
          do {
            char_code *= 16;
            if (PEEK <= '9') {
              char_code += PEEK - '0';
            } else {
              char_code += 10 + PEEK - 'a';
            }
            ADVANCE;
          } while (isxdigit(PEEK));
          if (PEEK != 'U') {
            goto fail;
          }
          if (str_buf_push_char_code(&buf, char_code)) {
            goto fail;
          }
          ADVANCE;
          continue;
        }
        if (PEEK < 'a' || PEEK > 'z') {
          goto fail;
        }
        char out = z_chars[PEEK - 'a'];
        if (out == '\0') {
          goto fail;
        }
        PUSH(out);
        ADVANCE;
        continue;
      case 'Z':
        ADVANCE;
        if (isdigit(PEEK)) {
          uint32_t arity = 0;
          do {
            arity *= 10;
            arity += PEEK - '0';
            ADVANCE;
          } while (isdigit(PEEK));
          switch (PEEK) {
            case 'T':
              ADVANCE;
              switch (arity) {
                case 0:
                  RESERVE(2);
                  buf.data[buf.length++] = '(';
                  buf.data[buf.length++] = ')';
                  continue;
                case 1:
                  goto fail;
                default:
                  // Two for "()", and one per comma
                  RESERVE(arity + 1);
                  buf.data[buf.length++] = '(';
                  for (size_t i = 1; i < arity; i++) {
                    buf.data[buf.length++] = ',';
                  }
                  buf.data[buf.length++] = ')';
                  continue;
              }
              continue;
            case 'H':
              ADVANCE;
              switch (arity) {
                case 0:
                  goto fail;
                case 1:
                  PUSH_STR("(# #)");
                  continue;
                default:
                  // Four for "(##)", and one per comma
                  RESERVE(arity + 3);
                  buf.data[buf.length++] = '(';
                  buf.data[buf.length++] = '#';
                  for (size_t i = 1; i < arity; i++) {
                    buf.data[buf.length++] = ',';
                  }
                  buf.data[buf.length++] = '#';
                  buf.data[buf.length++] = ')';
                  continue;
              }
              continue;
            default:
              goto fail;
          }
        } else {
          if (PEEK < 'C' || PEEK > 'Z') {
            goto fail;
          }
          char out = Z_chars[PEEK - 'C'];
          if (out == '\0') {
            goto fail;
          }
          PUSH(out);
          ADVANCE;
          continue;
        }
        continue;
      case '\0':
        PUSH('\0');
        char *res = realloc(buf.data, buf.length);
        if (res == NULL) {
          goto fail;
        }
        return res;
      default:
        PUSH(PEEK);
        ADVANCE;
        continue;
    }
  }

fail:
  free(buf.data);
  return NULL;
}
