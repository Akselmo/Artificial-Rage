package game

import "src:game/scene"
import "src:game/settings"
import rl "vendor:raylib"

playerCamera: rl.Camera
state: GameState

GameState :: enum {
	MainMenu,
	Loading,
	GameRunning,
	GameOver,
	GamePauseMenu,
}

Initialize :: proc() {
	state = GameState.Loading

	settings.Initialize()

	rl.InitWindow(settings.screenWidth, settings.screenHeight, settings.GameTitle)
	rl.SetTargetFPS(settings.maxFPS)

	playerCamera = scene.Initialize()
	state = GameState.MainMenu

}

Update :: proc() {
	rl.BeginDrawing()

	rl.ClearBackground(rl.BLACK)

	if state == GameState.GameRunning {
		rl.BeginMode3D(playerCamera)
		scene.Update(&playerCamera)
		rl.EndMode3D()
		HudUpdate()
	} else if state == GameState.MainMenu {
		MenuUpdate()
	}
	rl.EndDrawing()
}

MenuUpdate :: proc() {
	rl.DisableCursor()
	//https://www.raylib.com/examples/textures/loader.html?name=textures_image_processing
	MenuButton("Press space to start", rl.WHITE, rl.Rectangle{40.0, 50.0, 150.0, 30.0})

	// menu presses etc come here
	// Enable and disable cursor based on if menu is on or off
	if rl.IsKeyDown(rl.KeyboardKey.SPACE) {
		state = GameState.GameRunning
	}

}

