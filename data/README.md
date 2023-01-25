# NightDriverStrip Web Site

<!-- markdownlint-disable MD036 /no-emphasis-as-heading -->
_Louisr, 1/24/2023_

## Setup
In order to locally develop this site, you need to have the following requirements:
- Linux/WSL
- nodejs v16+
- npm latest

With the requirements filled, open a bash terminal, go to the data folder and type `npm install`.

Once this is finished, please run `npm run start` to have the site available for testing.

If you make code changes, you will need to reatart the `npm run start` command. If you are developing and wish to see your changes live without needing to kill and restart the server, you can simply type `npm run rebuild`, and this will re-compile after a file has been modified. Please note that if you add new files/folders, you will need to restart the `npm run rebuild` command.