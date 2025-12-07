extends Node3D
class_name ButtonPuzzleController

## Controls the button sequence puzzle

@export var correct_sequence: Array[int] = [1, 2, 3]
@export var door: PuzzleDoor

var current_progress: Array[int] = []

signal puzzle_solved
signal puzzle_reset

func _ready() -> void:
	# Connect all child buttons
	for child in get_children():
		if child is PuzzleButton:
			child.button_pressed.connect(_on_button_pressed)

func _on_button_pressed(button_id: int) -> void:
	var next_index = current_progress.size()

	if next_index >= correct_sequence.size():
		return

	if correct_sequence[next_index] == button_id:
		current_progress.append(button_id)
		print("Correct! Progress: ", current_progress.size(), "/", correct_sequence.size())

		if current_progress.size() == correct_sequence.size():
			solve()
	else:
		print("Wrong button! Resetting...")
		show_error()
		reset()

func solve() -> void:
	print("Puzzle solved!")
	puzzle_solved.emit()
	if door:
		door.unlock()
		door.open()

func reset() -> void:
	current_progress.clear()
	puzzle_reset.emit()

func show_error() -> void:
	for child in get_children():
		if child is PuzzleButton:
			child.show_error()
