#!/bin/sh

# Reproduce the hash set experiments from the Rock ASPLOS paper:

# Figure 1 shows our results from experiments with 50% inserts/50%
# deletes, for key ranges of (a) 256 and (b) 128,000. In each case, we
# prepopulate the hash table to contain about half of the keys, and then
# measure the time taken for the threads to complete 1,000,000
# operations each, chosen at random according to the specified operation
# distributions and key ranges. An "unsuccessful" operation (insert of a
# value already in the hash table, or delete of one not there) does not
# modify memory; thus approximately 50% of operations modify memory in
# this experiment.

duration=10000
seed=1
update=100

if [ $# -gt 0 ]; then
    date=$1
else
    date=`date +%Y.%m.%d-%H.%M`
fi

logdir="logs/${date}"
mkdir -p ${logdir}

for range in 256 128000
do

initial=`expr $range / 2`

for nb in 1 2 4 6 8
do

for run in 1
do

log="${logdir}/${range}_${nb}_${run}.log"

echo "Log file: $log"

if [ -f $log ]
then
    echo "Skipping execution (log file found): $log"
    continue
fi

cmd="./intset_hs \
    -d $duration \
    -i $initial \
    -n $nb \
    -r $range \
    -s $seed \
    -u $update \
    -a"

${cmd} -p "-core asfooo -run" 1>${log} 2>&1

done
done
done
