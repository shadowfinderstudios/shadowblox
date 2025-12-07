extends Area3D
class_name PuzzleKey

## A collectible key that can be used to unlock doors

@export var key_name: String = "Key1"
@export var float_speed: float = 2.0
@export var float_height: float = 0.2
@export var rotation_speed: float = 1.0

var start_y: float
var time: float = 0.0

func _ready() -> void:
	start_y = position.y
	body_entered.connect(_on_body_entered)

func _process(delta: float) -> void:
	# Floating animation
	time += delta
	position.y = start_y + sin(time * float_speed) * float_height
	# Rotation animation
	rotate_y(rotation_speed * delta)

func _on_body_entered(body: Node3D) -> void:
	if body.has_method("collect_key"):
		body.collect_key(key_name)
		queue_free()

func interact(player: Node3D) -> void:
	if player.has_method("collect_key"):
		player.collect_key(key_name)
		queue_free()
