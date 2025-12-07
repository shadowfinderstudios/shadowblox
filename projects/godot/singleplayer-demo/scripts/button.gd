extends Area3D
class_name PuzzleButton

## A button that can be pressed as part of a sequence puzzle

@export var button_id: int = 0
@export var button_color: Color = Color.RED
@export var pressed_color: Color = Color.GREEN

var is_pressed: bool = false
var original_color: Color

@onready var mesh: MeshInstance3D = $MeshInstance3D

signal button_pressed(button_id: int)

func _ready() -> void:
	original_color = button_color
	# Make material unique so each button can have its own color
	var mat = mesh.get_active_material(0)
	if mat:
		mesh.set_surface_override_material(0, mat.duplicate())
	update_color()

func interact(_player: Node3D) -> void:
	press()

func press() -> void:
	if not is_pressed:
		is_pressed = true
		button_pressed.emit(button_id)
		show_pressed()
		# Reset after a short delay
		await get_tree().create_timer(0.3).timeout
		reset()

func show_pressed() -> void:
	var mat = mesh.get_active_material(0)
	if mat is StandardMaterial3D:
		mat.albedo_color = pressed_color

func reset() -> void:
	is_pressed = false
	update_color()

func update_color() -> void:
	var mat = mesh.get_active_material(0)
	if mat is StandardMaterial3D:
		mat.albedo_color = original_color

func show_error() -> void:
	var mat = mesh.get_active_material(0)
	if mat is StandardMaterial3D:
		mat.albedo_color = Color.DARK_RED
	await get_tree().create_timer(0.5).timeout
	update_color()
