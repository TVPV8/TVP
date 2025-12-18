function onUpdateDatabase()
	db.query("CREATE TABLE IF NOT EXISTS `kill_statistics` ( `name` varchar(35) NOT NULL, `killed_by` smallint unsigned NOT NULL, `killed` smallint unsigned NOT NULL, `time` int(10) unsigned NOT NULL DEFAULT '0', KEY `name` (`name`), KEY `time` (`time`) ) ENGINE=InnoDB DEFAULT CHARSET=utf8;")
	return true
end
