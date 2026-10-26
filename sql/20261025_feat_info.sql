-- Optional pre-creation: the MUD also creates these tables on boot.
-- These tables are generated from feat_list; boot replaces their contents.
CREATE TABLE IF NOT EXISTS feat_info (
  feat_id INT NOT NULL PRIMARY KEY,
  name TEXT NULL,
  in_game TINYINT NOT NULL,
  can_learn TINYINT NOT NULL,
  can_stack TINYINT NOT NULL,
  feat_type INT NOT NULL,
  short_description LONGTEXT NULL,
  description LONGTEXT NULL,
  epic TINYINT NOT NULL,
  combat_feat TINYINT NOT NULL,
  event INT NOT NULL,
  teamwork_feat TINYINT NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS feat_info_prerequisites (
  feat_id INT NOT NULL,
  prerequisite_order INT NOT NULL,
  prerequisite_type INT NOT NULL,
  description LONGTEXT NULL,
  value_0 INT NOT NULL,
  value_1 INT NOT NULL,
  value_2 INT NOT NULL,
  PRIMARY KEY (feat_id, prerequisite_order),
  FOREIGN KEY (feat_id) REFERENCES feat_info(feat_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
