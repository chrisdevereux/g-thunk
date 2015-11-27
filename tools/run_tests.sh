for f in $(ls _test | grep tests$)
do
 echo "$(echo $f | sed 's/tests//'):" 1>&2
 "_test/$f"

 echo "" 1>&2
done
