#!/usr/bin/env python3
"""Verify pool session initialization against strict and non-strict servers."""
from clan_database import ROOT, database
import shlex
import subprocess

source = (ROOT / 'src/mysql.c').read_text()
start = source.index('static MYSQL *mysql_pool_connect(')
helper = source[start:source.index('\n}', start) + 2]
assert source.count('mysql_pool_connect(pc->conn)') == 4
with database() as (tmp, socket, sql, client):
    sql("SET GLOBAL sql_mode='NO_ENGINE_SUBSTITUTION'")
    harness = tmp / 'session.c'
    harness.write_text('''#include <mysql.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
struct pool_config { const char *host,*username,*password,*database; };
static struct pool_config config={"localhost","root","","clan_test"};
static struct pool_config *mysql_pool=&config;
''' + helper.replace('mysql_pool->database, 0, NULL, 0',
                        'mysql_pool->database, 0, socket_path, 0').replace(
                            'static MYSQL *mysql_pool_connect(',
                            'static const char *socket_path;\nstatic MYSQL *mysql_pool_connect(') + '''
static void verify(MYSQL *db) {
 MYSQL_RES *r; MYSQL_ROW row;
 assert(!mysql_query(db,"SELECT @@SESSION.sql_mode,@@character_set_connection"));
 r=mysql_store_result(db); assert(r); row=mysql_fetch_row(r); assert(row);
 assert(strstr(row[0],"STRICT_TRANS_TABLES"));
 assert(strstr(row[0],"NO_ENGINE_SUBSTITUTION"));
 assert(!strcmp(row[1],"utf8mb4")); mysql_free_result(r);
 assert(mysql_query(db,"CREATE TEMPORARY TABLE session_probe(n INT NOT NULL)")==0);
 assert(mysql_query(db,"INSERT INTO session_probe VALUES(NULL)")!=0);
}
int main(int argc,char **argv) {
 MYSQL *db; assert(argc==2); socket_path=argv[1];
 db=mysql_init(NULL); assert(db); assert(mysql_pool_connect(db)); verify(db);
 mysql_close(db);
 return 0;
}
''')
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'mariadb'], text=True))
    binary = tmp / 'session'
    subprocess.run(['cc', '-Wall', '-Wextra', '-Werror', str(harness),
                    *flags, '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(socket)], check=True)
    sql("SET GLOBAL sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'")
    subprocess.run([str(binary), str(socket)], check=True)
print('Pool sessions preserve SQL modes, enable strict writes and use utf8mb4')
