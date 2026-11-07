#!/usr/bin/env python3
"""Back up, promote, and restore zone files for the staff zpromote command."""
import argparse
from datetime import datetime, timezone
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import sys
import tempfile

KINDS = ('wld', 'mob', 'obj', 'zon', 'trg', 'shp', 'qst', 'hlq')
MAX_FILE = 32 * 1024 * 1024


class PromotionError(Exception):
    pass


def read_file(path):
    """A missing optional record is distinct from an empty or unreadable file."""
    try:
        fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    except FileNotFoundError:
        return None
    try:
        before = os.fstat(fd)
        if not stat.S_ISREG(before.st_mode) or before.st_size > MAX_FILE:
            raise PromotionError(f'Not a regular world file, or over 32 MiB: {path}')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            data = stream.read(MAX_FILE + 1)
        after = os.fstat(fd)
        if len(data) > MAX_FILE or (before.st_size, before.st_mtime_ns, before.st_ctime_ns) != (
                after.st_size, after.st_mtime_ns, after.st_ctime_ns):
            raise PromotionError(f'File changed while being read: {path}')
        return data, stat.S_IMODE(before.st_mode)
    finally:
        os.close(fd)


def sync_dir(path):
    fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def stage_file(path, data, mode):
    fd, name = tempfile.mkstemp(prefix=f'.{path.name}.promote-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fchmod(stream.fileno(), mode & 0o777)
            os.fsync(stream.fileno())
        return Path(name)
    except BaseException:
        Path(name).unlink(missing_ok=True)
        raise


def atomic_write(path, data, mode=0o600):
    temporary = stage_file(path, data, mode)
    try:
        os.replace(temporary, path)
        sync_dir(path.parent)
    finally:
        temporary.unlink(missing_ok=True)


def backup_file(path, record):
    # Backups are never overwritten, even if two commands share a timestamp.
    data, mode = record
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'wb') as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())


def index_update(record, zone, kind, present, label='Live'):
    if record is None:
        raise PromotionError(f'{label} {kind}/index is missing; repair it before promotion.')
    try:
        lines = record[0].decode('ascii').splitlines()
    except UnicodeError as error:
        raise PromotionError(f'Invalid {kind}/index: {error}') from error
    entries = [line.strip() for line in lines if line.strip()]
    if not entries or entries[-1] != '$' or '$' in entries[:-1]:
        raise PromotionError(f'Invalid {kind}/index terminator.')
    if any(not re.fullmatch(r'\d+\.' + kind, entry) for entry in entries[:-1]):
        raise PromotionError(f'Unexpected entry in {kind}/index.')
    name = f'{zone}.{kind}'
    # Preserve other entries/order; remove duplicate entries for this zone.
    new = [entry for entry in entries[:-1] if entry != name]
    if present:
        insert = next((i for i, entry in enumerate(new) if int(entry.split('.')[0]) > zone), len(new))
        new.insert(insert, name)
    new.append('$')
    # Leave byte-for-byte formatting intact when membership is already correct.
    if entries.count(name) == int(present):
        return record[0]
    return ('\n'.join(new) + '\n').encode('ascii')


def describe(record):
    return None if record is None else dict(size=len(record[0]), mode=record[1],
                                           sha256=hashlib.sha256(record[0]).hexdigest())


def plan_zone(dev, live, zone):
    if not 0 <= zone <= 2147483647:
        raise PromotionError('Zone must be a nonnegative integer no greater than 2147483647.')
    source, previous, desired = {}, {}, {}
    for kind in KINDS:
        for root in (dev, live):
            folder = root / kind
            if folder.is_symlink() or not folder.is_dir():
                raise PromotionError(f'Missing or symlinked world directory: {folder}')
        relative = Path(kind) / f'{zone}.{kind}'
        source[relative] = read_file(dev / relative)
        previous[relative] = read_file(live / relative)
        desired[relative] = (source[relative][0], previous[relative][1] if previous[relative] is not None
                             else source[relative][1]) if source[relative] is not None else None
        if source[relative] is not None:
            data = source[relative][0]
            if not data.rstrip().endswith((b'$', b'$~')):
                raise PromotionError(f'Missing world-file terminator: {dev / relative}')
        index = Path(kind) / 'index'
        previous[index] = read_file(live / index)
        # Validate saved source membership as well as target indexes.
        source[index] = read_file(dev / index)
        source_updated = index_update(source[index], zone, kind, source[relative] is not None, label='Dev')
        if source_updated != source[index][0]:
            raise PromotionError(f'Dev {kind}/index disagrees with {relative}; save/repair it first.')
        updated = index_update(previous[index], zone, kind, source[relative] is not None)
        desired[index] = updated, previous[index][1]
    for kind in ('zon', 'wld'):
        if source[Path(kind) / f'{zone}.{kind}'] is None:
            raise PromotionError(f'Dev zone requires both {zone}.zon and {zone}.wld.')
    zone_data = source[Path('zon') / f'{zone}.zon'][0]
    if not re.match(rb'\s*#' + str(zone).encode() + rb'\s*(?:\r?\n)', zone_data):
        raise PromotionError('Dev zone header does not match the requested zone number.')
    return source, previous, desired


def promote(dev, live, backups, zone, actor, dry_run=False, restore_id=None):
    dev, live, backups = (Path(p).resolve() for p in (dev, live, backups))
    if dev == live or dev in live.parents or live in dev.parents:
        raise PromotionError('Dev and live must be separate world directory trees.')
    if any(backups == p or p in backups.parents or backups in p.parents for p in (dev, live)):
        raise PromotionError('Backups must have a dedicated tree outside both world directories.')
    if not dev.is_dir() or not live.is_dir():
        raise PromotionError('Both dev and live world directories must already exist.')
    if not 0 <= zone <= 2147483647:
        raise PromotionError('Invalid zone number.')
    def plan():
        return plan_restore(live, backups, zone, restore_id) if restore_id is not None else plan_zone(dev, live, zone)
    # Preview has no filesystem side effects, including no lock/backup creation.
    if dry_run:
        _, previous, desired = plan()
        changes = [str(p) for p in desired if desired[p] != previous[p]]
        return dict(status='preview', changes=changes, backup_root=str(backups),
                    actions={str(p): 'remove' if desired[p] is None else 'replace' if previous[p] is not None
                             else 'create' for p in desired if str(p) in changes})
    # Serialize all promotions, since indexes are shared between zones.
    lock_path = live.parent / '.zone-promotion.lock'
    lock_fd = os.open(lock_path, os.O_WRONLY | os.O_CREAT | os.O_NOFOLLOW, 0o600)
    try:
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise PromotionError('Another zone promotion is already running.') from error
        if restore_id is not None:
            source, previous, desired = plan()
            return apply_plan(dev, live, backups, zone, actor, source, previous, desired,
                              source_root=backups / str(zone) / restore_id,
                              operation='restore', restore_from=restore_id)
        return promote_locked(dev, live, backups, zone, actor)
    finally:
        os.close(lock_fd)


def promote_locked(dev, live, backups, zone, actor):
    source, previous, desired = plan_zone(dev, live, zone)
    return apply_plan(dev, live, backups, zone, actor, source, previous, desired)


def apply_plan(dev, live, backups, zone, actor, source, previous, desired,
               source_root=None, operation='promote', restore_from=None):
    source_root = dev if source_root is None else source_root
    changes = [p for p in desired if desired[p] != previous[p]]
    if not changes and operation == 'promote':
        return dict(status='unchanged', changes=[])
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    zone_backups = backups / str(zone)
    zone_backups.mkdir(parents=True, exist_ok=True)
    sync_dir(backups.parent)
    sync_dir(backups)
    backup = Path(tempfile.mkdtemp(prefix=stamp + '-', dir=zone_backups))
    sync_dir(zone_backups)
    manifest = dict(zone=zone, actor=actor, timestamp_utc=stamp, dev_world=str(dev),
                    live_world=str(live), status='backing-up', files={},
                    operation=operation, restore_from=restore_from)
    receipt = backup / f'manifest.{stamp}.json'
    def journal(status):
        manifest['status'] = status
        atomic_write(receipt, (json.dumps(manifest, indent=2) + '\n').encode())
    journal('backing-up')
    try:
        for relative, record in previous.items():
            folder = backup / relative.parent
            folder.mkdir(exist_ok=True)
            destination = folder / f'{relative.name}.{stamp}.bak'
            if record is not None:
                backup_file(destination, record)
            manifest['files'][str(relative)] = dict(
                before=describe(record), after=describe(desired[relative]),
                backup=str(destination.relative_to(backup)) if record is not None else None)
            sync_dir(folder)
        # All eight existing live zone files AND all indexes are backed up before
        # the first live pathname is replaced or deleted.
        journal('backed-up')
        sync_dir(backup)
    except Exception as error:
        raise PromotionError(f'Backup failed; no live files replaced. Incomplete backup: {backup}: {error}') from error
    staged, applied = {}, []
    try:
        for root, snapshot in ((source_root, source), (live, previous)):
            for relative, record in snapshot.items():
                if read_file(root / relative) != record:
                    raise PromotionError(f'{root / relative} changed during backup; retry after saving.')
        for relative in changes:
            record = desired[relative]
            if record is not None:
                staged[relative] = stage_file(live / relative, record[0], record[1])
        journal('copying')
        for relative in changes:
            if read_file(live / relative) != previous[relative]:
                raise PromotionError(f'{relative} changed during staging.')
            # Record even an ambiguous failure (e.g. fsync after replacement).
            applied.append(relative)
            if desired[relative] is None:
                (live / relative).unlink()
            else:
                os.replace(staged[relative], live / relative)
            sync_dir((live / relative).parent)
        journal('complete')
    except Exception as error:
        failures = []
        for relative in reversed(applied):
            try:
                if previous[relative] is None:
                    (live / relative).unlink(missing_ok=True)
                    sync_dir((live / relative).parent)
                else:
                    atomic_write(live / relative, *previous[relative])
            except Exception as rollback_error:
                failures.append(f'{relative}: {rollback_error}')
        try:
            journal('rollback-incomplete' if failures else 'rolled-back')
        except Exception as journal_error:
            failures.append(f'journal: {journal_error}')
        detail = '; rollback INCOMPLETE: ' + '; '.join(failures) if failures else '; live replacements rolled back'
        raise PromotionError(f"{'Restore' if operation == 'restore' else 'Promotion'} failed: {error}{detail}. Backup: {backup}") from error
    finally:
        for temporary in staged.values():
            temporary.unlink(missing_ok=True)
    return dict(status='complete', changes=[str(p) for p in changes], backup=str(backup))



def selected_backup(backups, zone, backup_id):
    if not isinstance(backup_id, str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]{0,127}', backup_id) or '..' in backup_id:
        raise PromotionError('Choose a backup ID from zpromote <zone> backups; paths are not accepted.')
    parent = Path(backups) / str(zone)
    folder = parent / backup_id
    if parent.is_symlink() or folder.is_symlink() or not folder.is_dir():
        raise PromotionError('Backup directory is missing or symlinked.')
    receipts = list(folder.glob('manifest.*.json'))
    if len(receipts) != 1:
        raise PromotionError('Backup must contain exactly one manifest.')
    receipt = read_file(receipts[0])
    try:
        manifest = json.loads(receipt[0])
    except (ValueError, TypeError) as error:
        raise PromotionError('Invalid backup manifest.') from error
    if not isinstance(manifest, dict) or manifest.get('zone') != zone:
        raise PromotionError('Backup manifest belongs to a different zone.')
    return folder, receipts[0], receipt, manifest


def plan_restore(live, backups, zone, backup_id):
    folder, receipt_path, receipt, manifest = selected_backup(backups, zone, backup_id)
    if manifest.get('status') not in ('backed-up', 'copying', 'complete', 'rolled-back', 'rollback-incomplete'):
        raise PromotionError('Backup did not finish; choose a complete snapshot.')
    entries = manifest.get('files')
    expected = {str(Path(kind) / name) for kind in KINDS for name in (f'{zone}.{kind}', 'index')}
    if not isinstance(entries, dict) or set(entries) != expected:
        raise PromotionError('Backup manifest does not contain all zone files and indexes.')
    source = {receipt_path.relative_to(folder): receipt}
    previous, desired = {}, {}
    saved_indexes = {}
    for name, entry in entries.items():
        relative = Path(name)
        kind = relative.parts[0]
        if not isinstance(entry, dict) or 'before' not in entry or 'backup' not in entry:
            raise PromotionError(f'Invalid backup entry: {name}')
        before = entry['before']
        if before is None:
            if entry['backup'] is not None or relative.name == 'index':
                raise PromotionError(f'Invalid absent-file entry: {name}')
            record = None
        else:
            if not isinstance(before, dict) or not isinstance(before.get('mode'), int) or not 0 <= before['mode'] <= 0o7777:
                raise PromotionError(f'Invalid backup metadata: {name}')
            # Only the generated, same-kind filename is allowed, never a path
            # supplied by the manifest that can escape the selected snapshot.
            backup_name = entry['backup']
            if not isinstance(backup_name, str):
                raise PromotionError(f'Missing backup file path: {name}')
            path = Path(backup_name)
            if path.parent != Path(kind) or not path.name.startswith(relative.name + '.') or not path.name.endswith('.bak'):
                raise PromotionError(f'Invalid backup file path: {name}')
            if (folder / kind).is_symlink() or not (folder / kind).is_dir():
                raise PromotionError(f'Invalid backup type directory: {kind}')
            raw = read_file(folder / path)
            if raw is None or len(raw[0]) != before.get('size') or hashlib.sha256(raw[0]).hexdigest() != before.get('sha256'):
                raise PromotionError(f'Backup file missing or checksum mismatch: {name}')
            source[path] = raw
            record = raw[0], before['mode'] & 0o777
        if (live / kind).is_symlink() or not (live / kind).is_dir():
            raise PromotionError(f'Invalid live world directory: {kind}')
        previous[relative] = read_file(live / relative)
        if relative.name == 'index':
            saved_indexes[kind] = record
        else:
            desired[relative] = record
    for kind, record in saved_indexes.items():
        # Validate the saved index, then recover only this zone's membership.
        index_update(record, zone, kind, False, label='Backup')
        present = f'{zone}.{kind}' in record[0].decode('ascii').split()
        relative = Path(kind) / 'index'
        updated = index_update(previous[relative], zone, kind, present)
        desired[relative] = updated, previous[relative][1]
    return source, previous, desired


def list_backups(backups, zone):
    parent = Path(backups) / str(zone)
    if parent.is_symlink():
        raise PromotionError('Symlinked zone backup directory refused.')
    result = []
    if not parent.exists():
        return result
    for folder in sorted(parent.iterdir(), reverse=True):
        if folder.is_symlink() or not folder.is_dir():
            continue
        try:
            _, _, _, manifest = selected_backup(backups, zone, folder.name)
            result.append(dict(id=folder.name, status=manifest.get('status', 'unknown'),
                               operation=manifest.get('operation', 'promote'), actor=manifest.get('actor', 'unknown')))
        except (OSError, PromotionError):
            continue
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zone', required=True, type=int)
    parser.add_argument('--actor', required=True)
    parser.add_argument('--from-world', required=True)
    parser.add_argument('--check', action='store_true')
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--restore', metavar='BACKUP_ID')
    selection.add_argument('--list-backups', action='store_true')
    args = parser.parse_args()
    dev = Path(os.environ.get('KRYNN_DEV_WORLD', Path.home() / 'dev/mud/lib/world')).resolve()
    live = Path(os.environ.get('KRYNN_LIVE_WORLD', Path.home() / 'live/mud/lib/world')).resolve()
    backups = Path(os.environ.get('KRYNN_ZONE_BACKUPS', live.parent / 'zone-backups')).resolve()
    try:
        if Path(args.from_world).resolve() not in (dev, live):
            raise PromotionError('zpromote must run from the configured dev or live world.')
        if not 0 <= args.zone <= 2147483647:
            raise PromotionError('Invalid zone number.')
        if args.list_backups:
            saved = list_backups(backups, args.zone)
            print(f'Zone {args.zone} backups (newest first):')
            for item in saved:
                print(f"  {item['id']} [{item['status']}; {item['operation']}; {item['actor']}]")
            if not saved:
                print('  No backups found.')
            return 0
        result = promote(dev, live, backups, args.zone, args.actor, args.check, args.restore)
        if result['status'] == 'preview':
            print(f"Zone {args.zone}: {len(result['changes'])} file changes planned.")
            for name in result['changes']:
                print(f"  {result['actions'][name]} {name}")
            print(f"Backup tree: {result['backup_root']}/{args.zone}/<UTC timestamp>/")
        elif result['status'] == 'unchanged':
            print(f'Zone {args.zone}: dev and live files already match; nothing copied.')
        else:
            print(f"Zone {args.zone} {'restored' if args.restore is not None else 'promoted'}: {len(result['changes'])} file changes.")
            print(f"Live backup: {result['backup']}")
            print('Live memory is unchanged. Reload/restart live before saving or editing that zone there.')
        return 0
    except (OSError, PromotionError) as error:
        print(f'zpromote: {error}')
        return 1


if __name__ == '__main__':
    sys.exit(main())
