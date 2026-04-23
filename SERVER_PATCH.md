# Patching SpaceNinjaServer
By default, SpaceNinjaServer intentionally rejects login requests from clients running the latest Warframe version. To use OpenWF Enabler with the latest Warframe version, you must disable this check.

1. If running, close your current SpaceNinjaServer instance.
2. Download [the patched launch script](extra/UPDATE%20AND%20START%20SERVER%20%5Bopenopenwf%5D.bat).
3. Place the patched script inside your SpaceNinjaServer directory (the same place that contains `UPDATE AND START SERVER.bat`)
4. Use the newly downloaded script `UPDATE AND START SERVER [openopenwf].bat` in order to launch SpaceNinjaServer.