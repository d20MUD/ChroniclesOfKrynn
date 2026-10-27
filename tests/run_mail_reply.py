#!/usr/bin/env python3
"""Test production mail reply authorization/editor setup using disposable MariaDB."""
from pathlib import Path
import os, shlex, subprocess, tempfile, time
root = Path(__file__).resolve().parents[1]
source = (root/'src/new_mail.c').read_text()
def function(signature):
    start = source.index(signature+'\n{')
    return source[start:source.index('\n}',start)+2]+'\n'
code = '\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','mysql.h','modify.h'])
code += r'''
#include <assert.h>
#include <limits.h>
MYSQL *conn;
static int editor_calls;
static char output[8192];
bool ensure_mysql_connection(MYSQL *c,const char *caller) { return !mysql_ping(c); }
char *mysql_escape_string_alloc(MYSQL *c,const char *s) {
  char *out=malloc(strlen(s)*2+1);mysql_real_escape_string(c,out,s,strlen(s));return out;
}
void send_editor_help(struct descriptor_data *d) {}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return s; }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap;va_start(ap,fmt);int n=vsnprintf(output,sizeof(output),fmt,ap);va_end(ap);return n;
}
void string_write(struct descriptor_data *d,char **txt,size_t len,long mailto,void *data) {
  assert(len==MAX_STRING_LENGTH && mailto==-999);editor_calls++;d->str=txt;
}
static void sql(const char *q) { if(mysql_query(conn,q)) {fprintf(stderr,"%s\n",mysql_error(conn));abort();} }
'''
code += function('static void mail_start_editor(struct char_data *ch, const char *receiver, const char *subject)')
code += function('static void perform_mail_reply(struct char_data *ch, const char *argument)')
code += r'''
int main(int argc,char **argv) {
  struct char_data ch={0};struct player_special_data saved={0};struct descriptor_data desc={0};
  assert(argc==2);conn=mysql_init(NULL);
  assert(mysql_real_connect(conn,NULL,"root",NULL,"mail_reply_test",0,argv[1],0));
  ch.player.name="Alice";ch.player_specials=&saved;ch.desc=&desc;desc.character=&ch;
  sql("CREATE TABLE player_data(name VARCHAR(50) PRIMARY KEY)");
  sql("CREATE TABLE player_mail(mail_id INT PRIMARY KEY,sender VARCHAR(50),receiver VARCHAR(50),subject VARCHAR(255),message TEXT)");
  sql("CREATE TABLE player_mail_deleted(mail_id INT,player_name VARCHAR(50))");
  sql("INSERT INTO player_data VALUES('Alice'),('Bob'),('Carol')");
  sql("INSERT INTO player_mail VALUES(1,'Bob','Alice','Meeting','body'),(2,'Carol','Bob','Secret','body'),(3,'Bob','All','Announcement','body'),(4,'Bob','Alice','Re: Meeting','body'),(5,'Gone','Alice','Old','body'),(6,'Alice','Bob','Sent','body'),(7,'Bob','Alice','Deleted','body'),(8,'Bob','O''Brien','Quote','body')");
  sql("INSERT INTO player_mail_deleted VALUES(7,'Alice')");
  const char *bad[]={"","0","-1","1junk","1 2","999999999999999999999999","2147483648","2","5","6","7","999"};
  for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++) {
    perform_mail_reply(&ch,bad[i]);assert(editor_calls==0);
  }
  perform_mail_reply(&ch,"1");assert(editor_calls==1 && STATE(&desc)==CON_NEWMAIL);
  assert(!strcmp(saved.new_mail_receiver,"Bob") && !strcmp(saved.new_mail_subject,"Re: Meeting"));
  assert(desc.str==&saved.new_mail_content && PLR_FLAGGED(&ch,PLR_MAILING));
  perform_mail_reply(&ch,"4");assert(editor_calls==2 && !strcmp(saved.new_mail_subject,"Re: Meeting"));
  perform_mail_reply(&ch,"3");assert(editor_calls==3 && !strcmp(saved.new_mail_receiver,"Bob"));
  assert(!strcmp(saved.new_mail_subject,"Re: Announcement"));
  ch.player.name="O'Brien";perform_mail_reply(&ch,"8");assert(editor_calls==4);
  assert(!strcmp(saved.new_mail_receiver,"Bob") && !strcmp(saved.new_mail_subject,"Re: Quote"));
  sql("DROP TABLE player_mail_deleted");perform_mail_reply(&ch,"8");assert(editor_calls==4);
  free(saved.new_mail_receiver);free(saved.new_mail_subject);free(saved.new_mail_content);mysql_close(conn);
  puts("Mail reply: ownership, deleted/missing senders, broadcasts, Re subjects, numeric IDs, escaped names, database failure and editor setup PASS");
}
'''
assert 'perform_mail_reply(ch, arg4);' in source
with tempfile.TemporaryDirectory(prefix='mail-reply-') as tmp:
    temp=Path(tmp);data=temp/'data';data.mkdir();socket=temp/'mysql.sock';log=temp/'mysql.log'
    path=temp/'check.c';path.write_text(code);binary=temp/'check'
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb'],text=True))
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-I'+str(root/'src'),str(path),*flags,'-o',str(binary)],check=True)
    subprocess.run(['mariadb-install-db','--no-defaults','--datadir='+str(data),'--auth-root-authentication-method=normal','--skip-test-db'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
    server=subprocess.Popen(['mariadbd','--no-defaults','--datadir='+str(data),'--socket='+str(socket),'--pid-file='+str(temp/'pid'),'--skip-networking','--log-error='+str(log),'--user='+os.environ.get('USER','krynn')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    try:
        client=['mariadb','--no-defaults','--socket='+str(socket),'-uroot']
        for _ in range(100):
            if subprocess.run([*client,'-e','SELECT 1'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0: break
            if server.poll() is not None: raise RuntimeError(log.read_text())
            time.sleep(.1)
        else: raise RuntimeError('Temporary MariaDB did not become ready')
        subprocess.run([*client,'-e','CREATE DATABASE mail_reply_test'],check=True)
        subprocess.run([str(binary),str(socket)],check=True)
    finally:
        server.terminate();server.wait(timeout=15)
