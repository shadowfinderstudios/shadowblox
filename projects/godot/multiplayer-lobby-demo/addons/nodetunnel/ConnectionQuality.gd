extends Node
class_name ConnectionQuality

signal metrics_updated(metrics: Dictionary)
signal ping_test_completed(ping_ms: float, jitter_ms: float)

const PING_TEST_COUNT = 3

var _metrics: Dictionary = {
	"ping_ms": 0.0,
	"jitter_ms": 0.0,
	"packet_loss_percent": 0.0,
	"stability_score": 1.0,
	"cpu_usage_percent": 0.0,
	"available_memory_mb": 0.0
}

var _ping_history: Array[float] = []
var _packet_sent_count: int = 0
var _packet_received_count: int = 0
var _last_update_time: int = 0

## Initialize connection quality monitoring
func _ready():
	_update_system_metrics()

## Start quality test (ping only)
func start_quality_test(master_server_url: String) -> void:
	await _test_ping(master_server_url)
	_update_system_metrics()
	_calculate_stability_score()

	metrics_updated.emit(_metrics)

## Test ping and jitter
func _test_ping(server_url: String) -> void:
	print("[Quality] Testing ping...")

	var ping_times: Array[float] = []
	var http = HTTPRequest.new()
	add_child(http)

	for i in range(PING_TEST_COUNT):
		var start_time = Time.get_ticks_msec()

		# Simple ping using HTTP GET to health endpoint
		var url = server_url + "/health"
		http.request(url, [], HTTPClient.METHOD_GET)
		var response = await http.request_completed

		if response[1] == 200:
			var ping = Time.get_ticks_msec() - start_time
			ping_times.append(ping)
			_packet_received_count += 1
		else:
			# Packet lost
			pass

		_packet_sent_count += 1

		# Small delay between pings
		await get_tree().create_timer(0.1).timeout

	http.queue_free()

	# Calculate metrics
	if ping_times.size() > 0:
		var avg_ping = ping_times.reduce(func(acc, val): return acc + val, 0.0) / ping_times.size()
		_metrics.ping_ms = avg_ping
		_ping_history.append(avg_ping)

		# Keep only last 100 pings
		if _ping_history.size() > 100:
			_ping_history.pop_front()

		# Calculate jitter (variance in ping)
		if ping_times.size() > 1:
			var variance = 0.0
			for ping in ping_times:
				variance += pow(ping - avg_ping, 2)
			variance /= ping_times.size()
			_metrics.jitter_ms = sqrt(variance)

		# Calculate packet loss
		_metrics.packet_loss_percent = (1.0 - float(ping_times.size()) / PING_TEST_COUNT) * 100.0

		print("[Quality] Ping: %.1f ms, Jitter: %.1f ms, Loss: %.1f%%" %
			[_metrics.ping_ms, _metrics.jitter_ms, _metrics.packet_loss_percent])

	ping_test_completed.emit(_metrics.ping_ms, _metrics.jitter_ms)

## Update system metrics (CPU, memory)
func _update_system_metrics() -> void:
	# Get CPU usage (Godot doesn't provide direct API, so we estimate)
	_metrics.cpu_usage_percent = Performance.get_monitor(Performance.TIME_PROCESS) * 100.0

	# Get available memory
	var total_memory = OS.get_memory_info().get("physical", 0)
	var available_memory = OS.get_memory_info().get("available", 0)

	if total_memory > 0:
		_metrics.available_memory_mb = available_memory / (1024.0 * 1024.0)

## Calculate stability score based on connection history
func _calculate_stability_score() -> void:
	if _ping_history.size() < 2:
		_metrics.stability_score = 1.0
		return

	# Calculate variance in ping times
	var avg_ping = _ping_history.reduce(func(acc, val): return acc + val, 0.0) / _ping_history.size()
	var variance = 0.0
	for ping in _ping_history:
		variance += pow(ping - avg_ping, 2)
	variance /= _ping_history.size()

	# Calculate stability (1.0 = perfect, 0.0 = unstable)
	var cv = sqrt(variance) / avg_ping if avg_ping > 0 else 0  # Coefficient of variation
	_metrics.stability_score = max(0.0, 1.0 - cv)

	# Factor in packet loss
	if _metrics.packet_loss_percent > 0:
		_metrics.stability_score *= (1.0 - _metrics.packet_loss_percent / 100.0)

## Get current metrics
func get_metrics() -> Dictionary:
	return _metrics.duplicate()

## Calculate host quality score (focuses on latency)
func calculate_score() -> float:
	# Normalize ping (0-1, lower is better, cap at 300ms)
	var normalized_ping = max(1.0 - (_metrics.ping_ms / 300.0), 0.0)

	# Packet loss penalty (0-1)
	var packet_loss_penalty = max(1.0 - (_metrics.packet_loss_percent / 10.0), 0.0)

	# Combine metrics with weights (emphasize ping and stability)
	return (normalized_ping * 0.5) + \
		   (_metrics.stability_score * 0.4) + \
		   (packet_loss_penalty * 0.1)

## Check if metrics meet minimum hosting requirements
func meets_minimum_requirements(player_count: int) -> bool:
	# check latency and stability
	return _metrics.ping_ms < 150 and \
		   _metrics.packet_loss_percent < 5.0 and \
		   _metrics.stability_score > 0.95

## Update metrics periodically
## Runs ping test at startup and then every interval_sec
func start_continuous_monitoring(master_server_url: String, interval_sec: float = 30.0) -> void:
	# Do initial test at startup
	await start_quality_test(master_server_url)

	# Continue monitoring with periodic ping tests
	while true:
		await get_tree().create_timer(interval_sec).timeout
		await start_quality_test(master_server_url)

## Get NAT type string
static func nat_type_to_string(nat_type: int) -> String:
	match nat_type:
		0: return "Unknown"
		1: return "Open"
		2: return "Moderate"
		3: return "Strict"
		4: return "Blocked"
		_: return "Unknown"
