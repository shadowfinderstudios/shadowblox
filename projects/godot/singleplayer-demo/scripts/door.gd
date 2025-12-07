extends StaticBody3D
class_name PuzzleDoor

## A door that can be opened with a key or by puzzle completion

@export var required_key: String = ""
@export var open_speed: float = 2.0
@export var open_offset: Vector3 = Vector3(0, 3, 0)

var is_open: bool = false
var is_locked: bool = true
var target_position: Vector3
var start_position: Vector3

@onready var collision_shape: CollisionShape3D = $CollisionShape3D
@onready var mesh: MeshInstance3D = $MeshInstance3D

signal door_opened

func _ready() -> void:
	start_position = position
	target_position = start_position
	# If no key required, door starts unlocked
	if required_key.is_empty():
		is_locked = false

func _process(delta: float) -> void:
	if position.distance_to(target_position) > 0.01:
		position = position.lerp(target_position, open_speed * delta)

	# Update collision when fully open
	if is_open and position.distance_to(target_position) < 0.1:
		collision_shape.disabled = true

func interact(player: Node3D) -> void:
	if is_open:
		return

	if is_locked:
		if required_key.is_empty():
			print("This door is locked by a puzzle")
		elif player.has_method("has_key") and player.has_key(required_key):
			unlock()
			open()
		else:
			print("You need ", required_key, " to open this door")
	else:
		open()

func unlock() -> void:
	is_locked = false
	print("Door unlocked!")

func open() -> void:
	if not is_open and not is_locked:
		is_open = true
		target_position = start_position + open_offset
		door_opened.emit()
		print("Door opened!")

func close() -> void:
	if is_open:
		is_open = false
		target_position = start_position
		collision_shape.disabled = false
