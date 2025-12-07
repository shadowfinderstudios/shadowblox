extends Control

@onready var lobby_container = %LobbyContainer
@onready var refresh_button = %RefreshButton
@onready var loading_label = %LoadingLabel
@onready var error_label = %ErrorLabel
@onready var refresh_timer: Timer = $RefreshTimer

var lobby_item_scene = preload("res://addons/nodetunnel/new_demo/lobby/lobby_list_item.tscn")

var REGISTRY_URL: String
var use_master_server: bool = false  # Set by parent if using P2P mode

var _already_refreshing: bool = false

signal lobby_selected(lobby_id: String)


func _ready() -> void:
	var master_server = OS.get_environment("NODETUNNEL_MASTER")
	if not master_server.is_empty():
		# P2P mode - use master server
		use_master_server = true
		REGISTRY_URL = master_server
		if not REGISTRY_URL.begins_with("http://") and not REGISTRY_URL.begins_with("https://"):
			REGISTRY_URL = "http://" + REGISTRY_URL
		print("Lobby list using master server: ", REGISTRY_URL)
	else:
		# Relay mode - use registry
		REGISTRY_URL = OS.get_environment("NODETUNNEL_REGISTRY")
		if not REGISTRY_URL or REGISTRY_URL.is_empty():
			REGISTRY_URL = "http://secure.nodetunnel.io:8099"
		else:
			if not REGISTRY_URL.begins_with("http://") and not REGISTRY_URL.begins_with("https://"):
				REGISTRY_URL = "http://" + REGISTRY_URL
			if not REGISTRY_URL.contains(":8099"):
				REGISTRY_URL = REGISTRY_URL + ":8099"
		print("Lobby list using registry: ", REGISTRY_URL)
	refresh_button.pressed.connect(_on_refresh_pressed)
	refresh_lobbies()
	refresh_timer.start()


func _on_visibility_changed() -> void:
	if refresh_timer:
		if is_visible_in_tree():
			refresh_timer.start()
		else:
			refresh_timer.stop()


func _on_refresh_pressed() -> void:
	if _already_refreshing: return
	_already_refreshing = true
	await get_tree().create_timer(5).timeout
	refresh_lobbies()
	_already_refreshing = false


func refresh_lobbies() -> void:
	loading_label.show()
	error_label.hide()
	refresh_button.disabled = true

	for child in lobby_container.get_children():
		child.queue_free()

	var http = HTTPRequest.new()
	add_child(http)
	http.request_completed.connect(_on_lobbies_fetched.bind(http))

	# Use different endpoints for master server vs relay registry
	var endpoint = "/api/host/list" if use_master_server else "/lobbies"
	var url = REGISTRY_URL + endpoint

	var error = http.request(url)
	if error != OK:
		_show_error("Failed to connect to server")
		loading_label.hide()
		refresh_button.disabled = false


func _on_lobbies_fetched(result: int, response_code: int, headers: PackedStringArray, body: PackedByteArray, http: HTTPRequest) -> void:
	loading_label.hide()
	refresh_button.disabled = false
	http.queue_free()

	if result != HTTPRequest.RESULT_SUCCESS:
		_show_error("Network error occurred")
		return

	if response_code != 200:
		_show_error("Server returned error: " + str(response_code))
		return

	var json_text = body.get_string_from_utf8()
	var json = JSON.new()
	var parse_result = json.parse(json_text)

	if parse_result != OK:
		_show_error("Failed to parse response")
		return

	var data = json.get_data()
	if typeof(data) != TYPE_DICTIONARY:
		_show_error("Invalid response format")
		return

	var lobbies = []

	# Handle both master server and relay registry formats
	if use_master_server:
		# Master server format: {"hosts": [...]}
		if not data.has("hosts"):
			_show_error("Invalid master server response")
			return

		# Convert master server format to lobby format
		for host in data["hosts"]:
			lobbies.append({
				"LobbyId": host.get("hostId", ""),
				"Name": host.get("gameName", "Unknown"),
				"Mode": host.get("gameMode", "Unknown"),
				"Map": host.get("mapName", "Unknown"),
				"CurrentPlayers": host.get("currentPlayers", 0),
				"MaxPlayers": host.get("maxPlayers", 4),
				"IsPassworded": host.get("isPasswordProtected", false)
			})
	else:
		# Relay registry format: {"lobbies": [...]}
		if not data.has("lobbies"):
			_show_error("Invalid registry response")
			return
		lobbies = data["lobbies"]

	if lobbies.size() == 0:
		_show_error("No lobbies available")
		return

	for lobby in lobbies:
		var item = lobby_item_scene.instantiate()
		lobby_container.add_child(item)
		item.set_lobby_data(lobby)
		item.join_pressed.connect(_on_lobby_join_pressed.bind(lobby["LobbyId"]))


func _on_lobby_join_pressed(lobby_id: String) -> void:
	lobby_selected.emit(lobby_id)


func _show_error(message: String) -> void:
	error_label.text = message
	error_label.show()
