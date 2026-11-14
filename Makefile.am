# LuminariMUD Makefile.am - for use with automake
# Based on the original Makefile.in structure

# Set up subdirectories
SUBDIRS = util

# Additional compiler flags
# Revert to project defaults: do NOT force global feature defines here.
AM_CFLAGS = -std=gnu90 @MYFLAGS@


# Main program
bin_PROGRAMS = circle

# Rebuild for header changes even when compiler dependency tracking is disabled.
$(circle_OBJECTS) $(cutest_OBJECTS): $(wildcard $(srcdir)/src/*.h)

# Source files for the main program
circle_SOURCES = \
	src/account.c \
	src/act.comm.c \
	src/act.comm.do_spec_comm.c \
	src/act.informative.c \
	src/act.item.c \
	src/movement.c \
	src/movement_validation.c \
	src/movement_cost.c \
	src/movement_position.c \
	src/movement_doors.c \
	src/movement_falling.c \
	src/movement_messages.c \
	src/movement_tracks.c \
	src/movement_events.c \
	src/act.offensive.c \
	src/act.other.c \
	src/act.social.c \
	src/act.wizard.c \
	src/actionqueues.c \
	src/actions.c \
	src/aedit.c \
	src/ai_cache.c \
	src/ai_events.c \
	src/ai_security.c \
	src/ai_service.c \
	src/alchemy.c \
	src/asciimap.c \
	src/assign_wpn_armor.c \
	src/backgrounds.c \
	src/ban.c \
	src/bardic_performance.c \
	src/boards.c \
	src/bedit.c \
	src/mysql_boards.c \
	src/bsd-snprintf.c \
	src/brew.c \
	src/cedit.c \
	src/char_descs.c \
	src/clan.c \
	src/clan_buff_queries.c \
	src/clan_buff_activation.c \
	src/clan_buff_commit.c \
	src/clan_buff_lifecycle.c \
	src/clan_buff_runtime.c \
	src/clan_buff_commands.c \
	src/clan_combat.c \
	src/clan_web_content.c \
	src/clan_web_content_runtime.c \
	src/clan_web.c \
	src/clan_web_runtime.c \
	src/clan_quartermaster.c \
	src/clan_quartermaster_runtime.c \
	src/clan_delivery.c \
	src/clan_delivery_runtime.c \
	src/clan_expedition_enrollment.c \
	src/clan_expedition_runtime.c \
	src/clan_expeditions.c \
	src/clan_expedition_world.c \
	src/clan_objectives.c \
	src/clan_objective_runtime.c \
	src/clan_sources.c \
	src/clan_qm_services.c \
	src/clan_enchant.c \
	src/clan_access.c \
	src/clan_status.c \
	src/clan_commands.c \
	src/clan_awards.c \
	src/clan_policy.c \
	src/clan_progression.c \
	src/clan_repository.c \
	src/clan_boot.c \
	src/clan_persistence.c \
	src/clan_economy.c \
	src/clan_edit.c \
	src/clan_transactions.c \
	src/class.c \
	src/combat_modes.c \
	src/comm.c \
	src/config.c \
	src/constants.c \
	src/copyover_diagnostic.c \
	src/craft.c \
	src/crafting_new.c \
	src/crafting_recipes.c \
	src/crafts.c \
	src/db.c \
	src/db_init.c \
	src/db_init_data.c \
	src/db_startup_init.c \
	src/db_admin_commands.c \
	src/deities.c \
	src/desc_engine.c \
	src/dg_comm.c \
	src/discord_bridge.c \
	src/dg_db_scripts.c \
	src/dg_event.c \
	src/dg_handler.c \
	src/dg_misc.c \
	src/dg_mobcmd.c \
	src/dg_objcmd.c \
	src/dg_olc.c \
	src/dg_scripts.c \
	src/dg_triggers.c \
	src/dg_variables.c \
	src/dg_wldcmd.c \
	src/domain_powers.c \
	src/domains_schools.c \
	src/dotenv.c \
	src/encounters.c \
	src/evolutions.c \
	src/feats.c \
	src/talents.c \
	src/perks.c \
	src/fight.c \
	src/gain.c \
	src/genmob.c \
	src/genobj.c \
	src/genolc.c \
	src/genqst.c \
	src/genshp.c \
	src/genwld.c \
	src/genzon.c \
	src/graph.c \
	src/grapple.c \
	src/handler.c \
	src/hedit.c \
	src/help.c \
	src/helpers.c \
	src/hlqedit.c \
	src/hlquest.c \
	src/house.c \
	src/housing.c \
	src/estate.c \
	src/legendary.c \
	src/legendary_equipment.c \
	src/legendary_techniques.c \
	src/legendary_effects.c \
	src/legendary_foundations.c \
	src/legendary_techniques.h \
	src/legendary_effects.h \
	src/legendary_foundations.h \
	src/legendary_equipment.h \
	src/legendary_commands.c \
	src/legendary_commands.h \
	src/legendary_earnings.c \
	src/legendary_earnings.h \
	src/legendary_transaction.c \
	src/legendary_transaction.h \
	src/durable_file.c \
	src/durable_file.h \
	src/legendary.h \
	src/legendary_store.c \
	src/legendary_store.h \
	src/legendary_identity.c \
	src/legendary_identity.h \
	src/auction_house.c \
	src/player_shop.c \
	src/hsedit.c \
	src/hunts.c \
	src/ibt.c \
	src/improved-edit.c \
	src/interpreter.c \
	src/introduce.c \
	src/kdtree.c \
	src/limits.c \
	src/lists.c \
	src/magic.c \
	src/epic_magic.c \
	src/mail.c \
	src/medit.c \
	src/metamagic_science.c \
	src/missions.c \
	src/mob_act.c \
	src/mob_memory.c \
	src/mob_utils.c \
	src/mob_race.c \
	src/mob_psionic.c \
	src/mob_class.c \
	src/mob_spells.c \
	src/mob_spellslots.c \
	src/mob_known_spells.c \
	src/modify.c \
	src/moon_bonus_spells.c \
	src/msgedit.c \
	src/mud_event.c \
	src/mud_event_list.c \
	src/mysql.c \
	src/new_mail.c \
	src/oasis.c \
	src/oasis_copy.c \
	src/oasis_delete.c \
	src/oasis_list.c \
	src/objsave.c \
	src/oedit.c \
	src/perfmon.c \
	src/perlin.c \
	src/players.c \
	src/prefedit.c \
	src/premadebuilds.c \
	src/protocol.c \
	src/psionics.c \
	src/pubsub.c \
	src/systems/pubsub/pubsub_handlers.c \
	src/systems/pubsub/pubsub_db.c \
	src/systems/pubsub/pubsub_commands.c \
	src/systems/pubsub/pubsub_queue.c \
	src/systems/pubsub/pubsub_spatial.c \
	src/systems/pubsub/pubsub_event_handlers.c \
	src/systems/spatial/spatial_core.c \
	src/systems/spatial/spatial_visual.c \
	src/systems/spatial/spatial_audio.c \
	src/systems/terrainbridge/terrain_bridge.c \
	src/systems/region_hints/region_hints.c \
	src/systems/narrative_weaver/narrative_weaver.c \
	src/systems/intermud3/i3_client.c \
	src/systems/intermud3/i3_commands.c \
	src/systems/intermud3/i3_utils.c \
	src/qedit.c \
	src/quest.c \
	src/race.c \
	src/random.c \
	src/random_names.c \
	src/rank.c \
	src/redit.c \
	src/resource_system.c \
	src/resource_depletion.c \
	src/resource_descriptions.c \
	src/resource_regeneration.c \
	src/roleplay.c \
	src/routing.c \
	src/sedit.c \
	src/shop.c \
	src/spec_abilities.c \
	src/spec_assign.c \
	src/spec_procs.c \
	src/specs.artifacts.c \
	src/casting_visuals.c \
	src/spellhelp.c \
	src/spell_parser.c \
	src/spell_prep.c \
	src/spellbook_scroll.c \
	src/spells.c \
	src/staff_events.c \
	src/study.c \
	src/tedit.c \
	src/templates.c \
	src/trade.c \
	src/transport.c \
	src/traps_new.c \
	src/treasure.c \
	src/treasure_const.c \
	src/utils.c \
	src/vehicles.c \
	src/vehicles_commands.c \
	src/vehicles_transport.c \
	src/transport_unified.c \
	src/vessels.c \
	src/vessels_db.c \
	src/vessels_docking.c \
	src/vessels_rooms.c \
	src/vessels_autopilot.c \
	src/weather.c \
	src/wilderness.c \
	src/wilderness_kb.c \
	src/zedit.c \
	src/zmalloc.c \
	src/zone_procs.c

# Libraries to link
circle_LDADD = @LIBS@ @CRYPTLIB@ @NETLIB@ -lcrypt -lgd -lm -lmariadb -lcurl -lssl -lcrypto -lpthread -ljson-c


# Unit testing with CuTest
check_PROGRAMS = cutest

# CuTest sources
cutest_SOURCES = \
	unittests/CuTest/CuTest.c \
	unittests/CuTest/test_alignment.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_char_cache.c \
	unittests/CuTest/test_char_skills.c \
	unittests/CuTest/test_faerun_wilderness.c \
	unittests/CuTest/test_random.c \
	unittests/CuTest/test_treasure.c \
	unittests/CuTest/test_utils.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/AllTests.c \
	$(circle_SOURCES)

cutest_CFLAGS = $(AM_CFLAGS) -DLUMINARI_CUTEST
cutest_LDADD = $(circle_LDADD)

cutest_test_files = \
	unittests/CuTest/test_alignment.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_char_cache.c \
	unittests/CuTest/test_char_skills.c \
	unittests/CuTest/test_faerun_wilderness.c \
	unittests/CuTest/test_random.c \
	unittests/CuTest/test_treasure.c \
	unittests/CuTest/test_utils.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c

# Generate AllTests.c before building cutest
unittests/CuTest/AllTests.c: $(cutest_test_files) unittests/CuTest/make-tests.sh
	unittests/CuTest/make-tests.sh $(cutest_test_files) > $@

BUILT_SOURCES = unittests/CuTest/AllTests.c

# Clean up generated files
CLEANFILES = unittests/CuTest/AllTests.c

# Install to local bin directory
bindir = $(top_builddir)/bin

# After installation, remove the executable from the source directory
install-exec-hook:
	rm -f $(bin_PROGRAMS)

# Ensure constants.c is rebuilt when other sources change
src/constants.c: $(filter-out src/constants.c,$(circle_SOURCES))
	touch $@

# Extra files to distribute
EXTRA_DIST = \
	src/clan_web.h \
	src/clan_web_content.h \
	docs/clan_progression_phase_42_45_handoff.md \
	docs/clan_progression_phase_46_47_handoff.md \
	docs/clan_progression_phase_48_49_handoff.md \
	docs/clan_progression_phase_50_52_handoff.md \
	docs/clan_progression_pilot_review.md \
	docs/clan_progression_pacing.md \
	docs/clan_progression_pacing/pacing.json \
	docs/clan_progression_pacing/level_comparison.csv \
	docs/clan_progression_pacing/favor_ranks.csv \
	docs/clan_progression_pacing/influence_ranks.csv \
	docs/clan_progression_release_checklist.md \
	docs/clan_progression_operator_notes.md \
	docs/help/clan_progression.hlp \
	sql/20261112_clan_progression_services_help.sql \
	lib/clan/quartermaster_services.json \
	sql/20261113_clan_quartermaster_services_v001.sql \
	docs/clan_progression_gear_and_quartermaster.md \
	lib/clan/quartermaster.json \
	src/clan_quartermaster.h \
	sql/20261112_clan_quartermaster_v001.sql \
	sql/20261112_clan_quartermaster_help.sql \
	docs/help/weapon_effects.hlp \
	docs/weapon_effects_mechanics_review.md \
	sql/20261113_weapon_effects_help.sql \
	docs/help/clan_quartermaster.hlp \
	docs/clan_progression_phase_39_40_41_handoff.md \
	lib/clan/expeditions.json \
	lib/clan/expeditions_legacy.json \
	scripts/validate_clan_expedition_roster.py \
	lib/clan/deliveries.json \
	lib/clan/objectives.json \
	docs/clan_progression_phase_32_33_handoff.md \
	src/clan_progression.h \
	src/clan_pacing.h \
	src/clan_policy.h \
	lib/clan/progression.json \
	src/clan_repository.h \
	src/clan_rewards.h \
	src/clan_awards.h \
	src/clan_buff_queries.h \
	src/clan_buff_activation.h \
	src/clan_buff_lifecycle.h \
	src/clan_buff_runtime.h \
	src/clan_buff_commands.h \
	src/clan_combat.h \
	src/clan_delivery.h \
	src/clan_delivery_runtime.h \
	src/clan_contract_rotation.h \
	src/clan_expedition_enrollment.h \
	sql/20261112_clan_expeditions_v001.sql \
	sql/20261112_clan_expeditions_v002.sql \
	sql/20261112_clan_expeditions_help.sql \
	docs/help/clan_expeditions.hlp \
	docs/clan_progression_phase_34_handoff.md \
	docs/clan_progression_phase_35_36_handoff.md \
	docs/clan_progression_phase_37_38_handoff.md \
	src/clan_expeditions.h \
	src/clan_objectives.h \
	src/clan_objective_runtime.h \
	src/clan_sources.h \
	sql/20261111_clan_buff_lifecycle_v001.sql \
	sql/20261111_clan_buff_activations_v001.sql \
	src/clan_display.h \
	src/clan_qm_services.h \
	src/clan_enchant.h \
	src/clan_access.h \
	src/clan_status.h \
	src/clan_commands.h \
	src/clan_persistence.h \
	src/clan_legacy_fields.h \
	src/clan_repository_schema.inc \
	sql/20261110_clan_progression_v001.sql \
	sql/20261110_clan_progression_v002.sql \
	sql/20261110_clan_progression_v003.sql \
	sql/20261110_clan_progression_v004.sql \
	sql/20261110_clan_progression_v005.sql \
	sql/20261113_clan_progression_v006.sql \
	sql/20261111_clan_progression_readouts_help.sql \
	scripts/generate_clan_schema_contract.py \
	scripts/build_clan_repository_tool.py \
	scripts/clan_legacy_import.py \
	tools/clan_repository_tool.c \
	lib/misc/zone_promote.py \
	lib/text/help/zpromote.hlp \
	docs/zone-promotion.md \
	sql/20261107_zone_promotion_help.sql \
	tests/run_zone_promote.py \
	src/epic_magic.h \
	src/housing.h \
	src/estate.h \
	src/auction_house.h \
	src/player_shop.h \
	docs/guides/PLAYER_SHOPS.md \
	lib/text/help/stalls.hlp \
	sql/20261023_player_shops.sql \
	sql/20261023_player_shops_help.sql \
	docs/housing-auction-port.md \
	sql/20261009_housing_auction.sql \
	sql/20261009_housing_auction_help.sql \
	tests/runtime_housing_rooms.c \
	tests/estate_integration.c \
	tests/estate_real_codec_adapter.c \
	tests/run_estate_integration.py \
	unittests/CuTest/make-tests.sh \
	frmaps/faerunmap.jpg \
	frmaps/legend.txt \
	README.md \
	CHANGELOG.md \
	LICENSE

# Run unit tests
test: check
	./cutest

# Additional cleanup for distclean
# Removes autotools-generated files and build artifacts
distclean-local:
	rm -f Makefile.in install-sh gdb.txt depcomp compile missing
	rm -f configure~ configure.ac~ conf.h.in~ conf.h.in aclocal.m4
	rm -f config.status config.log configure
	rm -rf autom4te.cache/
	rm -rf src/.deps/ src/.dirstamp src/stamp-h1
	rm -f stamp-h1 .dirstamp
	rm -rf util/.deps/ util/.dirstamp
	rm -f config.cache

# Custom clean target that removes build files but keeps Makefile and config.h
# KEEPS: Everything needed to avoid triggering autoreconf during make clean
scrub: clean
	rm -f gdb.txt valgrind.log WORK.md CODEBASE_SCOPE.md
	rm -f configure~ configure.ac~ conf.h.in~
	rm -f config.log
	rm -rf autom4te.cache/
	rm -f log/*.log
	rm -f *.orig *.rej *.patch
	rm -f .killscript .fastboot pause .autorun.lock
	rm -f .websocket_policy.pid .flash_policy.pid
	rm -f lib/copyover.dat
	rm -f core core.* vgcore.*
	rm -f lib/core lib/core.* lib/vgcore.*
	rm -f *~ *.swp *.swo
	rm -f src/*~ src/*.swp src/*.swo
	rm -f *.tmp *.temp
	rm -f test_*.out test_*.log
