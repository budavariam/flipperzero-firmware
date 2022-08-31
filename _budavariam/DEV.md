# DEV

If anything goes wrong: hard reset is left+back. You're welcome.

## Useful links

[Python wrapper to control flipper](https://github.com/wh00hw/pyFlipper)

## Create new application

[dev plugin](./DEV-plugin-tutorial.md)

Update applications/meta/application.fam with the new app (`MY_NEW_APP`)

```c
App(
    appid="basic_plugins",
    name="Basic applications for plug-in menu",
    apptype=FlipperAppType.METAPACKAGE,
    provides=[
        "music_player",
        "snake_game",
        "bt_hid",
        "MY_NEW_APP",
    ],
)

```



```bash
python3 -m venv venv
. ./venv/bin/activate
python3 -m pip install -r scripts/requirements.txt

# build with flags
./fbt COMPACT=1 DEBUG=0 VERBOSE=1 updater_package copro_dist

# update through usb
# Note: make sure that no app is running, not even the passport is visible
./fbt flash_usb

# create updater package
./fbt updater_package
```

### Publishing source code

```bash
# install clang format
# brew install clang-format
./fbt format
```
