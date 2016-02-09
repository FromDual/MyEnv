function showMyEnvStatus()
{
  cd $MYENV_BASE
  $MYENV_BASE/bin/showMyEnvStatus.php
  cd - >/dev/null
}

function setMyEnv()
{
  tmp=/tmp/myEnv.$$
  touch $tmp
  cd $MYENV_BASE
  $MYENV_BASE/bin/setMyEnv.php $tmp $1
  ret=$?
  if [ $ret -ne 0 ] ; then
    echo ret=$ret
  fi
  . $tmp
  if [ "$MYENV_DEBUG" != '' ] ; then
    /bin/cat $tmp
  fi
  if [ -f $tmp ] ; then
    # Do not remove in DEBUG mode
    if [ "$MYENV_DEBUG" == '' ] ; then
      rm -f $tmp
    fi
  fi
  cd - >/dev/null
}

function restart()
{
  $MYENV_BASE/bin/database.php $MYENV_DATABASE restart
}

function start()
{
  if [ $# -eq 0 ] ; then
    $MYENV_BASE/bin/database.php $MYENV_DATABASE start
  elif [ $# -eq 1 ] ; then

    pattern="^-{2}.*$"
    if [[ "$1" =~ $pattern ]] ; then
      $MYENV_BASE/bin/database.php $MYENV_DATABASE start $1
    else
      $MYENV_BASE/bin/database.php $1 start
    fi

  elif [ $# -ge 2 ] ; then
    $MYENV_BASE/bin/database.php $1 start $2
  fi
}

function stop()
{
  if [ -z "$1" ] ; then
    $MYENV_BASE/bin/database.php $MYENV_DATABASE stop
  else
    $MYENV_BASE/bin/database.php $1 stop
  fi
}


# function time_on()
# {
#   export PS1='\u@\h:\w [${MYENV_DATABASE}, ${MYSQL_TCP_PORT}, $(date "+%H:%M:%S.%N" | cut -b -12)]> '
# }
function time_on()
{
  red='\[\e[01;31m\]'
  rst='\[\e[00m\]'

  if [ "$MYENV_STAGE" = 'production' ] ; then
    stg=$red 
  else
    stg=''
    rst=''
  fi

  # export is necessary!
  export PS1="\\u@\\h:\\w [$stg${MYENV_DATABASE}$rst, ${MYSQL_TCP_PORT}, $(date "+%H:%M:%S.%N" | cut -b -12)]> "
}

# function time_off()
# {
#   export PS1='\u@\h:\w [${MYENV_DATABASE}, ${MYSQL_TCP_PORT}]> '
# }

function time_off()
{
  red='\[\e[01;31m\]'
  rst='\[\e[00m\]'

  if [ "$MYENV_STAGE" = 'production' ] ; then
    stg=$red 
  else
    stg=''
    rst=''
  fi

  # export is necessary!
  export PS1="\\u@\\h:\\w [$stg${MYENV_DATABASE}$rst, ${MYSQL_TCP_PORT}]> "
}

# allows cd xxx yyy --> cd .../yyy/...
function cd()
{
  if (( $# == 2 )) ; then
    PWD=$(pwd)
    command cd "${PWD/$1/$2}"
  else
    command cd "$@"
  fi
}

setMyEnv

alias up='showMyEnvStatus'
alias u='showMyEnvStatus'
alias ll='ls -l'

time_off
