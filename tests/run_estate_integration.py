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

if os.environ.get('LEGENDARY_COMMANDS_TEST') == '1':
    os.environ['LEGENDARY_TRANSACTION_TEST'] = '1'
    os.environ['LEGENDARY_PERSISTENCE_TEST'] = '1'

root = Path(__file__).resolve().parents[1]
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
extra_flags = shlex.split(os.environ.get('ESTATE_TEST_CFLAGS', ''))
with tempfile.TemporaryDirectory(prefix='krynn-estate-test-') as directory:
    temp = Path(directory)
    data = temp / 'data'
    data.mkdir()
    socket = temp / 'mysql.sock'
    log = temp / 'mysql.log'
    executable = temp / 'estate-test'
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'mariadb'], text=True))
    subprocess.run([os.environ.get('CC', 'cc'), *extra_flags, '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                    '-I'+str(root/'src'), str(root/'tests/estate_integration.c'),
                    *[str(root/'src'/name) for name in ['estate.c', 'housing.c', 'auction_house.c', 'player_shop.c', 'durable_file.c', 'legendary_transaction.c', 'legendary_store.c', 'legendary.c', 'legendary_equipment.c', 'legendary_foundations.c', 'legendary_effects.c', 'legendary_techniques.c', 'legendary_identity.c']],
                    '-Wl,--gc-sections', *flags, '-lcrypto', '-o', str(executable)], check=True)
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
    subprocess.run([os.environ.get('CC', 'cc'), *extra_flags, '-std=gnu99', '-I'+str(root/'src'),
                    str(root/'tests/runtime_housing_rooms.c'), str(room_functions),
                    *flags, '-o', str(room_executable)], check=True)
    subprocess.run([str(room_executable)], check=True)
    codec = temp/'real-codec.c'
    codec.write_text('\n'.join(['#include "conf.h"', '#include "sysdep.h"', '#include "structs.h"',
                               '#include "utils.h"', '#include "db.h"', '#include "handler.h"', '#include "legendary_identity.h"',
                               '#include "genolc.h"', '#include "spells.h"', '#include "act.h"', '#include "mysql.h"', '#define OBJSAVE_DB 1',
                               'static int estate_file_only;', 'int estate_write_record(struct obj_data *, FILE *);',
                               'int objsave_save_obj_record_db_sheath(struct obj_data *, struct char_data *, long, int);',
                               function('int objsave_save_obj_record_db(struct obj_data *obj'),
                               function('static int restore_persistent_object('),
                               function('obj_save_data *objsave_parse_objects(FILE *fl)'),
                               function('obj_save_data *objsave_parse_objects_db(char *name'),
                               function('int estate_write_record(struct obj_data *obj')]))
    real_executable = temp/'estate-real-codec-test'
    subprocess.run([os.environ.get('CC', 'cc'), *extra_flags, '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections', '-DESTATE_REAL_CODEC',
                    '-I'+str(root/'src'), str(root/'tests/estate_integration.c'),
                    str(root/'tests/estate_real_codec_adapter.c'), str(codec),
                    *[str(root/'src'/name) for name in ['estate.c', 'housing.c', 'auction_house.c', 'player_shop.c', 'durable_file.c', 'legendary_transaction.c', 'legendary_store.c', 'legendary.c', 'legendary_equipment.c', 'legendary_foundations.c', 'legendary_effects.c', 'legendary_techniques.c', 'legendary_identity.c']],
                    '-Wl,--gc-sections', *flags, '-lcrypto', '-o', str(real_executable)], check=True)
    legendary_executable = None
    if os.environ.get('LEGENDARY_PERSISTENCE_TEST') == '1':
        legendary_codec = temp/'legendary-codec.c'
        legendary_codec.write_text(codec.read_text() + '\n' + '\n'.join([
            function('int objsave_save_obj_record_db_pet('),
            function('int objsave_save_obj_record_db_sheath('),
            function('obj_save_data *objsave_parse_objects_db_pet('),
            function('obj_save_data *objsave_parse_objects_db_sheath('),
            function('char **tokenize(', (root/'src/mysql.c').read_text()),
            function('void free_tokens(', (root/'src/mysql.c').read_text()),
            '#include "genobj.h"', '#include "traps.h"',
            function('void free_object_strings_proto(', (root/'src/genobj.c').read_text()),
            function('static void copy_object_strings(', (root/'src/genobj.c').read_text()),
            function('int copy_object_main(', (root/'src/genobj.c').read_text()),
            function('int copy_object(', (root/'src/genobj.c').read_text()),
            function('int copy_object_preserve(', (root/'src/genobj.c').read_text()),
            function('static int update_all_objects(', (root/'src/genobj.c').read_text()),
            'int test_update_all_objects(struct obj_data *obj) { return update_all_objects(obj); }',
            function('void free_trap(', (root/'src/traps_new.c').read_text()),
            function('struct trap_data *copy_trap(', (root/'src/traps_new.c').read_text()),
            function('char *str_udup(', (root/'src/genolc.c').read_text()),
            function('void copy_ex_descriptions(', (root/'src/genolc.c').read_text())]))
        legendary_executable = temp/'legendary-persistence-test'
        subprocess.run([os.environ.get('CC', 'cc'), *extra_flags, '-std=gnu99', '-Wall', '-Wextra',
                        '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                        '-DESTATE_REAL_CODEC', '-DLEGENDARY_PERSISTENCE_TEST', '-I'+str(root/'src'),
                        str(root/'tests/legendary_persistence.c'), str(root/'tests/estate_real_codec_adapter.c'),
                        str(legendary_codec), *[str(root/'src'/name) for name in
                        ['estate.c','housing.c','auction_house.c','player_shop.c','legendary.c', 'legendary_equipment.c', 'legendary_foundations.c', 'legendary_effects.c', 'legendary_techniques.c',
                         'legendary_store.c','legendary_identity.c','legendary_transaction.c','durable_file.c']],
                        '-Wl,--gc-sections', *flags, '-lcrypto', '-o', str(legendary_executable)], check=True)
    transaction_executable = None
    if os.environ.get('LEGENDARY_TRANSACTION_TEST') == '1' or os.environ.get('LEGENDARY_COMMANDS_TEST') == '1':
        transaction_executable = temp/'legendary-transaction-test'
        transaction_codec = temp/'transaction-codec.c'
        transaction_codec.write_text(legendary_codec.read_text() + '\n' + '\n'.join([
            '#include "estate.h"', '#include "durable_file.h"',
            function('int objsave_save_obj_record(struct obj_data *'),
            function('static int Crash_save(struct obj_data *', source[source.index('/* recursive function using linked lists'):]),
            function('static void Crash_restore_weight(struct obj_data *', source[source.index('static void Crash_restore_weight(struct obj_data *', source.index('// Like crash save')):]),
            function('static int objsave_write_rentcode(FILE *', source[source.index('static int objsave_write_rentcode(FILE *', source.index('void Crash_idlesave')):]),
            function('static int Crash_is_unrentable(struct obj_data *', source[source.index('static int Crash_is_unrentable(struct obj_data *', source.index('static void Crash_restore_weight(struct obj_data *', source.index('// Like crash save'))):]),
            function('static int Crash_save_rent(struct obj_data *'),
            function('static bool Crash_checkpoint(struct char_data *'),
            function('bool Crash_crashsave_checked(struct char_data *').replace('Crash_crashsave_checked(', 'test_real_crashsave_checked(', 1)]))

        subprocess.run([os.environ.get('CC', 'cc'), *extra_flags, '-std=gnu99', '-Wall', '-Wextra',
                        '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                        '-DESTATE_REAL_CODEC', '-DLEGENDARY_PERSISTENCE_TEST', '-DLEGENDARY_TRANSACTION_TEST',
                        '-I'+str(root/'src'), str(root/'tests/legendary_transactions.c'),
                        str(root/'tests/estate_real_codec_adapter.c'), str(transaction_codec),
                        *[str(root/'src'/name) for name in
                        ['estate.c','housing.c','auction_house.c','player_shop.c','legendary.c', 'legendary_equipment.c', 'legendary_foundations.c', 'legendary_effects.c', 'legendary_techniques.c',
                         'legendary_store.c','legendary_identity.c','legendary_transaction.c','durable_file.c']],
                        '-Wl,--gc-sections', *flags, '-lcrypto', '-o', str(transaction_executable)], check=True)
    earnings_executable = None
    if os.environ.get('LEGENDARY_EARNINGS_TEST') == '1':
        earnings_executable = temp/'legendary-earnings-test'
        limits_source = (root/'src/limits.c').read_text()
        xp_codec = temp/'earnings-xp.c'
        xp_codec.write_text('\n'.join(re.findall(r'^#include .+$',limits_source,re.M)) + '\n' +
            '\n'.join(re.findall(r'^#define (?:NEWBIE_EXP|MIN_NUM_MOBS_TO_KILL_\d+) .+$',limits_source,re.M)) + '\n' +
            function('int gain_exp(struct char_data *',limits_source) + '\n' +
            function('int gain_exp_with_source(struct char_data *',limits_source) + '\n' +
            function('bool process_healing(struct char_data *', (root/'src/magic.c').read_text()))

        subprocess.run([os.environ.get('CC', 'cc'), *extra_flags, '-std=gnu99', '-Wall', '-Wextra',
                        '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                        '-DESTATE_REAL_CODEC', '-DLEGENDARY_PERSISTENCE_TEST', '-DLEGENDARY_TRANSACTION_TEST',
                        '-DLEGENDARY_EARNINGS_TEST', '-I'+str(root/'src'), str(root/'tests/legendary_earnings.c'), str(xp_codec),
                        str(root/'tests/estate_real_codec_adapter.c'), str(transaction_codec),
                        *[str(root/'src'/name) for name in
                        ['estate.c','housing.c','auction_house.c','player_shop.c','legendary.c', 'legendary_equipment.c', 'legendary_foundations.c', 'legendary_effects.c', 'legendary_techniques.c',
                         'legendary_store.c','legendary_identity.c','legendary_transaction.c','durable_file.c','legendary_earnings.c']],
                        '-Wl,--gc-sections', *flags, '-lcrypto', '-o', str(earnings_executable)], check=True)
    commands_executable = None
    if os.environ.get('LEGENDARY_COMMANDS_TEST') == '1':
        commands_executable = temp/'legendary-commands-test'
        equipment_sources = []
        equipment_flags = []
        if os.environ.get('LEGENDARY_EQUIPMENT_TEST') == '1':
            equipment_flags = ['-DLEGENDARY_EQUIPMENT_TEST']
            if os.environ.get('LEGENDARY_EFFECTS_TEST') == '1':
                equipment_flags += ['-DLEGENDARY_EFFECTS_TEST']
            if os.environ.get('LEGENDARY_TECHNIQUES_TEST') == '1':
                equipment_flags += ['-DLEGENDARY_TECHNIQUES_TEST']
            if os.environ.get('LEGENDARY_SIGNATURES_TEST') == '1':
                equipment_flags += ['-DLEGENDARY_SIGNATURES_TEST']
            if os.environ.get('LEGENDARY_RANKS_TEST') == '1':
                equipment_flags += ['-DLEGENDARY_RANKS_TEST']
            if os.environ.get('LEGENDARY_FLOW_TEST') == '1':
                equipment_flags += ['-DLEGENDARY_FLOW_TEST']
            if os.environ.get('LEGENDARY_SECURITY_TEST') == '1':
                equipment_flags += ['-DLEGENDARY_SECURITY_TEST']
            if os.environ.get('LEGENDARY_FOUNDATIONS_TEST') == '1':
                equipment_flags += ['-DLEGENDARY_FOUNDATIONS_TEST']
            item_source = (root/'src/act.item.c').read_text()
            routes = temp/'legendary-routes.c'
            vanish = item_source[item_source.index('#define VANISH'):item_source.index('static int perform_drop(',item_source.index('#define VANISH'))]
            routes.write_text('\n'.join(['#include "conf.h"','#include "sysdep.h"','#include "structs.h"',
                '#include "utils.h"','#include "handler.h"','#include "comm.h"','#include "db.h"',
                '#include "dg_scripts.h"','#include "quest.h"','#include "legendary_equipment.h"',
                '#include "legendary_transaction.h"', '#include "interpreter.h"', '#include "act.h"', vanish,
                function('bool perform_give(',item_source).replace('perform_give(', 'test_real_give(',1),
                function('static int perform_drop(',item_source).replace('static int perform_drop(', 'int test_real_drop(',1)]))
            equipment_sources = [str(routes)]
            if os.environ.get('LEGENDARY_FOUNDATIONS_TEST') == '1':
                fight_source = (root/'src/fight.c').read_text()
                hit = function('int compute_hit_damage(', fight_source)
                tail = hit[hit.index('  /* Flat Force is added once'):]
                assert hit.index('  /* Flat Force is added once') > hit.rindex('dam *=')
                assert 'foundation_damage = MIN(legendary_foundation_weapon' in hit
                assert fight_source.index('bonuses[BONUS_TYPE_UNDEFINED] += accuracy;',fight_source.index('int compute_attack_bonus_full(')) < fight_source.index('return MIN(maximum_bab, calc_bab)',fight_source.index('int compute_attack_bonus_full('))
                with routes.open('a') as stream:
                    stream.write('\n#include "fight.h"\nint test_real_force_finish(int mode,int dam,int foundation_damage)\n{\n'+tail)
                    stream.write('\n#include "spells.h"\n#include "domains_schools.h"\n' + function('bool push_attempt(', (root/'src/utils.c').read_text()).replace('push_attempt(', 'test_real_push(',1))

            if os.environ.get('LEGENDARY_TECHNIQUES_TEST') == '1':
                player_source = (root/'src/players.c').read_text()
                save_block = player_source[player_source.index('  /* Save affects */'):player_source.index('  /* Save Damage Reduction */')]
                with routes.open('a') as stream:
                    stream.write('\n' + function('static void load_affects(',player_source).replace('static void load_affects(', 'void test_real_load_affects(',1))
                    stream.write('\n#define BUFFER_WRITE(...) do {if(fprintf(fl,__VA_ARGS__)<0)goto restore_character;}while(0)\n')
                    stream.write('bool test_real_save_affects(FILE *fl,struct char_data *ch) {\nstruct affected_type tmp_aff[MAX_AFFECT]={{0}},*aff,*p=ch->affected;int i;\nfor(i=0;p&&i<MAX_AFFECT;i++,p=p->next)tmp_aff[i]=*p;\n'+save_block+'\nreturn true;restore_character:return false;\n}\n#undef BUFFER_WRITE\n')
                # Hook contracts: actual attacks, not stat-display/confirmation
                # queries; final damage after defenses; no PSP deduction before
                # target validation; no speculative normal-slot refunds.
                attack_body = function('int hit(',fight_source)
                assert attack_body.index('legendary_technique_attack') < attack_body.index('diceroll = d20(ch)')
                resolution = function('static int resolve_damage(',fight_source)
                assert resolution.index('dam = damage_handling') < resolution.index('legendary_combat_packet') < resolution.index('GET_HIT(victim) -= dam')
                with routes.open('a') as stream:
                    stream.write('\n#include "legendary_techniques.h"\n#include "legendary_earnings.h"\n' + function('int legendary_combat_packet(',fight_source))
                    if os.environ.get('LEGENDARY_SECURITY_TEST') == '1':
                        stream.write('\n#include "clan.h"\n#include "mudlim.h"\n#include "dg_event.h"\n#include "actions.h"\n#include "spec_procs.h"\nvoid check_room_lighting_special(room_rnum,struct char_data *,struct obj_data *,bool);\nvoid get_check_craft_material(struct char_data *,struct obj_data *);\n')
                        stream.write(function('static int can_take_obj(',item_source)+'\n'+function('static void get_check_money(',item_source)+'\n')
                        stream.write(function('static void perform_get_from_container(',item_source).replace('static void perform_get_from_container(', 'void test_real_corpse_get(',1))

                if os.environ.get('LEGENDARY_SECURITY_TEST') == '1':
                    craft_source = (root/'src/crafting_new.c').read_text()
                    reforge = function('static void impl_do_reforge_new_(',craft_source)
                    assert reforge.index('legendary_tree_check') < reforge.index('GET_GOLD(ch) -= cost')
                    interpreter_source = (root/'src/interpreter.c').read_text()
                    deletion_guard = interpreter_source.index('legendary_retire_owner(GET_IDNUM(d->character),GET_NAME(d->character))')
                    assert deletion_guard < interpreter_source.index('Crash_delete_file(GET_NAME(d->character))',deletion_guard)
                    player_source = (root/'src/players.c').read_text()
                    deletion = function('void remove_player(',player_source)
                    assert deletion.index('legendary_retire_owner') < deletion.index('unlink(')
                    # Mail is text-only; objects use the separately exercised estate delivery codec.
                    mail_header=(root/'src/mail.h').read_text()
                    assert 'char *body;' in mail_header and 'struct obj_data' not in mail_header
                parser_source = (root/'src/spell_parser.c').read_text()
                gen_cast = function('ACMDU(do_gen_cast)',parser_source)
                assert 'GET_PSP(ch) -= (effective_psp_cost' not in gen_cast and 'GET_PSP(ch) += psionic_powers' not in gen_cast
                assert gen_cast.index('if (!target)') < gen_cast.index('legendary_cast_queue')
                assert 'legendary_technique_skill(ch,skillnum,true)' in function('int skill_roll(', (root/'src/utils.c').read_text())
                assert 'legendary_technique_skill(ch,abilityNum,false)' in function('int compute_ability_full(', (root/'src/spec_procs.c').read_text())




        subprocess.run([os.environ.get('CC', 'cc'), *extra_flags, '-std=gnu99', '-Wall', '-Wextra',
                        '-Wno-unused-parameter', '-ffunction-sections', '-fdata-sections',
                        '-DESTATE_REAL_CODEC', '-DLEGENDARY_PERSISTENCE_TEST', '-DLEGENDARY_TRANSACTION_TEST',
                        *equipment_flags, '-I'+str(root/'src'), str(root/'tests/legendary_commands.c'), *equipment_sources,
                        str(root/'tests/estate_real_codec_adapter.c'), str(transaction_codec),
                        *[str(root/'src'/name) for name in
                        ['estate.c','housing.c','auction_house.c','player_shop.c','legendary.c', 'legendary_equipment.c', 'legendary_foundations.c', 'legendary_effects.c', 'legendary_techniques.c',
                         'legendary_store.c','legendary_identity.c','legendary_transaction.c','durable_file.c',
                         'legendary_earnings.c','legendary_commands.c']],
                        '-Wl,--gc-sections', *flags, '-lcrypto', '-o', str(commands_executable)], check=True)
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
        if legendary_executable:
            subprocess.run([*client, '-e', 'DROP DATABASE estate_test; CREATE DATABASE estate_test'], check=True)
            migration = (root/'sql/20261023_legendary_objects.sql').read_text() + (root/'sql/20261024_legendary_transactions.sql').read_text() + (root/'sql/20261024_legendary_earnings.sql').read_text() + (root/'sql/20261024_legendary_smiths.sql').read_text() + (root/'sql/20261024_legendary_effects.sql').read_text() + (root/'sql/20261025_legendary_item_passives.sql').read_text() + (root/'sql/20261025_legendary_release_controls.sql').read_text() + (root/'sql/20261026_legendary_mob_bosses.sql').read_text() + (root/'sql/20261026_legendary_smith_specproc.sql').read_text() + (root/'sql/20261026_legendary_daily_limits.sql').read_text()
            for _ in range(2):
                subprocess.run([*client, 'estate_test'], input=migration, text=True, check=True)
            subprocess.run([str(legendary_executable), str(socket)], cwd=temp, check=True)
        if transaction_executable:
            subprocess.run([*client, '-e', 'DROP DATABASE estate_test; CREATE DATABASE estate_test'], check=True)
            migration = (root/'sql/20261023_legendary_objects.sql').read_text() + (root/'sql/20261024_legendary_transactions.sql').read_text() + (root/'sql/20261024_legendary_earnings.sql').read_text() + (root/'sql/20261024_legendary_smiths.sql').read_text() + (root/'sql/20261024_legendary_effects.sql').read_text() + (root/'sql/20261025_legendary_item_passives.sql').read_text() + (root/'sql/20261025_legendary_release_controls.sql').read_text() + (root/'sql/20261026_legendary_mob_bosses.sql').read_text() + (root/'sql/20261026_legendary_smith_specproc.sql').read_text() + (root/'sql/20261026_legendary_daily_limits.sql').read_text()
            subprocess.run([*client, 'estate_test'], input=migration, text=True, check=True)
            subprocess.run([str(transaction_executable), str(socket)], cwd=temp, check=True)
        if earnings_executable:
            subprocess.run([*client, '-e', 'DROP DATABASE estate_test; CREATE DATABASE estate_test'], check=True)
            migration = ''.join((root/'sql'/name).read_text() for name in
                ['20261023_legendary_objects.sql','20261024_legendary_transactions.sql','20261024_legendary_earnings.sql','20261024_legendary_smiths.sql','20261024_legendary_effects.sql','20261025_legendary_item_passives.sql','20261025_legendary_release_controls.sql','20261026_legendary_mob_bosses.sql','20261026_legendary_smith_specproc.sql','20261026_legendary_daily_limits.sql'])
            for _ in range(2):
                subprocess.run([*client, 'estate_test'], input=migration, text=True, check=True)
            subprocess.run([str(earnings_executable), str(socket)], cwd=temp, check=True)
        if commands_executable:
            subprocess.run([*client, '-e', 'DROP DATABASE estate_test; CREATE DATABASE estate_test'], check=True)
            migration = ''.join((root/'sql'/name).read_text() for name in
                ['20261023_legendary_objects.sql','20261024_legendary_transactions.sql',
                 '20261024_legendary_earnings.sql','20261024_legendary_smiths.sql','20261024_legendary_effects.sql','20261025_legendary_item_passives.sql','20261025_legendary_release_controls.sql','20261026_legendary_mob_bosses.sql','20261026_legendary_smith_specproc.sql','20261026_legendary_daily_limits.sql'])
            for _ in range(2):
                subprocess.run([*client, 'estate_test'], input=migration, text=True, check=True)
            subprocess.run([str(commands_executable), str(socket)], cwd=temp, check=True)
    finally:
        server.terminate()
        server.wait(timeout=15)
