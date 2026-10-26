#ifndef KRYNN_LEGENDARY_IDENTITY_H
#define KRYNN_LEGENDARY_IDENTITY_H
#include <stdbool.h>
#include <stdio.h>
struct obj_data;
/* Fixed lowercase hex, 128 bits; nil UUID is invalid. */
bool legendary_uuid_valid(const char *uuid);
bool legendary_uuid_generate(char out[33]);
/* Serialized form: LgId: 1 <uuid>. Malformed/unknown versions remain quarantined
 * and are preserved as an invalid identity marker, never silently made ordinary. */
void legendary_identity_restore(struct obj_data *obj, const char *value);
bool legendary_identity_line(const struct obj_data *obj, char *out, size_t size);
void legendary_object_live(struct obj_data *obj);
void legendary_object_unload(struct obj_data *obj);
void legendary_object_clear_identity(struct obj_data *obj);
/* Fail closed after authoritative writes; caller refreshes after SQL commit.
 * Match UUID, or a prototype when uuid is NULL. Does not delete ordinary gear. */
void legendary_object_invalidate(const char *uuid, int prototype);
/* Validate/claim only finalized live objects; previews never claim identities. */
bool legendary_object_verify(struct obj_data *obj);
#endif
