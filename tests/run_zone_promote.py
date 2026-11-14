#!/usr/bin/env python3
"""Exercise real promotion I/O and staff dispatch using disposable trees only."""
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / 'lib/misc/zone_promote.py'
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('zone_promote', HELPER)
promoter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(promoter)


class PromotionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='zone-promote-test-')
        self.base = Path(self.temp.name)
        self.build = self.base / 'build/lib/world'
        self.live = self.base / 'live/lib/world'
        self.backups = self.base / 'live/lib/zone-backups'
        for root in (self.build, self.live):
            for kind in promoter.KINDS:
                folder = root / kind
                folder.mkdir(parents=True)
                (folder / 'index').write_text(f'1.{kind}\n7.{kind}\n99.{kind}\n$\n')
                for zone in (1, 7, 99):
                    (folder / f'{zone}.{kind}').write_text(f'#{zone}\n{root.parent.parent.name} {kind}\n$~\n')
        self.before = self.snapshot(self.live)
        self.source_before = self.snapshot(self.build)

    def tearDown(self):
        self.temp.cleanup()

    @staticmethod
    def snapshot(root):
        return {str(p.relative_to(root)): (p.read_bytes(), p.stat().st_mode & 0o777)
                for p in root.rglob('*') if p.is_file()}

    def run_promotion(self, **kwargs):
        return promoter.promote(self.build, self.live, self.backups, 7, 'TestStaff', **kwargs)

    def receipt(self, result):
        folder = Path(result['backup'])
        return folder, json.loads(next(folder.glob('manifest.*.json')).read_text())

    def test_backup_precedes_every_live_replacement_and_is_complete(self):
        replace = promoter.os.replace
        def checked_replace(source, target):
            if self.live in Path(target).parents:
                receipts = list(self.backups.glob('7/*/manifest.*.json'))
                self.assertEqual(len(receipts), 1)
                receipt = json.loads(receipts[0].read_text())
                self.assertEqual(receipt['status'], 'copying')
                self.assertEqual(len(receipt['files']), 16)
                for name, record in receipt['files'].items():
                    backup = receipts[0].parent / record['backup']
                    self.assertEqual(backup.read_bytes(), self.before[name][0])
                    self.assertRegex(backup.name, r'\.\d{8}T\d{6}\.\d{6}Z\.bak$')
            return replace(source, target)
        with patch.object(promoter.os, 'replace', side_effect=checked_replace):
            result = self.run_promotion()
        folder, manifest = self.receipt(result)
        self.assertEqual(result['status'], 'complete')
        self.assertEqual(manifest['status'], 'complete')
        self.assertEqual(manifest['actor'], 'TestStaff')
        self.assertEqual(self.snapshot(self.build), self.source_before)
        after = self.snapshot(self.live)
        for kind in promoter.KINDS:
            self.assertEqual(after[f'{kind}/7.{kind}'][0], self.source_before[f'{kind}/7.{kind}'][0])
            for name in (f'{kind}/1.{kind}', f'{kind}/99.{kind}', f'{kind}/index'):
                self.assertEqual(after[name], self.before[name])
        self.assertEqual(len(result['changes']), 8)
        self.assertFalse(any('.promote-' in name for name in after))

    def test_preview_is_read_only(self):
        result = self.run_promotion(dry_run=True)
        self.assertEqual(result['status'], 'preview')
        self.assertEqual(self.snapshot(self.live), self.before)
        self.assertFalse(self.backups.exists())
        self.assertFalse((self.live.parent / '.zone-promotion.lock').exists())

    def test_new_zone_updates_indexes_and_records_absence(self):
        for kind in promoter.KINDS:
            (self.live / kind / f'7.{kind}').unlink()
            (self.live / kind / 'index').write_text(f'1.{kind}\n99.{kind}\n$\n')
        result = self.run_promotion()
        _, manifest = self.receipt(result)
        for kind in promoter.KINDS:
            self.assertIsNone(manifest['files'][f'{kind}/7.{kind}']['before'])
            self.assertIsNone(manifest['files'][f'{kind}/7.{kind}']['backup'])
            self.assertEqual((self.live / kind / 'index').read_text(), f'1.{kind}\n7.{kind}\n99.{kind}\n$\n')

    def test_missing_build_optional_files_are_backed_up_then_removed(self):
        for kind in ('shp', 'qst', 'hlq'):
            (self.build / kind / f'7.{kind}').unlink()
            (self.build / kind / 'index').write_text(f'1.{kind}\n99.{kind}\n$\n')
        result = self.run_promotion()
        folder, manifest = self.receipt(result)
        for kind in ('shp', 'qst', 'hlq'):
            name = f'{kind}/7.{kind}'
            self.assertFalse((self.live / name).exists())
            self.assertNotIn('7.' + kind, (self.live / kind / 'index').read_text())
            record = manifest['files'][name]
            self.assertEqual((folder / record['backup']).read_bytes(), self.before[name][0])
            self.assertIsNone(record['after'])

    def test_backup_failure_never_replaces_live(self):
        original = promoter.backup_file
        calls = 0
        def fail_later(*args):
            nonlocal calls
            calls += 1
            if calls == 4:
                raise OSError('simulated backup disk full')
            return original(*args)
        with patch.object(promoter, 'backup_file', side_effect=fail_later):
            with self.assertRaisesRegex(promoter.PromotionError, 'Backup failed; no live files replaced'):
                self.run_promotion()
        self.assertEqual(self.snapshot(self.live), self.before)

    def test_staging_failure_never_replaces_live(self):
        original = promoter.stage_file
        def fail_live(target, *args):
            if self.live in target.parents:
                raise OSError('simulated staging disk full')
            return original(target, *args)
        with patch.object(promoter, 'stage_file', side_effect=fail_live):
            with self.assertRaisesRegex(promoter.PromotionError, 'rolled back'):
                self.run_promotion()
        self.assertEqual(self.snapshot(self.live), self.before)

    def test_copy_failure_rolls_back_new_files_and_indexes(self):
        for kind in promoter.KINDS:
            (self.live / kind / f'7.{kind}').unlink()
            (self.live / kind / 'index').write_text(f'1.{kind}\n99.{kind}\n$\n')
        original_state = self.snapshot(self.live)
        original = promoter.os.replace
        count = 0
        def fail_once(source, target):
            nonlocal count
            if self.live in Path(target).parents:
                count += 1
                if count == 5:
                    raise OSError('simulated replacement failure')
            return original(source, target)
        with patch.object(promoter.os, 'replace', side_effect=fail_once):
            with self.assertRaisesRegex(promoter.PromotionError, 'rolled back'):
                self.run_promotion()
        self.assertEqual(self.snapshot(self.live), original_state)
        receipt = next(self.backups.glob('7/*/manifest.*.json'))
        self.assertEqual(json.loads(receipt.read_text())['status'], 'rolled-back')

    def test_fsync_failure_after_rename_rolls_back_originals(self):
        original = promoter.sync_dir
        failed = False
        def fail_once(path):
            nonlocal failed
            if path == self.live / 'mob' and not failed:
                failed = True
                raise OSError('simulated live fsync failure')
            return original(path)
        with patch.object(promoter, 'sync_dir', side_effect=fail_once):
            with self.assertRaisesRegex(promoter.PromotionError, 'rolled back'):
                self.run_promotion()
        self.assertEqual(self.snapshot(self.live), self.before)

    def test_rollback_failure_reports_backup_for_manual_recovery(self):
        original = promoter.os.replace
        count = 0
        def fail_forward_and_rollback(source, target):
            nonlocal count
            if self.live in Path(target).parents:
                count += 1
                if count >= 2:
                    raise OSError('filesystem unavailable')
            return original(source, target)
        with patch.object(promoter.os, 'replace', side_effect=fail_forward_and_rollback):
            with self.assertRaisesRegex(promoter.PromotionError, 'rollback INCOMPLETE.*Backup:'):
                self.run_promotion()
        receipt = next(self.backups.glob('7/*/manifest.*.json'))
        self.assertEqual(json.loads(receipt.read_text())['status'], 'rollback-incomplete')

    def test_simultaneous_promotions_are_rejected(self):
        with (self.live.parent / '.zone-promotion.lock').open('w') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            with self.assertRaisesRegex(promoter.PromotionError, 'already running'):
                self.run_promotion()
        self.assertEqual(self.snapshot(self.live), self.before)

    def test_live_changes_during_backup_are_not_overwritten(self):
        original = promoter.backup_file
        changed = self.live / 'wld/7.wld'
        def concurrent_edit(*args):
            original(*args)
            changed.write_text('#7\na concurrent edit\n$~\n')
        with patch.object(promoter, 'backup_file', side_effect=concurrent_edit):
            with self.assertRaisesRegex(promoter.PromotionError, 'changed during backup'):
                self.run_promotion()
        self.assertIn('concurrent edit', changed.read_text())
        for name, record in self.before.items():
            if name != 'wld/7.wld':
                self.assertEqual(self.snapshot(self.live)[name], record)

    def test_timestamps_do_not_overwrite_previous_backups(self):
        first = self.run_promotion()
        first_snapshot = self.snapshot(Path(first['backup']))
        (self.build / 'mob/7.mob').write_text('#7\nsecond revision\n$~\n')
        second = self.run_promotion()
        self.assertNotEqual(first['backup'], second['backup'])
        self.assertEqual(self.snapshot(Path(first['backup'])), first_snapshot)

    def test_existing_live_permissions_are_preserved_and_noop_is_noop(self):
        (self.live / 'mob/7.mob').chmod(0o640)
        self.run_promotion()
        self.assertEqual((self.live / 'mob/7.mob').stat().st_mode & 0o777, 0o640)
        backup_count = len(list(self.backups.glob('7/*')))
        self.assertEqual(self.run_promotion()['status'], 'unchanged')
        self.assertEqual(len(list(self.backups.glob('7/*'))), backup_count)

    def test_symlinked_source_or_destination_files_are_rejected(self):
        for root in (self.build, self.live):
            target = root / 'obj/7.obj'
            old = target.read_bytes()
            target.unlink()
            target.symlink_to(root / 'obj/99.obj')
            with self.assertRaises(OSError):
                self.run_promotion()
            target.unlink()
            target.write_bytes(old)
        self.assertEqual(self.snapshot(self.live), self.before)

    def test_invalid_zone_records_and_indexes_fail_before_backup(self):
        mutations = [('zon/7.zon', b'#8\nwrong zone\n$~\n'),
                     ('mob/7.mob', b'#7\ntruncated'),
                     ('obj/index', b'1.obj\n99.obj\n$\n'),
                     ('shp/index', b'7.shp\n')]
        for name, bad in mutations:
            with self.subTest(name=name):
                path = self.build / name
                original = path.read_bytes()
                path.write_bytes(bad)
                with self.assertRaises(promoter.PromotionError):
                    self.run_promotion()
                path.write_bytes(original)
                self.assertEqual(self.snapshot(self.live), self.before)
                self.assertFalse(self.backups.exists())
        (self.build / 'wld/7.wld').unlink()
        (self.build / 'wld/index').write_text('1.wld\n99.wld\n$\n')
        with self.assertRaisesRegex(promoter.PromotionError, 'both'):
            self.run_promotion()

    def test_cli_rejects_unknown_origin_and_previews_both_ports_without_writes(self):
        env = dict(os.environ, KRYNN_BUILD_WORLD=str(self.build), KRYNN_LIVE_WORLD=str(self.live),
                   KRYNN_ZONE_BACKUPS=str(self.backups))
        command = ['python3', str(HELPER), '--zone', '7', '--actor', 'Staff', '--from-world']
        result = subprocess.run(command + [str(self.base)], env=env, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('configured build or live world', result.stdout)
        for origin in (self.build, self.live):
            result = subprocess.run(command + [str(origin), '--check'], env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('8 file changes planned', result.stdout)
        self.assertEqual(self.snapshot(self.live), self.before)
        self.assertFalse(self.backups.exists())

    def test_cli_defaults_to_build_port_and_ignores_legacy_dev_override(self):
        home = self.base / 'default-home'
        build = home / 'build/mud/lib/world'
        live = home / 'live/mud/lib/world'
        shutil.copytree(self.build, build)
        shutil.copytree(self.live, live)
        env = dict(os.environ, HOME=str(home), KRYNN_DEV_WORLD=str(self.base / 'obsolete-dev'))
        for key in ('KRYNN_BUILD_WORLD', 'KRYNN_LIVE_WORLD', 'KRYNN_ZONE_BACKUPS'):
            env.pop(key, None)
        command = ['python3', str(HELPER), '--zone', '7', '--actor', 'Staff', '--from-world']
        for origin in (build, live):
            result = subprocess.run(command + [str(origin), '--check'], env=env,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('8 file changes planned', result.stdout)
        rejected = subprocess.run(command + [str(self.build), '--check'], env=env,
                                  capture_output=True, text=True)
        self.assertNotEqual(rejected.returncode, 0)
        self.assertEqual(self.snapshot(live), self.before)
        self.assertFalse((live.parent / 'zone-backups').exists())

    def test_paths_cannot_overlap(self):
        for build, live, backups in ((self.build, self.build, self.backups),
                                    (self.build, self.live, self.live / 'backups')):
            with self.assertRaises(promoter.PromotionError):
                promoter.promote(build, live, backups, 7, 'Staff')


    def test_restore_backs_up_current_live_before_replacing_and_can_be_undone(self):
        promoted = self.run_promotion()
        current = self.snapshot(self.live)
        original_id = Path(promoted['backup']).name
        replace = promoter.os.replace
        def checked_replace(source, target):
            if self.live in Path(target).parents:
                receipts = [p for p in self.backups.glob('7/*/manifest.*.json')
                            if p.parent.name != original_id]
                self.assertEqual(len(receipts), 1)
                manifest = json.loads(receipts[0].read_text())
                self.assertEqual(manifest['operation'], 'restore')
                self.assertEqual(manifest['restore_from'], original_id)
                self.assertEqual(manifest['status'], 'copying')
                for name, entry in manifest['files'].items():
                    self.assertEqual((receipts[0].parent / entry['backup']).read_bytes(), current[name][0])
            return replace(source, target)
        with patch.object(promoter.os, 'replace', side_effect=checked_replace):
            result = self.run_promotion(restore_id=original_id)
        self.assertEqual(self.snapshot(self.live), self.before)
        self.assertEqual(self.snapshot(self.build), self.source_before)
        # The pre-restore backup is itself a normal, restorable snapshot.
        self.run_promotion(restore_id=Path(result['backup']).name)
        self.assertEqual(self.snapshot(self.live), current)

    def test_restore_preserves_later_other_zone_index_entries_and_data(self):
        promoted = self.run_promotion()
        for kind in promoter.KINDS:
            (self.live / kind / f'55.{kind}').write_text('#55\nlater zone\n$~\n')
            (self.live / kind / 'index').write_text(f'1.{kind}\n7.{kind}\n55.{kind}\n99.{kind}\n$\n')
        self.run_promotion(restore_id=Path(promoted['backup']).name)
        for kind in promoter.KINDS:
            self.assertIn('55.' + kind, (self.live / kind / 'index').read_text())
            self.assertIn('later zone', (self.live / kind / f'55.{kind}').read_text())
            self.assertEqual((self.live / kind / f'7.{kind}').read_bytes(), self.before[f'{kind}/7.{kind}'][0])

    def test_restore_previously_absent_zone_removes_promoted_files_and_membership(self):
        for kind in promoter.KINDS:
            (self.live / kind / f'7.{kind}').unlink()
            (self.live / kind / 'index').write_text(f'1.{kind}\n99.{kind}\n$\n')
        absent_state = self.snapshot(self.live)
        promoted = self.run_promotion()
        restored = self.run_promotion(restore_id=Path(promoted['backup']).name)
        self.assertEqual(self.snapshot(self.live), absent_state)
        _, receipt = self.receipt(restored)
        self.assertIsNotNone(receipt['files']['wld/7.wld']['before'])

    def test_unchanged_restore_still_backs_up_live(self):
        promoted = self.run_promotion()
        self.run_promotion(restore_id=Path(promoted['backup']).name)
        count = len(list(self.backups.glob('7/*')))
        restored = self.run_promotion(restore_id=Path(promoted['backup']).name)
        self.assertEqual(restored['status'], 'complete')
        self.assertEqual(restored['changes'], [])
        self.assertEqual(len(list(self.backups.glob('7/*'))), count + 1)

    def test_restore_preview_does_not_create_backup_or_change_live(self):
        promoted = self.run_promotion()
        current = self.snapshot(self.live)
        count = len(list(self.backups.glob('7/*')))
        result = self.run_promotion(restore_id=Path(promoted['backup']).name, dry_run=True)
        self.assertEqual(result['status'], 'preview')
        self.assertEqual(len(result['changes']), 8)
        self.assertEqual(self.snapshot(self.live), current)
        self.assertEqual(len(list(self.backups.glob('7/*'))), count)

    def test_restore_backup_failure_leaves_current_live_untouched(self):
        promoted = self.run_promotion()
        current = self.snapshot(self.live)
        with patch.object(promoter, 'backup_file', side_effect=OSError('backup full')):
            with self.assertRaisesRegex(promoter.PromotionError, 'no live files replaced'):
                self.run_promotion(restore_id=Path(promoted['backup']).name)
        self.assertEqual(self.snapshot(self.live), current)

    def test_restore_replacement_failure_rolls_back_and_retains_current_snapshot(self):
        promoted = self.run_promotion()
        current = self.snapshot(self.live)
        original = promoter.os.replace
        count = 0
        def fail_once(source, target):
            nonlocal count
            if self.live in Path(target).parents:
                count += 1
                if count == 3:
                    raise OSError('restore failed')
            return original(source, target)
        with patch.object(promoter.os, 'replace', side_effect=fail_once):
            with self.assertRaisesRegex(promoter.PromotionError, 'rolled back'):
                self.run_promotion(restore_id=Path(promoted['backup']).name)
        self.assertEqual(self.snapshot(self.live), current)
        receipts = [json.loads(p.read_text()) for p in self.backups.glob('7/*/manifest.*.json')]
        self.assertTrue(any(m['operation'] == 'restore' and m['status'] == 'rolled-back' for m in receipts))

    def test_corrupt_or_escaping_restore_sources_are_rejected_before_backup(self):
        promoted = self.run_promotion()
        folder, manifest = self.receipt(promoted)
        receipt = next(folder.glob('manifest.*.json'))
        original = receipt.read_bytes()
        current = self.snapshot(self.live)
        count = len(list(self.backups.glob('7/*')))
        entry = manifest['files']['wld/7.wld']
        backup_file = folder / entry['backup']
        original_file = backup_file.read_bytes()
        backup_file.write_bytes(b'corrupt')
        with self.assertRaisesRegex(promoter.PromotionError, 'checksum'):
            self.run_promotion(restore_id=folder.name)
        backup_file.write_bytes(original_file)
        for mutation in ('zone', 'path', 'status', 'files'):
            bad = json.loads(original)
            if mutation == 'zone':bad['zone'] = 99
            if mutation == 'path':bad['files']['wld/7.wld']['backup'] = '../../outside'
            if mutation == 'status':bad['status'] = 'backing-up'
            if mutation == 'files':del bad['files']['mob/index']
            receipt.write_text(json.dumps(bad))
            with self.subTest(mutation=mutation), self.assertRaises(promoter.PromotionError):
                self.run_promotion(restore_id=folder.name)
        receipt.write_bytes(original)
        for backup_id in ('', '../escape', str(folder), '.', '..', 'a/../../b'):
            with self.subTest(backup_id=backup_id), self.assertRaises(promoter.PromotionError):
                self.run_promotion(restore_id=backup_id)
        self.assertEqual(self.snapshot(self.live), current)
        self.assertEqual(len(list(self.backups.glob('7/*'))), count)

    def test_restore_recovers_original_permissions(self):
        (self.live / 'mob/7.mob').chmod(0o640)
        promoted = self.run_promotion()
        (self.live / 'mob/7.mob').chmod(0o644)
        self.run_promotion(restore_id=Path(promoted['backup']).name)
        self.assertEqual((self.live / 'mob/7.mob').stat().st_mode & 0o777, 0o640)

    def test_restore_rejects_symlinked_snapshot_files(self):
        promoted = self.run_promotion()
        folder, manifest = self.receipt(promoted)
        path = folder / manifest['files']['obj/7.obj']['backup']
        path.unlink()
        path.symlink_to(self.build / 'obj/7.obj')
        current = self.snapshot(self.live)
        with self.assertRaises(OSError):
            self.run_promotion(restore_id=folder.name)
        self.assertEqual(self.snapshot(self.live), current)

    def test_backup_listing_and_restore_cli_preserve_backup_id_case(self):
        promoted = self.run_promotion()
        backup_id = Path(promoted['backup']).name
        self.assertEqual(promoter.list_backups(self.backups, 7)[0]['id'], backup_id)
        env = dict(os.environ, KRYNN_BUILD_WORLD=str(self.build), KRYNN_LIVE_WORLD=str(self.live),
                   KRYNN_ZONE_BACKUPS=str(self.backups))
        base = ['python3', str(HELPER), '--zone', '7', '--actor', 'Staff', '--from-world', str(self.build)]
        listed = subprocess.run(base + ['--list-backups'], env=env, capture_output=True, text=True)
        self.assertEqual(listed.returncode, 0, listed.stdout + listed.stderr)
        self.assertIn(backup_id, listed.stdout)
        result = subprocess.run(base + ['--restore', backup_id], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('restored', result.stdout)
        self.assertIn('Live backup:', result.stdout)
        self.assertEqual(self.snapshot(self.live), self.before)


class StaffCommandTests(unittest.TestCase):
    def test_staff_dispatch_permissions_parsing_and_child_status(self):
        source = (ROOT / 'src/act.wizard.c').read_text()
        start = source.index('ACMD(do_zpromote)\n{')
        command = source[start:source.index('\nACMD(do_zreset)', start)]
        harness = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <assert.h>
#define MAX_INPUT_LENGTH 1024
#define LVL_GRSTAFF 33
#define NOWHERE -1
#define SL_CFG 5
#define BRF 1
#define TRUE 1
#define MAX(a,b) ((a)>(b)?(a):(b))
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
#define IS_NPC(c) ((c)->npc)
#define GET_LEVEL(c) ((c)->level)
#define GET_INVIS_LEV(c) 0
#define GET_NAME(c) ((c)->name)
typedef int zone_vnum;
struct char_data { int npc, level; char *name; };
struct save_list_data { int zone, type; struct save_list_data *next; };
static struct save_list_data *save_list;
static char messages[8192];
static int logged, success;
static void send_to_char(struct char_data *ch, const char *fmt, ...) {
  va_list ap; va_start(ap,fmt);
  vsnprintf(messages+strlen(messages),sizeof(messages)-strlen(messages),fmt,ap); va_end(ap);
}
static void mudlog(int type,int level,int file,const char *fmt,...) {
  char text[1024]; va_list ap; va_start(ap,fmt); vsnprintf(text,sizeof(text),fmt,ap); va_end(ap);
  assert(type==BRF && level==LVL_GRSTAFF); logged++; success=strstr(text,"success")!=NULL;
}
static int real_zone(zone_vnum zone) { return zone==7?0:NOWHERE; }
static int str_cmp(const char *a,const char *b) { return strcasecmp(a,b); }
static const char *one_argument(const char *input,char *arg,size_t size) {
  size_t i=0; while(isspace((unsigned char)*input))input++;
  while(*input && !isspace((unsigned char)*input)) { if(i+1<size)arg[i++]=tolower((unsigned char)*input); input++; }
  arg[i]=0; return input;
}
static void reap(int sig) { while(waitpid(-1,NULL,WNOHANG)>0); }
"""
        checks = r"""
int main(void) {
  struct char_data staff={0,33,"Staff; literal argument"};
  struct save_list_data pending={7,0,NULL};
  sigset_t mask;
  const char *bad[]={"", "7;id", "../7", "-7", "7 go", "7 check extra", "7 restore", "7 restore ID wrong", "7 backups extra", "999999999999999999999999999"};
  size_t i;
  staff.level=32; do_zpromote(&staff,"7",0,0); assert(strstr(messages,"Only senior staff"));
  staff.level=33; staff.npc=1; messages[0]=0;
  do_zpromote(&staff,"7",0,0); assert(strstr(messages,"Only senior staff")); staff.npc=0;
  for(i=0;i<sizeof(bad)/sizeof(*bad);i++) {
    messages[0]=0; do_zpromote(&staff,bad[i],0,0);
    assert(strstr(messages,"Usage:") || strstr(messages,"existing build zone"));
  }
  assert(logged==0);
  save_list=&pending; messages[0]=0; do_zpromote(&staff,"7",0,0);
  assert(strstr(messages,"pending OLC") && logged==0); save_list=NULL;
  signal(SIGCHLD,reap);
  messages[0]=0; do_zpromote(&staff,"7 check",0,0);
  assert(strstr(messages,"CHECK OK") && !strstr(messages,"did not complete") && logged==1 && success);
  sigprocmask(SIG_SETMASK,NULL,&mask); assert(!sigismember(&mask,SIGCHLD));
  messages[0]=0; do_zpromote(&staff,"007",0,0);
  assert(strstr(messages,"COPY OK") && logged==2 && success);
  messages[0]=0; do_zpromote(&staff,"7 backups",0,0);
  assert(strstr(messages,"LIST OK") && logged==3 && success);
  messages[0]=0; do_zpromote(&staff,"7 restore 20261107T120000.123456Z-AbCd check",0,0);
  assert(strstr(messages,"RESTORE CHECK OK") && logged==4 && success);
  messages[0]=0; do_zpromote(&staff,"7 restore 20261107T120000.123456Z-AbCd",0,0);
  assert(strstr(messages,"RESTORE OK") && logged==5 && success);
  setenv("PROMOTE_TEST_FAIL","1",1); messages[0]=0; do_zpromote(&staff,"7",0,0);
  assert(strstr(messages,"did not complete") && logged==6 && !success);
  return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix='zone-promote-staff-') as temp:
            folder = Path(temp)
            (folder / 'misc').mkdir()
            (folder / 'misc/zone_promote.py').write_text(
                'import os,sys\n'
                'assert sys.argv[1:7] == ["--zone","7","--actor","Staff; literal argument","--from-world","world"]\n'
                'assert sys.argv[7:] in ([],["--check"],["--list-backups"],["--restore","20261107T120000.123456Z-AbCd"],["--restore","20261107T120000.123456Z-AbCd","--check"])\n'
                'print(("RESTORE CHECK OK" if "--check" in sys.argv else "RESTORE OK") if "--restore" in sys.argv else "LIST OK" if "--list-backups" in sys.argv else "CHECK OK" if "--check" in sys.argv else "COPY OK")\n'
                'sys.exit(1 if os.environ.get("PROMOTE_TEST_FAIL") else 0)\n')
            (folder / 'check.c').write_text(harness + command + checks)
            binary = folder / 'check'
            subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu90', '-Wall', '-Wextra',
                            '-Wno-unused-parameter', str(folder / 'check.c'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], cwd=folder, check=True)
        table = (ROOT / 'src/interpreter.c').read_text()
        self.assertRegex(table, r'"zpromote", "zpromote", POS_DEAD, do_zpromote, LVL_GRSTAFF')


if __name__ == '__main__':
    unittest.main(verbosity=2)
