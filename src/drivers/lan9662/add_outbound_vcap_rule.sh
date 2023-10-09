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

COMMAND="vcap add $VCAM_HANDLE is1 $PRIORITY 0 \
VCAP_KFS_RT \
RT_VLAN_IDX $RT_VLAN_IDX 0x7 \
L2_MAC $DMAC ff:ff:ff:ff:ff:ff \
RT_TYPE 1 0x3 \
RT_FRMID $RT_FRMID 0xffff \
VCAP_AFS_S1_RT RTP_ID $RTP_ID \
FWD_ENA 1 FWD_MASK 0x10"

echo "$COMMAND"
if ! $COMMAND; then
   echo "Failed adding vcam rule for inbound data"
   exit 1
fi

exit 0
