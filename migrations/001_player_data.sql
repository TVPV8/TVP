-- Migration: Player data from TVPP files to database
-- This migration adds new tables and columns to store all player data in the database

-- Add missing columns to players table
ALTER TABLE `players` 
  ADD COLUMN IF NOT EXISTS `blessings` TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER `stamina`,
  ADD COLUMN IF NOT EXISTS `save` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  ADD COLUMN IF NOT EXISTS `conditions` BLOB NULL DEFAULT NULL,
  ADD COLUMN IF NOT EXISTS `unjusts` TEXT NOT NULL DEFAULT '',
  ADD COLUMN IF NOT EXISTS `cap` INT UNSIGNED NOT NULL DEFAULT 40000 AFTER `blessings`;

-- Player storage values (quest progress, etc.)
CREATE TABLE IF NOT EXISTS `player_storage` (
  `player_id` INT NOT NULL,
  `key` INT UNSIGNED NOT NULL,
  `value` INT NOT NULL,
  PRIMARY KEY (`player_id`, `key`),
  CONSTRAINT `player_storage_ibfk_1` FOREIGN KEY (`player_id`) 
    REFERENCES `players` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Player string storage values
CREATE TABLE IF NOT EXISTS `player_string_storage` (
  `player_id` INT NOT NULL,
  `key` TEXT NOT NULL,
  `value` TEXT NOT NULL,
  PRIMARY KEY (`player_id`, `key`(180)),
  CONSTRAINT `player_string_storage_ibfk_1` FOREIGN KEY (`player_id`) 
    REFERENCES `players` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Learned spells
CREATE TABLE IF NOT EXISTS `player_spells` (
  `player_id` INT NOT NULL,
  `name` VARCHAR(255) NOT NULL,
  PRIMARY KEY (`player_id`, `name`),
  CONSTRAINT `player_spells_ibfk_1` FOREIGN KEY (`player_id`) 
    REFERENCES `players` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- VIP list
CREATE TABLE IF NOT EXISTS `player_viplist` (
  `player_id` INT NOT NULL,
  `vip_id` INT NOT NULL,
  PRIMARY KEY (`player_id`, `vip_id`),
  CONSTRAINT `player_viplist_ibfk_1` FOREIGN KEY (`player_id`) 
    REFERENCES `players` (`id`) ON DELETE CASCADE,
  CONSTRAINT `player_viplist_ibfk_2` FOREIGN KEY (`vip_id`) 
    REFERENCES `players` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Depot items (stored per depot as binary BLOB)
CREATE TABLE IF NOT EXISTS `player_depotitems` (
  `player_id` INT NOT NULL,
  `depot_id` INT NOT NULL,
  `items` MEDIUMBLOB NOT NULL,
  PRIMARY KEY (`player_id`, `depot_id`),
  CONSTRAINT `player_depotitems_ibfk_1` FOREIGN KEY (`player_id`) 
    REFERENCES `players` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Modify player_items to store items as BLOB
ALTER TABLE `player_items` 
  ADD COLUMN IF NOT EXISTS `attributes` BLOB DEFAULT NULL AFTER `count`;
