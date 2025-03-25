/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf rtable.gperf  */
/* Computed positions: -k'' */


#define TOTAL_KEYWORDS 2
#define MIN_WORD_LENGTH 20
#define MAX_WORD_LENGTH 27
#define MIN_HASH_VALUE 20
#define MAX_HASH_VALUE 27
/* maximum key range = 8, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
hash (register const char *str, register size_t len)
{
  return len;
}

const char *
in_word_set (register const char *str, register size_t len)
{
  static const char * wordlist[] =
    {
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "",
      "static/css/style.css",
      "", "", "", "", "", "",
      "static/javascript/script.js"
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key];

          if (*str == *s && !strcmp (str + 1, s + 1))
            return s;
        }
    }
  return 0;
}
