package scene

import "core:fmt"
import "core:os"
import "core:strconv"
import "core:strings"
import "core:unicode/utf8"
import "src:game/entity"
import "src:game/utilities"
import rl "vendor:raylib"

MAX_PROJECTILE_AMOUNT: i32 : 254

name: string
height: i32
width: i32
squares: [dynamic]i32
ceilingHeight: i32
floorPlane: rl.Model
floorPlaneTexture: rl.Texture2D
ceilingPlane: rl.Model
ceilingPlaneTexture: rl.Texture2D
entitiesTotal: i32 // entitiesTotal = (last_entity_id + 1) in entities.json
// projectiles: [dynamic]projectile.Projectile
position: rl.Vector3
startPosition: rl.Vector3
endPosition: rl.Vector3
size: i32

Initialize :: proc() -> rl.Camera {
	Build()
	return entity.PlayerInitializeCamera(startPosition.x, startPosition.z)
}

Build :: proc() {
	LoadSceneConfig()
	PlaceEntities()
}

LoadSceneConfig :: proc() {
	fileName := "./assets/levels/level1.cfg"
	data, success := os.read_entire_file(fileName)

	if (!success) {
		fmt.printfln("Failed to open level config file %[0]v", fileName)
		return
	}

	sceneConfigText := string(data)
	sceneArr := strings.split_lines(sceneConfigText)
	for item in sceneArr {
		keyValuePair := strings.split(item, "=")
		if (len(keyValuePair) < 2) {
			continue
		}

		if (ParseConfig(keyValuePair[0], keyValuePair[1])) {
			fmt.printfln("Parsed level.cfg key-value: %[0]v - %[1]v", keyValuePair[0], keyValuePair[1])
		}
	}
}

ParseConfig :: proc(key: string, value: string) -> bool {
	texturesPath := strings.clone_to_cstring(fmt.aprintf("./assets/textures/%[0]v", value))
	switch key 
	{
	case "ceilingtexture":
		ceilingPlaneTexture = rl.LoadTexture(texturesPath)
		return true
	case "floortexture":
		floorPlaneTexture = rl.LoadTexture(texturesPath)
		return true
	case "name":
		name = value
		return true
	case "height":
		height = cast(i32)strconv.atoi(value)
		return true
	case "width":
		width = cast(i32)strconv.atoi(value)
		return true
	case "data":
		arr := strings.split(value, ",")
		for character in arr {
			append(&squares, cast(i32)strconv.atoi(character))
		}
		return true
	case:
		return false
	}
}

PlaceEntities :: proc() {
	mapPosZ := cast(f32)height
	mapPosX := cast(f32)width

	ceilingPlane = rl.LoadModelFromMesh(MakeCustomPlaneMesh(mapPosZ, mapPosX, 1.0))
	floorPlane = rl.LoadModelFromMesh(MakeCustomPlaneMesh(mapPosZ, mapPosX, 1.0))

	ceilingPlane.materials[0].maps[rl.MaterialMapIndex.ALBEDO].texture = ceilingPlaneTexture
	floorPlane.materials[0].maps[rl.MaterialMapIndex.ALBEDO].texture = floorPlaneTexture

	position = rl.Vector3{-mapPosX / 2.0, 0.5, -mapPosZ / 2.0}
	size = height * width

	for e: i32 = 0; e < size; e += 1 {
		entityPosX := e % width
		entityPosY := e / width
		mx := position.x - 0.5 + cast(f32)entityPosX * 1.0
		my := position.z - 0.5 + cast(f32)entityPosY * 1.0

		AddEntityToScene(cast(entity.Type)squares[e], mx, my, e)
	}

	fmt.printfln("Level has total %[0]v entities", size)
}

Update :: proc() {
	rl.DrawModel(floorPlane, rl.Vector3{position.x, 0.0, position.z}, 1.0, rl.WHITE)

	rl.DrawModelEx(
		ceilingPlane,
		rl.Vector3{position.x, 1.0, -position.z},
		rl.Vector3{-1.0, 0.0, 0.0},
		180.0,
		rl.Vector3{1.0, 1.0, 1.0},
		rl.WHITE,
	)

	//TODO I need to grow entity.inScene to the size
	for i := 0; i < len(entity.entitiesInScene); i += 1 {
		ent := &entity.entitiesInScene[i]
		if (ent.id != 0) {
			entity.Update(ent)
		}
	}
	// TODO i think this could be made to be in the entity.Update loop, and the projectiles could be part of the entity array
	for p := 0; p < len(entity.projectilesInScene); p += 1 {
		entity.ProjectileUpdate(&entity.projectilesInScene[p])
	}
}

UpdateProjectiles :: proc() {

}

MakeCustomPlaneMesh :: proc(height: f32, width: f32, textureSize: f32) -> rl.Mesh {
	mesh: rl.Mesh = {}

	mesh.vertexCount = 6
	mesh.triangleCount = 2

	verts := [?]f32 {
		0.0,
		0.0,
		0.0,
		width,
		0.0,
		height,
		width,
		0.0,
		0.0,
		0.0,
		0.0,
		0.0,
		0.0,
		0.0,
		height,
		width,
		0.0,
		height,
	}

	mesh.vertices = raw_data(verts[:])

	normals := [?]f32{0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0}

	mesh.normals = raw_data(normals[:])


	texcoords := [?]f32 {
		0.0,
		0.0,
		width / textureSize,
		height / textureSize,
		width / textureSize,
		0.0,
		0.0,
		0.0,
		0.0,
		height / textureSize,
		width / textureSize,
		height / textureSize,
	}

	mesh.texcoords = raw_data(texcoords[:])

	rl.UploadMesh(&mesh, false)

	return mesh

}

AddEntityToScene :: proc(type: entity.Type, mx: f32, my: f32, id: i32) {
	#partial switch type {
	case entity.Type.NONE:
		return
	case entity.Type.START:
		startPosition = rl.Vector3{mx, 0.0, my}
	case entity.Type.END:
		endPosition = rl.Vector3{mx, 0.0, my}
	case:
		append(&entity.entitiesInScene, entity.Create(type, rl.Vector3{mx, 0.5, my}, id))
	}
	fmt.printfln("Creating entity type %[0]v", type)
}

