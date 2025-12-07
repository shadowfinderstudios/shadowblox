extends Area3D
class_name PressurePlate

## A pressure plate that activates when stepped on

@export var door: PuzzleDoor
@export var stay_open: bool = false  # If true, door stays open after stepping off

var bodies_on_plate: int = 0

@onready var mesh: MeshInstance3D = $MeshInstance3D

signal plate_activated
signal plate_deactivated

func _ready() -> void:
	body_entered.connect(_on_body_entered)
	body_exited.connect(_on_body_exited)

func _on_body_entered(body: Node3D) -> void:
	if body is CharacterBody3D:
		bodies_on_plate += 1
		if bodies_on_plate == 1:
			activate()

func _on_body_exited(body: Node3D) -> void:
	if body is CharacterBody3D:
		bodies_on_plate -= 1
		if bodies_on_plate <= 0:
			bodies_on_plate = 0
			if not stay_open:
				deactivate()

func activate() -> void:
	plate_activated.emit()
	print("Pressure plate activated!")

	# Visual feedback - press down
	if mesh:
		mesh.position.y = -0.05

	if door:
		door.unlock()
		door.open()

func deactivate() -> void:
	plate_deactivated.emit()
	print("Pressure plate deactivated!")

	# Visual feedback - pop up
	if mesh:
		mesh.position.y = 0

	if door and not stay_open:
		door.close()
