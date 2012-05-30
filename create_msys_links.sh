#! /bin/sh

#msys2: SYNOP
#msys8: Aviation: SPECI, METAR etc.
dirs="/opdata/norcom/msys/msys2
      /opdata/norcom/msys/msys8"

destdir=/var/lib/kvalobs/synopreports


for dir in $dirs; do
    filelist=`ls -1 $dir/data????`
    msys=$(basename $dir)
    
    for file in $filelist; do
        filename=$(basename $file)
        ln -s $file $destdir/${msys}_${filename}
#        echo "hei"
    done
done
