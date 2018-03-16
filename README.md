# TwitchTest
Bandwidth tester for [Twitch](https://twitch.tv/)

![Screenshot](https://i.imgur.com/lpuqx0K.png)

TwitchTest creates a test stream to Twitch, using the "?bandwidthtest" option so your channel doesn't actually go live. It will attempt to stream at up to 10 mbps and show the achieved bitrate for each server. The TCP window size (SO_SNDBUF) can be adjusted to see what effects it has, the default setting matches what is used in [OBS](https://obsproject.com/).

Usually requires root privileges for the ping test used to test round trip time.

A pre-compiled binary for Windows can be downloaded at https://r-1.ch/TwitchTest.zip. Requires the [VS2015 x86 redistributable](https://www.microsoft.com/en-us/download/details.aspx?id=48145#4baacbe7-a8a1-8091-5597-393c6b9ace67).

This is the Linux/Unix version written with Qt and is missing a few features available in the Windows version. For that, see [the official TwitchTest Github page](https://github.com/notr1ch/TwitchTest).
