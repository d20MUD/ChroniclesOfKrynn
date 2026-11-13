#ifndef KRYNN_CLAN_PACING_H
#define KRYNN_CLAN_PACING_H
/* Rank pacing knobs. Lowering these percentages shortens progression without
 * changing stored balances, earned receipts, reward rates or buff prices.
 * 100% = the approved 2x Favor / 10x three-account Influence model.
 * These are build-time settings; rebuild and restart after changing them. */
#ifndef CLAN_FAVOR_PACE_PERCENT
#define CLAN_FAVOR_PACE_PERCENT 100
#endif
#ifndef CLAN_INFLUENCE_PACE_PERCENT
#define CLAN_INFLUENCE_PACE_PERCENT 100
#endif
#if CLAN_FAVOR_PACE_PERCENT < 1 || CLAN_FAVOR_PACE_PERCENT > 1000 || \
    CLAN_INFLUENCE_PACE_PERCENT < 1 || CLAN_INFLUENCE_PACE_PERCENT > 1000
#error Clan pacing percentages must be between 1 and 1000
#endif
#define CLAN_FAVOR_THRESHOLD(n) (((n) * CLAN_FAVOR_PACE_PERCENT + 99) / 100)
#define CLAN_INFLUENCE_THRESHOLD(n) (((n) * CLAN_INFLUENCE_PACE_PERCENT + 99) / 100)
#endif
