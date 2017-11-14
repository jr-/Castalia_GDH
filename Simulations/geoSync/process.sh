#!/bin/bash

echo "Preprocessing data..."

rm -v *-scenarios.csv 2>/dev/null

echo "Extracting simulation scenarios from omnet output..."
for OUTPUT in ../outputs/*-omnet-output.txt; do
    FILENAME=$(basename "$OUTPUT")
    RUNNAME=${FILENAME:0: -17}
    CONFIGNAME=$(echo $RUNNAME | sed 's:-[^-]*$::g')
    RUNNUMBER=$(echo $RUNNAME | sed 's:^[^-]*-::g')
    METADATA="$CONFIGNAME-scenarios.csv"
    echo "$OUTPUT -> ./$METADATA"

    if [ ! -f $METADATA ]; then
        VARIABLES=$(grep '^Scenario:' "$OUTPUT" | head -1 | sed -e 's/Scenario: //g' -e 's:, \$repetition=[0-9]*::g' -e 's:\$\([^=]*\)=[^ ,]*:\1:g' -e 's: ::g')
        echo "runNumber,$VARIABLES" > $METADATA
    fi

    grep '^Scenario:' "$OUTPUT" | sed -e 's/Scenario: //g' -e 's:, \$repetition=[0-9]*::g' | while read SCENARIO; do
        echo -n "$RUNNUMBER," >> $METADATA
        echo $SCENARIO | sed -e 's:\$[^=]*=\([^,]*\):\1:g' -e 's: ::g' >> $METADATA
    done
done

echo "Extracting relevant data from simulation results..."
for SCA in ../results/*.sca; do
    FILENAME=$(basename $SCA)
    DATA="${FILENAME:0: -4}-data.csv"
    echo "$SCA -> ./$DATA"
    echo "TXedPkts,DeliveryRatio,FairnessIndex,energyConsumed,ETEDelayMin,ETEDelayMean,ETEDelayMax,HashCollisions,HashCollisionsTotal,KeepAlives" > $DATA

    ETEDMIN=$(grep "\"E-T-E Delay Min\"" "$SCA" | head -1 | awk '{print $6}')
    ETEDMEAN=$(grep "\"E-T-E Delay Mean\"" "$SCA" | head -1 | awk '{print $6}')
    ETEDMAX=$(grep "\"E-T-E Delay Max\"" "$SCA" | head -1 | awk '{print $6}')
    DR=$(grep "\"Delivery Ratio\"" "$SCA" | head -1 | awk '{print $5}')
    FI=$(grep "\"Fairness Index\"" "$SCA" | grep Application | head -1 | awk '{print $5}')
    HASHTOTAL=$(grep "\"Total hash collisions\"" "$SCA" | head -1 | awk '{print $6}')

    LASTNODE="$(grep '^scalar SN\.node\[' $SCA | sed 's:^scalar SN.node\[\([0-9]*\)\].*:\1:g' | sort -un | tail -1)"

    for NODE in $(seq 0 $LASTNODE); do
        TXED=$(grep "\"TXed pkts\"" "$SCA" | grep "\[$NODE\]" | awk '{print $5}')
        echo -n "$TXED" >> $DATA
        echo -n ",$DR" >> $DATA
        echo -n ",$FI" >> $DATA
        ENERGY=$(grep "\"Consumed Energy\"" "$SCA" | grep "\[$NODE\]" | awk '{print $5}')
        echo -n ",$ENERGY" >> $DATA
        echo -n ",$ETEDMIN" >> $DATA
        echo -n ",$ETEDMEAN" >> $DATA
        echo -n ",$ETEDMAX" >> $DATA
        HASH=$(grep "\"Hash collisions\"" "$SCA" | grep "\[$NODE\]" | awk '{print $5}')
        echo -n ",$HASH" >> $DATA
        echo -n ",$HASHTOTAL" >> $DATA
        KEEPALIVES=$(grep "\"Keep Alives Sent\"" "$SCA" | grep "\[$NODE\]" | awk '{print $6}')
        echo -n ",$KEEPALIVES" >> $DATA
        echo "" >> $DATA
    done
done

echo "...preprocessing done"
