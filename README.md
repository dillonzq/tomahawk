# Tomahawk

A C library to get high precision(origin of the name Tomahawk) timers through
reading time stamp counter for platform with modern CPUs
or calling clock_gettime for POSIX compliant systems with older CPUs.

Both compatible with Intel 64 and IA-32 architecture.

Both compatible with CPUs having `rdtscp` instruction and CPUs only having `rdtsc` instruction.
Calling `clock_gettime` on POSIX compliant systems is the most secure method without very high precision.
You can get **Tomahawk status** by calling ``th_status``.

All the results were **statistically corrected for average errors**.
You can get **relative error** by calling ``th_relative_error``.

Read source code for more information.

Issues and PRs are welcome.

Thanks for guidance from my mentor Maomeng.
