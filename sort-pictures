#! /bin/sh

destdir () {
 if identify $1 > /dev/null 2>&1; then
  if [ `wc -c $1 | awk '{print $1}'` -lt 100000 ]; then
   echo small
  else
   dest=`exif -t 0x132 $1 | grep 'Value:' | tail -1 | awk '{print $2}' | tr : -`
   dest=${dest:-undated}
   echo $dest
  fi
 else
  echo invalid
 fi
}

for i in image?????.jpg; do
 d=`destdir $i`
 mkdir -p $d
 echo $i $d
 ln -f $i $d
done
