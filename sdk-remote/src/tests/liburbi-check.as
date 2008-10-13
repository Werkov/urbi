m4_pattern_allow([^URBI_(PATH|SERVER)$])         -*- shell-script -*-

AS_INIT()dnl
URBI_PREPARE()

set -e

case $VERBOSE in
  x) set -x;;
esac

# Avoid zombies and preserve debugging information.
cleanup ()
{
  exit_status=$?

  # In case we were caught by set -e, kill the children.
  children_kill
  children_harvest
  children_report

  children_sta=$(children_status remote)
  # Don't clean before calling children_status...
  test x$VERBOSE != x || children_clean

  case $exit_status:$children_sta in
    0:0) ;;
    0:*) # Maybe a children exited for SKIP etc.
	 exit $children_sta;;
    *:*) # If liburbi-check failed, there is a big problem.
	 error SOFTWARE "liburbi-check itself failed with $exit_status";;
  esac
  # rst_expect sets exit=false if it saw a failure.
  $exit
}
for signal in 1 2 13 15; do
  trap 'error $((128 + $signal)) \
	      "received signal $signal ($(kill -l $signal))"' $signal
done
trap cleanup 0


# Overriden to include the test name.
stderr ()
{
  echo >&2 "$(basename $0): $me: $@"
  echo >&2
}


# Sleep some time, but taking into account the fact that
# instrumentation slows down a lot.
my_sleep ()
{
  if $INSTRUMENT; then
    sleep $(($1 * 5))
  else
    sleep $1
  fi
}

## -------------- ##
## Main program.  ##
## -------------- ##

exec 3>&2

: ${abs_builddir='@abs_builddir@'}
check_dir abs_builddir liburbi-check
: ${abs_top_builddir='@abs_top_builddir@'}
check_dir abs_top_builddir config.status
: ${abs_top_srcdir='@abs_top_srcdir@'}
check_dir abs_top_srcdir configure.ac

# Make it absolute.
chk=$(absolute "$1")
if test ! -f "$chk.cc"; then
  fatal "no such file: $chk.cc"
fi

period=32
# ./../../../tests/2.x/andexp-pipeexp.chk -> 2.x
medir=$(basename $(dirname "$chk"))
# ./../../../tests/2.x/andexp-pipeexp.chk -> 2.x/andexp-pipeexp
me=$medir/$(basename "$chk" ".cc")
# ./../../../tests/2.x/andexp-pipeexp.chk -> andexp-pipeexp
meraw=$(basename $me)    # MERAW!

srcdir=$(absolute $srcdir)
export srcdir
# Move to a private dir.
rm -rf $me.dir
mkdir -p $me.dir
cd $me.dir

# Help debugging
set | rst_pre "$me variables"

# $URBI_SERVER.
#
# If this SDK-Remote is part of the Kernel package, then we should not
# use an installed urbi-console, but rather the one which is part of
# this package.
if test -x $abs_top_builddir/../src/urbi-console; then
  URBI_SERVER=$abs_top_builddir/../src/urbi-console
  export URBI_PATH=$abs_top_srcdir/../share
else
  # Leaves trailing files, so run it in subdir.
  find_urbi_server
fi

# compute expected output
sed -n -e 's@//= @@p' $chk.cc >output.exp
touch error.exp
echo 0 >status.exp

#start it
valgrind=$(instrument "server.val")
cmd="$valgrind $URBI_SERVER --port 0 -w server.port --period $period"
echo "$cmd" >server.cmd
$cmd >server.out 2>server.err &
children_register server

my_sleep 2

#start the test
valgrind=$(instrument "remote.val")
cmd="$valgrind ../../tests localhost $(cat server.port) $meraw"
echo "$cmd" >remote.cmd
$cmd >remote.out.raw 2>remote.err &
children_register remote

# Let some time to run the tests.
children_wait 10

# Ignore the "client errors".
sed -e '/^E client error/d' remote.out.raw >remote.out.eff
# Compare expected output with actual output.
rst_expect output remote.out
rst_pre "Error output" remote.err

# Display Valgrind report.
rst_pre "Valgrind" remote.val

# Exit with success: liburbi-check made its job.  But now clean (on
# trap 0) will check $exit to see if there is a failure in the tests
# and adjust the exit status accordingly.
exit 0
