-- Religion phase 2: native quest boards and character-wide weekly rewards.
CREATE TABLE IF NOT EXISTS religion_rank_names (
 deity INT NOT NULL, rank_num INT NOT NULL, title VARCHAR(90) NOT NULL,
 PRIMARY KEY(deity,rank_num)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE IF NOT EXISTS religion_task_config (
 temple_id INT NOT NULL, slot INT NOT NULL, deity INT NOT NULL, quest_vnum INT NOT NULL,
 PRIMARY KEY(deity,slot), UNIQUE KEY template(deity,quest_vnum), KEY temple(temple_id)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS religion_task_weeks (
 deity INT NOT NULL, week_id BIGINT NOT NULL, PRIMARY KEY(deity,week_id)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS religion_task_offers (
 temple_id INT NOT NULL, week_id BIGINT NOT NULL, slot INT NOT NULL, deity INT NOT NULL,
 quest_vnum INT NOT NULL, PRIMARY KEY(deity,week_id,slot),
 UNIQUE KEY template(deity,week_id,quest_vnum)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS religion_standing (
 player_id BIGINT NOT NULL, deity INT NOT NULL, standing INT NOT NULL DEFAULT 0,
 PRIMARY KEY(player_id,deity)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS religion_task_active (
 player_id BIGINT PRIMARY KEY, enrollment BIGINT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE,
 temple_id INT NOT NULL, deity INT NOT NULL, quest_vnum INT NOT NULL,
 story_only TINYINT NOT NULL DEFAULT 0, ready TINYINT NOT NULL DEFAULT 0,
 objective_hash BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS religion_task_completions (
 enrollment BIGINT UNSIGNED PRIMARY KEY, player_id BIGINT NOT NULL, deity INT NOT NULL,
 quest_vnum INT NOT NULL, week_id BIGINT NOT NULL, award INT NOT NULL,
 KEY weekly(player_id,week_id), KEY template(player_id,week_id,quest_vnum)
) ENGINE=InnoDB;
