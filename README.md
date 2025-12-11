# OpenWF Enabler
**OpenWF Enabler** is an open-source alternative to the established bootstrapper used by the [OpenWF](https://openwf.io/) project. The purpose of OpenWF Enabler is to redirect Warframe's requests to an OpenWF server.

**This project is intended to be used with Warframe versions 41.0 and above.**

## Usage
1. Follow the [Server Installation steps on the OpenWF website](https://openwf.io/guide) and configure the OpenWF server to your liking.
2. Download the [latest release of OpenWF Enabler](https://github.com/mskimi7/openopenwf/releases).
3. Run `openopenlauncher.exe`. The launcher will populate the launch parameters based on currently installed Warframe instance - adjust if incorrect.
4. Start the game!

## Configuration
OpenWF Enabler uses the [same configuration file and format](https://openwf.io/bootstrapper-manual) as the standard bootstrapper, i.e. `OpenWF/Client Config.json`. However, the only configurable settings at this point are:
- `server_host`, which allows you to specify the OpenWF's server hostname
- `disable_nrs_connection`, which prevents Warframe from testing P2P connectivity and raising a firewall warning
- `http_port` and `https_port`, which specifies a custom OpenWF's server port for HTTP and HTTPS traffic, respectively.

All other settings are (for the time being) ignored.

## Notes
- If you're using the standard version of OpenWF bootstrapper, you must remove it before using OpenWF Enabler. You cannot use both redirectors simultaneously.
- This project, for the time being, ONLY supports request redirection. Other features such as scripting, metadata patching, or content replacement are not supported.
- You cannot use this tool to connect to official Warframe servers because it manipulates the requests in such a way that is incompatible with them.

## Screenshots
<img width="544" height="202" alt="untitled" src="https://github.com/user-attachments/assets/c40de05c-00ae-472b-9a0e-f0d954385007" />
<img width="999" height="597" alt="inspector" src="https://github.com/user-attachments/assets/0d66ecdb-e154-4f20-8489-759ec0f3da3c" />
