#!/usr/bin/env bash

# This file is part of Trip Server 2, a program to support trip recording and
# itinerary planning.
#
# Copyright (C) 2022-2024 Frank Dean <frank.dean@fdsd.co.uk>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Uncomment the following to debug the script
#set -x

export DEBIAN_FRONTEND=noninteractive
DEB_OPTIONS="--yes"
apt-get update
apt-get upgrade $DEB_OPTIONS

sed -i -e 's/# en_GB.UTF-8 UTF-8/en_GB.UTF-8 UTF-8/' /etc/locale.gen
locale-gen
export LANG=en_GB.utf8
localedef -i en_GB -c -f UTF-8 -A /usr/share/locale/locale.alias en_GB.UTF-8
update-locale LANG=en_GB.UTF-8 LANGUAGE

ln -fs /usr/share/zoneinfo/Europe/London /etc/localtime
apt-get install -y tzdata
dpkg-reconfigure tzdata

apt-get install $DEB_OPTIONS apt-transport-https
apt-get install $DEB_OPTIONS g++ git postgresql postgresql-contrib libpqxx-dev \
	screen autoconf autoconf-doc automake autoconf-archive libtool gettext \
	valgrind uuid-dev uuid-runtime make nginx

if [ "$VB_GUI" == "y" ]; then
    apt-get install -y lxde
fi
