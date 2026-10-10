#!/usr/bin/env python3
"""Run the estate services against disposable MariaDB, never the game's database."""
from pathlib import Path
import os
import resource
import shlex
import re
import subprocess
import tempfile
import time

root = Path(__file__).resolve().parents[1]
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
with tempfile.TemporaryDirectory(prefix='krynn-estate-test-') as directory:
    temp = Path(directory)
    data = temp / 'data'
    data.mkdir()
    socket = temp / 'mysql.sock'
    log = temp / 'mysql.log'
    executable = temp / 'estate-test'
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'mariadb'], text=True))
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                    '-I'+str(root/'src'), str(root/'tests/estate_integration.c'),
                    *[str(root/'src'/name) for name in ['estate.c', 'housing.c', 'auction_house.c']],
                    '-Wl,--gc-sections', *flags, '-o', str(executable)], check=True)
    # Extract the production functions verbatim, avoiding unrelated rental code dependencies.
    source = (root/'src/objsave.c').read_text()
    masked = re.sub(r"""/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'""",
                    lambda match: ' '*len(match.group()), source, flags=re.S)
    def function(signature, text=source):
        masked_text = re.sub(r"""/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'""",
                             lambda match: ' '*len(match.group()), text, flags=re.S)
        match = re.search(re.escape(signature) + r'[^{;]*\n\{', text)
        assert match, signature
        start = match.start()
        brace = masked_text.index('{', start)
        level = 1
        end = brace + 1
        while level:
            level += (masked_text[end] == '{') - (masked_text[end] == '}')
            end += 1
        return text[start:end]
    room_functions = temp/'runtime-rooms.c'
    room_functions.write_text('\n'.join(['#include "conf.h"', '#include "sysdep.h"', '#include "structs.h"',
                                        '#include "utils.h"', '#include "db.h"', '#include "dg_scripts.h"',
                                        '#include "dg_event.h"', '#include "genwld.h"', 'int runtime_room_mutation;',
                                        function('void update_wait_events(', (root/'src/dg_handler.c').read_text()),
                                        function('room_rnum add_runtime_room(', (root/'src/genwld.c').read_text()),
                                        function('int delete_runtime_room(', (root/'src/genwld.c').read_text())]))
    room_executable = temp/'runtime-room-test'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-I'+str(root/'src'),
                    str(root/'tests/runtime_housing_rooms.c'), str(room_functions),
                    *flags, '-o', str(room_executable)], check=True)
    subprocess.run([str(room_executable)], check=True)
    codec = temp/'real-codec.c'
    codec.write_text('\n'.join(['#include "conf.h"', '#include "sysdep.h"', '#include "structs.h"',
                               '#include "utils.h"', '#include "db.h"', '#include "handler.h"',
                               '#include "genolc.h"', '#include "spells.h"', '#include "act.h"', '#include "mysql.h"', '#define OBJSAVE_DB 1',
                               'static int estate_file_only;', 'int estate_write_record(struct obj_data *, FILE *);',
                               'int objsave_save_obj_record_db_sheath(struct obj_data *, struct char_data *, long, int);',
                               function('int objsave_save_obj_record_db(struct obj_data *obj'),
                               function('static int restore_persistent_object('),
                               function('obj_save_data *objsave_parse_objects(FILE *fl)'),
                               function('obj_save_data *objsave_parse_objects_db(char *name'),
                               function('int estate_write_record(struct obj_data *obj')]))
    real_executable = temp/'estate-real-codec-test'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections', '-DESTATE_REAL_CODEC',
                    '-I'+str(root/'src'), str(root/'tests/estate_integration.c'),
                    str(root/'tests/estate_real_codec_adapter.c'), str(codec),
                    *[str(root/'src'/name) for name in ['estate.c', 'housing.c', 'auction_house.c']],
                    '-Wl,--gc-sections', *flags, '-o', str(real_executable)], check=True)
    subprocess.run(['mariadb-install-db', '--no-defaults', '--datadir='+str(data),
                    '--auth-root-authentication-method=normal', '--skip-test-db'],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    server = subprocess.Popen(['mariadbd', '--no-defaults', '--datadir='+str(data),
                               '--socket='+str(socket), '--pid-file='+str(temp/'mysql.pid'),
                               '--skip-networking', '--log-error='+str(log),
                               '--user='+os.environ.get('USER', 'krynn')],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        client = ['mariadb', '--no-defaults', '--socket='+str(socket), '-uroot']
        for attempt in range(100):
            result = subprocess.run([*client, '-e', 'SELECT 1'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if result.returncode == 0:
                break
            if server.poll() is not None:
                raise RuntimeError(log.read_text())
            time.sleep(.1)
        else:
            raise RuntimeError('Temporary MariaDB did not become ready')
        subprocess.run([*client, '-e', 'CREATE DATABASE estate_test CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci'], check=True)
        subprocess.run([str(executable), str(socket)], cwd=temp, check=True)
        subprocess.run([*client, '-e', 'DROP DATABASE estate_test; CREATE DATABASE estate_test CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci'], check=True)
        print('Testing the production object record codec:', flush=True)
        subprocess.run([str(real_executable), str(socket)], cwd=temp, check=True)
    finally:
        server.terminate()
        server.wait(timeout=15)
