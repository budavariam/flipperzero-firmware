# Using the Flipper CLI

## How to connect to Flipper CLI on macOS

[Source](https://forum.flipperzero.one/t/cli-command-line-interface-examples/1874/3)

For anyone looking for a simple macOS example of using the CLI until the full docs arrive here's a quick set of instructions.

1. Connect your Flipper Zero to your Mac via USB.
1. Open a Terminal window.
1. To find your Flipper Zero connection to your Mac in the `/dev` dir type the following in the terminal: `ls /dev/cu.usbmodemflip*`
This should list your Flipper Zero with it's unique NAME at the end.
1. Now to connect use the `screen` util followed by the complete `/dev/cu.` path you just found: `screen /dev/cu.usbmodemflip_NAME`

   > Make sure to replace NAME with your Flipper Zero name.

1. This should start the screen util and you should see the Flipper CLI Welcome message.
1. At the prompt type: `help` to see a list of the commands that are available from the Flipper Zero CLI.
1. A good first test is to type: `device_info` to get a detailed listing of your Flipper Zero hardware info.
1. To exit the screen util when done hit `Ctrl-A` then hit `K`. This will ask if you want to kill the session. Hit `y` and screen will exit back to the terminal command line.
