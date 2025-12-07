# secure-nodetunnel-server
A multiplayer server and plugin for Godot.

It supports authentication tokens, encryption, rate limiting and banning with a time limit. It supports searchable Lobbies and host migration. Also, there is now an improved Godot new_demo project in the SecureNodeTunnelExample folder for Godot 4.5.

![Secure-Nodetunnel](secure_nt_logo.png?raw=true "Secure Nodetunnel for Godot")

### License

This code is released under the MIT license.

### Lobbies

The new Lobby Schema design allows the developer to establish a message contract in the expected parameters that lobbies will communicate via the lobby registration and search system. This dictionary of parameters must match on both the server and on the client. And, are present in the HTTP/LobbySchema.cs on the server, and in nodetunnel/LobbyMetadata.gd in the client plugin. Both client and server MUST match.

In the addons folder is the secure version of the addons plugin for Godot. In nodetunnel addon make certain to edit the internal/_PacketEncryption.gd and set the variable MASTER_KEY. Use the NODETUNNEL_MASTER_KEY environment variable while testing so the key doesn't end up in your repo. When shipping you can hardcode this instead. Also, NODETUNNEL_MASTER and NODETUNNEL_REGISTRY and NODETUNNEL_RELAY env vars are available for providing ip and port for the master server, the registry and the relay.

### Setup
**This varies a lot depending on your setup.**
1. Setup .NET on your server, this varies depending on your setup. See: https://learn.microsoft.com/en-us/dotnet/core/install/
2. Open 9999, 9051 UDP and 9998, 9050 TCP for both incoming and outgoing traffic
3. Clone this repository
4. Run the MasterServer and run the RelayServer in each of their subdirectories
   ```dotnet run --configuration Release```
6. The server(s) should now be running! 

### Linux Instructions

Linux specific instructions:

If you want it to run as a daemon, create
/etc/systemd/system/nodetunnel-master.service
```
[Unit]
Description=Nodetunnel Master server
After=syslog.target network.target

[Service]
User=nodetunnel
Group=nodetunnel
WorkingDirectory=/home/nodetunnel/nodetunnel
ExecStart=/home/nodetunnel/nodetunnel/MasterServer
KillMode=process
Restart=on-failure

[Install]
WantedBy=multi-user.target 
```

Also, create the relay service and fill in the NODETUNNEL_MASTER_KEY with the master key generated when running the secure nodetunnel the first time.
/etc/systemd/system/nodetunnel-relay.service
```
[Unit]
Description=Nodetunnel Relay server
After=syslog.target network.target

[Service]
Environment="NODETUNNEL_MASTER_KEY="
User=nodetunnel
Group=nodetunnel
WorkingDirectory=/home/nodetunnel/nodetunnel
ExecStart=/home/nodetunnel/nodetunnel/RelayServer
KillMode=process
Restart=on-failure

[Install]
WantedBy=multi-user.target 
```

Then:
```
add-apt-repository ppa:dotnet/backports
apt update
apt install -y dotnet-sdk-9.0
dotnet --list-sdks

adduser nodetunnel
su nodetunnel
cd /home/nodetunnel
mkdir src
cd src
git clone PUT_THE_NODETUNNEL_SERVER_GITHUB_URL HERE
cd ayo_nodetunnel/MasterServer
dotnet publish --configuration Release -p:PublishDir=/home/nodetunnel/nodetunnel
cd ../RelayServer
dotnet publish --configuration Release -p:PublishDir=/home/nodetunnel/nodetunnel
dotnet dev-certs https --trust

As superuser, create the systemd service file I provided and then:
systemctl daemon-reload
systemctl enable nodetunnel-master.service
systemctl start nodetunnel-master.service
systemctl status nodetunnel-master.service
systemctl enable nodetunnel-relay.service
systemctl start nodetunnel-relay.service
systemctl status nodetunnel-relay.service

apt install tmux
tmux kill-session -t node_session
tmux new-session -s node_session -n Master 'bash' \; \
    split-window -v -l 50% -t Master \; \
    rename-window -t Master Relay
echo "bind-key -n C-Up select-pane -t :.-1" >> ~/.tmux.conf
echo "bind-key -n C-Down select-pane -t :.+1" >> ~/.tmux.conf
```

Then in the first pane (using CTRL UP):
```
journalctl -u nodetunnel-master.service -f
```

Then in the second pane (using CTRL DOWN):
```
journalctl -u nodetunnel-relay.service -f
```

Now it starts with the system as a daemon and you can watch the output as players connect.

### Disclaimer of Liability and Security

THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. This includes, but is not limited to, any and all security vulnerabilities, data breaches, or safety issues arising from the use of the nodetunnel project for multiplayer in Godot. You assume all risk associated with the security, reliability, and safety of your application using this project.

This software includes cryptographic features. Users are responsible for complying with all applicable U.S. export control laws and regulations, including the U.S. Export Administration Regulations (EAR). Specifically, this software may not be exported or re-exported to any country or entity to which the U.S. has embargoed goods.

### Origin

Based on:

https://github.com/curtjs/nodetunnel

https://github.com/curtjs/nodetunnel-server/
