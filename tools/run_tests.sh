for f in $(ls _test | grep tests$)
do
 echo "$(echo $f | sed 's/tests//'):"
 "_test/$f"

 echo ""
done
