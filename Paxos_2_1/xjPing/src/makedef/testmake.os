all:
	if test -n `uname -r | awk '/^4.*/ { printf("BSD"); }'`; then \
		echo "SunOS 4.1.* --- BSD type"; \
	else \
		echo "SunOS 5.1.* --- SYSV type"; \
	fi
	echo "HOST_ARCH   = $(HOST_ARCH)"
	echo "HOST_MACH   = $(HOST_MACH)"
	echo "TARGET_ARCH = $(TARGET_ARCH)"
	echo "TARGET_MACH = $(TARGET_MACH)"

.SILENT:
