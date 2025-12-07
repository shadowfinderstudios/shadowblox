extends Node
class_name NodeTunnelGameMode

## Network topology configuration
## Change this to switch between relay-only and P2P modes
enum NetworkTopology {
	RELAY_ONLY,    # Traditional relay server (current working mode)
	P2P_PREFERRED, # P2P with relay fallback (experimental, requires master server)
	P2P_ONLY       # P2P only, no relay fallback (experimental, requires master server)
}

## =============================================================================
## CONFIGURATION - Change this to test different network topologies
## =============================================================================
const NETWORK_TOPOLOGY: NetworkTopology = NetworkTopology.P2P_PREFERRED

## Master server URL (only needed for P2P modes)
## Set NODETUNNEL_MASTER environment variable or change this
const DEFAULT_MASTER_SERVER: String = "http://localhost:9050"
## =============================================================================

const MAX_PLAYERS = 4

const PLAYER_SCENE = preload("res://addons/nodetunnel/new_demo/player/node_tunnel_player.tscn")

var peer: NodeTunnelPeer = null
var players: Array[NodeTunnelPlayer] = []
var bullets: Array[NodeTunnelBullet] = []
var relay_ready: bool = false

var world_state_data: Dictionary = {}

# Host migration state
var migration_in_progress: bool = false
var players_before_migration: Array[NodeTunnelPlayer] = []


func _quit_game():
	if peer and (peer.connection_state == NodeTunnelPeer.ConnectionState.JOINED
			or peer.connection_state == NodeTunnelPeer.ConnectionState.HOSTING):
		peer.leave_room()
	get_tree().quit()


func _notification(what):
	# Windows / Linux
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_quit_game()
	# Android
	elif what == NOTIFICATION_WM_GO_BACK_REQUEST:
		_quit_game()


func _perform_spawn_player(data: Dictionary) -> Node:
	var id: int = data["id"]
	var player: NodeTunnelPlayer = PLAYER_SCENE.instantiate()
	player.set_multiplayer_authority(id)
	player.name = str(id)
	player.position = data["position"]
	players.append(player)
	return player


func _perform_spawn_bullet(data: Dictionary) -> Node:
	var bullet: Node = preload("res://addons/nodetunnel/new_demo/objects/node_tunnel_bullet.tscn").instantiate()
	bullet.start(data["ownerid"], data["position"], data["rotation"])
	bullets.append(bullet)
	return bullet


func _ready() -> void:
	if multiplayer.is_server():
		$PlayerSpawner.set_spawn_function(Callable(self, "_perform_spawn_player"))
		$BulletSpawner.set_spawn_function(Callable(self, "_perform_spawn_bullet"))

	# Initialize peer based on configured topology
	_initialize_peer()

	#peer.debug_enabled = true
	multiplayer.multiplayer_peer = peer
	peer.set_encryption_enabled(true)

	# Connect to relay server
	var relay: String = OS.get_environment("NODETUNNEL_RELAY")
	if relay:
		await _connect_to_network(relay)
	else:
		push_error("Set NODETUNNEL_RELAY environment variable to point to your relay server.")
		return

	# Connect signals
	peer.peer_connected.connect(_add_player)
	peer.peer_disconnected.connect(_remove_player)
	peer.room_left.connect(_cleanup_room)

	%IDLabel.text = "Online ID: " + peer.online_id

	# Update UI based on topology
	_update_topology_label()

	relay_ready = true


## Initialize the peer based on configured network topology
func _initialize_peer() -> void:
	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			print("[NetworkTopology] Initializing in RELAY_ONLY mode")
			peer = NodeTunnelPeer.new()

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			print("[NetworkTopology] Initializing in P2P mode: ", NetworkTopology.keys()[NETWORK_TOPOLOGY])
			peer = NodeTunnelPeerP2P.new()

			# Setup P2P components (requires Node context)
			(peer as NodeTunnelPeerP2P).setup_p2p_components(self)

			# Configure P2P settings
			var master_server = OS.get_environment("NODETUNNEL_MASTER")
			if master_server.is_empty():
				master_server = DEFAULT_MASTER_SERVER

			(peer as NodeTunnelPeerP2P).set_p2p_enabled(true, master_server)
			(peer as NodeTunnelPeerP2P).enable_host_migration = true

			# Set connection mode
			if NETWORK_TOPOLOGY == NetworkTopology.P2P_PREFERRED:
				(peer as NodeTunnelPeerP2P).set_connection_mode(NodeTunnelPeerP2P.ConnectionMode.P2P_PREFERRED)
			else:
				(peer as NodeTunnelPeerP2P).set_connection_mode(NodeTunnelPeerP2P.ConnectionMode.P2P_ONLY)

			# Connect P2P-specific signals
			(peer as NodeTunnelPeerP2P).p2p_connection_established.connect(_on_p2p_connection_established)
			(peer as NodeTunnelPeerP2P).p2p_connection_failed.connect(_on_p2p_connection_failed)
			(peer as NodeTunnelPeerP2P).host_migration_started.connect(_on_host_migration_started)
			(peer as NodeTunnelPeerP2P).host_migration_completed.connect(_on_host_migration_completed)
			(peer as NodeTunnelPeerP2P).host_migration_failed.connect(_on_host_migration_failed)


## Connect to the network (relay and optionally master server)
func _connect_to_network(relay_address: String) -> void:
	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			# Simple relay connection
			peer.connect_to_relay(relay_address, 9998)
			await peer.relay_connected

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			# Connect to both master server and relay
			var master_server = OS.get_environment("NODETUNNEL_MASTER")
			if master_server.is_empty():
				master_server = DEFAULT_MASTER_SERVER

			if not master_server.begins_with("http://") and not master_server.begins_with("https://"):
				master_server = "http://" + master_server

			print("[NetworkTopology] Connecting to master server: ", master_server)
			print("[NetworkTopology] Connecting to relay server: ", relay_address)

			(peer as NodeTunnelPeerP2P).set_fallback_relay(relay_address, 9998, 9999)
			await (peer as NodeTunnelPeerP2P).connect_to_master_and_relay(master_server, relay_address, 9998)


## Update UI to show current topology
func _update_topology_label() -> void:
	var topology_name = ""
	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			topology_name = "RELAY"
		NetworkTopology.P2P_PREFERRED:
			topology_name = "P2P+RELAY"
		NetworkTopology.P2P_ONLY:
			topology_name = "P2P ONLY"
	%IDLabel.text = "Online ID: " + peer.online_id + " [" + topology_name + "]"


func _on_host_pressed() -> void:
	if not relay_ready:
		print("Waiting for network connection...")
		relay_ready = true

	%ConnectionControls.set_process_mode(Node.PROCESS_MODE_DISABLED)
	%Host.text = "Hosting..."
	%Host.disabled = true
	%Join.disabled = true
	%BrowseLobbies.disabled = true

	var lobby_name = %LobbyNameInput.text
	if lobby_name.is_empty():
		lobby_name = "My Lobby"

	var metadata = LobbyMetadata.create(
		lobby_name,
		"Deathmatch",
		"Arena",
		MAX_PLAYERS,
		false,
		{
			"Version": "1.0.0",
			"Region": "US-West",
			"Difficulty": "Normal"
		}
	)

	print("Online ID: ", peer.online_id)

	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			var registry: String = OS.get_environment("NODETUNNEL_REGISTRY")
			if not registry or registry.is_empty():
				registry = ""
				push_warning("NODETUNNEL_REGISTRY not set. Lobby won't be listed in browser.")
			else:
				if not registry.begins_with("http://") and not registry.begins_with("https://"):
					registry = "http://" + registry
				if not registry.contains(":8099"):
					registry = registry + ":8099"
			if not registry.is_empty():
				print("Using registry: ", registry)
				peer.enable_lobby_registration(self, registry, metadata)
			peer.host()
			await peer.hosting

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			peer.lobby_metadata = metadata
			await (peer as NodeTunnelPeerP2P).host_p2p()

	%Host.text = "Host"
	%Host.disabled = false
	%Join.disabled = false
	%BrowseLobbies.disabled = false
	%ConnectionControls.set_process_mode(Node.PROCESS_MODE_INHERIT)

	_add_player()
	%ConnectionControls.hide()
	%LeaveRoom.show()


@rpc("any_peer", "call_local", "reliable")
func receive_world_state(_world_state_data: Dictionary):
	if multiplayer.is_server(): return
	print("World state received from server.")
	world_state_data = _world_state_data
	var found_us: bool = false
	for i in range(players.size()):
		var id: int = players[i].name.to_int()
		NodeTunnelPlayer.from_dict(players[i], world_state_data[id])
		if id == multiplayer.get_unique_id():
			found_us = true
		if id == 1: %Label_Score1.text = str(players[i].score)
		elif id == 2: %Label_Score2.text = str(players[i].score)
		elif id == 3: %Label_Score3.text = str(players[i].score)
		elif id == 4: %Label_Score4.text = str(players[i].score)
	if not found_us:
		# only 4 players, and we're not one of them
		# note: you could remove this to have spectators
		peer.leave_room()


@rpc("any_peer", "call_local", "reliable")
func fetch_world_state():
	if not multiplayer.is_server(): return

	# Clean up any freed players from the array first
	var valid_players: Array[NodeTunnelPlayer] = []
	for player in players:
		if is_instance_valid(player) and player.is_inside_tree():
			valid_players.append(player)
	players = valid_players

	# Build player states dictionary
	var player_states: Dictionary = {}
	for player in players:
		var id: int = player.name.to_int()
		player_states[id] = player.to_dict()

	rpc_id(multiplayer.get_remote_sender_id(), "receive_world_state", player_states)


func _on_join_pressed() -> void:
	var host_id = %HostID.text

	%ConnectionControls.set_process_mode(Node.PROCESS_MODE_DISABLED)
	%Join.text = "Joining..."
	%Host.disabled = true
	%Join.disabled = true
	%BrowseLobbies.disabled = true

	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			peer.join(host_id)
			await peer.joined

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			await (peer as NodeTunnelPeerP2P).join_p2p(host_id)

	fetch_world_state.rpc_id(1)

	%Join.text = "Join"
	%Host.disabled = false
	%Join.disabled = false
	%BrowseLobbies.disabled = false
	%ConnectionControls.set_process_mode(Node.PROCESS_MODE_INHERIT)
	%ConnectionControls.hide()
	%LeaveRoom.show()


func _on_browse_lobbies_pressed() -> void:
	%LobbyListContainer.show()
	_load_lobby_list()


func _on_close_lobby_list_pressed() -> void:
	%LobbyListContainer.hide()


func _load_lobby_list() -> void:
	var placeholder = %LobbyListPlaceholder
	if placeholder.get_child_count() == 0:
		var lobby_list_scene = preload("res://addons/nodetunnel/new_demo/lobby/lobby_list.tscn")
		var lobby_list = lobby_list_scene.instantiate()
		placeholder.add_child(lobby_list)
		lobby_list.lobby_selected.connect(_on_lobby_selected)
		lobby_list.anchors_preset = Control.PRESET_FULL_RECT
		lobby_list.anchor_right = 1.0
		lobby_list.anchor_bottom = 1.0
	else:
		var lobby_list = placeholder.get_child(0)
		lobby_list.refresh_lobbies()


func _on_lobby_selected(lobby_id: String) -> void:
	%LobbyListContainer.hide()
	%HostID.text = lobby_id
	_on_join_pressed()


func _add_player(peer_id: int = 1) -> Node:
	if !multiplayer.is_server(): return
	if players.size()+1 > MAX_PLAYERS:
		print("Cannot accept more than ",MAX_PLAYERS," players.")
		return
	print("Player Joined: ", peer_id)
	var pos: Vector2 = $Level.get_children()[players.size() % MAX_PLAYERS].global_position
	return $PlayerSpawner.spawn({"id": peer_id, "position": pos})


func _get_random_spawnpoint():
	return $Level.get_children().pick_random().global_position


func _remove_player(peer_id: int) -> void:
	if !multiplayer.is_server(): return

	# During migration, don't remove players - we'll handle it after migration completes
	if migration_in_progress:
		print("[Demo] Skipping player removal during migration for peer ", peer_id)
		return

	var player = get_node_or_null(str(peer_id))
	if player:
		print("[Demo] Removing player ", peer_id)
		players.erase(player)
		player.queue_free()


func _on_leave_room_pressed() -> void:
	peer.leave_room()


func _cleanup_room() -> void:
	%LeaveRoom.hide()
	%ConnectionControls.show()
	players = []
	bullets = []
	world_state_data = {}
	reset_scores()


func _on_close_button_pressed() -> void:
	%LobbyListContainer.hide()


func add_bullet(pos: Vector2, rot: float) -> void:
	$BulletSpawner.spawn({"ownerid": multiplayer.get_remote_sender_id(), "position": pos, "rotation": rot})


func add_score(playerid: int):
	for i in range(players.size()):
		var id: int = players[i].name.to_int()
		if playerid == id:
			players[i].score = players[i].score + 1
			_update_score_rpc.rpc(playerid, players[i].score)
			if id == 1: %Label_Score1.text = str(players[i].score)
			elif id == 2: %Label_Score2.text = str(players[i].score)
			elif id == 3: %Label_Score3.text = str(players[i].score)
			elif id == 4: %Label_Score4.text = str(players[i].score)
			break


func respawn_player(playerid: int):
	for i in range(players.size()):
		var id: int = players[i].name.to_int()
		if playerid == id:
			players[i].health = players[i].MAX_HEALTH
			_update_health_rpc.rpc(playerid, players[i].health)
			break


func reset_scores():
	%Label_Score1.text = "0"
	%Label_Score2.text = "0"
	%Label_Score3.text = "0"
	%Label_Score4.text = "0"



@rpc("any_peer", "call_local", "reliable")
func _update_score_rpc(playerid: int, score: int):
	if multiplayer.is_server(): return
	for i in range(players.size()):
		var id: int = players[i].name.to_int()
		if playerid == id:
			players[i].score = score
			if id == 1: %Label_Score1.text = str(score)
			elif id == 2: %Label_Score2.text = str(score)
			elif id == 3: %Label_Score3.text = str(score)
			elif id == 4: %Label_Score4.text = str(score)
			break


@rpc("any_peer", "call_local", "reliable")
func _update_health_rpc(playerid: int, health: int):
	if multiplayer.is_server(): return
	for i in range(players.size()):
		var id: int = players[i].name.to_int()
		if playerid == id:
			players[i].health = health
			break


func _on_p2p_connection_established(peer_id: String) -> void:
	print("[P2P] Direct connection established to peer: ", peer_id)


func _on_p2p_connection_failed(peer_id: String) -> void:
	# P2P failure is expected in many network setups, not an error
	if NETWORK_TOPOLOGY == NetworkTopology.P2P_PREFERRED:
		print("[P2P] Direct connection not possible with peer %s, using relay fallback" % peer_id)


func _on_host_migration_started(reason: String) -> void:
	print("[Demo] Host migration started: ", reason)
	migration_in_progress = true
	players_before_migration = players.duplicate()


func _on_host_migration_completed(new_host_id: String) -> void:
	print("[Demo] Host migration completed. New host: ", new_host_id)
	print("[Demo] My online_id: ", peer.online_id)
	print("[Demo] My unique_id BEFORE wait: ", multiplayer.get_unique_id())
	print("[Demo] Am I server BEFORE wait: ", multiplayer.is_server())
	print("[Demo] Connection state: ", peer.connection_state)
	migration_in_progress = false

	await get_tree().process_frame
	await get_tree().process_frame

	print("[Demo] My unique_id AFTER wait: ", multiplayer.get_unique_id())
	print("[Demo] Am I server AFTER wait: ", multiplayer.is_server())

	# If we're the new host, ensure all connected peers have players
	if multiplayer.is_server():
		print("[Demo] We are the new host - ensuring all players exist")

		# Clean up freed players first
		var valid_players: Array[NodeTunnelPlayer] = []
		for player in players:
			if is_instance_valid(player) and player.is_inside_tree():
				valid_players.append(player)
		players = valid_players

		# Get list of currently connected peers
		var connected_peer_ids = multiplayer.get_peers()
		connected_peer_ids.append(1)  # Add ourselves (the server)

		print("[Demo] Connected peers after migration: ", connected_peer_ids)
		print("[Demo] Valid players in scene: ", players.map(func(p): return p.name))

		# Check if each connected peer has a player node
		for peer_id in connected_peer_ids:
			var player_node = get_node_or_null(str(peer_id))

			if player_node == null:
				# Check if this peer had a player with a different NID before migration
				# (happens when we become the new host and our NID changes from 2 → 1)
				var old_player = null
				var old_nid = -1
				for saved_player in players_before_migration:
					if is_instance_valid(saved_player) and saved_player.is_inside_tree():
						# Check if this saved player belongs to the current peer
						# For us (peer_id == 1), check if we were the one promoted
						if peer_id == 1 and peer.online_id == new_host_id:
							# We're the new host - look for our old player
							var saved_nid = int(saved_player.name)
							if saved_nid != 1:  # Not the old host's player
								old_player = saved_player
								old_nid = saved_nid
								break

				if old_player != null:
					# Rename the existing player to the new NID
					print("[Demo] Found our old player (NID ", old_nid, ") - renaming to ", peer_id)
					old_player.name = str(peer_id)
					old_player.set_multiplayer_authority(peer_id)

					# Update the players array reference
					if old_player not in players:
						players.append(old_player)

					print("[Demo] Player ", peer_id, " preserved at ", old_player.position)
				else:
					# Player doesn't exist, need to spawn them
					print("[Demo] Missing player for peer ", peer_id, " - spawning now")

					# Use saved player data if available
					var spawn_pos = _get_random_spawnpoint()
					var found_saved = false
					for saved_player in players_before_migration:
						if is_instance_valid(saved_player) and int(saved_player.name) == peer_id:
							if saved_player.is_inside_tree():
								spawn_pos = saved_player.position
								found_saved = true
								print("[Demo] Using saved position for peer ", peer_id, ": ", spawn_pos)
							break

					if not found_saved:
						print("[Demo] No saved position for peer ", peer_id, ", using random spawn: ", spawn_pos)

					var player_data = {
						"id": peer_id,
						"position": spawn_pos
					}
					print("[Demo] Calling $PlayerSpawner.spawn() for peer ", peer_id)
					$PlayerSpawner.spawn(player_data)
			else:
				print("[Demo] Player ", peer_id, " already exists at ", player_node.position)
	else:
		print("[Demo] Not the new host (server check failed)")

	players_before_migration.clear()
	print("[Demo] Migration completion handler finished")


func _on_host_migration_failed(reason: String) -> void:
	push_error("[Demo] Host migration failed: ", reason)
	migration_in_progress = false
	players_before_migration.clear()
	# Could disconnect players or show error
