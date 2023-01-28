# NightDriverStrip Web Site

<!-- markdownlint-disable MD036 /no-emphasis-as-heading -->
_Louisr, 1/24/2023_

## Setup
In order to locally develop this site, you need to have the following requirements:
- Linux/WSL
- nodejs v16+
- npm latest

With the requirements filled, open a bash terminal, go to the site folder and type `npm install`.

In order to test locally, you need to put the ip address in (site/src/espaddr.jsx)[https://github.com/Louis-Riel/NightDriverStrip/blob/main/site/src/espaddr.jsx]

Once this is finished, please run `npm run start` to have the site available for testing.

If you make code changes, you will need to reatart the `npm run start` command. If you are developing and wish to see your changes live without needing to kill and restart the server, you can simply type `npm run rebuild`, and this will re-compile after a file has been modified. Please note that if you add new files/folders, you will need to restart the `npm run rebuild` command.

Once local testing is done, run `npm run build`, and then use platformio to build the filesystem, and then flash the ASP32.

<img src="../assets/dark.PNG" width="400" />
<img src="../assets/light.PNG" width="400" />