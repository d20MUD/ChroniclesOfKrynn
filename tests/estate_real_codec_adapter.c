/* Minimal world adapter for the real objsave.c record writer/parser tests. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
struct index_data *obj_index;
struct obj_data *obj_proto;
obj_rnum top_of_objt;
obj_rnum real_object(obj_vnum v)
{
#ifdef LEGENDARY_PERSISTENCE_TEST
  obj_rnum i;
  for(i=0;obj_index && i<=top_of_objt;i++) if(obj_index[i].vnum==v)return i;
#else
  (void)v;
#endif
  return NOTHING;
}
struct obj_data *read_object(obj_vnum v, int type)
{
  (void)v;
  (void)type;
#ifdef LEGENDARY_PERSISTENCE_TEST
  struct obj_data *obj=create_obj();
  obj->item_number=real_object(v);
  return obj;
#else
  return create_obj();
#endif
}
void strip_cr(char *s)
{
  char *out = s;
  while (*s)
  {
    if (*s != '\r')
      *out++ = *s;
    s++;
  }
  *out = 0;
}
int get_line(FILE *fp, char *out)
{
  while (fgets(out, READ_SIZE, fp))
  {
    if (*out == '*' || *out == '\r' || *out == '\n')
      continue;
    out[strcspn(out, "\r\n")] = 0;
    return 1;
  }
  return 0;
}
void tag_argument(char *s, char *tag)
{
  char *value = s + 4;
  memcpy(tag, s, 4);
  tag[4] = 0;
  while (*value == ':' || *value == ' ')
    value++;
  memmove(s, value, strlen(value) + 1);
}
bitvector_t asciiflag_conv(const char *s) { return atol(s); }
char *fread_string(FILE *fp, const char *error)
{
  char *s = NULL;
  size_t n = 0;
  FILE *out = open_memstream(&s, &n);
  int c;
  (void)error;
  while ((c = fgetc(fp)) != EOF && c != '~')
    fputc(c, out);
  fclose(out);
  return s;
}
#ifndef LEGENDARY_PERSISTENCE_TEST
int objsave_save_obj_record_db_sheath(struct obj_data *obj, struct char_data *ch, long id, int slot)
{
  (void)obj;
  (void)ch;
  (void)id;
  (void)slot;
  abort();
}
#endif
#ifndef LEGENDARY_PERSISTENCE_TEST
/* New-format records must take the shared file-parser path in this test. */
char **tokenize(const char *input, const char *delim)
{
  (void)input;
  (void)delim;
  abort();
}
void free_tokens(char **tokens)
{
  (void)tokens;
  abort();
}

#endif
