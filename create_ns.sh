#!/bin/bash
set -e

UE_NS="ue_ns"
HOST_IF="v-eth1"
UE_IF="v-ue1"
HOST_IP="192.168.71.136/24"
UE_IP="192.168.71.137/24"

# root確認
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root: sudo bash $0"
  exit 1
fi

# namespace削除（存在する場合のみ）
ip netns list | grep -q "^${UE_NS}" && ip netns delete ${UE_NS}

# veth削除（存在する場合のみ）
ip link show ${HOST_IF} > /dev/null 2>&1 && ip link delete ${HOST_IF}

# namespace作成
ip netns add ${UE_NS}

# veth作成
ip link add ${HOST_IF} type veth peer name ${UE_IF}

# UE側vethをnamespaceへ移動
ip link set ${UE_IF} netns ${UE_NS}

# Host側設定
ip addr add ${HOST_IP} dev ${HOST_IF}
ip link set ${HOST_IF} up

# UE namespace側設定
ip netns exec ${UE_NS} ip link set lo up
ip netns exec ${UE_NS} ip addr add ${UE_IP} dev ${UE_IF}
ip netns exec ${UE_NS} ip link set ${UE_IF} up

# 確認表示
echo "Created namespace: ${UE_NS}"
echo "Host side: ${HOST_IF} = ${HOST_IP}"
echo "UE namespace side: ${UE_IF} = ${UE_IP}"
echo
echo "Check:"
echo "  ip addr show ${HOST_IF}"
echo "  ip netns exec ${UE_NS} ip addr"
