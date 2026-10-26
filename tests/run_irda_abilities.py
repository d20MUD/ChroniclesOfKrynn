#!/usr/bin/env python3
"""Compile production Irda commands with deterministic combat boundaries."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='krynn-irda-') as directory:
    executable = Path(directory) / 'irda-abilities'
    # Compile the production size calculation without unrelated utils.c dependencies.
    source = (root/'src/utils.c').read_text()
    size_function = source.split('int compute_current_size(', 1)[1].split('/* Take a room', 1)[0]
    size_source = Path(directory) / 'size.c'
    size_source.write_text('\n'.join([
        '#include "conf.h"', '#include "sysdep.h"', '#include "structs.h"',
        '#include "utils.h"', '#include "handler.h"', '#include "spells.h"',
        'int compute_current_size(' + size_function,
    ]))
    # Exercise the actual tagged player-file readers, writers and legacy backfill.
    players = (root/'src/players.c').read_text()
    load = players.split('        if (!strcmp(tag, "AiAu"))', 1)[1].split(
        '        if (!strcmp(tag, "AuDm")', 1)[0]
    load = '        if (!strcmp(tag, "AiAu"))' + load
    save = players.split('  if (ch->player_specials->saved.irda_augment_ready)', 1)[1].split(
        '  if (ch->player_specials->saved.aurak_dominate_ready)', 1)[0]
    save = '  if (ch->player_specials->saved.irda_augment_ready)' + save
    backfill = players.split('  /* Backfill all Irda racial feats', 1)[1].split(
        '  reconcile_alter_self(ch);', 1)[0]
    backfill = '  /* Backfill all Irda racial feats' + backfill
    codec = Path(directory) / 'irda-codec.c'
    codec.write_text('\n'.join([
        '#include "conf.h"', '#include "sysdep.h"', '#include "structs.h"', '#include "utils.h"',
        'void irda_test_load(struct char_data *ch, const char *tag, const char *line) {', load, '}',
        '#define BUFFER_WRITE(...) used += snprintf(output+used, size-used, __VA_ARGS__)',
        'void irda_test_write(struct char_data *ch, char *output, size_t size) {',
        'size_t used = 0; output[0] = 0;', save, '}',
        'void irda_test_backfill(struct char_data *ch) {', backfill, '}',
    ]))
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                    '-I'+str(root/'src'), str(root/'src/race.c'), str(root/'tests/irda_abilities.c'), str(size_source), str(codec),
                    '-Wl,--gc-sections', '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
