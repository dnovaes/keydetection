
#############################################################################
#																			#
#								MYSQL										#
#																			#
#############################################################################
SHOW CREATE TABLE `login`;

CREATE TABLE `login` (
 `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
 `name` varchar(50) NOT NULL,
 `username` varchar(30) NOT NULL,
 `password` varchar(64) NOT NULL,
 `email` varchar(50) NOT NULL,
 `created_at` timestamp NOT NULL,
 `lastlogin` timestamp NULL DEFAULT NULL,
 `viptime` int(10) unsigned NOT NULL,
 PRIMARY KEY (`id`),
 UNIQUE KEY `username` (`username`),
 UNIQUE KEY `email` (`email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `section` (
 `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
 `datetime` timestamp NOT NULL,
 `ip` varchar(20) NOT NULL,
 `login_id` int(10) unsigned NOT NULL,
 PRIMARY KEY (`id`),
 UNIQUE KEY `id` (`id`,`login_id`),
 UNIQUE KEY `ip` (`ip`),
 KEY `fk_login_id` (`login_id`),
 CONSTRAINT `fk_login_id` FOREIGN KEY (`login_id`) REFERENCES `login` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;