'''
This will read the entries in the ISS.log file, parse it and present it in a rich.table.

The entries are stylized to match the severity of the loglevel.

Heads up: This needs Python3.10 or newer, as the Structural Pattern Matching feature is quite new.
See PEP634 for specification and PEP636 for a tutorial showing implmentation.

This also requires the Rich library as found here: https://github.com/Textualize/rich and available through PIP.
'''

import re
from rich.table import Table
from rich.console import Console
from rich import box

# Create and stylise table
table = Table(border_style= "yellow", box = box.SIMPLE_HEAVY, pad_edge=False)
table.add_column("TIMESTAMP", no_wrap = True, justify = "left", header_style="bold magenta")
table.add_column("LEVEL", no_wrap = True, justify = "center")
table.add_column("MODULE", no_wrap = True, justify = "center")
table.add_column("MESSAGE", no_wrap = False)  # This column will potentially contain a lot of stuff, so it's good to allow it to wrap.
table.row_styles = ["none", "none", "none", "dim"]  # Every fourth row is "brighter", to give the eye a "guide" when moving leftwards in a crowded table.

# Open and read log file.
with open("ISS.log", "r") as logfile:
	logfileContent = logfile.read()

# Parse the contents of the log file and populate the table accordingly inculding stylisation according to loglevel.
regex = r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3}) (\w+) (\w+) - (.+)'

for line in logfileContent.split("\n"):
	match = re.search(regex, line)
	if match:
		match match.group(2):
			case "DEBUG":
				table.add_row(match.group(1), match.group(2), match.group(3), match.group(4), style="green")
			case "INFO":
				table.add_row(match.group(1), match.group(2), match.group(3), match.group(4), style="cyan")
			case "WARNING":
				table.add_row(match.group(1), match.group(2), match.group(3), match.group(4), style="bright_yellow")
			case "ERROR":
				table.add_row(match.group(1), match.group(2), match.group(3), match.group(4), style="red on grey")
			case "CRITICAL":
				table.add_row(match.group(1), match.group(2), match.group(3), match.group(4), style="white on red")

# Use rich.console to send the table as stdout (At least i'm assuming that's why rich.table needs rich.console)
# For now this also clears the console, as issueing the "clear" command in fish will be "undone" once this is run.
# Running console.clear() will make it easier to use the "Home" key to find the top of the table.
console = Console()
console.clear()
console.print(table)
