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

# Whilst this script is primarily intended as a Vagrant provisioning script,
# it can also be run in a VM such as QEMU to setup up a working environment by
# overriding the EXAMPLE_SOURCE, USERNAME and GROUPNAME variables before
# running the script.

# Uncomment the following to debug the script
#set -x

# This is the folder where the example source code is available in the VM
EXAMPLE_SOURCE=${EXAMPLE_SOURCE:-/vagrant}
# The user and group name for building the example
USERNAME=${USERNAME:-vagrant}
GROUPNAME=${GROUPNAME:-${USERNAME}}

echo "Source folder: ${EXAMPLE_SOURCE}"
echo "Username: ${USERNAME}"
echo "Groupname: ${USERNAME}:${GROUPNAME}"

SU_CMD="su ${USERNAME} -c"

PG_VERSION=15
#PGPASSWORD='SECRET'

# Configure PostgeSQL
su - postgres -c "createuser -drs ${USERNAME}" 2>/dev/null
if [ "$WIPE_DB" == "y" ]; then
	su - ${USERNAME} -c "dropdb ${USERNAME}" 2>/dev/null
fi
su - ${USERNAME} -c "createdb ${USERNAME}" 2>/dev/null
if [ $? -eq 0 ]; then
    su - ${USERNAME} -c "psql ${USERNAME}" >/dev/null <<EOF
CREATE EXTENSION pgcrypto;
EOF
fi
EXAMPLE_RC=/etc/trip-server-common-examplerc
if [ ! -e "${EXAMPLE_RC}" ]; then
    umask 0026
    echo "pg_uri postgresql://%2Fvar%2Frun%2Fpostgresql/${USERNAME}" > ${EXAMPLE_RC}
    echo "#pg_uri postgresql://vagrant@/vagrant?host=" >>${EXAMPLE_RC}
    echo "#pg_uri postgresql:${PGPASSWORD}://vagrant@/vagrant?host=" >>${EXAMPLE_RC}
    echo "# pg_uri dbname=vagrant user=vagrant password=${PGPASSWORD}" >>${EXAMPLE_RC}
    echo "worker_count 4" >>${EXAMPLE_RC}
    echo "pg_pool_size 6" >>${EXAMPLE_RC}
    umask 0022
    chown ${USERNAME}:${GROUPNAME} ${EXAMPLE_RC}
fi
# Setup nginx as an example
if [ ! -e /etc/nginx/conf.d/example.conf ]; then
	cp ${EXAMPLE_SOURCE}/provisioning/nginx/conf.d/example.conf /etc/nginx/conf.d/
fi
if [ ! -e /etc/nginx/sites-available/trip ]; then
	cp ${EXAMPLE_SOURCE}/provisioning/nginx/sites-available/example /etc/nginx/sites-available
fi
cd /etc/nginx/sites-enabled
if [ -e default ]; then
	rm -f default
	ln -fs ../sites-available/example
fi
systemctl restart nginx

# Build the application
if [ ! -d /home/${USERNAME}/build ]; then
    $SU_CMD "mkdir /home/${USERNAME}/build"
fi
cd /home/${USERNAME}/build
# Don't configure if we appear to already have an installed version of trip-server
if [ ! -x /usr/local/bin/example ]; then
    $SU_CMD "cd /home/${USERNAME}/build && ${EXAMPLE_SOURCE}/configure"
fi
# Run make to ensure build is up-to-date
$SU_CMD "make -C /home/${USERNAME}/build check"
if [ $? -eq 0 ] && [ -x /home/${USERNAME}/build/src/example ]; then
    echo "Installing example"
    make install
fi
# Configure systemd
if [ ! -e /etc/systemd/system/example.service ]; then
    cp ${EXAMPLE_SOURCE}/provisioning/systemd/example.service /etc/systemd/system/
fi
if [ -e /etc/systemd/system/example.service ]; then
    grep -i 'vagrant' /etc/systemd/system/example.service
    if [ $? -eq 0 ]; then
	sed -i "s/vagrant/${USERNAME}/g" /etc/systemd/system/example.service
    fi
    grep -i "Group=${GROUPNAME}" /etc/systemd/system/example.service
    if [ $? -ne 0 ]; then
	sed -i "s/Group=${USERNAME}/Group=${GROUPNAME}/" /etc/systemd/system/example.service
    fi
fi
sudo systemctl daemon-reload
if [ -x /usr/local/bin/example ]; then
    systemctl is-active example.service >/dev/null
    if [ $? -ne 0 ]; then
	systemctl enable example.service 2>/dev/null
	systemctl start example.service 2>/dev/null
    fi
fi
# Vi as default editor (no favouritism, just simpler install then Emacs)
egrep '^export\s+EDITOR' /home/${USERNAME}/.profile >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "export EDITOR=/usr/bin/vi" >>/home/${USERNAME}/.profile
fi
if [ ! -e /var/log/example.log ]; then
    touch /var/log/example.log
fi
chown ${USERNAME}:${GROUPNAME} /var/log/example.log
chmod 0640 /var/log/example.log
if [ ! -e /etc/logrotate.d/example ]; then
    cp ${EXAMPLE_SOURCE}/provisioning/logrotate.d/example /etc/logrotate.d/
fi
