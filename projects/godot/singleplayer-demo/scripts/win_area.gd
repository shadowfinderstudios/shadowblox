extends Area3D
class_name WinArea

## The final area that triggers the win condition

func _ready() -> void:
	body_entered.connect(_on_body_entered)

func _on_body_entered(body: Node3D) -> void:
	if body.has_method("win_game"):
		body.win_game()
