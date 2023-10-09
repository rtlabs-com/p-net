#!/bin/sh
#############################################
# Add vcap rule for inbound data / outgoing cyclic frames
#
# Definitions:
# VLAN_ID :== VLAN ID
# DMAC :== xx:xx:xx:xx:xx:xx
# RT_VLAN_IDX :== 0..7
# PRIORITY :== 0..0xffff
# VCAM_HANDLE :== 0..0xffffffff
# RT_FRMID :== 0..0xffff
# RTP_ID :== 0..0x1f
# RTP_SUBID :== 0..1
# RTE_INB_UPD :== 1 for inbound RTE processing, 0 otherwise

#set -x

PRIORITY=10
VCAM_HANDLE=$1
VLAN_ID=$2
DMAC=$3
RT_VLAN_IDX=$4

RT_FRMID=$5
RTP_ID=$6
RTP_SUBID=0
RTE_INB_UPD=1

COMMAND="vcap add $VCAM_HANDLE is1 $PRIORITY 0 \
VCAP_KFS_RT L2_MAC $DMAC ff:ff:ff:ff:ff:ff \
RT_VLAN_IDX $RT_VLAN_IDX 0x7 RT_FRMID $RT_FRMID 0xffff \
VCAP_AFS_S1_RT RTP_ID $RTP_ID RTP_SUBID $RTP_SUBID \
RTE_INB_UPD $RTE_INB_UPD FWD_ENA 1 FWD_MASK 0x10"

echo "$COMMAND"
if ! $COMMAND; then
   echo "Failed adding vcap rule for inbound data"
   exit 1
fi

exit 0
