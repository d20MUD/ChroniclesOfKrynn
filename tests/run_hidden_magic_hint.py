#!/usr/bin/env python3
"""Check detect-magic room hints without revealing or modifying hidden objects."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/act.informative.c').read_text()
a=source.index('static void show_hidden_magic_hint(struct char_data *ch)\n{')
code=source[a:source.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h'])
stubs=r'''
#include <assert.h>
struct room_data *world;
static int messages;
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  assert(strstr(fmt,"There is something magical hidden here.")); messages++; return 1;
}
'''
checks=r'''
int main(void) {
  struct room_data rooms[2]={0}; struct descriptor_data desc={0}; struct char_data ch={0};
  struct obj_data obj={0}, second={0}, container={0};
  world=rooms; ch.desc=&desc; IN_ROOM(&ch)=0;
  show_hidden_magic_hint(NULL); show_hidden_magic_hint(&ch); assert(messages==0);
  rooms[0].contents=&obj;
  GET_OBJ_TYPE(&obj)=ITEM_WEAPON;
  SET_BIT_AR(GET_OBJ_EXTRA(&obj),ITEM_HIDDEN);
  SET_BIT_AR(GET_OBJ_EXTRA(&obj),ITEM_MAGIC);
  show_hidden_magic_hint(&ch); assert(messages==0);
  SET_BIT_AR(AFF_FLAGS(&ch),AFF_DETECT_MAGIC);
  show_hidden_magic_hint(&ch); assert(messages==1);
  assert(OBJ_FLAGGED(&obj,ITEM_HIDDEN) && OBJ_FLAGGED(&obj,ITEM_MAGIC));
  /* Visible magical objects and hidden mundane objects do not produce hints. */
  REMOVE_BIT_AR(GET_OBJ_EXTRA(&obj),ITEM_HIDDEN);
  show_hidden_magic_hint(&ch); assert(messages==1);
  SET_BIT_AR(GET_OBJ_EXTRA(&obj),ITEM_HIDDEN); REMOVE_BIT_AR(GET_OBJ_EXTRA(&obj),ITEM_MAGIC);
  show_hidden_magic_hint(&ch); assert(messages==1);
  /* Hidden chests need not use ITEM_HIDDEN or ITEM_MAGIC. */
  memset(&obj,0,sizeof(obj)); GET_OBJ_TYPE(&obj)=ITEM_TREASURE_CHEST; GET_OBJ_VAL(&obj,3)=25;
  show_hidden_magic_hint(&ch); assert(messages==2 && GET_OBJ_VAL(&obj,3)==25);
  GET_OBJ_VAL(&obj,3)=0; show_hidden_magic_hint(&ch); assert(messages==2);
  SET_BIT_AR(GET_OBJ_EXTRA(&obj),ITEM_HIDDEN);
  show_hidden_magic_hint(&ch); assert(messages==3 && OBJ_FLAGGED(&obj,ITEM_HIDDEN));
  /* Multiple hidden sources produce one blurb, without exposing their names. */
  second=obj; obj.next_content=&second;
  show_hidden_magic_hint(&ch); assert(messages==4);
  /* Items inside containers or another room are not direct room discoveries. */
  rooms[0].contents=&container; GET_OBJ_TYPE(&container)=ITEM_CONTAINER; container.contains=&obj;
  show_hidden_magic_hint(&ch); assert(messages==4);
  rooms[0].contents=NULL; rooms[1].contents=&obj;
  show_hidden_magic_hint(&ch); assert(messages==4);
  IN_ROOM(&ch)=NOWHERE; show_hidden_magic_hint(&ch); assert(messages==4);
  IN_ROOM(&ch)=1; ch.desc=NULL; show_hidden_magic_hint(&ch); assert(messages==4);
  ch.desc=&desc; REMOVE_BIT_AR(AFF_FLAGS(&ch),AFF_DETECT_MAGIC);
  show_hidden_magic_hint(&ch); assert(messages==4);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hidden-magic-hint-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Detect-magic hints: hidden magic/chests, single-message behavior, unchanged concealment, and room/connection boundaries passed.')
