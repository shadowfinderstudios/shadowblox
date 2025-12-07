extends Area3D
class_name CodeLock

## A code lock that requires entering the correct code

@export var correct_code: String = "1234"
@export var door: PuzzleDoor

var is_active: bool = false
var current_input: String = ""

signal code_entered(code: String)
signal code_correct
signal code_wrong

func interact(_player: Node3D) -> void:
	show_code_ui()

func show_code_ui() -> void:
	# Find or create the code input UI
	var ui = get_tree().get_first_node_in_group("code_ui")
	if ui:
		ui.show_for_lock(self)

func submit_code(code: String) -> void:
	code_entered.emit(code)

	if code == correct_code:
		print("Correct code!")
		code_correct.emit()
		if door:
			door.unlock()
			door.open()
	else:
		print("Wrong code!")
		code_wrong.emit()

func _unhandled_input(event: InputEvent) -> void:
	if not is_active:
		return

	if event is InputEventKey and event.pressed:
		var key = event.keycode
		if key >= KEY_0 and key <= KEY_9:
			current_input += str(key - KEY_0)
		elif key == KEY_ENTER or key == KEY_KP_ENTER:
			submit_code(current_input)
			current_input = ""
		elif key == KEY_BACKSPACE:
			if current_input.length() > 0:
				current_input = current_input.substr(0, current_input.length() - 1)
		elif key == KEY_ESCAPE:
			deactivate()

func activate() -> void:
	is_active = true
	current_input = ""
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func deactivate() -> void:
	is_active = false
	current_input = ""
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
