# Garry's mod -> Discord API Client
A clean & lightweight REST API communication from Garry's mod Lua to Discord that currently allows sending embed and normal messages to discord.

## Installation
- Either download a binary from release page
- Compile it for your platform

Place ``` gmsv_discordgateway_linux.dll ``` inside ``` <GarrysModDS Path>garrysmod/lua/bin ``` and then in your Lua code ``` require("discordgateway") ```

## Building (Linux 32-bit)
Currently, only 32-bit version is supported. All you need to do is to install curl, no other dependencies needed.

- Simply install curl

Ubuntu
```bash
sudo apt-get curl
```

Arch
```bash
sudo pacman -S curl
```

- Compile using make in root directory

## API Reference

#### Authorization token (https://discord.com/developers/applications)
| Function | Args     |
| :-------- | :------- |
| `DiscordGateway.Initialize(BOT_TOKEN)` | `string` |

#### Send message to specified channel trough bot
| Function | Args     |
| :-------- | :------- |
| `DiscordGateway.SendMessage(CHANNEL_ID, MESSAGE)` | `string`, `string` |

#### Send embed message to specified channel trough bot
| Function | Args     |
| :-------- | :------- |
| `DiscordGateway.SendEmbed(CHANNEL_ID, TITLE, DESCRIPTION, COLOR)` | `string`, `string`,`string`,`HEX Color (0x00FF00)` |
