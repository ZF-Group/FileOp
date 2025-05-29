#!/usr/bin/env bash
set -e
set -o pipefail
THIS_DIRECTORY="$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)"

export PATH=$THIS_DIRECTORY/../build:$PATH

max=100

rm -rf /tmp/$$
mkdir /tmp/$$
pushd /tmp/$$ > /dev/null

function append_rsp() {
   local current_dir=$1; shift
   local index=$1; shift

   if [ $index -le $max ] ; then
      echo "$current_dir " >> mkdir.rsp
      for i in $(seq 1 $max); do
         echo "$current_dir/f$i " >> touch.rsp
      done
      append_rsp $current_dir/$index "$(($index + 1))"
   fi
}

function time_it() {
   echo -n " "
   (time "$@") 2>&1 | sed --silent '/real/ { s/real\s*//; p; }' | tr '\r\n' '  '
   echo -n "|"
}

append_rsp root 0

echo """## Performance test

"""
sed --regexp-extended -s 's|/|\\\\|g' < mkdir.rsp > mkdir.backslash.rsp
sed --regexp-extended -s 's|/|\\\\|g' < touch.rsp > touch.backslash.rsp

###############################################################################
echo "| Environment | Create $(wc --lines < mkdir.rsp) directories | Create $(wc --lines < touch.rsp) files | Remove tree |"
echo "| ----------- | -------------------------------------------- | -------------------------------------- | ----------- |"
###############################################################################
echo -n "| cmd |"
time_it xargs --max-chars=8000 --arg-file mkdir.backslash.rsp -I '{}' cmd.exe //c "mkdir {}" \;
time_it xargs --max-chars=8000 --arg-file touch.backslash.rsp -I '{}' cmd.exe //c "for %f in ( {} ) do @( echo. > %f )" \;
time_it cmd.exe //c "rmdir /S /Q root"
echo ""
###############################################################################
echo -n "| bash |"
time_it xargs --max-chars=8000 --arg-file mkdir.rsp mkdir
time_it xargs --max-chars=8000 --arg-file touch.rsp touch
time_it rm -rf root
echo ""
###############################################################################
echo -n "| FileOp |"
time_it xargs --max-chars=8000 --arg-file mkdir.rsp FileOp.exe mkdir
time_it xargs --max-chars=8000 --arg-file touch.rsp FileOp.exe touch
time_it FileOp.exe remove --recursive --force root
echo ""
###############################################################################
echo -n "| FileOp with rsp file |"
time_it FileOp.exe mkdir @mkdir.rsp
time_it FileOp.exe touch @touch.rsp
time_it FileOp.exe remove --recursive --force root
echo ""

echo """
If no response file is supported \`xargs\` is used for execution and needs to call the command
$((xargs --max-chars=8000 --arg-file mkdir.rsp echo) | wc --lines) times for the directories and
$((xargs --max-chars=8000 --arg-file touch.rsp echo) | wc --lines) times for the file creation.
"""