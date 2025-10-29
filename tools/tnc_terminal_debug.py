#!/usr/bin/env python3
"""
Debug version of TNC Terminal - logs errors to file for troubleshooting
"""

import sys
import traceback
import logging
from datetime import datetime

# Setup logging
logging.basicConfig(
    filename=f'/tmp/tnc_terminal_debug_{datetime.now().strftime("%Y%m%d_%H%M%S")}.log',
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

def log_exception(exc_type, exc_value, exc_traceback):
    """Log unhandled exceptions"""
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return
    
    logging.critical("Unhandled exception", exc_info=(exc_type, exc_value, exc_traceback))
    print(f"FATAL ERROR: Check debug log at /tmp/tnc_terminal_debug_*.log")
    
# Set up exception handler
sys.excepthook = log_exception

# Now import and run the terminal
try:
    from tnc_terminal import TNCTerminal, main
    logging.info("Starting TNC Terminal in debug mode")
    main()
except Exception as e:
    logging.exception("Failed to start TNC Terminal")
    print(f"Startup error: {e}")
    print("Check debug log for details")