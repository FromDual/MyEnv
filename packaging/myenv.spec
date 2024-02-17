# Copyright (c) 2017 - 2024, FromDual GmbH. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston
# MA  02110-1301  USA.

# ----------------------------------------------------------------------------
# Some old safeguards - bettar safe than sorry
# ----------------------------------------------------------------------------

# Avoid debuginfo RPMs, leaves binaries unstripped
%define debug_package   %{nil}

# ----------------------------------------------------------------------------
# RPM build tools now automatically detect Perl module dependencies.
# It might not be possible to disable this in all versions of RPM, but here we
# try anyway.  See:
#  http://fedoraproject.org/wiki/Packaging/Perl#Filtering_Requires:_and_Provides
#  http://www.wideopen.com/archives/rpm-list/2002-October/msg00343.html
# ----------------------------------------------------------------------------
%undefine __perl_provides
%undefine __perl_requires


# ----------------------------------------------------------------------------
# Distribution support
# ----------------------------------------------------------------------------
#
# "myenv" is not platform- (distribution-)-specific, but the distributions differ
# in their naming of packages required by "myenv".
# Also, some distributions (in some versions) do not get their "dist" tag correct.

# Step 1: If target distribution is not passed as a parameter, get it from the build host.
#         We check the file "/etc/DISTRO-release", first for the distro and then for the version.
#         Admitted: Effectively, we mimic the usual "%%{dist}" because we dare not rely on it.
#
# To bypass this step and override autodetect:
#
#   $ rpmbuild --define="distro_releasetag STRING" ...
#

%if %{undefined distro_releasetag}
  %if %(test -f /etc/fedora-release && echo 1 || echo 0)
    # Fedora behaves like RedHat, but the numbers differ, so we need a separate section.
    # The check for Fedora MUST precede the one for RedHat, as there is a "/etc/redhat-release" also on Fedora
    %define fedoraver %(rpm -qf --qf '%%{version}\\n' /etc/fedora-release | sed -e 's/^\\([0-9]*\\).*/\\1/g')
    %define distro_releasetag     fc
  %else
    %if %(test -f /etc/redhat-release && echo 1 || echo 0)
      %define rhelver %(rpm -qf --qf '%%{version}\\n' /etc/redhat-release | sed -e 's/^\\([0-9]*\\).*/\\1/g')
      %if "%rhelver" == "8"
        %define distro_releasetag     el8
      %else
        %if "%rhelver" == "9"
          %define distro_releasetag     el9
        %else
          %{error:Red Hat Enterprise Linux %{rhelver} is unsupported}
        %endif
      %endif
    %else
      %if %(test -f /etc/SuSE-release && echo 1 || echo 0)
        %define susever %(rpm -qf --qf '%%{version}\\n' /etc/SuSE-release | cut -d. -f1)
        %if "%susever" == "12"
          %define distro_releasetag   suse12
        %else
          %if "%susever" == "15"
            %define distro_releasetag   suse15
          %else
            %{error:SuSE %{susever} is unsupported}
          %endif
        %endif
      %else
        %{error:Unsupported distribution}
      %endif
    %endif
  %endif
%endif


# Step 2: Determine all settings according to the target distribution.
#         The macro "distro_releasetag" is now defined, as a parameter or via step 1.
#         The macros "DISTROver" will be undefined if step 1 was bypassed because of a parameter,
#         and there is no string pattern support, so we get a long chain of "if ... else if ... else if ..."

# Currently, there are no build requirements. A single global "define" will do.
# When this becomes non-empty, the "BuildRequires:" line (below) must be activated.
%define distro_buildreq      %{nil}

%if "%{distro_releasetag}" == "fc"
  %define distro_description    Fedora Linux
  %define distro_requires       php-cli php-mysql php-posix systemd
  %global systemd 1
%else
  %if "%{distro_releasetag}" == "el8"
    %define distro_description    Red Hat / Oracle / Rocky Linux / AlmaLinux 8
    %define distro_requires       php-cli php-mysql php-posix /sbin/chkconfig /sbin/service
    %global systemd 0
  %else
    %if "%{distro_releasetag}" == "el9"
      %define distro_description   Red Hat / Oracle / Rocky Linux / AlmaLinux 9
      %define distro_requires      php-cli php-mysql php-posix systemd
      %global systemd 1
    %else
      %if "%{distro_releasetag}" == "suse12"
        %define distro_description   SuSE Linux 12
        %define distro_requires      php-posix php-pcntl php-mysql
        %global systemd 0
      %else
        %if "%{distro_releasetag}" == "suse15"
          %define distro_description  SuSE Linux 15
          %define distro_requires     php-posix php-pcntl php-mysql systemd
          %global systemd 1
        %else
          %{error:Unknown distro_releasetag %{distro_releasetag}}
        %endif
      %endif
    %endif
  %endif
%endif


# Confusion:
# 'release' = '%%{release}'   is a RPM feature, the spec file version, set manually
# 'Version' = '%%{version}' = '%2.1.0%'  is the software version, set during export

%global release         1

BuildArchitectures: noarch

# To be activated when "%%{distro_buildreq}" becomes non-empty
# BuildRequires:  %%{distro_buildreq}
Requires:       %{distro_requires}

Name:           myenv
Version:        2.1.0
Release:        %{release}.%{distro_releasetag}
Distribution:   %{distro_description} (or compatible)

License:        GPL v2
Vendor:         FromDual GmbH
URL:            https://www.fromdual.com/myenv-mysql-mariadb-basenv
Source0:        https://support.fromdual.com/admin/download/myenv-%{version}.tar.gz

Summary:        Managing multiple MySQL versions and instances on a single machine

%description
MyEnv is a tool to operate MySQL and MariaDB multi-instance set-ups.

With MyEnv multi-instance set-ups should be easier to handle than with
mysqld_multi. This type of consolidation has less overhead than virtualization
solutions. To do proper resource fencing MyEnv provides cgroup integration.

MyEnv is licensed under the GPL v2: https://www.gnu.org/licenses/gpl-2.0.html

MyEnv is documented here: https://www.fromdual.com/myenv-mysql-mariadb-basenv

For questions, feedback and comments, please go to the MyEnv forum at:
https://www.fromdual.com/forum/373.

Bugs and feature request can be reported here: https://support.fromdual.com/bugs

# Some settings
%global mysql_home_dir  /home/mysql
%global mysqld_user     mysql
%global mysqld_group    mysql


##############################################################################
# Prepare the sources - that means: unpack them
##############################################################################

%prep
%setup -q


##############################################################################
# Build - trivial, no compile or link
##############################################################################

%build
echo "Build the RPM of '%{name}' version '%{version}' for platform '%{distro_releasetag}'"
echo "Features: systemd = %{systemd}"
echo "Required packages for installation: '%{distro_requires}'"

rm -fr packaging/ debian/   # Not needed in the user's installation


##############################################################################
# Install - file tree at build end
##############################################################################

%install
# rm -rf $RPM_BUILD_ROOT   Automatically done by the '%%install' macro

SRC=${RPM_BUILD_DIR}/%{name}-%{version}     # Where '%%prep' put the files
ABS=%{mysql_home_dir}/product             # Absolute path on user's machine

install -d ${RPM_BUILD_ROOT}${ABS}
cp -R ${SRC} ${RPM_BUILD_ROOT}${ABS}
install -d ${RPM_BUILD_ROOT}${ABS}/%{name}-%{version}/log

install -d ${RPM_BUILD_ROOT}/etc/%{name}
install -D -b ${SRC}/tpl/aliases.conf.template   ${RPM_BUILD_ROOT}/etc/%{name}/aliases.conf
install -D -b ${SRC}/tpl/variables.conf.template ${RPM_BUILD_ROOT}/etc/%{name}/variables.conf
install -D -b ${SRC}/etc/MYENV_BASE              ${RPM_BUILD_ROOT}/etc/%{name}/MYENV_BASE
install -d -m 0755 ${RPM_BUILD_ROOT}/run/myenv


%if 0%{?systemd}
install -D -m 0644 ${SRC}/tpl/systemd.myenv.mysql.unit.template ${RPM_BUILD_ROOT}/%{_unitdir}/%{name}.service
%else
install -D -m 0755 ${SRC}/tpl/myenv.server ${RPM_BUILD_ROOT}/etc/init.d/%{name}
%endif

find ${RPM_BUILD_ROOT} -type f | sed -e "s=${RPM_BUILD_ROOT}==" | sort > ${SRC}/myenv-file-list


# There is no %%clean section, on intention: "rpmbuild" does it implicit.
# Use "--noclean" if you want to analyze it after build.


##############################################################################
# Scriptlets on the user machine
##############################################################################

# Fedora docs have info on "$1":
# " ... a count of the number of versions of the package that are installed.
#   Action                           Count
#   Install the first time           1
#   Upgrade                          2 or higher (depending on the number of versions installed)
#   Remove last version of package   0 "
#
#  http://docs.fedoraproject.org/en-US/Fedora_Draft_Documentation/0.1/html/RPM_Guide/ch09s04s05.html

%pre -p /bin/bash
# This is the code running at the beginning of a RPM install or upgrade action,
# before the (new) files have been written.
# It uses bash features - ensure it is not given to a Bourne shell.

if [ $1 -eq 1 ] ; then
    # Create a MySQL user and group, for "myenv" now and the MySQL daemon later.
    # If group 'mysql' exists, just use it.
    # If user 'mysql' exists, ensure HOME = '/home/mysql' and SHELL = '/bin/bash'.
    getent group %{mysqld_group} >/dev/null || groupadd -r %{mysqld_group} 2>/dev/null
    USERLINE=$(getent passwd %{mysqld_user})
    if [ -z "$USERLINE" ] ; then
        # No such user - create it from scratch
        useradd -m -N -r -d %{mysql_home_dir} -s /bin/bash -c "MySQL server" \
            -g %{mysqld_group} %{mysqld_user} 2>/dev/null
    else
        REGEX=".*:([^:]*):([^:]*)$"
        if [[ $USERLINE =~ $REGEX ]] ; then
            OLD_HOMEDIR=${BASH_REMATCH[1]}
            OLD_SHELL=${BASH_REMATCH[2]}
        else
            echo "User 'mysql' exists, but analyzing its settings fails - ABORT"
            exit 1
        fi
        usermod -g %{mysqld_group} %{mysqld_user} 2>/dev/null
        if [ -n "$OLD_SHELL" -a "$OLD_SHELL" = "/bin/bash" ] ; then
            :
        else
            usermod -s /bin/bash %{mysqld_user}
        fi
        if [ "$OLD_HOMEDIR" = "%{mysql_home_dir}" ] ; then
            :
        else
            PROCS=$(ps --no-headers -fu %{mysqld_user})
            if [ -n "$PROCS" ] ; then
                echo "User '%{mysqld_user}' has running processes, cannot change '$HOME' - ABORT"
                exit 1
            fi
            usermod -d %{mysql_home_dir} %{mysqld_user}
        fi
        mkdir %{mysql_home_dir} 2>/dev/null || :
        SKEL=/etc/skel
        eval $(grep '^SKEL=' /etc/default/useradd)
        for F in .bashrc .bash_profile .bash_logout .profile
        do
            if [ -f ${SKEL}/${F} -a ! -f %{mysql_home_dir}/${F} ] ; then
                cp ${SKEL}/${F} %{mysql_home_dir}
            fi
        done
        chown -R %{mysqld_user}: %{mysql_home_dir}
    fi
fi


%post
# This is the code running at the end of a RPM install or upgrade action,
# after the (new) files have been written.

cd %{mysql_home_dir}/product
rm -f %{name}                      # On upgrade, old files are still present.
ln -s %{name}-%{version} %{name}   # Remove symlink, then set it again.

# Empty "/etc/myenv/myenv.conf" - only on first installation
if [ $1 -eq 1 ] ; then
    cp %{name}/tpl/myenv.conf.template     /etc/%{name}/myenv.conf
fi

# Make the "myenv" functions accessible in the user's shell
if [ $1 -gt 1 ] ; then
    ed %{mysql_home_dir}/.bash_profile <<-EOF 2>/dev/null || :
	/^# BEGIN MyEnv$/,/^# END MyEnv$/d
	w
	q
	EOF
fi
cat %{name}/tpl/profile.template >> %{mysql_home_dir}/.bash_profile

echo 'export MYENV_BASE=%{mysql_home_dir}/product/%{name}' > /etc/%{name}/MYENV_BASE
chown -R %{mysqld_user}:%{mysqld_group}  \
    %{mysql_home_dir}/product/ %{mysql_home_dir}/.bash_profile /etc/%{name}

# The 'myenv' service must be enabled, but there is no use in starting it now:
# - On first install, there is no MySQL instance which it might control.
# - On upgrade of 'myenv', we don't want it to affect any MySQL instance.
# Enabling it for next boot will let it start the autostart MySQL instances.
%if 0%{?systemd}
systemctl daemon-reload >/dev/null 2>&1 || :
systemctl enable myenv >/dev/null 2>&1 || :
%else
/sbin/chkconfig --add myenv
%endif


%preun -p /bin/bash
if [ $1 -eq 0 ] ; then
    # Package removal, not upgrade
    INSTANCES=$(%{mysql_home_dir}/product/%{name}/bin/getInstanceNames.php)
    RC=$?
    COUNT=$(echo $INSTANCES | wc -w)
    if [ $COUNT -gt 0 ] ; then
        echo "'%{name}' is still controlling $COUNT instances of MySQL, it cannot be uninstalled."
        echo $INSTANCES
        echo "Use 'installMyEnv' to delete them."
        exit 1
    fi
    %if 0%{?systemd}
        systemctl --no-reload disable myenv > /dev/null 2>&1 || :
        systemctl stop myenv > /dev/null 2>&1 || :
    %else
        /sbin/service myenv stop >/dev/null 2>&1 || :
        /sbin/chkconfig --del myenv
    %endif
fi


%postun
if [ $1 -eq 0 ] ; then
    # Uninstall the last version
    rm %{mysql_home_dir}/product/%{name}
    ed %{mysql_home_dir}/.bash_profile <<-EOF 2>/dev/null || :
	/^# BEGIN MyEnv$/,/^# END MyEnv$/d
	w
	q
	EOF
    # "here document" above requires leading tab!
fi
# The file list is lacking the directories, cleanup manually
find %{mysql_home_dir}/product/%{name}-%{version} -depth -type d | grep -v '/log$' | xargs rmdir || :
%if 0%{?systemd}
systemctl daemon-reload >/dev/null 2>&1 || :
%endif
# No service restart, to not restart the MySQL instances controlled by 'myenv'.
# We do not remove the mysql user since it may still own a lot of
# database and log files.


##############################################################################
# Files
##############################################################################

%files -f myenv-file-list
# Using the list resulting from the %%install section makes changes easy,
# but it also prevents consistency checks. Change it when everything is stable.
# Currently, we have no license or doc file in the package.
# Just some directories must be handled explicitly ...

%dir %attr(755, mysql, mysql) /run/myenv
%dir %attr(755, mysql, mysql) /etc/%{name}
%dir %attr(755, mysql, mysql) %{mysql_home_dir}/product/%{name}-%{version}
%dir %attr(755, mysql, mysql) %{mysql_home_dir}/product/%{name}-%{version}/log


##############################################################################
# The spec file changelog only includes changes made to the spec file
# itself - note that they must be ordered by date (important when
# merging GIT trees)
##############################################################################
%changelog
* Wed Aug 23 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- Protect any MySQL instances from 'myenv' upgrade / un-install:
  No service stop/start on upgrade, and no un-install while there are any.

* Tue Jul 11 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- No 'condrestart' for service 'myenv' (SysV style).
  Tests pass on CentOS 6 and on SuSE 11.

* Mon Jul 10 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- Correct dependencies for SuSE platforms,
- add code to handle existing 'mysql' user,
- change distribution strings to be more general.
  Tests pass on SuSE 13 (no SuSE 12 with PHP available).

* Thu Jul  6 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- Finish (un)install scriptlets, tests pass on CentOS 7.

* Tue Jul  4 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- Work on autostart, both with and without systemd.

* Mon Jul  3 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- Added "%%pre" + "%%post" script (no autostart yet).

* Tue Jun 27 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- Get it to work on Fedora, also for RHEL + SLES.

* Thu Jun 22 2017 Jörg Brühe <joerg.bruehe@fromdual.com>
- Initial version.
