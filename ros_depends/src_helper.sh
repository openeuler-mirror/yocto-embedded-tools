#!/bin/bash
input_file="src.txt"
while read rows
do
    pkg=`echo $rows | awk '{print $1}'`
    url=`echo $rows | awk '{print $2}'`
    mkdir -p $pkg > /dev/null
    pushd $pkg > /dev/null
        echo "---- getting $pkg"
            exp_file=`echo $url | awk -F "/" '{print $(NF)}'`
            if [ -f $exp_file ]; then
                echo "File exists or NULL-url."
            else
                wget  ${url/github.com/download.fastgit.org}
            fi
        echo "---- done $pkg"
    popd > /dev/null
done < $input_file
