
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
You should specify telegram-cli path when running program.
(e.g. ./Telegram-Bot telegram-cli)

You can specify python3 path in run.sh file.
You should create an unprivillaged user named noone and have sudo and pkill.
(Because this program uses them to run calc commands in a controlled environment)

The program makes its database in the first run but it won't work.
After the first run exit and insert headadmin id and bot id in tf_config
table (with two columns headadmin_id and bot_id) in tf.db sqlite database
and then rerun the bot.

You need "graphviz" for the tree module!
You need "gnuplot" for the activity part in the statistics module!
