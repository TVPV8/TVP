function onUpdateDatabase()
	db.query("ALTER TABLE `account_bans` DROP FOREIGN KEY `account_bans_ibfk_2`;")
	db.query("ALTER TABLE `account_ban_history` DROP FOREIGN KEY `account_ban_history_ibfk_2`;")
	db.query("ALTER TABLE `ip_bans` DROP FOREIGN KEY `ip_bans_ibfk_1`;")
	return true
end
