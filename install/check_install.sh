echo "[INFO] Checking kernel version"
uname -r | grep dfl
echo "[INFO] Expected 6.1.41-dfl"

echo ""
echo "[INFO] Checking DFL modules"
lsmod | grep dfl*
echo "[INFO] Expected non-empty list (if PAC installed)"

echo ""
echo "[INFO] Checking OPAE-SDK"
rpm -qa | grep opae*
echo "[INFO] Expected non-empty list"

echo ""
echo "[INFO] Checking FME"
fpgainfo fme
echo "[INFO] Expecting FME output (if OPAE-SDK and PAC installed)"
