#!/usr/bin/env bash
set -e

if [ "$(id -u)" -ne 0 ]; then
  echo "Execute como root: sudo $0"
  exit 1
fi

apt-get update
apt-get install -y build-essential cmake qtbase5-dev qtbase5-dev-tools libqt5serialport5-dev

echo "Dependências instaladas. Você pode compilar o projeto com cmake."