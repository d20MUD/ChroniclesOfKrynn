#ifndef KRYNN_HOUSING_H
#define KRYNN_HOUSING_H
int housing_command(struct char_data *ch, const char *argument);
int housing_admin(struct char_data *ch, const char *argument);
int housing_is_room(room_vnum vnum);
int housing_can_enter(struct char_data *ch, room_vnum vnum);
void housing_capture_location(struct char_data *ch);
room_rnum housing_login_room(struct char_data *ch);
int housing_put(struct char_data *ch, struct obj_data *obj, struct obj_data *container);
int housing_get(struct char_data *ch, struct obj_data *container, const char *argument, int amount);
int housing_contents(struct char_data *ch, struct obj_data *container);
void housing_save_all(void);
void housing_object_extracted(struct obj_data *);
#endif
