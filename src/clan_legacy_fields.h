#ifndef CLAN_LEGACY_FIELDS_H
#define CLAN_LEGACY_FIELDS_H
/* SQL/JSON name, legacy struct member, legacy file tag. No cache or locks. */
#define CLAN_LEGACY_FIELDS(X)                                                                      \
  X(application_min_level,applev,AppL)                                                           \
  X(application_fee_gold,appfee,AppF)                                                            \
  X(tax_rate,taxrate,Tax)                                                                        \
  X(war_ticks_remaining,war_timer,WarT)                                                          \
  X(hall_zone_vnum,hall,Hall)                                                                    \
  X(max_members,max_members,MaxM)                                                                \
  X(pk_wins,pk_win,PWin)                                                                         \
  X(pk_losses,pk_lose,PLos)                                                                      \
  X(raids,raided,Raid)                                                                           \
  X(total_deposits,total_deposits,TDep)                                                          \
  X(total_withdrawals,total_withdrawals,TWit)                                                    \
  X(total_members_joined,total_members_joined,TMJn)                                              \
  X(total_members_left,total_members_left,TMLf)                                                  \
  X(total_zones_claimed,total_zones_claimed,TZCl)                                                \
  X(current_zones_owned,current_zones_owned,CZOw)                                                \
  X(highest_member_count,highest_member_count,HiMC)                                              \
  X(total_taxes_collected,total_taxes_collected,TTax)                                            \
  X(total_wars_won,total_wars_won,TWWn)                                                          \
  X(total_wars_lost,total_wars_lost,TWLs)                                                        \
  X(total_alliances_formed,total_alliances_formed,TAFm)
#endif
