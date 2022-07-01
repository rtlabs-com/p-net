#!/bin/sh

############################################
# Add vcap rule for outbound data / incoming cyclic frames
#
# Definitions:
# VLAN_ID :== VLAN ID
# RT_VLAN_IDX :== 0..7
# PRIORITY :== 0..0xffff
# VCAM_HANDLE :== 0..0xffffffff
# RT_FRMID :== 0..0xffff
# RTP_ID :== 0..0x1f
# RTP_SUBID :== 0..1
# RTE_INB_UPD :== 1 for inbound RTE processing, 0 otherwise

# set -x

PRIORITY=10
VCAM_HANDLE=$1
VLAN_ID=$2
RT_VLAN_IDX=$3

RT_FRMID=$4
RTP_ID=$5
DMAC=$6

# Enable untagged RTP processing
symreg ana_rt_vlan_pcp[$RT_VLAN_IDX].pcp_mask 0xff     # Wildcard on pcp value
symreg ana_rt_vlan_pcp[$RT_VLAN_IDX].vlan_id $VLAN_ID  # Actual vid
symreg ana_rt_vlan_pcp[$RT_VLAN_IDX].vlan_pcp_ena 1    # Enable entry

COMMAND="vcap add is1 $PRIORITY $VCAM_HANDLE \
s1_rt first 0 \
rt_vlan_idx $RT_VLAN_IDX 0x7 \
l2_mac $DMAC ff:ff:ff:ff:ff:ff \
rt_type 1 0x3 \
rt_frmid $RT_FRMID 0xffff \
s1_rt rtp_id $RTP_ID \
fwd_ena 1 fwd_mask 0x10"

echo "$COMMAND"
if ! $COMMAND; then
   echo "Failed adding vcam rule for inbound data"
   exit 1
fi

exit 0
