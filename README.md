
Telegram-Bot
======================

License: GPLv3 Or Later
Project Site: http://www.shervin.org
Contact: admin@shervin.org Or shervinkh145@gmail.com

Compile Notes
=============

Compilable with both qt4 and qt5.
Compile With: qmake && make

Usage Notes
===========

This program requires qt framework, telegram-cli program, and python3.
You should specify telegram-cli and python3 path when running program.
(e.g. ./Telegram-Bot telegram-cli python)

The program makes its database in the first run but it won't work.
After the first run exit and insert telegram group (chat) IDs you want
the bot to manage and their admins' user ID in the tf_groups table
(with two columns gid and admin_id) in tf.db sqlite database and then
rerun the bot.
