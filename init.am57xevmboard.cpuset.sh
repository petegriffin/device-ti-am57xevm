#!/system/bin/sh

# Assign all CPUs for foreground (AM571x and AM572x have different number of cores)
cat /sys/devices/system/cpu/present > /dev/cpuset/foreground/cpus
cat /sys/devices/system/cpu/present > /dev/cpuset/foreground/boost/cpus
