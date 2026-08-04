#!/bin/bash

set -e

FORGEJO_URL="http://forgejo"
PACKAGE_OWNER="greergan"
PACKAGE_NAME="slimcommon"

PACKAGE_URL="${FORGEJO_URL}/${PACKAGE_OWNER}/-/packages/generic/${PACKAGE_NAME}"

packages=$(dpkg-query -W -f='${binary:Package}\t${db:Status-Abbrev}\n' 2>/dev/null |
	awk '$1 ~ /^slimcommon/ && $2 ~ /^ii/ {print $1}')

if [ -n "$packages" ]; then
	echo "Removing installed SlimCommon packages:"
	echo "$packages"
	echo

	apt remove -y $packages
fi

echo "Finding latest SlimCommon package..."

version=$(curl -fsSL "$PACKAGE_URL" |
	grep -oE '/slimcommon/[0-9]+\.[0-9]+\.[0-9]+' |
	sed 's|.*/||' |
	sort -V |
	tail -n 1)

if [ -z "$version" ]; then
	echo "No SlimCommon package found."
	exit 1
fi

filename="slimcommon-v${version}-amd64.deb"

echo "Latest version: $version"
echo "Downloading: $filename"

curl -fL \
	"${FORGEJO_URL}/api/packages/${PACKAGE_OWNER}/generic/${PACKAGE_NAME}/${version}/${filename}" \
	-o "/tmp/${filename}"

echo "Installing $filename..."

apt install -y "/tmp/${filename}"

rm -f "/tmp/${filename}"

echo
echo "SlimCommon $version installed."
