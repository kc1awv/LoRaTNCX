#!/usr/bin/env python3
from datetime import datetime

# Get current UTC date and time
now = datetime.utcnow()
date_str = now.strftime("%Y-%m-%d")
time_str = now.strftime("%H:%M:%S")

# Generate a header file with build date/time
with open('include/build_info.h', 'w') as f:
    f.write('#ifndef BUILD_INFO_H\n')
    f.write('#define BUILD_INFO_H\n\n')
    f.write(f'#define FIRMWARE_BUILD_DATE_UTC "{date_str}"\n')
    f.write(f'#define FIRMWARE_BUILD_TIME_UTC "{time_str}"\n\n')
    f.write('#endif // BUILD_INFO_H\n')

print(f"Generated build_info.h with date: {date_str}, time: {time_str}")