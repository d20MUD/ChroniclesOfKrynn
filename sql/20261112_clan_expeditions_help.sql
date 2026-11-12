-- Optional help publication only; does not enable expeditions.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('CLANEXPEDITIONS CLANOUTINGS','general',REPLACE('CLAN EXPEDITION ENROLLMENT

clan expeditions list
clan expeditions create <definition-id>
clan expeditions join <outing-id>
clan expeditions active
clan expeditions start
clan expeditions leave
clan expeditions abandon
clan expeditions claims
clan expeditions claim <outing-id>

Expeditions require enabled pilot services and accepted membership in the same
clan. Ordinary delivery, gathering and bounty contracts remain available with
their own independent slots and limits. Expedition enrollment earns no reward.
Accepted outings advance only on qualifying designated combat encounters.

Create reserves the clan''s one recruiting/active outing and enrolls its creator.
Share the displayed outing ID with eligible members. Each member must join
personally at a quartermaster, outside combat and timed crafting. The roster
allows two to six DIFFERENT authenticated accounts; an alt cannot fill another
slot. The definition''s character band and clan unlock must be satisfied.

The creator starts the outing at a quartermaster. All enrolled members must be
present in that room, outside combat/crafting, with current eligible membership,
account and level. Start rechecks the entire roster and the accepted targets.
Recruiting expires at the weekly boundary. A started outing expires two real
hours after its first start, or Monday 00:00 UTC if earlier. Starting again does
not extend the timer or allocate a new outing ID. Active shows the saved ID,
roster, sealed status and expiry; reconnects do not substitute new identities.

Before the first counted objective seals the roster, members can leave and new
accounts can opt in. A removed character/account cannot rejoin that same outing.
Once sealed, substitutions are refused. Leaving the clan, changing clans, or
returning to applicant status invalidates enrollment even if you immediately
rejoin. A reconnect with the original eligible character/account is allowed.

Any currently enrolled accepted member may abandon the outing. Its history is
retained; a new outing receives a new identity and no transferred progress.
Expired outings cannot start or accept enrollment. Use create to open a fresh
recruiting outing after expiry. No command clears account reward limits.

If a storage acknowledgement is uncertain, use active to inspect the clan''s
saved outing before retrying. Do not delete roster or outing history to clear
a lock. Availability depends on validated world definitions and storage; this
command does not enable expedition services or clan buffs.

Each counted encounter needs meaningful participation by at least two enrolled
accounts. Damage, effective healing, mitigation, useful support/defense and
tanking use the normal clan contribution rules. A killing blow is not required.
Trivial, owned, summoned, test and manipulated encounters never count. A kill
advances at most ONE objective; named targets do not also fill defender quotas.
Normal respawns can supply more qualifying kills, but replaying one encounter
cannot add progress twice. Active shows the three saved objective counters.

Stronghold Assault requires 20 defenders and two named objectives, with listed
alternatives. Regional Hunt requires ten kills in each of three categories
across at least two areas. To earn personal completion you must participate in
at least half the counted objectives, rounded up, AND the final objective.
At least two enrolled accounts must satisfy that full participation rule to
finish. An attempted final kill without two eligible finishers grants no final
progress; meet the requirements with a legitimate final encounter or abandon.

A completed outing closes its active slot. Use claims to find your earned
personal completions and claim <outing-id> at a quartermaster. Personal base
credit is 100 Favor, subject to the account Favor cap. It grants no character
XP, clan XP or Influence. Each account can claim at most one completion per
expedition family per UTC week, shared across alts and clans. A later eligible
member can earn personal Favor even when the shared reward has already paid.

The first qualifying completion per clan/week/family awards 600 clan XP and
300 Influence, subject to clan caps. Repeating the outing, changing the roster
or increasing party size never repeats that shared award. These are separate
from the personal Favor limits and from ordinary kill/bounty awards.

An unclaimed personal completion expires at its original Monday 00:00 UTC
boundary and requires membership in its original clan. Leaving and rejoining
cannot restore that entitlement. Once a claim has been durably accepted, its
pending receipt recovers with its original identity/week, including after
membership changes or reset. Repeating a paid claim returns the same result.
Storage outages cannot fabricate missed kills; a retained pending SQL receipt
can be retried, while an encounter never stored successfully is not recreated.
',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,34,0)
ON DUPLICATE KEY UPDATE entry=VALUES(entry),category=VALUES(category);
COMMIT;
