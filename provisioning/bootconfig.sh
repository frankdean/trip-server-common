#!/usr/bin/env bash

# This file is part of Trip Server 2, a program to support trip recording and
# itinerary planning.
#
# Copyright (C) 2022 Frank Dean <frank.dean@fdsd.co.uk>
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

SU_CMD="su vagrant -c"

##Debian 10
#PG_VERSION=11
# Debian 11
PG_VERSION=13
#PGPASSWORD='SECRET'

# Configure PostgeSQL
su - postgres -c 'createuser -drs vagrant' 2>/dev/null
if [ "$WIPE_DB" == "y" ]; then
	su - vagrant -c 'dropdb vagrant' 2>/dev/null
fi
su - vagrant -c 'createdb vagrant' 2>/dev/null
if [ $? -eq 0 ]; then
    su - vagrant -c 'psql vagrant' >/dev/null <<EOF
CREATE EXTENSION pgcrypto;
EOF
fi
EXAMPLE_RC=/etc/trip-server-common-examplerc
if [ ! -e "${EXAMPLE_RC}" ]; then
    umask 0026
    echo "pg_uri postgresql://vagrant@/vagrant?host=" >${EXAMPLE_RC}
    echo "#pg_uri postgresql:${PGPASSWORD}://vagrant@/vagrant?host=" >>${EXAMPLE_RC}
    echo "# pg_uri dbname=vagrant user=vagrant password=${PGPASSWORD}" >>${EXAMPLE_RC}
    echo "worker_count 10" >>${EXAMPLE_RC}
    echo "pg_pool_size 12" >>${EXAMPLE_RC}
    umask 0022
    chown vagrant:vagrant ${EXAMPLE_RC}
fi
# Setup nginx as an example
if [ ! -e /etc/nginx/conf.d/example.conf ]; then
	cp /vagrant/provisioning/nginx/conf.d/example.conf /etc/nginx/conf.d/
fi
if [ ! -e /etc/nginx/sites-available/trip ]; then
	cp /vagrant/provisioning/nginx/sites-available/example /etc/nginx/sites-available
fi
cd /etc/nginx/sites-enabled
if [ -e default ]; then
	rm -f default
	ln -fs ../sites-available/example
fi
systemctl restart nginx

# Build the application
if [ ! -d /home/vagrant/build ]; then
    $SU_CMD 'mkdir /home/vagrant/build'
fi
cd /home/vagrant/build
# Don't configure if we appear to already have an installed version of trip-server
if [ ! -x /usr/local/bin/example ]; then
    $SU_CMD 'cd /home/vagrant/build && /vagrant/configure'
fi
# Run make to ensure build is up-to-date
$SU_CMD 'make -C /home/vagrant/build check'
if [ $? -eq 0 ] && [ -x /home/vagrant/build/src/example ]; then
    echo "Installing example"
    make install
fi
# Configure systemd
if [ ! -e /etc/systemd/system/example.service ]; then
    cp /vagrant/provisioning/systemd/example.service /etc/systemd/system/
fi
if [ -x /usr/local/bin/example ]; then
    systemctl is-active example.service >/dev/null
    if [ $? -ne 0 ]; then
	systemctl enable example.service 2>/dev/null
	systemctl start example.service 2>/dev/null
    fi
fi
# Vi as default editor (no favouritism, just simpler install then Emacs)
egrep '^export\s+EDITOR' /home/vagrant/.profile >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "export EDITOR=/usr/bin/vi" >>/home/vagrant/.profile
fi
if [ ! -e /var/log/example.log ]; then
    touch /var/log/example.log
fi
chown vagrant:vagrant /var/log/example.log
chmod 0640 /var/log/example.log
if [ ! -e /etc/logrotate.d/example ]; then
    cp /vagrant/provisioning/logrotate.d/example /etc/logrotate.d/
fi
