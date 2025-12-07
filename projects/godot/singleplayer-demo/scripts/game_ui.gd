extends CanvasLayer

## Main game UI - shows messages, inventory, and code input

@onready var message_label: Label = $MessagePanel/MessageLabel
@onready var inventory_label: Label = $InventoryPanel/InventoryLabel
@onready var code_panel: Panel = $CodePanel
@onready var code_input: LineEdit = $CodePanel/VBoxContainer/CodeInput
@onready var win_panel: Panel = $WinPanel

var current_lock: CodeLock = null
var player: Node3D = null

func _ready() -> void:
	code_panel.visible = false
	win_panel.visible = false
	add_to_group("code_ui")

	# Find player and connect signals
	await get_tree().process_frame
	player = get_tree().get_first_node_in_group("player")
	if player:
		player.key_collected.connect(_on_key_collected)
		player.game_won.connect(_on_game_won)

func _on_key_collected(key_name: String) -> void:
	show_message("Collected: " + key_name)
	update_inventory()

func _on_game_won() -> void:
	win_panel.visible = true
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func show_message(text: String, duration: float = 2.0) -> void:
	message_label.text = text
	message_label.visible = true
	await get_tree().create_timer(duration).timeout
	message_label.visible = false

func update_inventory() -> void:
	if player and player.keys.size() > 0:
		inventory_label.text = "Keys: " + ", ".join(player.keys)
	else:
		inventory_label.text = "Keys: None"

func show_for_lock(lock: CodeLock) -> void:
	current_lock = lock
	code_panel.visible = true
	code_input.text = ""
	code_input.grab_focus()
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func _on_submit_pressed() -> void:
	if current_lock:
		current_lock.submit_code(code_input.text)
		hide_code_panel()

func _on_cancel_pressed() -> void:
	hide_code_panel()

func hide_code_panel() -> void:
	code_panel.visible = false
	current_lock = null
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

func _unhandled_input(event: InputEvent) -> void:
	if code_panel.visible and event.is_action_pressed("ui_cancel"):
		hide_code_panel()
